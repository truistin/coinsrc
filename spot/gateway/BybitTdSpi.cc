#include <functional>
#include <spot/gateway/BybitTdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Compatible.h>
#include <spot/bybit/BybitApi.h>
#include <spot/bybit/BybitStruct.h>
#include <spot/gateway/OrderStoreCallBack.h>
#include "spot/base/InitialData.h"

#include <spot/strategy/OrderController.h>
#include <spot/restful/UriFactory.h>
#include <spot/utility/MeasureFunc.h>
#include "spot/cache/CacheAllocator.h"


using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spotrapidjson;

BybitTdSpi::BybitTdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore)
{
	apiInfo_ = apiInfo;
	orderStore_ = orderStore;
	traderApi_ = new BybitApi(apiInfo->userId_,apiInfo->passwd_,apiInfo->investorId_, this);
	//traderApi_->Init();
}
BybitTdSpi::~BybitTdSpi()
{
	delete traderApi_;
}
void BybitTdSpi::Init()
{
	// MeasureFunc::addMeasureData(1, "BybitTdSpi ReqOrderInsert time calc", 10);
	// MeasureFunc::addMeasureData(6, "BybitTdSpi httpAsynReqCallback time calc", 10);

    string wsUrl = apiInfo_->frontTdAddr_;

	char *buff = CacheAllocator::instance()->allocate(sizeof(WebSocketApi));
	websocketApi_ = new (buff) WebSocketApi();

	websocketApi_->SetCallBackOpen(std::bind(&BybitTdSpi::com_callbak_open, this));
	websocketApi_->SetCallBackMessage(std::bind(&BybitTdSpi::com_login_message, this, std::placeholders::_1));
	websocketApi_->SetCallBackClose(std::bind(&BybitTdSpi::com_callbak_close, this));

    websocketApi_->Set_Protocol_Mode(true);
    websocketApi_->SetUri(wsUrl);
	LOG_INFO << "BybitTdSpi wsUrl: "<<  wsUrl;


	std::thread BybitTdSpiSocket(std::bind(&WebSocketApi::Run, websocketApi_));
	pthread_setname_np(BybitTdSpiSocket.native_handle(), "BybitTdSpiSocket");
	BybitTdSpiSocket.detach();

	std::thread BybitTdRun(&BybitTdSpi::Run, this);
	pthread_setname_np(BybitTdRun.native_handle(), "BybitTdRun");
	BybitTdRun.detach();
	// SLEEP(5000);
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	// traderApi_->UpdateAccount();
	traderApi_->CancelAllOrders();
}

void BybitTdSpi::com_callbak_open()
{
	websocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&BybitTdSpi::com_login_message, this, std::placeholders::_1));
	auth();
	std::cout << "BybitTdSpi callback open! " << std::endl;
}

void BybitTdSpi::auth() {
	//post arams: {"op":"login","args":[{"channel":"tickers","instId":"BTC-USDT-SWAP"}]}
	string ak = apiInfo_->userId_;
	string timestamp = std::to_string(CURR_MSTIME_POINT + 3000);
	string val = "GET/realtime" + timestamp;
	string sk = apiInfo_->passwd_;

	unsigned char* output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	unsigned int output_length = 0;
	HmacEncode("sha256",  sk.c_str(), val.c_str(), output, &output_length);

	char buf[64];
    std::string dst = "";
    for (int i = 0; i < 32; i++) {
        sprintf(buf, "%02x", output[i]);
        buf[0] = ::toupper(buf[0]);
        buf[1] = ::toupper(buf[1]);
        dst = dst + buf;
    }

    string sig_base64 = UrlEncode(dst.c_str());

	if (output)
	{
		free(output);
		LOG_INFO << "BybitTdSpi::auth ak: " << ak << ", sk: " << sk
			<< ", timestamp: " << timestamp;
	}

    string str =
        "{ \"op\": \"auth\", \"args\" : [\"" + ak + "\"," + timestamp
			 + ",\"" + sig_base64 + "\"]}";

    LOG_INFO << "Bybit auth timestamp : " << timestamp << ", val: " << val <<  ", apikey: " << ak
             << ", sk: " << sk << ", sign: " << sig_base64 << ", str: " << str;
    websocketApi_->send(str);
}

void BybitTdSpi::com_callbak_close()
{
	std::cout << "BybitTdSpi callback close! " << std::endl;
}

void BybitTdSpi::subscribe(const std::string &channel)
{ // tickers.BTCUSDT
    string str =
            "{ \"op\": \"subscribe\", \"args\" : [\"" + channel + "\"] }";      

    LOG_INFO << "Bybit Subscribe: " << ", channel: " << channel
            << ", str: " << str;
    websocketApi_->send(str);
} 

