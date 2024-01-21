#ifndef __OKSWAPSTRUCT_H__
#define __OKSWAPSTRUCT_H__

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
#include <spot/utility/Logging.h>
#include <vector>
#include "spot/base/DataStruct.h"
#ifdef __LINUX__
#include <time.h>
#endif

using namespace std;
using namespace std::placeholders;
using namespace spotrapidjson;
using namespace spot::base;
namespace spot
{
class OKEXSwapRspOrderInsert
{
public:
	OKEXSwapRspOrderInsert() {
		memset(this, 0, sizeof(OKEXSwapRspOrderInsert));
	}
	
	char		order_id[40 + 1]{};
	char		client_oid[40 + 1]{};
	char		error_code[40 + 1]{};
	char		error_message[40 + 1]{};
	char		tag[40+1]{};
	char		statusCode[40+1]{};
	char		statusMsg[100+1]{};

	void to_string()
	{
		ostringstream os;
		os << "OKEXSwapRspOrderInsert";
		os << ",order_id:" << order_id;
		os << ",client_oid:" << client_oid;
		os << ",error_code:" << error_code;
		os << ",error_message:" << error_message;
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "OKEXSwapRspOrderInsert::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			// if (doc["code"].IsString())
			// {
			    string s = doc["code"].GetString();
				memcpy(error_code, s.c_str(), min(s.size(), sizeof(error_code)));
				if (strcmp(error_code , "0") != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						// if (doc["msg"].IsString())
						// {
							errorMsg = doc["msg"].GetString();
						// }
					}
					LOG_WARN << " OkSwapApi::OkSwapReqRspOrder result:" << error_code << ", errorMsg:" << errorMsg;
					cout << " OkSwapApi::OkSwapReqRspOrder result:" << error_code << ", errorMsg:" << errorMsg << endl;
					return stoi(error_code);
				}
			// }
		}

		if (doc.HasMember("data")) {
			spotrapidjson::Value& dataNode = doc["data"];
			if (dataNode.Size() != 1) {
				LOG_WARN << " OkSwapApi::OkSwapReqRspOrder data err";
				return -1;
			}
			spotrapidjson::Value& rspNode = doc["data"][0];
			if (rspNode.HasMember("ordId"))
			{
                string s = rspNode["ordId"].GetString();
				memcpy(order_id, s.c_str(), min(s.size(), sizeof(order_id) - 1));
			}

			if (rspNode.HasMember("clOrdId"))
			{
                string s = rspNode["clOrdId"].GetString();
				memcpy(client_oid, s.c_str(), min(s.size(), sizeof(client_oid) - 1));
			}
		

			if (rspNode.HasMember("sCode"))
			{
                string s = rspNode["sCode"].GetString();
				memcpy(statusCode, s.c_str(), min(s.size(), sizeof(statusCode) - 1));
			}

			if (rspNode.HasMember("sMsg"))
			{
                string s = rspNode["sMsg"].GetString();
				memcpy(statusMsg, s.c_str(), min(s.size(), sizeof(statusMsg) - 1));
			}

			if (rspNode.HasMember("tag"))
			{
                string s = rspNode["tag"].GetString();
				memcpy(tag, s.c_str(), min(s.size(), sizeof(tag) - 1));
			}
		}

		return 0;
	}

};


class OKEXSwapRspOrderCancel
{
public:
	OKEXSwapRspOrderCancel() {
		memset(this, 0, sizeof(OKEXSwapRspOrderCancel));
	}
	
	char		order_id[40 + 1]{};
	char		client_oid[40 + 1]{};
	char		result[40 + 1]{};
	char		error_code[40 + 1]{};
	char		error_message[40 + 1]{};
	char		statusCode[40+1]{};
	char		statusMsg[100+1]{};

