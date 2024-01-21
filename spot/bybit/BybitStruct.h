#ifndef __BYBITSTRUCT_H__
#define __BYBITSTRUCT_H__
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/stringbuffer.h>
#include <spot/rapidjson/writer.h>
#include <functional>
#include <sstream>
#include <list>
#include <iostream>
#include "spot/utility/algo_hmac.h"
#include <chrono>
#include <cinttypes>
#include <ctime>
#include <spot/base/MqStruct.h>
#include <spot/base/DataStruct.h>
#include <spot/utility/Logging.h>
#include <vector>
#ifdef __LINUX__
#include <time.h>
#endif

using namespace spot::base;
using namespace std;
using namespace std::placeholders;
using namespace spotrapidjson;
namespace spot
{
class  BybitQryPositionInfo
{
public:
    BybitQryPositionInfo()
    {
        memset(this, 0, sizeof(BybitQryPositionInfo));
    }
    char instrumentID[InstrumentIDLen + 1];
    char symbol[SymbolLen + 1];
	double qty;
    double avgPrice;
	double riskLimitValue;
	double bustPrice;
    int leverage;
    double liqPrice;
    double markPrice;
    double unrealisedPnl;
    double cumRealisedPnl;
	uint64_t createdTime;
    uint64_t updateTime;
	double positionIM; // posi init deposit
	double positionMM; // posi sustain deposit

    void to_string() {
        ostringstream os;
        os << "BybitQryPositionInfo";
        os << ",instrument_id:" << instrumentID;
        os << ",qty:" << qty;
		os << ",symbol:" << symbol;
        os << ",avgPrice:" << avgPrice;
		os << ",riskLimitValue:" << riskLimitValue;
		os << ",bustPrice:" << bustPrice;
        os << ",leverage:" << leverage;
        os << ",liqPrice:" << liqPrice;
        os << ",markPrice:" << markPrice;
        os << ",unrealisedPnl:" << unrealisedPnl;
        os << ",cumRealisedPnl:" << cumRealisedPnl;
        os << ",updateTime:" << updateTime;
		os << ",positionIM:" << positionIM;
		os << ",positionMM:" << positionMM;
    }
};

class BybitQueryOrder
{
public:
	BybitQueryOrder()
	{
		memset(this, 0, sizeof(BybitQueryOrder));
	}
public:
	char		orderStatus[40+1];
	char 		orderId[128+1];
	double 		volumeFilled;
	double		volumeTotalOriginal;
	double		volumeRemained;

	int decode(const char* json) {
	try{
        Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BybitQueryOrder::Parse error. result:" << json;
            return -1;
        }

		if (doc.HasMember("retCode"))
		{
			if (doc["retCode"].IsNumber())
			{
				int errorCode = doc["retCode"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("retMsg"))
					{
						if (doc["retMsg"].IsString())
						{
							errorMsg = doc["retMsg"].GetString();
						}
					}
					LOG_ERROR << "BybitQueryOrder retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json;
					cout << "BybitQueryOrder retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json << endl;
					return errorCode;
				}
			}
		}

		spotrapidjson::Value& node = doc["result"];
		spotrapidjson::Value& dataNodes = node["list"];

		if (dataNodes.Size() == 0) {
		    LOG_WARN << "BybitQueryOrder has no position " << doc.Size();
		    return -3;
	   	}

		spotrapidjson::Value& dataNode = dataNodes[0];
		Value &vstatus = dataNode["orderStatus"];
		if (vstatus.IsString()){
			std::string s = vstatus.GetString();
			memcpy(orderStatus, s.c_str(), min(sizeof(orderStatus) - 1, (s.size())));
		}

		Value &vorderRef = dataNode["orderLinkId"];
		if (vorderRef.IsString()){
			std::string s = vorderRef.GetString();
			memcpy(orderId, s.c_str(), min(sizeof(orderId) - 1, (s.size())));
		}

		Value &vVolFilled = dataNode["cumExecQty"];
		if (vVolFilled.IsString()){
			std::string s = vVolFilled.GetString();
			volumeFilled = stod(s);
		}

