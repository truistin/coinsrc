#ifndef __BIANSTRUCT_H__
#define __BIANSTRUCT_H__
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
#include "spot/base/DataStruct.h"
#include <spot/utility/Logging.h>
#ifdef __LINUX__
#include <time.h>
#endif

using namespace spot::base;
using namespace std;
using namespace std::placeholders;
using namespace spotrapidjson;

namespace spot
{ 
static std::string epochTimeToStr(long long epoch)
{
	struct tm tm_time;
	int kMicroSecondsPerSecond = 1000 * 1000;
	time_t seconds = static_cast<time_t>(epoch / kMicroSecondsPerSecond);
#ifdef __LINUX__
	localtime_r(&seconds, &tm_time);
#endif

#ifdef WIN32
	_localtime64_s(&tm_time, &seconds);
#endif
	int microseconds = static_cast<int>(epoch % kMicroSecondsPerSecond);
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%2d:%2d:%2d", 
		tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

	return buf;
}

enum BianOrderStatus
{
	NEW = 0,
	PARTIALLY_FILLED,
	FILLED,
	CANCELED,
	REJECTED,
	EXPIRED,
	NEW_INSURANCE, //���ձ��ϻ���(ǿƽ)
	NEW_ADL, //�Զ���������(ǿƽ)
};

class BnAccountInfo
{
public:
	BnAccountInfo() {
		memset(this, 0, sizeof(BnAccountInfo));
	}
public:
	double		uniMMR;
	int64_t		updateTime;	
public:
	int decode(const char* json) {
		uniMMR = 0;
		updateTime = 0;
		Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BnSpotAsset Parse error. result:" << json;
            return -1;
        }

		spotrapidjson::Value& mr = doc["uniMMR"];
		spotrapidjson::Value& time = doc["updateTime"];

		if (mr.IsString()){
			std::string s = mr.GetString();
			uniMMR = stod(s);
		}

		updateTime = time.GetUint64();
		return 0;
	}
};


class BnCmAssetInfo
{
public:
	BnCmAssetInfo() {
		memset(this, 0, sizeof(BnCmAssetInfo));
	}
public:
	char		asset[20];
	double		crossWalletBalance;
	double 		crossUnPnl;
	int64_t		updateTime;	
};

class BnCmPositionInfo
{
public:
	BnCmPositionInfo()
	{
		memset(this, 0, sizeof(BnCmPositionInfo));
	}
public:
	char		symbol[20];
	double		positionAmt;
	double		entryPrice;
	int			leverage;
	char		side[10];
	int64_t		updateTime;
};


class BnCmAccount
{
public:
	BnCmAccount()
	{
	}

	int decode(const char* json) {
		info1_.clear();
		info_.clear();
		Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BnSpotAsset Parse error. result:" << json;
            return -1;
        }

		spotrapidjson::Value& dataNodes = doc["assets"];

        for (int i = 0; i < dataNodes.Capacity(); ++i) {
			spotrapidjson::Value& asset = dataNodes[i]["asset"];
			spotrapidjson::Value& walletBalance = dataNodes[i]["crossWalletBalance"];
			spotrapidjson::Value& unPnl = dataNodes[i]["crossUnPnl"];
		
			BnCmAssetInfo info;
			if (asset.IsString()){
				std::string s = asset.GetString();
				memcpy(info.asset, s.c_str(), min(sizeof(info.asset), s.size()));
			}

			if (walletBalance.IsString()){
				std::string s = walletBalance.GetString();
				info.crossWalletBalance = stod(s);
			}

			if (unPnl.IsString()){
				std::string s = unPnl.GetString();
				info.crossUnPnl = stod(s);
			}
			info.updateTime = CURR_MSTIME_POINT;
			info_.push_back(info);
		}