	void to_string()
	{
		ostringstream os;
		os << "OKEXSwapRspOrderCancel";
		os << ",order_id:" << order_id;
		os << ",result:" << result;
		os << ",error_code:" << error_code;
		os << ",error_message:" << error_message;
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "OKEXSwapRspOrderCancel::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			// if (doc["code"].IsString())
			// {
                string s = doc["code"].GetString();
                memcpy(error_code, s.c_str(), min(s.size(), sizeof(error_code)));
				if (s != "0")
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						// if (doc["msg"].IsString())
						// {
							errorMsg = doc["msg"].GetString();
						// }
					}
					LOG_WARN << " OkSwapApi::OKEXSwapRspOrderCancel result:" << error_code << ", errorMsg:" << errorMsg;
					cout << " OkSwapApi::OKEXSwapRspOrderCancel result:" << error_code << ", errorMsg:" << errorMsg << endl;
					return stoi(s);
				}
			// }
		}
		if (doc.HasMember("data")) {
			spotrapidjson::Value& dataNode = doc["data"];
			if (dataNode.Size() != 1) {
				LOG_WARN << " OkSwapApi::OKEXSwapRspOrderCancel data err";
				return -1;
			}
			spotrapidjson::Value& rspNode = dataNode[0];
			if (rspNode.HasMember("ordId"))
			{
				
				// if (rspNode["ordId"].IsString())
				// {
				    string s = rspNode["ordId"].GetString();
					memcpy(order_id, s.c_str(), min(s.size(), sizeof(order_id) - 1));
				// }
			}
			if (rspNode.HasMember("clOrdId"))
			{
				// if (rspNode["clOrdId"].IsString())
				// {
				    string s = rspNode["clOrdId"].GetString();
					memcpy(client_oid, s.c_str(), min(s.size(), sizeof(client_oid) - 1));
				// }
			}
			if (rspNode.HasMember("sCode"))
			{
				// if (rspNode["sCode"].IsString())
				// {
                    string s = rspNode["sCode"].GetString();
					memcpy(statusCode, s.c_str(), min(s.size(), sizeof(statusCode) - 1));
				// }
			}
			if (rspNode.HasMember("sMsg"))
			{
				// if (rspNode["sMsg"].IsString())
				// {
                    string s = rspNode["sMsg"].GetString();
					memcpy(statusMsg, s.c_str(), min(s.size(), sizeof(statusMsg) - 1));
				// }
			}
		}

		return 0;
	}
};

class OKexMarkPQryInfo
{
public:
	OKexMarkPQryInfo() {
		memset(this, 0, sizeof(OKexMarkPQryInfo));
	}

	char error_code[40 + 1];
	char instType[40 + 1];
	char instId[40 + 1];
	char markPx[40+1];
	char ts[40+1];

	string to_string()
	{
		ostringstream os;
		os << "OKxSwap OKexMarkPQryInfo";
		os << "error_code: " << error_code;
		os << ",instType: " << instType;
		os << ",instId: " << instId;
		os << ",markPx: " << markPx;
		os << ",ts: " << ts;	
		return os.str();
	}

	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "OKexMarkPQryInfo::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			// if (doc["code"].IsString())
			// {
                string s = doc["code"].GetString();
                memcpy(error_code, s.c_str(), min(s.size(), sizeof(error_code)));
				if (strcmp(error_code , "0") != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						if (doc["msg"].IsString())
						{
							errorMsg = doc["msg"].GetString();
						}
					}
					LOG_WARN << " OkSwapApi::OKexMarkPQryInfo result:" << error_code << ", errorMsg:" << errorMsg;
					cout << " OkSwapApi::OKexMarkPQryInfo result:" << error_code << ", errorMsg:" << errorMsg << endl;
					return stoi(error_code);
				}
			// }
		}

		if (doc.HasMember("data")) {
			spotrapidjson::Value& dataNode = doc["data"];
			if (dataNode.Size() != 1) {
				LOG_WARN << " OkSwapApi::OKexMarkPQryInfo data err";
				return -1;
			}
			spotrapidjson::Value& rspNode = dataNode[0];

			if (rspNode.HasMember("instType"))
			{
				// if (rspNode["instType"].IsString())
				// {
					std::string str = rspNode["instType"].GetString();
					memcpy(instType, str.c_str(), min(sizeof(instType) - 1, str.size()));
				// }
			}

			if (rspNode.HasMember("instId"))
			{
				// if (rspNode["instId"].IsString())
				// {
					std::string str = rspNode["instId"].GetString();
					memcpy(instId, str.c_str(), min(sizeof(instId) - 1, str.size()));
				// }
			}
		
			if (rspNode.HasMember("markPx"))
			{
				// if (rspNode["markPx"].IsString())
				// {
					std::string str = rspNode["markPx"].GetString();
					memcpy(markPx, str.c_str(), min(sizeof(markPx) - 1, str.size()));
				// }
			}

			if (rspNode.HasMember("ts"))
			{
				// if (rspNode["ts"].IsString())
				// {
					std::string str = rspNode["ts"].GetString();
					memcpy(ts, str.c_str(), min(sizeof(ts) - 1, str.size()));
				// }
			}

		}
		return 0;
	}
	
};