		Value &vinputVol = dataNode["qty"];
		if (vinputVol.IsString()){
			std::string s = vinputVol.GetString();
			volumeTotalOriginal = stod(s);
		}

		Value &vremain = dataNode["leavesQty"];
		if (vremain.IsString()){
			std::string s = vremain.GetString();
			volumeRemained = stod(s);
		}

		return 0;

	} catch (std::exception ex) {
			LOG_WARN << "BybitQueryOrder exception occur:" << ex.what();
			cout << "BybitQueryOrder exception occur:" << ex.what();
			return -1;
	}
	}
};

class BybitQryStruct
{
public:
	BybitQryStruct()
	{
		// memset(this, 0, sizeof(BybitQryStruct)); has non pod objuct (vector)
	}
    int decode(const char* json) {
		try{
        Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BybitQryPositionInfo::Parse error. result:" << json;
            return -1;
        }

		if (doc.HasMember("retCode"))
		{
			if (doc["retCode"].IsNumber())
			{
				int errorCode = doc["retCode"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("retMsg"))
					{
						if (doc["retMsg"].IsString())
						{
							errorMsg = doc["retMsg"].GetString();
						}
					}
					LOG_ERROR << "BybitQryPositionInfo retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json;
					cout << "BybitQryPositionInfo retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json << endl;
					return errorCode;
				}
			}
		}

		spotrapidjson::Value& node = doc["result"];

		if (!node.HasMember("list")) {
			LOG_ERROR << "BybitQryPositionInfo list json: " << json;
			cout << "BybitQryPositionInfo list json: " << json;
		}

		spotrapidjson::Value& dataNodes = node["list"];

	   	if (dataNodes.Size() == 0) {
		    LOG_WARN << "BybitQryPositionInfo has no position " << doc.Size();
		    return 2;
	   	}
       	else if(dataNodes.Size() != 1) {
            LOG_ERROR << "BybitQryPositionInfo size error " << doc.Size();
		    return -1;
      	 }
		for (int i = 0; i < dataNodes.Size(); i++) {
			spotrapidjson::Value& dataNode = dataNodes[i];
			commQryPosiNode qryOrder;
			Value &vsymbol = dataNode["symbol"];
			if (vsymbol.IsString()){
				std::string s = vsymbol.GetString();
				memcpy(qryOrder.symbol, s.c_str(), min(InstrumentIDLen, uint16_t(s.size())));
			}
			
			Value &vsize = dataNode["size"];
			if (vsize.IsString()) {
				string str = vsize.GetString();
				if (str.size() != 0) {
					qryOrder.size = (stod(str));
				}
			}

			Value &vliquidationPrice = dataNode["liqPrice"];
			if (vliquidationPrice.IsString()) {
				string str = vliquidationPrice.GetString();
				if (str.size() != 0) {
					qryOrder.liqPrice = stod(str);
				}
			}
			else
				qryOrder.liqPrice = vliquidationPrice.GetDouble();

			Value &vtime = dataNode["updatedTime"];
			if (vtime.IsString()) {
				if (strlen(vtime.GetString()) != 0) {
					qryOrder.updatedTime = stoull(vtime.GetString());
				}
			} else
				qryOrder.updatedTime = vtime.GetUint64();


			Value &vpositionSide = dataNode["side"];
			if (vpositionSide.IsString()){
				std::string s = vpositionSide.GetString();
				memcpy(qryOrder.side, s.c_str(), min(DateLen, uint16_t(s.size())));
			}

			vecOrd_.push_back(qryOrder);
		}
		
		}
		catch (std::exception ex) {
			LOG_WARN << "BybitQryPositionInfo exception occur:" << ex.what();
			cout << "BybitQryPositionInfo exception occur:" << ex.what();
			return -1;
		}
		return 0;
	}

public:
	vector<commQryPosiNode> vecOrd_;
};

