#include <functional>
#include <spot/gateway/BianTdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Compatible.h>
#include <spot/bian/BianApi.h>
#include <spot/bian/BnApi.h>
#include <spot/bian/BianStruct.h>
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

//POST /fapi/v1/listenKey
#define Bian_LISTENKEY_API "/fapi/v1/listenKey"
#define Bn_LISTENKEY_API "/papi/v1/listenKey"

BianTdSpi::BianTdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore)
{
	apiInfo_ = apiInfo;
	orderStore_ = orderStore;
	// traderApi_ = new BianApi(apiInfo->userId_,apiInfo->passwd_,apiInfo->investorId_, this);
	traderApi_ = new BnApi(apiInfo->userId_,apiInfo->passwd_,apiInfo->investorId_, this);
	//traderApi_->Init();
}
BianTdSpi::~BianTdSpi()
{
	delete traderApi_;
}
void BianTdSpi::Init()
{
	// MeasureFunc::addMeasureData(1, "BianTdSpi ReqOrderInsert time calc", 10);
	// MeasureFunc::addMeasureData(6, "BianTdSpi httpAsynReqCallback time calc", 10);

//	setTradeLoginStatusMap(InterfaceBinance, true);

	getListkey();

    string wsUrl = apiInfo_->frontTdAddr_+listenKey_;
	string wsUrl1 = apiInfo_->frontQueryAddr_+listenKey_;
	LOG_INFO << "BianTdSpi wsUrl: "<<  wsUrl << ", wsUrl1: " << wsUrl1 
		<< ", wsUrl: " << apiInfo_->frontTdAddr_ 
		<< ", wsUrl1: " << apiInfo_->frontQueryAddr_
		<< ", listenKey_: "<< listenKey_;
	char *buff = CacheAllocator::instance()->allocate(sizeof(WebSocketApi));
	websocketApi_ = new (buff) WebSocketApi();

	websocketApi_->SetCallBackOpen(std::bind(&BianTdSpi::com_callbak_open, this));
	websocketApi_->SetCallBackMessage(std::bind(&BianTdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi_->SetCallBackClose(std::bind(&BianTdSpi::com_callbak_close, this));

    websocketApi_->Set_Protocol_Mode(true);
    websocketApi_->SetUri(wsUrl);

	std::thread BianTdSpiSocket(std::bind(&WebSocketApi::Run, websocketApi_));
	pthread_setname_np(BianTdSpiSocket.native_handle(), "BaTdSocket");
	BianTdSpiSocket.detach();

	buff = CacheAllocator::instance()->allocate(sizeof(WebSocketApi));
	websocketApi1_ = new (buff) WebSocketApi();
	websocketApi1_->SetCallBackOpen(std::bind(&BianTdSpi::com_callbak_open, this));
	websocketApi1_->SetCallBackMessage(std::bind(&BianTdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi1_->SetCallBackClose(std::bind(&BianTdSpi::com_callbak_close, this));

    websocketApi1_->Set_Protocol_Mode(true);
    websocketApi1_->SetUri(wsUrl1);

	std::thread BianTdSpiSocket1(std::bind(&WebSocketApi::Run, websocketApi1_));
	pthread_setname_np(BianTdSpiSocket1.native_handle(), "BaTdSocket_1");
	BianTdSpiSocket1.detach();

	std::thread BianTdRun(&BianTdSpi::Run, this);
	pthread_setname_np(BianTdRun.native_handle(), "BianTdRun");
	BianTdRun.detach();
	PostponeListenKey();
	// SLEEP(5000);
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	traderApi_->CancelAllOrders();

	traderApi_->GetSpotAsset();
	traderApi_->GetUm_Cm_Account();

	traderApi_->GetLeverageBracket();
	traderApi_->GetCollateralRate();
	traderApi_->GetAccountInfo();
}

void BianTdSpi::getListkey()
{
	Uri m_uri;
	m_uri.clear();

	//#test Bian_DOMAIN "testnet.binancefuture.com"
	//#real Bian_DOMAIN "fapi.binance.com"
	m_uri.domain = Bn_DOMAIN;
	m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
	m_uri.api = Bn_LISTENKEY_API;
	m_uri.method = METHOD_POST;
	m_uri.isinit = true;
	string userId = apiInfo_->userId_;
	string header = "X-MBX-APIKEY:" + userId;
    m_uri.AddHeader(header.c_str());
	LOG_INFO << "getListkey urlstr: " << m_uri.urlstr << ", uri->params: " << m_uri.params << ", Bn_DOMAIN: " << Bn_DOMAIN
		<< ", header: " << header;
	m_uri.Request();
	string& res = m_uri.result;

	spotrapidjson::Document documentCOM_;
	documentCOM_.Parse<0>(res.c_str());

	if (documentCOM_.HasParseError())
	{
		LOG_FATAL << "listenkey Parsing to document failure res" <<  res;
		return;
	}

	listenKey_ = documentCOM_["listenKey"].GetString();
	LOG_INFO << "BianTdSpi listenKey_: " << listenKey_;
}

void BianTdSpi::uriPingOnHttp(char* message, uint64_t clientId)
{
    LOG_DEBUG << "uriPingOnHttp message: " << message << ", clientId: " << clientId;;
}

void BianTdSpi::com_callbak_open()
{
	std::cout << "BianTdSpi callback open! " << std::endl;
}

void BianTdSpi::com_callbak_close()
{
	std::cout << "BianTdSpi callback close! " << std::endl;
}

void BianTdSpi::com_callbak_message(const char *message)
{
	if (message == nullptr || strlen(message) == 0) {
		LOG_ERROR << "com_callbak_message BianWssRspOrder error";
		return;
	}
	//std::lock_guard<std::mutex> lock(mutex_);
	//LOG_INFO << "BianTdSpi com_callbak_message message: " << message;
	BianWssRspOrder rspOrder;
	int ret = rspOrder.decode(message);
	if (ret == 0) {
		LOG_INFO << "com_callbak_message BianWssRspOrder result:" << message 
		// << ", message: " << message
		<< ", ret result: " << ret;
	} else {
		LOG_WARN << "com_callbak_message BianWssRspOrder other msg: " << message;
	}
	
	PutOrderToQueue(rspOrder);

}

void BianTdSpi::httpAsynPostponeListenKey(char * result, uint64_t clientId)
{
	return;
}

void BianTdSpi::PostponeListenKey()
{
	std::shared_ptr<Uri> uri = make_shared<Uri>();
	uri->protocol = HTTP_PROTOCOL_HTTPS;
	uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
	uri->exchangeCode = Exchange_BINANCE;
	uri->domain = Bn_DOMAIN;
	uri->method = METHOD_PUT;

	uri->api = Bn_LISTENKEY_API;

	string userId = apiInfo_->userId_;
	string header = "X-MBX-APIKEY:" + userId;
	uri->AddHeader(header.c_str());
	uri->setResponseCallback(std::bind(&BianTdSpi::httpAsynPostponeListenKey,this,std::placeholders::_1,std::placeholders::_2));
	CurlMultiCurl::instance().addUri(uri);
	
}

void BianTdSpi::Run()
{
	int listenTime = CURR_STIME_POINT;
	int AssetTime = CURR_STIME_POINT;
	int CM_UM_Time = CURR_STIME_POINT;

	int commonTime = CURR_STIME_POINT;

	while (true)
	{
		// SLEEP(InitialData::getOrderQueryIntervalMili());
		// SLEEP(600000);
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		 
		if (CurlMultiCurl::m_QueEasyHandMap != nullptr) {
			for (auto it : (*(CurlMultiCurl::m_QueEasyHandMap))) {
				if (it.first == "BINANCE") {
					std::shared_ptr<Uri> uri = make_shared<Uri>();
					uri->protocol = HTTP_PROTOCOL_HTTPS;
					uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
					uri->method = METHOD_GET;
					uri->domain = Bn_DOMAIN;
					uri->api = "/papi/v1/ping";
					uri->exchangeCode = Exchange_BINANCE;
					int order_id = OrderController::getOrderRef();
					uri->setUriClientOrdId((order_id));
					
					uri->setResponseCallback(std::bind(&BianTdSpi::uriPingOnHttp,this,std::placeholders::_1,std::placeholders::_2));
					
					CurlMultiCurl::instance().addUri(uri);
					LOG_DEBUG << "BianTdSpi PING: " << CURR_STIME_POINT  << ", order_id: " << order_id  << ", map size: "
						<< CurlMultiCurl::m_QueEasyHandMap->size();;
					break;
				}
    		}
		}

		if (CURR_STIME_POINT - listenTime >= 10) {
			listenTime = CURR_STIME_POINT;
			PostponeListenKey();
		}

		if (CURR_STIME_POINT - AssetTime >= 10) {
			AssetTime = CURR_STIME_POINT;
			traderApi_->GetSpotAsset();
			traderApi_->GetUm_Cm_Account();
		}

		if (CURR_STIME_POINT - commonTime >= 30) {
			commonTime = CURR_STIME_POINT;
			traderApi_->GetLeverageBracket();
			traderApi_->GetCollateralRate();
			traderApi_->GetAccountInfo();
		}
	}
		// if (gBianListenKeyFlag) { // all 60s
		// 	PostponeListenKey();
			//MutexLockGuard lock(mutex1_);
			// traderApi_->QryAsynInfo();
			// gBianListenKeyFlag = false;
		// }
			
		//BianMrkQryInfo queryOrder;
		//traderApi_->QryInfo(queryOrder); // markprice& fundingrate
		//traderApi_->QryPosiInfo(); //syn
}

void BianTdSpi::PutOrderToQueue(BianWssRspOrder &queryOrder)
{
	//LOG_INFO << "recv order record. Bian Order:" << queryOrder.to_string();
	Order rspOrder;
	memcpy(rspOrder.CounterType, InterfaceBinance.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBinance.size()));

	rspOrder.OrderRef = atoi(queryOrder.clOrdId);
	rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
	rspOrder.TimeStamp = queryOrder.dealTimeStamp;

	if (strcmp(queryOrder.state, "EXPIRED") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::NewRejected;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
	} else if (strcmp(queryOrder.state, "NEW") == 0) {
		rspOrder.OrderStatus = OrderStatus::New;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
	}
	else if (strcmp(queryOrder.state, "FILLED") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::Filled;
		rspOrder.Volume = queryOrder.volume;
		rspOrder.VolumeFilled = queryOrder.volumeFilled;
		rspOrder.VolumeRemained = rspOrder.VolumeTotalOriginal - queryOrder.volumeFilled;

		rspOrder.Price = (queryOrder.price);
		rspOrder.AvgPrice = (queryOrder.avgPrice);

		rspOrder.Fee = (queryOrder.fee);
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.TradeID = queryOrder.tradeID;

		orderStore_->storeHandle(std::move(rspOrder));
	//	LOG_INFO << "Fill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
	}
	else if (strcmp(queryOrder.state, "PARTIALLY_FILLED") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::Partiallyfilled;
		rspOrder.Volume = (queryOrder.volume);
		rspOrder.VolumeFilled = (queryOrder.volumeFilled);
		rspOrder.VolumeRemained = rspOrder.VolumeTotalOriginal - queryOrder.volumeFilled;

		rspOrder.Price = (queryOrder.price);
		rspOrder.AvgPrice = queryOrder.avgPrice;

		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.Fee = (queryOrder.fee);
		rspOrder.TradeID = queryOrder.tradeID;

		orderStore_->storeHandle(std::move(rspOrder));
		//LOG_INFO << "PartifiallyFill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
	}
	else if (strcmp(queryOrder.state, "CANCELED") == 0)
	{
			rspOrder.OrderStatus = OrderStatus::Cancelled;
			rspOrder.OrderMsgType = RtnOrderType;
			rspOrder.VolumeFilled = (queryOrder.volumeFilled);
			// LOG_INFO << "BianTdSpi PutOrderToQueue CANCELED OrderRef: " << rspOrder.OrderRef 
			// 	<< ", Volume : " << rspOrder.Volume;

			orderStore_->storeHandle(std::move(rspOrder));
	}
	else
	{
		LOG_ERROR << "status OrderSysID: " << queryOrder.clOrdId << " OrderStatus " << rspOrder.OrderStatus;
	}
}

int BianTdSpi::httpQryOrderCallback(char * result, uint64_t clientId)
{
	if (result == nullptr|| strlen(result) == 0) {
		LOG_FATAL << "BianTdSpi httpQryOrderCallback result error clientId: " << clientId;
	}
	LOG_INFO << "BianTdSpi httpQryOrderCallback: " << result << ", clientId: " << clientId;

	BianQueryOrder qryOrder;
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
		LOG_WARN << "BianTdSpi httpQryOrderCallback: " << result << ", clientId: " << clientId << ", ret: " << ret;
		return -1;
	}

	rspOrder.VolumeFilled = qryOrder.volumeFilled;
	rspOrder.Volume = qryOrder.volumeFilled;
	rspOrder.VolumeRemained = qryOrder.volumeTotalOriginal - qryOrder.volumeFilled;

	if (strcmp(qryOrder.orderStatus, "REJECTED") == 0 || strcmp(qryOrder.orderStatus, "EXPIRED") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::NewRejected;
	} else if (strcmp(qryOrder.orderStatus, "NEW") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::New;
	} else if (strcmp(qryOrder.orderStatus, "CANCELED") == 0) {
		rspOrder.OrderMsgType = RtnOrderType;
		rspOrder.OrderStatus = OrderStatus::Cancelled;
	} else if (strcmp(qryOrder.orderStatus, "PARTIALLY_FILLED") == 0) {
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.OrderStatus = OrderStatus::Partiallyfilled;
	} else if (strcmp(qryOrder.orderStatus, "FILLED") == 0) {
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.OrderStatus = OrderStatus::Filled;
	}

	orderStore_->storeHandle(std::move(rspOrder));
	return 0;
}

int BianTdSpi::httpAsynReqCallback(char * result, uint64_t clientId)
{
	if (result == nullptr || strlen(result) == 0) {
		LOG_ERROR << "BianTdSpi httpAsynReqCallback result error clientId: " << clientId;
		return -1;
		// LOG_ERROR << "BianTdSpi httpAsynReqCallback result error clientId: " << clientId;
		// Order rspOrder;
		// memcpy(rspOrder.CounterType, InterfaceBinance.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBinance.size()));
		// rspOrder.OrderRef = clientId;
		// rspOrder.OrderStatus  = OrderStatus::NewRejected;
		// orderStore_->storeHandle(std::move(rspOrder));
		// return -1;
	}
	LOG_INFO << "BianTdSpi httpAsynReqCallback: " << result << ", clientId: " << clientId;
	BianRspReqOrder rspOrderInsert;
	auto ret = rspOrderInsert.decode(result);//client_oid

	if (ret != 0) {
		LOG_ERROR << "BianTdSpi httpAsynReqCallback BianRspReqOrder decode fail: " << result << ", clientId: " << clientId;
		cout << "BianTdSpi httpAsynReqCallback BianRspReqOrder decode fail: " << result << ", clientId: " << clientId << endl;

		// std::lock_guard<std::mutex> lock(orderMapMutex_);
		// auto ordermap =  OrderController::orderMap();
		// auto it = ordermap.find(clientId);
		// if (it == ordermap.end()) {
		// 	LOG_ERROR << "bian req Rsp Order can not find clientId: " << clientId;
		// 	return -1;
		// }
		// Order rspOrder;
		// memcpy(&rspOrder, &(it->second), sizeof(Order));
		// rspOrder.OrderStatus  = OrderStatus::NewRejected;
		// orderStore_->storeHandle(std::move(rspOrder));
		Order rspOrder;
		memcpy(rspOrder.CounterType, InterfaceBinance.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBinance.size()));
		rspOrder.OrderRef = clientId;
		rspOrder.OrderStatus  = OrderStatus::NewRejected;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	return 0;
}

int BianTdSpi::ReqOrderInsert(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	// MeasureFunc f(1);
	logInfoInnerOrder("BianTdSpi ReqOrderInsert innerOrder", innerOrder);

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

int BianTdSpi::httpAsynCancelCallback(char * result, uint64_t clientId)
{
	// MeasureFunc f(6);
	if (result == nullptr || strlen(result) == 0) {
		LOG_FATAL << "BianTdSpi httpAsynCancelCallback result error clientId: " << clientId;
		// LOG_ERROR << "BianTdSpi httpAsynCancelCallback result error clientId: " << clientId;
		// Order rspOrder;
		// memcpy(rspOrder.CounterType, InterfaceBinance.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBinance.size()));
		// rspOrder.OrderRef = clientId;
		// rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		// orderStore_->storeHandle(std::move(rspOrder));
		// return -1;
	}
	LOG_DEBUG << "BianTdSpi httpAsynCancelCallback: " << result << ", clientId: " << clientId;
    BianRspCancelOrder rspCancleOrder;
    int ret = rspCancleOrder.decode(result);

	if (ret != 0) {
		LOG_ERROR << "BianTdSpi httpAsynCancelCallback BianRspCancelOrder decode fail" << result<< ", clientId: " << clientId;
		cout << "BianTdSpi httpAsynCancelCallback BianRspCancelOrder decode fail" << result << ", clientId: " << clientId<< endl;
		Order rspOrder;
		memcpy(rspOrder.CounterType, InterfaceBinance.c_str(), min(sizeof(rspOrder.CounterType) - 1, InterfaceBinance.size()));

		rspOrder.OrderRef = clientId;
		rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}

	return 0;
}


int BianTdSpi::ReqOrderCancel(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	logInfoInnerOrder("BianTdSpi ReqOrderCancel innerOrder", innerOrder);
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

int BianTdSpi::Query(const Order &order)
{
	int ret = 0;
	if (order.QryFlag == QryType::QRYPOSI) {
		ret = traderApi_->QryPosiBySymbol(order);
	} else if (order.QryFlag == QryType::QRYMPRICE) {
		// ret = traderApi_->QryAsynInfo();
	} else if (order.QryFlag == QryType::QRYORDER) {
		ret = traderApi_->QryAsynOrder(order);
	}
	
	return ret;
}