		spotrapidjson::Value& dataNodes1 = doc["positions"];
		for (int i = 0; i < dataNodes1.Capacity(); i++) {
			spotrapidjson::Value& symbol = dataNodes[i]["symbol"];
			spotrapidjson::Value& positionAmt = dataNodes[i]["positionAmt"];
			spotrapidjson::Value& entryPrice = dataNodes[i]["entryPrice"];
			spotrapidjson::Value& side = dataNodes[i]["positionSide"];
			spotrapidjson::Value& leverage = dataNodes[i]["leverage"];

			BnCmPositionInfo info;
			if (symbol.IsString()){
				std::string s = symbol.GetString();
				memcpy(info.symbol, s.c_str(), min(sizeof(info.symbol), s.size()));
			}

			if (side.IsString()){
				std::string s = side.GetString();
				memcpy(info.side, s.c_str(), min(sizeof(info.side), s.size()));
			}

			if (positionAmt.IsString()){
				std::string s = positionAmt.GetString();
				info.positionAmt = stod(s);
			}

			if (entryPrice.IsString()){
				std::string s = entryPrice.GetString();
				info.entryPrice = stod(s);
			}

			if (leverage.IsString()){
				std::string s = leverage.GetString();
				info.leverage = stoi(s);
			}
			info.updateTime = CURR_MSTIME_POINT;
			info1_.push_back(info);
		}

		return 0;
	}
	
public:
	vector<BnCmAssetInfo> info_;
	vector<BnCmPositionInfo> info1_;
};

class BnUmAssetInfo
{
public:
	BnUmAssetInfo()
	{
		memset(this, 0, sizeof(BnUmAssetInfo));
	}
public:
	char		asset[20];
	double		crossWalletBalance;
	double 		crossUnPnl;
	int64_t		updateTime;	
};

class BnUmPositionInfo
{
public:
	BnUmPositionInfo()
	{
		memset(this, 0, sizeof(BnUmPositionInfo));
	}
public:
	char		symbol[20];
	double		positionAmt;
	double		entryPrice;
	int			leverage;
	char		side[10];
	int64_t		updateTime;
};

class BnUmAccount
{
public:
	BnUmAccount()
	{
	}

	int decode(const char* json) {
		info1_.clear();
		info_.clear();
		Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BnSpotAsset Parse error. result:" << json;
            return -1;
        }

		spotrapidjson::Value& dataNodes = doc["assets"];

        for (int i = 0; i < dataNodes.Capacity(); ++i) {
			spotrapidjson::Value& asset = dataNodes[i]["asset"];
			spotrapidjson::Value& walletBalance = dataNodes[i]["crossWalletBalance"];
			spotrapidjson::Value& unPnl = dataNodes[i]["crossUnPnl"];
			BnUmAssetInfo info;
			if (asset.IsString()){
				std::string s = asset.GetString();
				memcpy(info.asset, s.c_str(), min(sizeof(info.asset), s.size()));
			}

			if (walletBalance.IsString()){
				std::string s = walletBalance.GetString();
				info.crossWalletBalance = stod(s);
			}

			if (unPnl.IsString()){
				std::string s = unPnl.GetString();
				info.crossUnPnl = stod(s);
			}
			info.updateTime = CURR_MSTIME_POINT;

			info_.push_back(info);
		}
		
		spotrapidjson::Value& dataNodes1 = doc["positions"];
		for (int i = 0; i < dataNodes1.Capacity(); i++) {
			spotrapidjson::Value& symbol = dataNodes1[i]["symbol"];
			spotrapidjson::Value& positionAmt = dataNodes1[i]["positionAmt"];
			spotrapidjson::Value& entryPrice = dataNodes1[i]["entryPrice"];
			spotrapidjson::Value& side = dataNodes1[i]["positionSide"];
			spotrapidjson::Value& leverage = dataNodes1[i]["leverage"];

			BnUmPositionInfo info;
			if (symbol.IsString()){
				std::string s = symbol.GetString();
				memcpy(info.symbol, s.c_str(), min(sizeof(info.symbol), s.size()));
			}

			if (side.IsString()){
				std::string s = side.GetString();
				memcpy(info.side, s.c_str(), min(sizeof(info.side), s.size()));
			}

			if (positionAmt.IsString()){
				std::string s = positionAmt.GetString();
				info.positionAmt = stod(s);
			}

			if (entryPrice.IsString()){
				std::string s = entryPrice.GetString();
				info.entryPrice = stod(s);
			}

			if (leverage.IsString()){
				std::string s = leverage.GetString();
				info.leverage = stoi(s);
			}
			info.updateTime = CURR_MSTIME_POINT;
			info1_.push_back(info);
		}
		return 0;
	}