class  BybitMrkQryInfo
{
public:
	BybitMrkQryInfo()
	{
		memset(this, 0, sizeof(BybitMrkQryInfo));
	}
	char instrumentID[InstrumentIDLen + 1];
	char category[InstrumentIDLen + 1];
	double markPrice;
	double lastPrice;
	double fundingRate; 
	uint64_t nextFundingTime; 
	double interestRate; 
	uint64_t updateTime; 
	string to_string() {
		ostringstream os;
		os << "BybitMrkQryInfo";
		os << ",instrument_id:" << instrumentID;
		os << ",markPrice:" << markPrice;
		os << ",lastPrice:" << lastPrice;
		os << ",fundingRate:" << fundingRate;
		os << ",nextFundingTime:" << nextFundingTime;
		os << ",interestRate:" << interestRate;
		os << ",updateTime:" << updateTime;
		return os.str();
	}

};

class  BybitMrkStruct
{
public:
	BybitMrkStruct()
	{
		// memset(this, 0, sizeof(BybitMrkStruct)); has non pod objuect(vector)
		memset(category, 0, sizeof(category));
		updateTime = 0;
	}
	int decode(const char* json) {
		try {
		Document doc;
		doc.Parse(json, strlen(json));

		if (doc.HasParseError())
		{
			LOG_WARN << "BybitMrkQryInfo::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("retCode"))
		{
			if (doc["retCode"].IsNumber())
			{
                int errorCode = doc["retCode"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("retMsg"))
					{
						if (doc["retMsg"].IsString())
						{
							errorMsg = doc["retMsg"].GetString();
						}
					}
					LOG_WARN << " BybitApi::QryInfo result:" << errorCode << ", errorMsg:" << errorMsg;
					cout << " BybitApi::QryInfo result:" << errorCode << ", errorMsg:" << errorMsg << endl;
					return errorCode;
				}
			}
		}

		Value &vtime = doc["time"];
		if (vtime.IsString())
			updateTime = stoull(vtime.GetString());
		else
			updateTime = vtime.GetUint64();

		Value &result = doc["result"];

		Value &vcategory = result["category"];
		if (vcategory.IsString())
        {
            std::string c = vcategory.GetString();
            memcpy(category, c.c_str(), min(sizeof(category) - 1, c.size()));
        }

		Value &list = result["list"];

		if (list.Size() == 0) {
			cout << "BybitMrkQryInfo list len error";
			LOG_ERROR << "BybitMrkQryInfo list len error";
		}

	if (list.IsArray()) {
			for (int i = 0; i < list.Capacity(); ++i) {
				spotrapidjson::Value &dataNode = list[i];
				commQryMarkPxNode qryInfo;
				Value &vsymbol = dataNode["symbol"];
				if (vsymbol.IsString())
				{
					std::string s = vsymbol.GetString();
					memcpy(qryInfo.symbol, s.c_str(), min(sizeof(qryInfo.symbol) - 1, s.size()));
				}

				Value &vmarkprice = dataNode["markPrice"];
				if (vmarkprice.IsString()) {
					string str = vmarkprice.GetString();
					if (str.size() != 0) {
						qryInfo.markPx = stod(str);
					}
				}
				else
					qryInfo.markPx = vmarkprice.GetDouble();

				Value &vfundingrate = dataNode["fundingRate"];
				if (vfundingrate.IsString()) {
					string str = vfundingrate.GetString();
					if (str.size() != 0) {
						qryInfo.fundingRate = stod(str);
					}
				}
				else
					qryInfo.fundingRate = vfundingrate.GetDouble();

				Value &vfundingtime = dataNode["nextFundingTime"];
				if (vfundingtime.IsString())
					qryInfo.updatedTime = stoull(vfundingtime.GetString());
				else
					qryInfo.updatedTime = vfundingtime.GetUint64();

				qryInfo.updatedTime = updateTime;
				vecOrd_.push_back(qryInfo);
			}
		} else {
			LOG_FATAL << "BybitMrkQryInfo size ERROR";
		}
	}

	catch (std::exception ex) {
        LOG_WARN << "BybitMrkQryInfo exception occur:" << ex.what();
		cout << "BybitMrkQryInfo exception occur:" << ex.what();
        return -1;
    }
	return 0;
	}
public:
	vector<commQryMarkPxNode> vecOrd_;
	char category[10+1];
	uint64_t updateTime;
};
class BybitRspReqOrder
{
public:
	BybitRspReqOrder()
	{
		memset(this, 0, sizeof(BybitRspReqOrder));
	}
	char orderId[128+1];
	uint64_t cOrderId;
	uint32_t retCode;
	char retMsg[128+1];
	char retExtInfo[128];
	uint64_t time;
	string to_string()
	{
		ostringstream os;
		os << "BybitRspReqOrder";
		os << "cOrderId:"<< cOrderId <<",id:" << orderId;
		return os.str();
	}
	int decode(const char* json)
	{
		try {
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "Bybit::CancelOrder Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("retMsg"))
		{
			if (doc["retMsg"].IsString())
			{
				string str = doc["retMsg"].GetString();
				memcpy(retMsg, str.c_str(), min(sizeof(retMsg) - 1, str.size()));

			}
		}

		if (doc.HasMember("time")) {
			if (doc["time"].IsString())
			{
				string str = doc["time"].GetString();
				time =  stoull(str);
			}
		}

		if (doc.HasMember("retExtInfo")) {
			if (doc["retExtInfo"].IsString())
			{
				string str = doc["retExtInfo"].GetString();
				memcpy(retExtInfo, str.c_str(), min(sizeof(retExtInfo) - 1, str.size()));
			}
		}

		if (doc.HasMember("result")) {
			spotrapidjson::Value& dataNode = doc["result"];
			if (dataNode.HasMember("orderId")) {
				if (dataNode["orderId"].IsString())
				{
					string str = dataNode["orderId"].GetString();
					memcpy(orderId, str.c_str(), min(sizeof(orderId) - 1, str.size()));
				}
			}

			if (dataNode.HasMember("orderLinkId")) {
				if (dataNode["orderLinkId"].IsString())
				{
					string str = dataNode["orderLinkId"].GetString();
					cOrderId =  stoull(str);
				}
			}
		}

		if (doc.HasMember("retCode"))
		{
			if (doc["retCode"].IsNumber())
			{
                retCode = doc["retCode"].GetInt();
				if (retCode != 0)
				{
					LOG_WARN << " Bybit::BybitRspReqOrder result: " << json << ", errorCode: " << retCode<<", errorMsg:"<< retExtInfo
						<< ", retExtInfo: " << retExtInfo << ", retMsg: " << retMsg;
					// cout << " Bybit::BybitRspReqOrder result: " << json << ", errorCode: " << retCode<<", errorMsg:"<< retExtInfo
					// 	<< ", retExtInfo: " << retExtInfo << ", retMsg: " << retMsg << endl;
					return retCode;
				}
			}
		}
		return 0;	
	} catch (std::exception ex) {
        LOG_WARN << "BybitRspCancelOrder cancelOrder exception occur:" << ex.what();
		cout << "BybitRspCancelOrder cancelOrder exception occur:" << ex.what();
        return -1;
    }
	}
};
class BybitRspCancelOrder
{
public:
	BybitRspCancelOrder()
	{
		memset(this, 0, sizeof(BybitRspCancelOrder));
	}
	char orderId[128+1];
	uint64_t cOrderId;
	uint32_t retCode;
	char retMsg[128+1];
	char retExtInfo[128];
	uint64_t time;
	string to_string()
	{
		ostringstream os;
		os << "BybitRspCancelOrder";
		os << "cOrderId:"<< cOrderId <<",id:" << orderId;
		return os.str();
	}
	int decode(const char* json)
	{
	try{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "Bybit::CancelOrder Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("retMsg"))
		{
			if (doc["retMsg"].IsString())
			{
				string str = doc["retMsg"].GetString();
				memcpy(retMsg, str.c_str(), min(sizeof(retMsg) - 1, str.size()));

			}
		}

		if (doc.HasMember("time")) {
			if (doc["time"].IsString())
			{
				string str = doc["time"].GetString();
				time =  stoull(str);
			}
		}

		if (doc.HasMember("retExtInfo")) {
			if (doc["retExtInfo"].IsString())
			{
				string str = doc["retExtInfo"].GetString();
				memcpy(retExtInfo, str.c_str(), min(sizeof(retExtInfo) - 1, str.size()));
			}
		}

		if (doc.HasMember("result")) {
			spotrapidjson::Value& dataNode = doc["result"];
			if (dataNode.HasMember("orderId")) {
				if (dataNode["orderId"].IsString())
				{
					string str = dataNode["orderId"].GetString();
					memcpy(orderId, str.c_str(), min(sizeof(orderId) - 1, str.size()));
				}
			}

			if (dataNode.HasMember("orderLinkId")) {
				if (dataNode["orderLinkId"].IsString())
				{
					string str = dataNode["orderLinkId"].GetString();
					cOrderId =  stoull(str);
				}
			}
		}

		if (doc.HasMember("retCode"))
		{
			if (doc["retCode"].IsNumber())
			{
                retCode = doc["retCode"].GetInt();
				if (retCode != 0)
				{
					LOG_WARN << " Bybit::BybitRspCancelOrder result: " << json << ", errorCode: " << retCode<<", errorMsg:"<< retExtInfo
						<< ", retExtInfo: " << retExtInfo << ", retMsg: " << retMsg;
					// cout << " Bybit::BybitRspCancelOrder result: " << json << ", errorCode: " << retCode<<", errorMsg:"<< retExtInfo
					// 	<< ", retExtInfo: " << retExtInfo << ", retMsg: " << retMsg << endl;
					return retCode;
				}
			}
		}
		return 0;
	} catch (std::exception ex) {
        LOG_WARN << "BybitRspCancelOrder cancelOrder exception occur:" << ex.what();
		cout << "BybitRspCancelOrder cancelOrder exception occur:" << ex.what();
        return -1;
    }
	}
};

