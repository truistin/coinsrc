#include <spot/gateway/OKxV5TdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Compatible.h>
#include <functional>
#include "spot/base/Instrument.h"
#include "spot/base/InstrumentManager.h"
#include "spot/okex/OkSwapApi.h"
#include "spot/base/InitialData.h"
#include <spot/utility/Logging.h>
#include <functional>
#include <spot/utility/gzipwrapper.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <spot/strategy/OrderController.h>
#include <spot/utility/MeasureFunc.h>
#include "spot/cache/CacheAllocator.h"

using namespace std;
using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spotrapidjson;

#define OKEX_V5_TRADE_DOMAIN  "wss://wspap.okx.com:8443/ws/v5/public?brokerId=9999"

OKxV5TdSpi::OKxV5TdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore)
{
	loginFlag_ = false;
	apiInfo_ = apiInfo;
	orderStore_ = orderStore;
	std::string apiKey = apiInfo_->userId_;
	std::string secret_key = apiInfo_->passwd_;

//	std::string apiKey = "1a9b9ff1-56a7-47c5-acf2-e7cd22b365b6";
//	std::string secret_key = "4BE5DA99CF3159CE1E1FD551E146DFF8 ";

	std::string customerId = apiInfo_->investorId_;
	traderApi_ = new OkSwapApi(apiKey, secret_key, customerId, this);
	LOG_INFO << "OkSwapApi apikey:" << apiKey << ", secret_key: " << secret_key << " passwd:" << customerId << " skey-len:" << secret_key.length();
}

OKxV5TdSpi::~OKxV5TdSpi()
{
}

void OKxV5TdSpi::Init()
{
	// MeasureFunc::addMeasureData(2, "OKxV5TdSpi ReqOrderInsert time calc", 1);
	// MeasureFunc::addMeasureData(5, "OKxV5TdSpi RspHttptime calc", 10);
	//setTradeLoginStatusMap(InterfaceOKExV5, true);
	char *buff = CacheAllocator::instance()->allocate(sizeof(WebSocketApi));
	okWebsocketApi_ = new (buff) WebSocketApi();
	
	okWebsocketApi_->SetUri(apiInfo_->frontTdAddr_);
	okWebsocketApi_->SetCallBackOpen(std::bind(&OKxV5TdSpi::com_callbak_open, this));
	okWebsocketApi_->SetCallBackMessage(std::bind(&OKxV5TdSpi::com_login_message, this, std::placeholders::_1));
	okWebsocketApi_->SetCallBackClose(std::bind(&OKxV5TdSpi::com_callbak_close, this));

	LOG_INFO << "apiInfo_->frontTdAddr_: " << apiInfo_->frontTdAddr_;

	std::thread OKxV5TdSpiSocket(std::bind(&WebSocketApi::Run, okWebsocketApi_));
	pthread_setname_np(OKxV5TdSpiSocket.native_handle(), "OKxV5TdSocket");
	OKxV5TdSpiSocket.detach();

	std::thread OKxV5TdRun(&OKxV5TdSpi::Run, this);
	pthread_setname_np(OKxV5TdRun.native_handle(), "OKxV5TdRun");
	OKxV5TdRun.detach();
	// SLEEP(5000);
	

}

void OKxV5TdSpi::Run()
{
	while (true)
	{
		// SLEEP(InitialData::getOrderQueryIntervalMili());
		// SLEEP(20000);
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		string s = "ping";
		// cout<<"OKxV5TdSpi Run ping time: " << CURR_MSTIME_POINT;
		okWebsocketApi_->send(s);
		// MutexLockGuard lock(mutex1_);

		// traderApi_->QryAsyPosiMPriceInfo();
		// if (gOkPingPongFlag) {
		// 	gOkPingPongFlag = false;
		// 	string s = "ping";
		// 	okWebsocketApi_->send(s);
		//	cout << "gOkPingPongFlag 10s" <<endl;
		// }
	}
}