public:
	vector<BnUmAssetInfo> info_;
	vector<BnUmPositionInfo> info1_;
};

struct BnSpotAssetInfo
{
public:
	BnSpotAssetInfo()
	{
		memset(this, 0, sizeof(BnSpotAssetInfo));
	}
public:
	char		asset[20];
	double		crossMarginAsset;
	double		crossMarginFree;
	double 		crossMarginLocked;
	double		crossMarginBorrowed;
	double		crossMarginInterest;
	double		umWalletBalance;
	double 		umUnrealizedPNL;
	double		cmWalletBalance;
	double		cmUnrealizedPNL;

};

class BnSpotAsset
{
public:
	BnSpotAsset()
	{
	}

	int decode(const char* json) {
		Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BnSpotAsset Parse error. result:" << json;
            return -1;
        }

		spotrapidjson::Value dataNodes = doc.GetArray();
        for (int i = 0; i < dataNodes.Capacity(); ++i) {
			spotrapidjson::Value& asset = dataNodes[i]["asset"];
			spotrapidjson::Value& crossMargin = dataNodes[i]["crossMarginAsset"];
			spotrapidjson::Value& crossFree = dataNodes[i]["crossMarginFree"];
			spotrapidjson::Value& crossLocked = dataNodes[i]["crossMarginLocked"];
			spotrapidjson::Value& crossBorrowed = dataNodes[i]["crossMarginBorrowed"];
			spotrapidjson::Value& crossInterest = dataNodes[i]["crossMarginInterest"];
			spotrapidjson::Value& umWalletBalance = dataNodes[i]["umWalletBalance"];
			spotrapidjson::Value& umUnrealizedPNL = dataNodes[i]["umUnrealizedPNL"];
			spotrapidjson::Value& cmWalletBalance = dataNodes[i]["cmWalletBalance"];
			spotrapidjson::Value& cmUnrealizedPNL = dataNodes[i]["cmUnrealizedPNL"];

			BnSpotAssetInfo info;
			if (asset.IsString()){
				std::string s = asset.GetString();
				memcpy(info.asset, s.c_str(), min(sizeof(info.asset), s.size()));
			}

			if (crossMargin.IsString()){
				std::string s = crossMargin.GetString();
				info.crossMarginAsset = stod(s);
			}

			if (crossFree.IsString()){
				std::string s = crossFree.GetString();
				info.crossMarginFree = stod(s);
			}

			if (crossLocked.IsString()){
				std::string s = crossLocked.GetString();
				info.crossMarginLocked = stod(s);
			}

			if (crossBorrowed.IsString()){
				std::string s = crossBorrowed.GetString();
				info.crossMarginBorrowed = stod(s);
			}

			if (crossInterest.IsString()){
				std::string s = crossInterest.GetString();
				info.crossMarginInterest = stod(s);
			}

			if (umWalletBalance.IsString()){
				std::string s = umWalletBalance.GetString();
				info.umWalletBalance = stod(s);
			}

			if (umUnrealizedPNL.IsString()){
				std::string s = umUnrealizedPNL.GetString();
				info.umUnrealizedPNL = stod(s);
			}

			if (cmWalletBalance.IsString()){
				std::string s = cmWalletBalance.GetString();
				info.cmWalletBalance = stod(s);
			}

			if (cmUnrealizedPNL.IsString()){
				std::string s = cmUnrealizedPNL.GetString();
				info.cmUnrealizedPNL = stod(s);
			}

			info_.push_back(info);
		}

		return 0;

	}
public:
	vector<BnSpotAssetInfo> info_;
};

class BianQueryOrder
{
public:
	BianQueryOrder()
	{
		memset(this, 0, sizeof(BianQueryOrder));
	}
public:
	char		orderStatus[40+1];
	char 		orderId[128+1];
	double 		volumeFilled;
	double		volumeTotalOriginal;