class BybitWssRspOrder
{
public:
	BybitWssRspOrder() {
		memset(this, 0, sizeof(BybitWssRspOrder));
	}
	~BybitWssRspOrder() {
	}
	// trade & order common
	char		message_id[InstrumentIDLen + 1];
	char		topic[InstrumentIDLen + 1];
	char		orderType[10+1]; // market or limit
	char		instrument_id[InstrumentIDLen + 1];
	char 		orderId[128+1];
	int			clOrdId; //orderRef order
	char		orderStatus[40+1];
	char		direction[10+1];
	char		category[10+1];
	double      limitPrice; // wei tuo price
	double 		volumeTotalOriginal; // wei tuo volume
	double		tradeAvgPrice;
	double		volumeFilled;  //accumulate filled volume
	double		volumeRemained; // left undeal volume
	char 		timeInForce[10+1]; 
	uint64_t	creationTime;
	uint64_t	updatedTime;

	// trade info
	double 		price;  // execPrice trade price
	double		volume; // execQty trade amount
	double		fee; // feeRate trade fee
	uint64_t	tradeTime; // execTime time
	char		tradeId[128+1]; //execId
	bool		isMaker; //maker:true taker:false
	double		markPrice; // markprice when trade 

	string to_string()
	{
		ostringstream os;
		os << "BybitWssRspOrder";
		os << ",message_id: " << message_id;
		os << ",topic: " << topic;
		os << ",instrument_id: " << instrument_id;
		os << ",orderType: " << orderType;
		os << ",orderId: " << orderId;
		os << ",clOrdId: " << clOrdId;
		os << ",orderStatus: " << orderStatus;
		os << ",direction: " << direction;
		os << ",category: " << category;
		os << ",limitPrice: " << limitPrice;
		os << ",volumeTotalOriginal: " << volumeTotalOriginal;
		os << ",tradeAvgPrice: " << tradeAvgPrice;
		os << ",volumeFilled: " << volumeFilled;
		os << ",volumeRemained: " << volumeRemained;
		os << ",creationTime: " << creationTime;
		os << ",updatedTime: " << updatedTime;
		os << ",price: " << price;
		os << ",volume: " << volume;
		os << ",tradeTime: " << tradeTime;
		os << ",tradeId: " << tradeId;
		os << ",isMaker: " << isMaker;
		os << ",markPrice: " << markPrice;
		return os.str();
	}
};