class OKexPosQryInfo
{
public:
	OKexPosQryInfo() {
		memset(this, 0, sizeof(OKexPosQryInfo));
	}

	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "OKexPosQryInfo::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("code"))
		{
			// if (doc["code"].IsString())
			// {
                string s = doc["code"].GetString();
                memcpy(error_code, s.c_str(), min(s.size(), sizeof(error_code)));
				if (strcmp(error_code , "0") != 0)
				{
					string errorMsg = "";
					if (doc.HasMember("msg"))
					{
						if (doc["msg"].IsString())
						{
							errorMsg = doc["msg"].GetString();
						}
					}
					LOG_WARN << " OkSwapApi::OKexPosQryInfo result:" << error_code << ", errorMsg:" << errorMsg;
					cout << " OkSwapApi::OKexPosQryInfo result:" << error_code << ", errorMsg:" << errorMsg << endl;
					return stoi(error_code);
				}
			// }
		}

		if (doc.HasMember("data")) {
			spotrapidjson::Value& dataNode = doc["data"];
			if (dataNode.Size() == 0) {
				LOG_WARN << " OkSwapApi::OKexPosQryInfo has no position";
				return -2;
			}
			else if (dataNode.Size() != 1) {
				LOG_ERROR << " OkSwapApi::OKexPosQryInfo data err";
				// return -1;
			}
			spotrapidjson::Value& rspNode = dataNode[0];

			if (rspNode.HasMember("instId"))
			{
				// if (rspNode["instId"].IsString())
				// {
					std::string str = rspNode["instId"].GetString();
					memcpy(posiNode_.symbol, str.c_str(), min(sizeof(posiNode_.symbol) - 1, str.size()));
				// }
			}
			if (rspNode.HasMember("posSide"))
			{
				// if (rspNode["posSide"].IsString())
				// {
					std::string str = rspNode["posSide"].GetString();
					memcpy(posiNode_.side, str.c_str(), min(sizeof(posiNode_.side) - 1, str.size()));
				// }
			}

			
			// if (vpos.IsString()) {
			if (rspNode.HasMember("pos")) {
				Value &vpos = rspNode["pos"];
				string str = vpos.GetString();
				if (str.size() != 0) {
					posiNode_.size = stod(str);
				}
			}
			// }
			// else
			// 	posiNode_.size = vpos.GetDouble();

			if (rspNode.HasMember("liqPx")) {
				Value &vliqPx = rspNode["liqPx"];
				string str = vliqPx.GetString();
				if (str.size() != 0) {
					posiNode_.liqPrice = stod(str);
				}
			}
			// }
			// else
			// 	posiNode_.liqPrice = vliqPx.GetDouble();
			if (rspNode.HasMember("uTime")) {
				Value &vtime = rspNode["uTime"];
				posiNode_.updatedTime = stoll(vtime.GetString());
			}
			// else
			// 	posiNode_.updatedTime = vtime.GetUint64();

		}
		return 0;
	}
public:
	commQryPosiNode posiNode_;
	char error_code[40 + 1];
};


class OKxWssSwapOrder
{
public:
	OKxWssSwapOrder() {
		memset(this, 0, sizeof(OKxWssSwapOrder));
	}

	char		instrument_id[InstrumentIDLen + 1]{};
	char 		tradeID[40+1];
	char		state[40+1];
	char		ordId[40+1]; //orderRef system order
	char		clOrdId[40+1]; //orderref
	char		direction[40+1];
	char		posSide[40+1];
	char        limitPrice[40+1]; // wei tuo price
	char		price[40+1]; // deal price
	char		avgPrice[40+1];
	char		volume[40+1];  // current deal volume
	char 		volumeTotalOriginal[40+1]; // wei tuo volume
	char		volumeFilled[40+1];  //accumulate filled volume
	char		volumeRemained[40+1]; // left undeal volume
	char		fee[40+1]; // deal fee , fillFee
	char		dealTimeStamp[40+1]; // the last deal time
	char		epochTimeReturn[40+1];
	char		error_code[40+1];

	string to_string()
	{
		ostringstream os;
		os << "OKxWssSwapOrder";
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
};

class OKxWssStruct {
public:
	OKxWssStruct() {
		// memset(this, 0, sizeof(OKxWssStruct));
	}

	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "OKxWssSwapOrder::Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("arg"))
		{
			spotrapidjson::Value& dataNode = doc["arg"];
			if (!dataNode.HasMember("channel") || !dataNode.HasMember("instType") ||
				!dataNode.HasMember("uid")) {
				LOG_FATAL << "OKxWssSwapOrder decode err";
			}
		}