	int decode(const char* json) {
	try{
        Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BianQueryOrder::Parse error. result:" << json;
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
					LOG_ERROR << "BianQueryOrder retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json;
					cout << "BianQueryOrder retCode:" << errorCode << ", errorMsg:" << errorMsg << ",json: " << json << endl;
					return errorCode;
				}
			}
		}

		spotrapidjson::Value& node = doc["result"];
		spotrapidjson::Value& dataNodes = node["list"];

		if (dataNodes.Size() == 0) {
		    LOG_WARN << "BianQueryOrder has no position " << doc.Size();
		    return -3;
	   	}

		spotrapidjson::Value& dataNode = dataNodes[0];
		Value &vstatus = dataNode["status"];
		if (vstatus.IsString()){
			std::string s = vstatus.GetString();
			memcpy(orderStatus, s.c_str(), min(sizeof(orderStatus) - 1, (s.size())));
		}

		Value &vorderRef = dataNode["clientOrderId"];
		if (vorderRef.IsString()){
			std::string s = vorderRef.GetString();
			memcpy(orderId, s.c_str(), min(sizeof(orderId) - 1, (s.size())));
		}

		Value &vVolFilled = dataNode["executedQty"];
		if (vVolFilled.IsString()){
			std::string s = vVolFilled.GetString();
			volumeFilled = stod(s);
		}

		Value &vinputVol = dataNode["origQty"];
		if (vinputVol.IsString()){
			std::string s = vinputVol.GetString();
			volumeTotalOriginal = stod(s);
		}

		return 0;

	} catch (std::exception ex) {
			LOG_WARN << "BianQueryOrder exception occur:" << ex.what();
			cout << "BianQueryOrder exception occur:" << ex.what();
			return -1;
	}
	}
};

class  BianQryPositionInfo
{
public:
    BianQryPositionInfo()
    {
        memset(this, 0, sizeof(BianQryPositionInfo));
    }
    int decode(const char* json) {
        Document doc;
        doc.Parse(json, strlen(json));

        if (doc.HasParseError())
        {
            LOG_WARN << "BianMrkQryInfo::Parse error. result:" << json;
            return -1;
        }

	   if (!doc.IsArray()) {
			if (doc.HasMember("code"))
			{
				if (doc["code"].IsNumber())
				{
					int errorCode = doc["code"].GetInt();
					if (errorCode != 0)
					{
						string errorMsg = "";
						if (doc.HasMember("msg"))
						{
							if (doc["msg"].IsString())
							{
								errorMsg = doc["msg"].GetString();
							}
						}
						LOG_ERROR << " BianApi::BianQryPositionInfo result:" << errorCode << ", errorMsg:" << errorMsg;
						cout << " BianApi::BianQryPositionInfo result:" << errorCode << ", errorMsg:" << errorMsg << endl;
						return errorCode;
					}
				}
			}
	   }
	   if (doc.Size() == 0) {
		   LOG_WARN << "BianQryPositionInfo has no position " << doc.Size();
		   return 2;
	   }
       else if(doc.Size() != 1) {
           LOG_ERROR << "BianQryPositionInfo size error " << doc.Size();
		   return -1;
       }
        spotrapidjson::Value& dataNode = doc[0];

        Value &vsymbol = dataNode["symbol"];
        if (vsymbol.IsString()){
            std::string s = vsymbol.GetString();
            memcpy(posiNode_.symbol, s.c_str(), min(InstrumentIDLen, uint16_t(s.size())));
        }

        Value &vliquidationPrice = dataNode["liquidationPrice"];
        if (vliquidationPrice.IsString())
            posiNode_.liqPrice = atof(vliquidationPrice.GetString());
        else
            posiNode_.liqPrice = vliquidationPrice.GetDouble();

        Value &vpositionAmt = dataNode["positionAmt"];
        if (vpositionAmt.IsString())
            posiNode_.size = (atof(vpositionAmt.GetString())); // may be positive ,may be negitive
        else
            posiNode_.size = (vpositionAmt.GetDouble());

        Value &vpositionSide = dataNode["positionSide"];
        if (vpositionAmt.IsString()){
            std::string s = vpositionSide.GetString();
            memcpy(posiNode_.side, s.c_str(), min(DateLen, uint16_t(s.size())));
        }

        Value &vtime = dataNode["updateTime"];
        if (vtime.IsString())
            posiNode_.updatedTime = stoll(vtime.GetString());
        else
            posiNode_.updatedTime = vtime.GetUint64();

        Value &ventryPrice = dataNode["entryPrice"];
        if (ventryPrice.IsString())
            posiNode_.entryPrice = stod(ventryPrice.GetString());
        else
            posiNode_.entryPrice = ventryPrice.GetDouble();

        return 0;
    }
public:
	commQryPosiNode posiNode_;
};