int OKxV5TdSpi::httpAsynReqCallback(char * result, uint64_t clientId)
{
	// MeasureFunc f(5);
	LOG_INFO << "result for httpAsynReqCallback: " << result << ", clientId: " << clientId;
	if (result == nullptr || strlen(result) == 0) {
		LOG_ERROR << "OKxV5TdSpi httpAsynReqCallback result error clientId: " << clientId << ", result: " << result;
		std::lock_guard<std::mutex> lock(orderMapMutex_);
		auto ordermap =  OrderController::orderMap();
		auto it = ordermap.find(clientId);
		if (it == ordermap.end()) {
			LOG_ERROR << "okex req Rsp Order can not find clientId: " << clientId;
			return -1;
		}
		Order rspOrder;
		memcpy(&rspOrder, &(it->second), sizeof(Order));
		rspOrder.OrderStatus  = OrderStatus::NewRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	uint64_t t0 = CURR_USTIME_POINT;
	// LOG_INFO << "OKxV5TdSpi httpAsynReqCallback t0: " << t0 << ", result" << result;

	OKEXSwapRspOrderInsert rspOrderInsert;
	auto ret = rspOrderInsert.decode(result);
	if ((ret != 0) || (strcmp(rspOrderInsert.statusCode, "0") != 0)) {
		LOG_ERROR << "OKxV5TdSpi httpAsynReqCallback OKEXSwapRspOrderInsert decode fail" << result << ", clientId: " << clientId;
		cout << "OKxV5TdSpi httpAsynReqCallback OKEXSwapRspOrderInsert decode fail" << result  << ", clientId: " << clientId << endl;
		std::lock_guard<std::mutex> lock(orderMapMutex_);
		auto ordermap =  OrderController::orderMap();
		auto it = ordermap.find(clientId);
		if (it == ordermap.end()) {
			LOG_ERROR << "okex req Rsp Order can not find clientId: " << clientId;
			return -1;
		}
		Order rspOrder;
		memcpy(&rspOrder, &(it->second), sizeof(Order));
		rspOrder.OrderStatus  = OrderStatus::NewRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	return 0;
}

int OKxV5TdSpi::ReqOrderInsert(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	// MeasureFunc f(2);
	uint64_t t0 = CURR_USTIME_POINT;
	// LOG_INFO << "OKxV5TdSpi ReqOrderInsert t0: " << t0;
	logInfoInnerOrder("OKxV5TdSpi ReqOrderInsert innerOrder", innerOrder);
	//string result;
	//result = traderApi_->OrderInsert(&innerOrder, std::to_string(innerOrder.OrderRef));

	traderApi_->OrderInsert(&innerOrder, std::to_string(innerOrder.OrderRef));
	return 0;
// 	LOG_INFO << "OKxV5TdSpi ReqOrderInsert result: " << result;

// 	OKEXSwapRspOrderInsert rspOrderInsert;
// 	auto ret = rspOrderInsert.decode(result.c_str());
//     Order rspOrder;
//     memcpy(&rspOrder, &innerOrder, sizeof(Order));
//     rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;

// 	if (ret != 0)
// 	{
// 		LOG_ERROR << "Network Error - Unable to send New trade to OKEx V5R - Http Restful not returning a result, Error = " << result;
// 		rspOrder.OrderMsgType = RtnRejectType;
// 		rspOrder.OrderStatus = OrderStatus::NewRejected;
// 		orderStore_->storeHandle(&rspOrder);
// 		return -1;
// 	}
// 	else
// 	{
// 		if (strcmp(rspOrderInsert.statusCode, "0") != 0)
// 		{
// 			LOG_ERROR << "Order New Rejected by OKEX V5 error_code  = " << rspOrderInsert.error_code << ", error_message: " << rspOrderInsert.error_message;
// 			rspOrder.OrderMsgType = RtnRejectType;
// 			rspOrder.OrderStatus = OrderStatus::NewRejected;
// 			orderStore_->storeHandle(&rspOrder);
// 			return -1;
// 		}
// 		else
// 		{
// 			rspOrder.OrderMsgType = RtnOrderType;
// 			rspOrder.OrderStatus = OrderStatus::New;
// 			orderStore_->storeHandle(&rspOrder);
// 			return 0;
// 		}
// 	}
// 	return 0;
// }
}

int OKxV5TdSpi::httpAsynCancelCallback(char * result, uint64_t clientId)
{
	LOG_INFO << "result for httpAsynCancelCallback: " << result << ", clientId: " << clientId;
	if (result == nullptr || strlen(result) == 0) {
		LOG_ERROR << "OKxV5TdSpi httpAsynCancelCallback result error clientId: " << clientId << ", result: " << result;
		std::lock_guard<std::mutex> lock(orderMapMutex_);
		auto ordermap =  OrderController::orderMap();
		auto it = ordermap.find(clientId);
		if (it == ordermap.end()) {
			LOG_ERROR << "okex cancel Rsp Order can not find clientId: " << clientId;
			return -1;
		}
		Order rspOrder;
		memcpy(&rspOrder, &(it->second), sizeof(Order));
		rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}
	OKEXSwapRspOrderCancel rspOrderCancel;
	int ret = rspOrderCancel.decode(result);//client_oid

	if ((ret != 0) || (strcmp(rspOrderCancel.statusCode, "0") != 0)) {
		LOG_ERROR << "OKxV5TdSpi httpAsynCancelCallback OKEXSwapRspOrderCancel decode fail" << result;
		cout << "OKxV5TdSpi httpAsynCancelCallback OKEXSwapRspOrderCancel decode fail" << result << endl;
		std::lock_guard<std::mutex> lock(orderMapMutex_);
		auto ordermap =  OrderController::orderMap();
		auto it = ordermap.find(clientId);
		if (it == ordermap.end()) {
			LOG_ERROR << "okex cancel Rsp Order can not find clientId: " << clientId;
			return -1;
		}
		Order rspOrder;
		memcpy(&rspOrder, &(it->second), sizeof(Order));
		rspOrder.OrderStatus  = OrderStatus::CancelRejected;
		orderStore_->storeHandle(std::move(rspOrder));
		return -1;
	}

	return 0;
}

int OKxV5TdSpi::ReqOrderCancel(const Order &innerOrder)
{
	//std::lock_guard<std::mutex> lock(mutex_);
	logInfoInnerOrder("OKxV5TdSpi ReqOrderCancel innerOrder", innerOrder);
	//string result;
	traderApi_->OrderCancel(innerOrder.InstrumentID, std::to_string(innerOrder.OrderRef));
	return 0;
	//LOG_INFO << "OKxV5TdSpi ReqOrderCancel result: " << result;

	// Order rspOrder;
	// memcpy(&rspOrder, &innerOrder, sizeof(Order));
	// rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
	// OKEXSwapRspOrderCancel rspOrderCancel;
	// auto ret = rspOrderCancel.decode(result.c_str());

	// if (ret != 0)
	// {
	// 	rspOrder.OrderStatus = OrderStatus::CancelRejected;
	// 	rspOrder.OrderMsgType = RtnRejectType;
	// 	orderStore_->storeHandle(&rspOrder);
	// 	LOG_INFO << "OKxV5TdSpi ReqOrderCancel fail result: " << result;
	// 	return -1;
	// } else {
	// 	if (strcmp(rspOrderCancel.statusCode, "0") != 0)
	// 	{
	// 		LOG_ERROR << "Cancel Order Rejected by OKEX V5 error_code  = " << rspOrderCancel.statusCode << ", error_message: "<< rspOrderCancel.statusMsg;
	// 		rspOrder.OrderStatus = OrderStatus::CancelRejected;
	// 		rspOrder.OrderMsgType = RtnRejectType;
	// 		orderStore_->storeHandle(&rspOrder);
	// 		return -1;
	// 	}
	// 	else
	// 	{
	// 		LOG_INFO << "Cancel Order request submitted sucessfully, result will follow later, orderid =  " << innerOrder.OrderRef;
	// 	}
	// }
	// return 0;
}

void OKxV5TdSpi::PutOrderToQueue(OKxWssSwapOrder &queryOrder, Order &preOrder)
{
	//LOG_INFO << "recv order record. okex Order:" << queryOrder.to_string();
	Order rspOrder;
	memcpy(&rspOrder, &preOrder, sizeof(Order));

	rspOrder.EpochTimeReturn = CURR_MSTIME_POINT;
	rspOrder.TimeStamp = atoll(queryOrder.dealTimeStamp);

	if (strcmp(queryOrder.state, "live") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::New;
		rspOrder.OrderMsgType = RtnOrderType;
		orderStore_->storeHandle(std::move(rspOrder));
	}
	else if (strcmp(queryOrder.state ,"filled") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::Filled;
		rspOrder.Volume = atof(queryOrder.volume);
		rspOrder.VolumeFilled = atof(queryOrder.volumeFilled);
		rspOrder.VolumeRemained = rspOrder.VolumeTotalOriginal - rspOrder.VolumeFilled;

		rspOrder.Price = atof(queryOrder.price);
		rspOrder.AvgPrice = atof(queryOrder.avgPrice);

		rspOrder.Fee = atof(queryOrder.fee);
		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.TradeID = atoi(queryOrder.tradeID);

		orderStore_->storeHandle(std::move(rspOrder));
	//	LOG_INFO << "Fill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
	}
	else if (strcmp(queryOrder.state ,"partially_filled") == 0)
	{
		rspOrder.OrderStatus = OrderStatus::Partiallyfilled;
		rspOrder.Volume = atof(queryOrder.volume);
		rspOrder.VolumeFilled = atof(queryOrder.volumeFilled);
		rspOrder.VolumeRemained = rspOrder.VolumeTotalOriginal - rspOrder.VolumeFilled;

		rspOrder.Price = atof(queryOrder.price);
		rspOrder.AvgPrice = atof(queryOrder.avgPrice);

		rspOrder.OrderMsgType = RtnTradeType;
		rspOrder.Fee = atof(queryOrder.fee);
		rspOrder.TradeID = atoi(queryOrder.tradeID);

		orderStore_->storeHandle(std::move(rspOrder));
		//LOG_INFO << "PartifiallyFill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
	}
	else if (strcmp(queryOrder.state ,"canceled") == 0)
	{
			rspOrder.OrderStatus = OrderStatus::Cancelled;
			rspOrder.OrderMsgType = RtnOrderType;
			rspOrder.VolumeFilled = atof(queryOrder.volumeFilled);
			LOG_INFO << "OKxV5TdSpi PutOrderToQueue CANCELED OrderRef: " << rspOrder.OrderRef 
				<< ", Volume : " << rspOrder.Volume;

			orderStore_->storeHandle(std::move(rspOrder));
	}
	else
	{
		LOG_ERROR << "status " << "OrderSysID " << queryOrder.clOrdId << " OrderRef " << preOrder.OrderRef << " OrderStatus " << rspOrder.OrderStatus << " old Status " << preOrder.OrderStatus;
	}
}

string OKxV5TdSpi::CreateSignature(string& timestamp, string& method, string& requestPath, 
	string& sk, const char* encode_mode)
{

	string signStr = timestamp + method + requestPath;

	unsigned char* output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	unsigned int output_length = 0;
	HmacEncode(encode_mode,  sk.c_str(), signStr.c_str(), output, &output_length);
	string sig_base64 = websocketpp::base64_encode(output, output_length);

	if (output)
	{
		free(output);
	}
	return sig_base64;
}

void OKxV5TdSpi::com_callbak_open()
{
	auth();
	okWebsocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&OKxV5TdSpi::com_login_message, this, std::placeholders::_1));
	std::cout << "OKxV5TdSpi callback open! " << std::endl;
	LOG_INFO << "OKxV5TdSpi callback open! ";
}