		if (doc.HasMember("data")) {
			spotrapidjson::Value& dataNode = doc["data"];
			if (dataNode.Size() == 0) {
				LOG_WARN << " OkSwapApi::OKxWssSwapOrder err no data";
				return -1;
			}

			for (int i = 0; i < dataNode.Size(); i++) {
				spotrapidjson::Value& rspNode = doc["data"][i];
				OKxWssSwapOrder rspOrder;
				if (rspNode.HasMember("instId"))
				{
					std::string str = rspNode["instId"].GetString();
					memcpy(rspOrder.instrument_id, str.c_str(), min(sizeof(rspOrder.instrument_id) - 1, str.size()));
				}

				if (rspNode.HasMember("tradeId"))
				{
					// if (rspNode["tradeId"].IsString())
					// {
						std::string str = rspNode["tradeId"].GetString();
						memcpy(rspOrder.tradeID, str.c_str(), min(sizeof(rspOrder.tradeID) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("state"))
				{
					// if (rspNode["state"].IsString())
					// {
						std::string str = rspNode["state"].GetString();
						memcpy(rspOrder.state, str.c_str(), min(sizeof(rspOrder.state) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("ordId"))
				{
					// if (rspNode["ordId"].IsString())
					// {
						std::string str = rspNode["ordId"].GetString();
						memcpy(rspOrder.ordId, str.c_str(), min(sizeof(rspOrder.ordId) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("clOrdId"))
				{
					// if (rspNode["clOrdId"].IsString())
					// {
						std::string str = rspNode["clOrdId"].GetString();
						memcpy(rspOrder.clOrdId, str.c_str(), min(sizeof(rspOrder.clOrdId) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("side"))
				{
					// if (rspNode["side"].IsString())
					// {
						std::string str = rspNode["side"].GetString();
						memcpy(rspOrder.direction, str.c_str(), min(sizeof(rspOrder.direction) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("posSide"))
				{
					// if (rspNode["posSide"].IsString())
					// {				
						std::string str = rspNode["posSide"].GetString();
						memcpy(rspOrder.posSide, str.c_str(), min(sizeof(rspOrder.posSide) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("px"))
				{
					// if (rspNode["px"].IsString())
					// {
						std::string str = rspNode["px"].GetString();
						memcpy(rspOrder.limitPrice, str.c_str(), min(sizeof(rspOrder.limitPrice) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("fillPx"))
				{
					// if (rspNode["fillPx"].IsString())
					// {
						std::string str = rspNode["fillPx"].GetString();
						memcpy(rspOrder.price, str.c_str(), min(sizeof(rspOrder.price) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("avgPx"))
				{
					// if (rspNode["avgPx"].IsString())
					// {
						std::string str = rspNode["avgPx"].GetString();
						memcpy(rspOrder.avgPrice, str.c_str(), min(sizeof(rspOrder.avgPrice) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("fillSz"))
				{
					// if (rspNode["fillSz"].IsString())
					// {
						std::string str = rspNode["fillSz"].GetString();
						memcpy(rspOrder.volume, str.c_str(), min(sizeof(rspOrder.volume) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("sz"))
				{
					// if (rspNode["sz"].IsString())
					// {
						std::string str = rspNode["sz"].GetString();
						memcpy(rspOrder.volumeTotalOriginal, str.c_str(), min(sizeof(rspOrder.volumeTotalOriginal) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("accFillSz"))
				{
					// if (rspNode["accFillSz"].IsString())
					// {
						std::string str = rspNode["accFillSz"].GetString();
						memcpy(rspOrder.volumeFilled, str.c_str(), min(sizeof(rspOrder.volumeFilled) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("volumeRemained"))
				{
					// if (rspNode["volumeRemained"].IsString())
					// {
						std::string str = rspNode["volumeRemained"].GetString();
						memcpy(rspOrder.volumeRemained, str.c_str(), min(sizeof(rspOrder.volumeRemained) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("fillFee"))
				{
					// if (rspNode["fillFee"].IsString())
					// {
						std::string str = rspNode["fillFee"].GetString();
						memcpy(rspOrder.fee, str.c_str(), min(sizeof(rspOrder.fee) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("fillTime"))
				{
					// if (rspNode["fillTime"].IsString())
					// {
						std::string str = rspNode["fillTime"].GetString();
						memcpy(rspOrder.dealTimeStamp, str.c_str(), min(sizeof(rspOrder.dealTimeStamp) - 1, str.size()));
					// }
				}
				if (rspNode.HasMember("epochTimeReturn"))
				{
					// if (rspNode["epochTimeReturn"].IsString())
					// {
						std::string str = rspNode["epochTimeReturn"].GetString();
						memcpy(rspOrder.epochTimeReturn, str.c_str(), min(sizeof(rspOrder.epochTimeReturn) - 1, str.size()));            
						// }
				}
				vecOrd_.push_back(rspOrder);
			}
			
		}
		return 0;
	}

public:
	vector<OKxWssSwapOrder> vecOrd_;
};
}

#endif