class  BianMrkQryInfo
{
public:
	BianMrkQryInfo()
	{
		memset(this, 0, sizeof(BianMrkQryInfo));
	}
	int decode(const char* json) {
		Document doc;
		doc.Parse(json, strlen(json));

		if (doc.HasParseError())
		{
			LOG_WARN << "BianMrkQryInfo::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			if (doc["code"].IsNumber())
			{
                int errorCode = doc["code"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						if (doc["msg"].IsString())
						{
							errorMsg = doc["msg"].GetString();
						}
					}
					LOG_WARN << " BianApi::QryInfo result:" << errorCode << ", errorMsg:" << errorMsg;
					cout << " BianApi::QryInfo result:" << errorCode << ", errorMsg:" << errorMsg << endl;
					return errorCode;
				}
			}
		}

		Value &vsymbol = doc["symbol"];
		if (vsymbol.IsString())
        {
            std::string s = vsymbol.GetString();
            memcpy(markPxNode_.symbol, s.c_str(), min(InstrumentIDLen, uint16_t(s.size())));
        }

		Value &vprice = doc["markPrice"];
		if (vprice.IsString())
			markPxNode_.markPx = atof(vprice.GetString());
		else
			markPxNode_.markPx = vprice.GetDouble();

		Value &vfundingrate = doc["lastFundingRate"];
		if (vfundingrate.IsString())
			markPxNode_.fundingRate = atof(vfundingrate.GetString());
		else
			markPxNode_.fundingRate = vfundingrate.GetDouble();

		Value &vtime = doc["time"];
		if (vtime.IsString())
			markPxNode_.updatedTime = stoll(vtime.GetString());
		else
			markPxNode_.updatedTime = vtime.GetUint64();

		return 0;
	}
public:
	commQryMarkPxNode markPxNode_;
};