void BybitTdSpi::com_login_message(const char *message)
{
	if (message == nullptr) {
		LOG_ERROR << "BybitTdSpi com_login_message message nullptr: ";
	}
	LOG_INFO << "com_login_message message: " << message;

	if (strcmp(message, "pong") == 0) {
		return;
	} else {
		LOG_INFO << "BybitTdSpi com_login_message err: " << message;
	}

	Document doc;
	doc.Parse(message, strlen(message));
	if (doc.HasParseError())
	{
		LOG_FATAL << "com_login_message Parse error. result:" << message;
		return;
	}

	if (doc.HasMember("op") && doc.HasMember("success"))
	{
		string op = doc["op"].GetString();
		bool succ = doc["success"].GetBool();
		if (strcmp("auth", op.c_str()) == 0 && succ) {
			websocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&BybitTdSpi::com_subscribe_message, this, std::placeholders::_1));
			subscribe("order");
			subscribe("execution");
			return;
		} else {
			LOG_FATAL << "com_login_message login err" << message;
		}
		return;
	} else {
		LOG_FATAL << "com_login_message login err" << message;
	}
}

void BybitTdSpi::com_subscribe_message(const char *message)
{
	LOG_INFO << "com_subscribe_message message: " << message;

	Document doc;
	doc.Parse(message, strlen(message));
	if (doc.HasParseError())
	{
		LOG_FATAL << "com_subscribe_message Parse error. result:" << message;
		return;
	}

	if (doc.HasMember("op") && doc.HasMember("success"))
	{
		string op = doc["op"].GetString();
		bool succ = doc["success"].GetBool();
		if (strcmp("subscribe", op.c_str()) == 0 && succ) {
			websocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&BybitTdSpi::com_callbak_message, this, std::placeholders::_1));
			return;
		} else {
			LOG_FATAL << "com_subscribe_message subcribe err" << message;
		}
		return;
	} else {
		LOG_FATAL << "com_subscribe_message subcribe err" << message;
	}

}

void BybitTdSpi::com_callbak_message(const char *message)
{
	if (message == nullptr || strlen(message) == 0) {
		LOG_ERROR << "com_callbak_message BybitWssRspOrder error";
		return;
	}
	//std::lock_guard<std::mutex> lock(mutex_);
	// cout << "BybitTdSpi com_callbak_message message: " << message <<endl;
	// LOG_INFO << "BybitTdSpi com_callbak_message message: " << message;
	BybitWssStruct rspOrder;

	int ret = rspOrder.decode(message);
	if (ret == 0) {
		// cout << "BybitTdSpi com_callbak_message true message: " << message <<endl;
		LOG_INFO << "com_callbak_message BybitWssRspOrder result:" << message<< ", ret result: " << ret;
	} else if (ret == 2) {
		LOG_DEBUG << "com_callbak_message BybitWssRspOrder pong msg: " << message;
		return;
	} else {
		// cout << "BybitTdSpi com_callbak_message false message: " << message <<endl;
		LOG_INFO << "com_callbak_message BybitWssRspOrder other msg: " << message;
		return;
	}

	for (auto& iter : rspOrder.vecOrd_) {
		PutOrderToQueue(iter);
	}
}

void BybitTdSpi::uriPingOnHttp(char* message, uint64_t clientId)
{
    LOG_DEBUG << "uriPingOnHttp message: " << message << ", clientId: " << clientId;
}

void BybitTdSpi::Run()
{
	int listenTime = CURR_MSTIME_POINT;
	while (true)
	{
		// SLEEP(20000);
		std::this_thread::sleep_for(std::chrono::milliseconds(20000));
		if (CurlMultiCurl::m_QueEasyHandMap != nullptr) {
			for (auto it : (*(CurlMultiCurl::m_QueEasyHandMap))) {
				if (it.first == "BYBIT") {
					std::shared_ptr<Uri> uri = make_shared<Uri>();
					uri->protocol = HTTP_PROTOCOL_HTTPS;
					uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
					uri->method = METHOD_GET;
					uri->domain = Bybit_DOMAIN;
					uri->api = "/v5/market/time";
					uri->exchangeCode = Exchange_BYBIT;
					int order_id = OrderController::getOrderRef();
					uri->setUriClientOrdId((order_id));
					
					uri->setResponseCallback(std::bind(&BybitTdSpi::uriPingOnHttp,this,std::placeholders::_1,std::placeholders::_2));
					
					CurlMultiCurl::instance().addUri(uri);
					LOG_DEBUG << "BybitTdSpi PING: " << CURR_MSTIME_POINT << ", order_id: " << order_id << ", map size: "
						<< CurlMultiCurl::m_QueEasyHandMap->size();
					break;
				}
    		}
		}

		if (CURR_MSTIME_POINT - listenTime >= 20000) {
			listenTime = CURR_MSTIME_POINT;
			string s = "{ \"op\": \"ping\"}";
			websocketApi_->send(s);
		}

		// MutexLockGuard lock(mutex1_);
		// if (gBybitQryFlag) {
		// 	traderApi_->QryAsynInfo();
		// 	gBybitQryFlag = false;
		// }

	}
}