void OKxV5TdSpi::com_callbak_close()
{
	std::cout << "OKxV5TdSpi callback close! " << std::endl;
	LOG_INFO << "OKxV5TdSpi callback close! ";
}

void OKxV5TdSpi::com_login_message(const char *message)
{
	if (message == nullptr) {
		LOG_ERROR << "OKxV5TdSpi com_login_message message nullptr: ";
	}
	LOG_INFO << "com_login_message message: " << message;

	if (strcmp(message, "pong") == 0) {
		return;
	} else {
		LOG_INFO << "OKxV5TdSpi com_login_message err: " << message;
	}

	Document doc;
	doc.Parse(message, strlen(message));
	if (doc.HasParseError())
	{
		LOG_FATAL << "com_login_message Parse error. result:" << message;
		return;
	}

	if (doc.HasMember("event") && doc.HasMember("code"))
	{
		string event = doc["event"].GetString();
		string code = doc["code"].GetString();
		if (strcmp("login", event.c_str()) == 0 && strcmp("0", code.c_str()) == 0) {
			subscribe("orders");
			okWebsocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&OKxV5TdSpi::com_subscribe_message, this, std::placeholders::_1));
			cout << "change com_subscribe_message already" << endl;
			return;
		} else {
			LOG_FATAL << "com_login_message login err" << message;
		}
		return;
	} else {
		LOG_FATAL << "com_login_message login err" << message;
	}
}