class BianRspReqOrder
{
public:
	BianRspReqOrder()
	{
		memset(this, 0, sizeof(BianRspReqOrder));
	}
	uint64_t orderID;
	char cOrderId[40+1];
	string to_string()
	{
		ostringstream os;
		os << "BianRspReqOrder";
		os << ",orderID:" << orderID;
		return os.str();
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			if (doc["code"].IsNumber())
			{
                int errorCode = doc["code"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						if (doc["msg"].IsString())
						{
							errorMsg = doc["msg"].GetString();
						}
					}
					LOG_WARN << " BianApi::ReqOrderInsert result: " << json << ", errorCode: " << errorCode<<", errorMsg:"<< errorMsg;
					cout << " BianApi::ReqOrderInsert result:" << errorCode << ", errorMsg:" << errorMsg << endl;
					return errorCode;
				}
			}
		}
		if (!doc.HasMember("symbol")) {
			LOG_ERROR << "BianRspReqOrder decode json";
			return -1;
		}
		spotrapidjson::Value& vsymbol = doc["symbol"];
		spotrapidjson::Value& vorder_id = doc["orderId"];
		spotrapidjson::Value& vclientOrderId = doc["clientOrderId"];
		spotrapidjson::Value& vupdateTime = doc["updateTime"];

		if (vorder_id.IsString())
			orderID = stoll(vorder_id.GetString());
		else
			orderID = vorder_id.GetUint64();

		if (vupdateTime.IsString())
		{
			string updateTime = vupdateTime.GetString();
		}
		else
		{
			uint64_t updateTime = vupdateTime.GetUint64();
		}

		if (vclientOrderId.IsString())
		{
            string s = vclientOrderId.GetString();
			memcpy(cOrderId, s.c_str(), min(sizeof(cOrderId) - 1, s.size()));
		}

		return 0;
	}
};
class BianRspCancelOrder
{
public:
	BianRspCancelOrder()
	{
		memset(this, 0, sizeof(BianRspCancelOrder));
	}
	char instrument_id[30 + 1];
	uint64_t orderID;
	char cOrderId[40+1]; 
	string to_string()
	{
		ostringstream os;
		os << "BianRspCancelOrder";
		os << "instrument_id:"<< instrument_id <<",id:" << orderID;
		return os.str();
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "BianApi::CancelOrder Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			if (doc["code"].IsNumber())
			{
                int errorCode = doc["code"].GetInt();
				if (errorCode != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						if (doc["msg"].IsString())
						{
							errorMsg = doc["msg"].GetString();
						}
					}
					LOG_WARN << " BianApi::BianRspCancelOrder result:" << errorCode << ", errorMsg:" << errorMsg;
					cout << " BianApi::BianRspCancelOrder result:" << errorCode << ", errorMsg:" << errorMsg << endl;
					return errorCode;
				}
			}
		}

		spotrapidjson::Value& vsymbol = doc["symbol"];
		spotrapidjson::Value& vorder_id = doc["orderId"];
		// 
		spotrapidjson::Value& clientOrderId = doc["clientOrderId"];

		if (!doc.HasMember("symbol")) {
			LOG_ERROR << "BianRspCancelOrder decode json";
			return -1;
		}
		if (vsymbol.IsString())
		{
            string s = vsymbol.GetString();
			memcpy(instrument_id, s.c_str(), min(sizeof(instrument_id) - 1, s.size()));
		}

		if (vorder_id.IsString())
			orderID = stoll(vorder_id.GetString());
		else
			orderID = vorder_id.GetUint64();

		if (clientOrderId.IsString())
		{
            string s = clientOrderId.GetString();
			memcpy(cOrderId, s.c_str(), min(sizeof(cOrderId) - 1, s.size()));
		}
		return 0;
	}
};

class BianWssRspOrder
{
public:
	BianWssRspOrder() {
		memset(this, 0, sizeof(BianWssRspOrder));
	}

	char		instrument_id[InstrumentIDLen + 1]{};
	uint64_t 		tradeID;
	char		state[40+1];
	char		clOrdId[40+1]; //orderRef order
	uint64_t			ordId; //system orderId
	char		direction[40+1];
	char		posSide[40+1];
	char        limitPrice[40+1]; // wei tuo price
	char		price[40+1]; // deal price
	char		avgPrice[40+1];
	char		volume[40+1];  // current deal volume
	char 		volumeTotalOriginal[40+1]; // wei tuo volume
	char		volumeFilled[40+1];  //accumulate filled volume
	char		volumeRemained[40+1]; // left undeal volume
	char		fee[40+1]; // deal fee
	uint64_t	dealTimeStamp; // the last deal time
	char		epochTimeReturn[40+1];
	char		error_code[40+1];
	//char		eventType[40+1];

	uint64_t	matchTimeStamp; // exchange match time
	uint64_t	eventTimeStamp; // exchange event time

	string to_string()
	{
		ostringstream os;
		os << "BianWssRspOrder";
		os << ",instrument_id: " << instrument_id;
		os << ",tradeID: " << tradeID;
		os << ",state: " << state;
		os << ",ordId: " << ordId;
		os << ",clOrdId: " << clOrdId;
		os << ",direction :" << direction;
		os << ",posSide: " << posSide;
		os << ",limitPrice: " << limitPrice;
		os << ",price: " << price;
		os << ",avgPrice: " << avgPrice;
		os << ",volume: " << volume;
		os << ",volumeTotalOriginal: " << volumeTotalOriginal;
		os << ",volumeFilled: " << volumeFilled;
		os << ",volumeRemained: " << volumeRemained;
		os << ",fee: " << fee;
		os << ",dealTimeStamp: " << dealTimeStamp;
		os << ",epochTimeReturn: " << epochTimeReturn;
		return os.str();
	}

	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "BianWssRspOrder::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("e"))
		{
			//spotrapidjson::Value& dataNode = doc["e"];
			if (doc["e"].IsString()) {
				//string str = dataNode["s"].GetString();
				string str = doc["e"].GetString();
				if (str != "ORDER_TRADE_UPDATE") {
					LOG_WARN << "BianTdSpi com_callbak_message return other dangerous msg: " << json;
					return -1;
				}
			}
			//LOG_FATAL  << "BianTdSpi com_callbak_message return WR msg";
		}