void BybitTdSpi::PutOrderToQueue(BybitWssRspOrder &queryOrder)
{
	//LOG_INFO << "recv order record. Bybit Order:" << queryOrder.to_string();
	Order rspOrder;
	memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
	rspOrder.OrderRef = queryOrder.clOrdId;
	rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
	rspOrder.TimeStamp = queryOrder.creationTime;

	if (strcmp(queryOrder.topic, "order") == 0) {
		if (strcmp(queryOrder.orderStatus, "Rejected") == 0)
		{
			rspOrder.OrderStatus = OrderStatus::NewRejected;
			rspOrder.OrderMsgType = RtnOrderType;
			orderStore_->storeHandle(std::move(rspOrder));
		} else if (strcmp(queryOrder.orderStatus, "New") == 0) {
			rspOrder.OrderStatus = OrderStatus::New;
			rspOrder.OrderMsgType = RtnOrderType;
			orderStore_->storeHandle(std::move(rspOrder));
		}
		else if (strcmp(queryOrder.orderStatus, "Cancelled") == 0)
		{
				rspOrder.OrderStatus = OrderStatus::Cancelled;
				rspOrder.OrderMsgType = RtnOrderType;
				rspOrder.VolumeFilled = queryOrder.volumeFilled;
				// LOG_INFO << "BybitTdSpi PutOrderToQueue CANCELED OrderRef: " << rspOrder.OrderRef 
				// 	<< ", volumeFilled : " << rspOrder.VolumeFilled;

				orderStore_->storeHandle(std::move(rspOrder));
		}
	} else if (strcmp(queryOrder.topic, "execution") == 0) {
			rspOrder.Volume = (queryOrder.volume);
			rspOrder.VolumeRemained = queryOrder.volumeRemained;
			rspOrder.VolumeFilled = queryOrder.volumeTotalOriginal - queryOrder.volumeFilled;
			rspOrder.Price = (queryOrder.price);

			rspOrder.OrderMsgType = RtnTradeType;
			rspOrder.Fee = (queryOrder.fee);

			if (IS_DOUBLE_EQUAL(rspOrder.VolumeRemained, 0)) {
				rspOrder.OrderStatus = OrderStatus::Filled;
			} else {
				rspOrder.OrderStatus = OrderStatus::Partiallyfilled;
			}

			orderStore_->storeHandle(std::move(rspOrder));
	} else {
			LOG_ERROR << "trade status OrderSysID " << queryOrder.clOrdId  << " OrderStatus " << rspOrder.OrderStatus << ", topic: " << queryOrder.topic;
	}
}

int BybitTdSpi::httpQryOrderCallback(char * result, uint64_t clientId)
{
	if (result == nullptr|| strlen(result) == 0) {
		LOG_FATAL << "BybitTdSpi httpQryOrderCallback result error clientId: " << clientId;
	}
	LOG_INFO << "BybitTdSpi httpQryOrderCallback: " << result << ", clientId: " << clientId;

	BybitQueryOrder qryOrder;
	auto ret = qryOrder.decode(result);//client_oid

	Order rspOrder;
	memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
	rspOrder.QryFlag = QryType::QRYORDER;
	rspOrder.OrderRef = clientId;

	if (ret == -3) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::NewRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return 1;
	}

	if (ret != 0) {
		LOG_WARN  << "BybitTdSpi httpQryOrderCallback: " << result << ", clientId: " << clientId << ", ret: " << ret;
		return -1;
	}

	rspOrder.VolumeFilled = qryOrder.volumeFilled;
	rspOrder.Volume = qryOrder.volumeFilled;
	rspOrder.VolumeRemained = qryOrder.volumeRemained;

	if (strcmp(qryOrder.orderStatus, "Rejected") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::NewRejected;
	} else if (strcmp(qryOrder.orderStatus, "New") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::New;
	} else if (strcmp(qryOrder.orderStatus, "Cancelled") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::Cancelled;
	} else if (strcmp(qryOrder.orderStatus, "PartiallyFilled") == 0) {
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.OrderStatus = OrderStatus::Partiallyfilled;
	} else if (strcmp(qryOrder.orderStatus, "Filled") == 0) {
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.OrderStatus = OrderStatus::Filled;
	}

	orderStore_->storeHandle(std::move(rspOrder));
	return 0;
}