void OKxV5TdSpi::com_subscribe_message(const char *message)
{
	LOG_INFO << "com_subscribe_message message: " << message;
	if (strcmp(message, "pong") == 0) {
		return;
	} else {
		LOG_INFO << "com_subscribe_message unknown: " << message;
	}
	Document doc;
	doc.Parse(message, strlen(message));
	if (doc.HasParseError())
	{
		LOG_FATAL << "com_subscribe_message Parse error. result:" << message;
		return;
	}

	if (doc.HasMember("event") && doc.HasMember("arg")) {
		okWebsocketApi_->UpdateCallBackMessageInWebSocket(std::bind(&OKxV5TdSpi::com_callbak_message, this, std::placeholders::_1));
		cout << "change com_callbak_message already" << endl;
		// traderApi_->CancelAllOrders();
		return;
	} else {
		LOG_FATAL << "com_subscribe_message subscribe err" << message;
	}
}

void OKxV5TdSpi::com_callbak_message(const char *message)
{
	if (message == nullptr || strlen(message) == 0) {
		LOG_ERROR << "com_callbak_message OKxWssSwapOrder error";
		return;
	}

	//std::lock_guard<std::mutex> lock(mutex_);
	if (strcmp(message, "pong") == 0) {
		LOG_DEBUG << "com_callbak_message PONG: " << message;
		return;
	} else {
		LOG_INFO << "com_callbak_message OKxWssSwapOrder: " << message;
	}
	OKxWssStruct rspOrder;
	int ret = rspOrder.decode(message);
	if (ret != 0) {
		LOG_ERROR << "OKxV5TdSpi OKxWssSwapOrder error:" << message;
	}
	
	//const unordered_map<int, Order>& ordermap =  OrderController::orderMap();
	//const unordered_map<int, Order>::iterator it = ordermap.find(atoi(rspOrder.clOrdId));
	std::lock_guard<std::mutex> lock(orderMapMutex_);
	auto ordermap =  OrderController::orderMap();

	for (auto& iter : rspOrder.vecOrd_) {
		auto it = ordermap.find(atoi(iter.clOrdId));
		//const unordered_map<int, Order>::iterator it = ordermap.find(atoi(rspOrder.clOrdId));
		if (it != ordermap.end()) {
			PutOrderToQueue(iter, it->second);
		} else {
			LOG_ERROR << "bian Rsp Order can not find: " << iter.clOrdId << ", message: " << message;
		}
	}
}