class BybitWssStruct
{
public:
	BybitWssStruct(){
		memset(message_id, 0, InstrumentIDLen + 1);
		memset(topic, 0, InstrumentIDLen + 1);
		creationTime = 0;
	}
public:
	int decode(const char* json)
	{
	try {
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "BybitWssRspOrder::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("op"))
		{
			if (doc["op"].IsString()) {
				string str = doc["op"].GetString();
				if (strcmp(str.c_str(), "pong") == 0) {
					return 2;
				}
			}
		}
		if (doc.HasMember("id"))
		{
			if (doc["id"].IsString()) {
				string str = doc["id"].GetString();
				memcpy(message_id, str.c_str(), min(sizeof(message_id) - 1, str.size()));

			}
		}

		if (doc.HasMember("topic"))
		{
			if (doc["topic"].IsString()) {
				string str = doc["topic"].GetString();
				memcpy(topic, str.c_str(), min(sizeof(topic) - 1, str.size()));

			}
		}

		if (doc.HasMember("creationTime"))
		{
			if (doc["creationTime"].IsString())
			{
				creationTime = stoull(doc["creationTime"].GetString());
			}
		}

		if (doc.HasMember("data"))
		{
			spotrapidjson::Value &dataNodes = doc["data"];
			if (!dataNodes.IsArray()) {
				cout << "BybitWssRspOrder decode failed: " << endl;
				LOG_FATAL << "BybitWssRspOrder decode failed: ";
				return -1;
			}

			if (dataNodes.Size() == 0) {
				cout << "BybitWssRspOrder has no position " << doc.Size() << endl;
				LOG_WARN << "BybitWssRspOrder has no position " << doc.Size();
				return -1;
			}
			for (int i = 0; i < dataNodes.Size(); i++) {
				BybitWssRspOrder rspOrder;
				memcpy(rspOrder.topic, topic, InstrumentIDLen+1);
				spotrapidjson::Value& dataNode = dataNodes[i];
				if (dataNode.HasMember("symbol"))
				{
					if (dataNode["symbol"].IsString())
					{
						string str = dataNode["symbol"].GetString();
						memcpy(rspOrder.instrument_id, str.c_str(), min(sizeof(rspOrder.instrument_id) - 1, str.size()));
					}
				}
				// to do
				if (dataNode.HasMember("orderId"))
				{
					string str = dataNode["orderId"].GetString(); // GetUint64();
					memcpy(rspOrder.orderId, str.c_str(), min(sizeof(rspOrder.orderId) - 1, str.size()));
					
				}

				if (dataNode.HasMember("orderLinkId"))
				{
					string str = dataNode["orderLinkId"].GetString(); // GetUint64();
					if (str.size() != 0) {
						rspOrder.clOrdId = stoi(str);
					}
				}

				if (dataNode.HasMember("orderType"))
				{
					string str = dataNode["orderType"].GetString(); // GetUint64();
					memcpy(rspOrder.orderType, str.c_str(), min(sizeof(rspOrder.orderType) - 1, str.size()));

				}

				if (dataNode.HasMember("orderStatus"))
				{
					string str = dataNode["orderStatus"].GetString(); // GetUint64();
					memcpy(rspOrder.orderStatus, str.c_str(), min(sizeof(rspOrder.orderStatus) - 1, str.size()));
				}

				if (dataNode.HasMember("side"))
				{
					if (dataNode["side"].IsString())
					{
						string str = dataNode["side"].GetString();
						memcpy(rspOrder.direction, str.c_str(), min(sizeof(rspOrder.direction) - 1, str.size()));
					}
				}

				if (dataNode.HasMember("category"))
				{
					if (dataNode["category"].IsString())
					{
						string str = dataNode["category"].GetString();
						memcpy(rspOrder.category, str.c_str(), min(sizeof(rspOrder.category) - 1, str.size()));
					}
				}

				if (dataNode.HasMember("price"))
				{
					if (dataNode["price"].IsString())
					{
						string str = dataNode["price"].GetString();
						if (str.size() != 0) {
							rspOrder.limitPrice = stod(str);
						}
						
					}
				}

				if (dataNode.HasMember("qty"))
				{
					if (dataNode["qty"].IsString())
					{
						string str =  dataNode["qty"].GetString();
						if (str.size() != 0) {
							rspOrder.volumeTotalOriginal = stod(str);
						}
					}
				}

				if (dataNode.HasMember("avgPrice"))
				{
					if (dataNode["avgPrice"].IsString())
					{
						string str =  dataNode["avgPrice"].GetString();
						if (str.size() != 0) {
							rspOrder.tradeAvgPrice = stod(str);
						}
					}
				}

				if (dataNode.HasMember("leavesQty"))
				{
					if (dataNode["leavesQty"].IsString())
					{
						string str =  dataNode["leavesQty"].GetString();
						if (str.size() != 0) {
							rspOrder.volumeRemained = stod(str);
						}
					}
				}
				if (dataNode.HasMember("cumExecQty"))
				{
					if (dataNode["cumExecQty"].IsString())
					{
						string str =  dataNode["cumExecQty"].GetString();
						if (str.size() != 0) {
							rspOrder.volumeFilled = stod(str);
						}
					}
				}

				if (dataNode.HasMember("timeInForce"))
				{
					if (dataNode["timeInForce"].IsString())
					{
						string str = dataNode["timeInForce"].GetString();
						memcpy(rspOrder.timeInForce, str.c_str(), min(sizeof(rspOrder.timeInForce) - 1, str.size()));
					}
				}

				if (dataNode.HasMember("updatedTime"))
				{
					if (dataNode["updatedTime"].IsString())
					{
						rspOrder.updatedTime = stoull(dataNode["updatedTime"].GetString());
					}
				}

				if (strcmp(topic, "execution") == 0) {
					if (dataNode.HasMember("orderQty"))
					{
						if (dataNode["orderQty"].IsString())
						{
							string str =  dataNode["orderQty"].GetString();
							if (str.size() != 0) {
								rspOrder.volumeTotalOriginal = stod(str);
							}
						}
					}
					if (dataNode.HasMember("execPrice"))
					{
						if (dataNode["execPrice"].IsString())
						{
							string str =  dataNode["execPrice"].GetString();
							if (str.size() != 0) {
								rspOrder.price = stod(str);
							}
						}
					}
					if (dataNode.HasMember("execQty"))
					{
						if (dataNode["execQty"].IsString())
						{
							string str =  dataNode["execQty"].GetString();
							if (str.size() != 0) {
								rspOrder.volume = stod(str);
							}
						}
					}
					if (dataNode.HasMember("execFee"))
					{
						if (dataNode["execFee"].IsString())
						{
							string str =  dataNode["execFee"].GetString();
							if (str.size() != 0) {
								rspOrder.fee = stod(str);
							}
						}
					}
					if (dataNode.HasMember("execTime"))
					{
						if (dataNode["execTime"].IsString())
						{
							rspOrder.tradeTime = stoull(dataNode["execTime"].GetString());
						}
					}
					if (dataNode.HasMember("execId"))
					{
						if (dataNode["execId"].IsString())
						{
							string str = dataNode["execId"].GetString();
							memcpy(rspOrder.tradeId, str.c_str(), min(sizeof(rspOrder.tradeId) - 1, str.size()));
						}
					}
					if (dataNode.HasMember("isMaker"))
					{
						if (dataNode["isMaker"].IsBool())
						{
							rspOrder.isMaker = dataNode["isMaker"].GetBool();
						}
					}
					if (dataNode.HasMember("markPrice"))
					{
						if (dataNode["markPrice"].IsString())
						{
							string str =  dataNode["markPrice"].GetString();
							if (str.size() != 0) {
								rspOrder.markPrice = stod(str);
							}
						}
					}
				}
				vecOrd_.push_back(rspOrder);	
			}
		}
	} catch (std::exception ex) {
        LOG_WARN << "BybitWssStruct RspOrder exception occur:" << ex.what();
		cout << "BybitWssStruct RspOrder exception occur:" << ex.what();
        return -1;
    }
		return 0;
	}

public:
	vector<BybitWssRspOrder> vecOrd_;
	char		message_id[InstrumentIDLen + 1];
	char		topic[InstrumentIDLen + 1];
	uint64_t	creationTime;
};
}
#endif