int BybitTdSpi::httpAsynReqCallback(char * result, uint64_t clientId)
{
	if (result == nullptr|| strlen(result) == 0) {
		LOG_ERROR << "BybitTdSpi httpAsynReqCallback result error clientId: " << clientId;
		return -1;
		// LOG_ERROR << "BybitTdSpi httpAsynReqCallback result error clientId: " << clientId;
		// Order rspOrder;
		// memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
		// rspOrder.OrderRef = clientId;
		// rspOrder.OrderStatus  = OrderStatus::NewRejected;
		// orderStore_->storeHandle(std::move(rspOrder));
		// return -1;
	}
	LOG_INFO << "BybitTdSpi httpAsynReqCallback: " << result << ", clientId: " << clientId;
	// MeasureFunc f(6);
	BybitRspReqOrder rspOrderInsert;
	auto ret = rspOrderInsert.decode(result);//client_oid

	if (ret != 0) {
		LOG_ERROR << "BybitTdSpi httpAsynReqCallback BybitRspReqOrder decode fail: " << result << ", clientId: " << clientId;
		cout << "BybitTdSpi httpAsynReqCallback BybitRspReqOrder decode fail: " << result << ", clientId: " << clientId << endl;
		Order rspOrder;
		memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
		rspOrder.OrderRef = clientId;
		rspOrder.OrderStatus  = OrderStatus::NewRejected;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	return 0;
}

int BybitTdSpi::ReqOrderInsert(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	// MeasureFunc f(1);
	logInfoInnerOrder("BybitTdSpi ReqOrderInsert innerOrder", innerOrder);

	auto id = traderApi_->ReqOrderInsert(innerOrder);
	if (id != 0)
	{
		Order rspOrder;
		memcpy(&rspOrder, &innerOrder, sizeof(Order));
		rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::NewRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	return 0;
}

int BybitTdSpi::httpAsynCancelCallback(char * result, uint64_t clientId)
{
	// MeasureFunc f(6);
	if (result == nullptr || strlen(result) == 0) {
		LOG_FATAL << "BybitTdSpi httpAsynCancelCallback result error clientId: " << clientId;
		// LOG_ERROR << "BybitTdSpi httpAsynCancelCallback result error clientId: " << clientId;
		// Order rspOrder;
		// memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
		// rspOrder.OrderRef = clientId;
		// rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		// orderStore_->storeHandle(std::move(rspOrder));
		// return -1;
	}
	LOG_DEBUG << "BybitTdSpi httpAsynCancelCallback: " << result << ", clientId: " << clientId;
    BybitRspCancelOrder rspCancleOrder;
    int ret = rspCancleOrder.decode(result);

	if (ret != 0) {
		LOG_ERROR << "BybitTdSpi httpAsynCancelCallback BybitRspCancelOrder decode fail" << result << ", clientId: " << clientId;
		Order rspOrder;
		memcpy(rspOrder.CounterType, InterfaceBybit.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBybit.size()));
		rspOrder.OrderRef = clientId;
		rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}

	return 0;
}


int BybitTdSpi::ReqOrderCancel(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	logInfoInnerOrder("BybitTdSpi ReqOrderCancel innerOrder", innerOrder);
	string order_id = to_string(innerOrder.OrderRef);
	string symbol(innerOrder.InstrumentID);
	auto ret = traderApi_->CancelOrder(innerOrder);
	Order rspOrder;
	memcpy(&rspOrder, &innerOrder, sizeof(Order));
	rspOrder.OrderMsgType = RtnOrderType;
	if (ret != 0)
	{
		rspOrder.OrderStatus = OrderStatus::CancelRejected;
		rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}

	return 0;
}

int BybitTdSpi::Query(const Order &order)
{
	int ret = 0;
	if (order.QryFlag == QryType::QRYPOSI) {
		// ret = traderApi_->QryAsynInfo();
		ret = traderApi_->QryPosiBySymbol(order);
	} else if (order.QryFlag == QryType::QRYMPRICE) {
		// ret = traderApi_->QryAsynInfo();
	} else if (order.QryFlag == QryType::QRYORDER) { 
		ret = traderApi_->QryAsynOrder(order);
	}
	return ret;
}