void OKxV5TdSpi::auth() {
	//post arams: {"op":"login","args":[{"channel":"tickers","instId":"BTC-USDT-SWAP"}]}
	string ak = apiInfo_->userId_;
	string passphrase = apiInfo_->investorId_;
	string timestamp = std::to_string(CURR_STIME_POINT);
	string method = "GET";
	string path = "/users/self/verify";
	string sk = apiInfo_->passwd_;

	string sign = CreateSignature(timestamp, method, path, sk, "sha256");

    string str =
        "{ \"op\": \"login\", \"args\" : [{\"apiKey\":\"" + ak + "\",\"passphrase\":\"" 
			+ passphrase + "\",\"timestamp\":\"" + timestamp + "\",\"sign\":\"" + sign + "\"}] }";

    LOG_INFO << "OKEx_V5 login timestamp : " << timestamp
             << ", sk: " << sk << ", sign: " << sign << "str" << str;
    okWebsocketApi_->send(str);
}

void OKxV5TdSpi::subscribe(const std::string &channel)
{
    string str =
        "{ \"op\": \"subscribe\", \"args\" : [{\"channel\":\"" + channel + "\",\"instType\":\"" + "ANY" + "\"}] }";

    LOG_INFO << "OKEx_V5 Subscribe orders : " << channel << ", str: " << str;
    okWebsocketApi_->send(str);
}

int OKxV5TdSpi::Query(const Order &order)
{
	int ret = 0;
	if (order.QryFlag == QryType::QRYPOSI) {
		ret = traderApi_->QryPosiBySymbol(order);
	} else if (order.QryFlag == QryType::QRYMPRICE) {
		// ret = traderApi_->QryAsynInfo();
	}
	
	return ret;
}