		if (doc.HasMember("E"))
		{
			eventTimeStamp = doc["E"].GetUint64();
		}

		if (doc.HasMember("T"))
		{
			matchTimeStamp = doc["T"].GetUint64();
		}

		if (doc.HasMember("o"))
		{
			spotrapidjson::Value& dataNode = doc["o"];
			if (dataNode.HasMember("s"))
			{
				if (dataNode["s"].IsString())
				{
					string str = dataNode["s"].GetString();
					memcpy(instrument_id, str.c_str(), min(sizeof(instrument_id) - 1, str.size()));
				}
			}
			// to do
			if (dataNode.HasMember("t"))
			{
				tradeID = dataNode["t"].GetUint64(); // GetUint64();
			}
			if (dataNode.HasMember("X"))
			{
				if (dataNode["X"].IsString())
				{
					string str = dataNode["X"].GetString();
					memcpy(state, str.c_str(), min(sizeof(state) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("i"))
			{
				ordId = dataNode["i"].GetUint64();
			}
			if (dataNode.HasMember("c"))
			{
				if (dataNode["c"].IsString())
				{
					string str = dataNode["c"].GetString();
					memcpy(clOrdId, str.c_str(), min(sizeof(clOrdId) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("S"))
			{
				if (dataNode["S"].IsString())
				{
					string str = dataNode["S"].GetString();
					memcpy(direction, str.c_str(), min(sizeof(direction) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("ps"))
			{
				if (dataNode["ps"].IsString())
				{
					string str = dataNode["ps"].GetString();
					memcpy(posSide, str.c_str(), min(sizeof(posSide) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("p"))
			{
				if (dataNode["p"].IsString())
				{
					string str = dataNode["p"].GetString();
					memcpy(limitPrice, str.c_str(), min(sizeof(limitPrice) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("L"))
			{
				if (dataNode["L"].IsString())
				{
					string str = dataNode["L"].GetString();
					memcpy(price, str.c_str(), min(sizeof(price) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("ap"))
			{
				if (dataNode["ap"].IsString())
				{
					string str = dataNode["ap"].GetString();
					memcpy(avgPrice, str.c_str(), min(sizeof(avgPrice) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("l"))
			{
				if (dataNode["l"].IsString())
				{
					string str = dataNode["l"].GetString();
					memcpy(volume, str.c_str(), min(sizeof(volume) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("q"))
			{
				if (dataNode["q"].IsString())
				{
					string str = dataNode["q"].GetString();
					memcpy(volumeTotalOriginal, str.c_str(), min(sizeof(volumeTotalOriginal) - 1, str.size()));
				}
			}

			if (dataNode.HasMember("z"))
			{
				if (dataNode["z"].IsString())
				{
					string str = dataNode["z"].GetString();
					memcpy(volumeFilled, str.c_str(), min(sizeof(volumeFilled) - 1, str.size()));
				}
			}

			// don't have volumeRemained
			//if (dataNode.HasMember("volumeRemained"))

			if (dataNode.HasMember("n"))
			{
				if (dataNode["n"].IsString())
				{
					string str = dataNode["n"].GetString();
					memcpy(fee, str.c_str(), min(sizeof(fee) - 1, str.size()));
				}
			}
			if (dataNode.HasMember("T"))
			{
				dealTimeStamp = dataNode["T"].GetUint64();
			}
			if (dataNode.HasMember("epochTimeReturn"))
			{
				if (dataNode["epochTimeReturn"].IsString())
				{
				    string s = dataNode["epochTimeReturn"].GetString();
					memcpy(epochTimeReturn, s.c_str(), min(sizeof(epochTimeReturn) - 1, s.size()));
				}
			}
			
		}
		return 0;
	}
};
}
#endif
