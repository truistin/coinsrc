#ifndef HUOBSTRUCT_H
#define HUOBSTRUCT_H
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/stringbuffer.h>
#include <spot/rapidjson/writer.h>
#include <functional>
#include <sstream>
#include <list>
#include <iostream>
#include "spot/utility/algo_hmac.h"
using namespace std;
using namespace std::placeholders;
using namespace spotrapidjson;

enum HuobOrderType
{
	huob_buy_limit = 0,
	huob_sell_limit,
	huob_buy_market,
	huob_sell_market,
};

enum HuobOrderStatus
{
	huob_reject =0,
	huob_pre_submitted,
	huob_submitting,
	huob_submitted,
	huob_partial_filled,
	huob_partial_canceled,
	huob_filled,
	huob_canceled,
	huob_canceling = 11,
};

class HuobFutureTransaction
{
public:
	HuobFutureTransaction()
    {
        memset(this,0,sizeof(HuobFutureTransaction));
    }

	char  datetime[8 + 1];
	double amount;
	double fee;
	uint64_t tid;
	uint64_t trade_id;
	uint64_t order_id;
    double price;
	char source[63+1];
	char symbol[63+1];
    int type;
    string to_string()
    {
        ostringstream os;
        os << "HuobFutureTransaction";
        os << ",datetime:" << datetime;
        os << ",amount:" << amount;
        os << ",fee:" << fee;
        os << ",tid:" << tid;
        os << ",trade_id:" << trade_id;
        os << ",order_id:" << order_id;
        os << ",price:" << price;
        os << ",source:" << source;
		os << ",symbol:" << symbol;
		os << ",type:" << type;
        return os.str();
    }
    int decode(const char* json)
    {
        Document doc;
        doc.Parse(json, strlen(json));
        if(doc.HasParseError())
        {
            return -1;
        }
		spotrapidjson::Value& statusNode = doc["status"];
		string status = statusNode.GetString();
		if (status == "ok")
		{
			spotrapidjson::Value& channelNode = doc["ch"];
			string channel = channelNode.GetString();
			//string inst = HuobApi::GetInstrumentID(channel.c_str());

			spotrapidjson::Value& tick = doc["tick"];
			spotrapidjson::Value& dataNodes = tick["data"];
			
			for (int i =0; i < dataNodes.Size(); i++)
			{
				spotrapidjson::Value& data = dataNodes[i];
				Value &vamount = data["amount"];
				if (vamount.IsString())
					amount = stod(vamount.GetString());
				else
					amount = vamount.GetDouble();

				Value &vprice = data["price"];
				if (vprice.IsString())
					price = stod(vprice.GetString());
				else
					price = vprice.GetDouble();

				memcpy(symbol, channel.c_str(), sizeof(symbol) - 1);


				Value &vtrade_id = data["id"];
				if (vtrade_id.IsString())
					trade_id = stoll(vtrade_id.GetString());
				else
					trade_id = vtrade_id.GetUint64();

			}
		}
        return 0;
    }
};
class HuobTransaction
{
public:
    HuobTransaction()
    {
        memset(this,0,sizeof(HuobTransaction));
    }

	char  datetime[8 + 1];
	double amount;
	double fee;
	uint64_t tid;
	uint64_t trade_id;
	uint64_t order_id;
    double price;
	char source[63+1];
	char symbol[63+1];
    int type;
    string to_string()
    {
        ostringstream os;
        os << "HuobTransaction";
        os << ",datetime:" << datetime;
        os << ",amount:" << amount;
        os << ",fee:" << fee;
        os << ",tid:" << tid;
        os << ",trade_id:" << trade_id;
        os << ",order_id:" << order_id;
        os << ",price:" << price;
        os << ",source:" << source;
		os << ",symbol:" << symbol;
		os << ",type:" << type;
        return os.str();
    }
    int decode(const char* json)
    {
        Document doc;
        doc.Parse(json, strlen(json));
        if(doc.HasParseError())
        {
            return -1;
        }
		spotrapidjson::Value& statusNode = doc["status"];
		string status = statusNode.GetString();
		if (status == "ok")
		{
			spotrapidjson::Value& data = doc["data"];
			Value &vdatetime = doc["created-at"];
			if (vdatetime.IsString())
				sprintf(datetime,"%d-%2d-%2d %2d:%2d:%2d",vdatetime.GetString());
			else
			{
				double  tradeTime = vdatetime.GetDouble();
				sprintf(datetime, "%d-%2d-%2d %2d:%2d:%2d", tradeTime, tradeTime, tradeTime, tradeTime, tradeTime, tradeTime);
			}
			Value &vamount = doc["filled-amount"];
			if (vamount.IsString())
				amount = stod(vamount.GetString());
			else
				amount = vamount.GetDouble();
			Value &vfee = doc["filled-fees"];
			if (vfee.IsString())
				fee = stod(vfee.GetString());
			else
				fee = vfee.GetDouble();
			Value &vtid = doc["tid"];
			if (vtid.IsString())
				tid = stoll(vtid.GetString());
			else
				tid = vtid.GetUint64(); 
			Value &vtrade_id = doc["match-id"];
			if (vtrade_id.IsString())
				trade_id = stoll(vtrade_id.GetString());
			else
				trade_id = vtrade_id.GetUint64();
			Value &vorder_id = doc["order-id"];
			if (vorder_id.IsString())
				order_id = stoll(vorder_id.GetString());
			else
				order_id = vorder_id.GetUint64();
			Value &vprice = doc["price"];
			if (vprice.IsString())
				price = stod(vprice.GetString());
			else
				price = vprice.GetDouble();

			Value &vsource = doc["source"];
			if (vsource.IsString())
				memcpy(source, vsource.GetString(), sizeof(source) - 1);

			Value &vsymbol = doc["symbol"];
			if (vsymbol.IsString())
				memcpy(symbol, vsymbol.GetString(),sizeof(symbol)-1);

			Value &vtype = doc["type"];
			if (vtype.IsString())
				type = stoi(vtype.GetString());
			else
				type = vtype.GetInt();			
		}
        return 0;
    }
};
class HuobOrder
{
public:
    HuobOrder()
    {
        memset(this,0,sizeof(HuobOrder));
    }
	char account_id[63 + 1];
    char instrument_id[30+1];
	double amount;
	double price;
	double tradeAmount;
	double tradeMoney;
	double fees;
	char finishedTime[8+1];
	char canceledTime[8 + 1];
	char createTime[8 + 1];
	int orderStatus;
	uint64_t orderID;
	uint64_t clientOrderID;
	int type;
	int offset;
	char source[63 + 1];

    string to_string()
    {
        ostringstream os;
        os << "HuobOrder";
        os << ",account_id:" << account_id;
        os << ",instrument_id:" << instrument_id;
        os << ",amount:" << amount;
        os << ",canceledTime:" << canceledTime;
        os << ",createTime:" << createTime;
        os << ",tradeAmount:" << tradeAmount;
        os << ",tradeMoney:" << tradeMoney;
		os << ",finishedTime:" << finishedTime;
		os << ",orderID:" << orderID;
		os << ",clientOrderID:" << clientOrderID;
		os << ",price:" << price;
		os << ",source:" << source;
		os << ",orderStatus:" << orderStatus;
		os << ",type:" << type;
		os << ",offset:" << offset;
        return os.str();
    }
    int decode(const char* json,bool isFuture)
    {
        Document doc;
        doc.Parse(json, strlen(json));
		cout << "json:" << json << endl;
        if(doc.HasParseError())
        {
			LOG_WARN << "HuobOrder::Parse error. result:" << json;
            return -1;
        }
		if (isFuture)
		{
			if (doc.HasMember("status"))
			{
				if (strcmp(doc["status"].GetString(), "ok") != 0)
				{
					LOG_WARN << "HuobFutureOrder::Request return error. result:" << json;
					return -1;
				}
				Value &dataNode = doc["data"];
				for (int i = 0; i < dataNode.Size(); i++)
				{
					spotrapidjson::Value& data = dataNode[i];
					Value &vsymbol = data["contract_code"];
					if (vsymbol.IsString())
						memcpy(instrument_id, vsymbol.GetString(), sizeof(instrument_id) - 1);

					Value &vamount = data["volume"];
					if (vamount.IsString())
						amount = stod(vamount.GetString()); 
					else
						amount = vamount.GetDouble();

					Value &vprice = data["price"];
					if (vprice.IsString())
						price = stod(vprice.GetString());
					else
						price = vprice.GetDouble();

					Value &vtradeAmount = data["trade_volume"];
					if (vtradeAmount.IsString())
						tradeAmount = stod(vtradeAmount.GetString());
					else
						tradeAmount = vtradeAmount.GetDouble();

					Value &vtradeMoney = data["trade_turnover"];
					if (vtradeMoney.IsString())
						tradeMoney = stod(vtradeMoney.GetString());
					else
						tradeMoney = vtradeMoney.GetDouble();

					Value &vfees = data["fee"];
					if (vfees.IsString())
						fees = stod(vfees.GetString()); 
					else
						fees = vfees.GetDouble();

					Value &vfinishedTime = data["created_at"];
					double fTime = vfinishedTime.GetDouble();
					sprintf(finishedTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
					sprintf(canceledTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
					sprintf(createTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);

					Value &vorder_id = data["order_id"];
					if (vorder_id.IsString())
						orderID = stoll(vorder_id.GetString());
					else
						orderID = vorder_id.GetInt64();

					Value &vclient_order_id = data["client_order_id"];
					if (vclient_order_id.IsString())
						clientOrderID = stoll(vclient_order_id.GetString());
					else
						clientOrderID = vclient_order_id.GetInt64();

					Value &vsource = data["order_source"];
					if (vsource.IsString())
						memcpy(source, vsource.GetString(), sizeof(vsource) - 1);

					Value &vdirection = data["direction"];
					string direction = vdirection.GetString();
					Value &vtype = data["order_price_type"];
					string ftype = vtype.GetString();
					Value &voffset = data["offset"];
					string foffset = voffset.GetString();

					if (strcmp(direction.c_str(), "buy") == 0)
					{
						if (strcmp(ftype.c_str(), "market") == 0)
						{
							type = huob_buy_market;
						}
						else if (strcmp(ftype.c_str(), "limit") == 0)
						{
							type = huob_buy_limit;
						}

						if (strcmp(foffset.c_str(), "open") == 0)
						{
							offset = 1;
						}
						else if (strcmp(foffset.c_str(), "close") == 0)
						{
							offset = 3;
						}
					}
					else if (strcmp(direction.c_str(), "sell") == 0)
					{
						if (strcmp(ftype.c_str(), "limit") == 0)
						{
							type = huob_sell_limit;
						}
						else if (strcmp(ftype.c_str(), "market") == 0)
						{
							type = huob_sell_market;
						}

						if (strcmp(foffset.c_str(), "open") == 0)
						{
							offset = 2;
						}
						else if (strcmp(foffset.c_str(), "close") == 0)
						{
							offset = 4;
						}

					}

					Value &vstate = data["status"];
					if (vstate.IsInt())
					{
						if (vstate.GetInt() == 1)
						{
							orderStatus = huob_pre_submitted;
						}
						else if (vstate.GetInt() == 2)
						{
							orderStatus = huob_submitting;
						}
						else if (vstate.GetInt() == 3)
						{
							orderStatus = huob_submitted;
						}
						else if (vstate.GetInt() == 4)
						{
							orderStatus = huob_partial_filled;
						}
						else if (vstate.GetInt() == 5)
						{
							orderStatus = huob_partial_canceled;
						}
						else if (vstate.GetInt() == 6)
						{
							orderStatus = huob_filled;
						}
						else if (vstate.GetInt() == 7)
						{
							orderStatus = huob_canceled;
						}
						else if (vstate.GetInt() == 11)
						{
							orderStatus = huob_canceling;
						}
					}
				}
			}
		}
		else
		{
			if (doc.HasMember("status") && strcmp(doc["status"].GetString(), "ok") != 0)
			{
				LOG_WARN << "HuobOrder::Request return error. result:" << json;
				return -1;
			}
			Value &data = doc["data"];

			Value &vaccount_id = data["account-id"];
			if (vaccount_id.IsString())
				memcpy(account_id, vaccount_id.GetString(), sizeof(account_id) - 1);

			Value &vsymbol = data["symbol"];
			if (vsymbol.IsString())
				memcpy(instrument_id, vsymbol.GetString(), sizeof(instrument_id) - 1);

			Value &vamount = data["amount"];
			if (vamount.IsString())
				amount = stod(vamount.GetString());

			Value &vprice = data["price"];
			if (vprice.IsString())
				price = stod(vprice.GetString());
			else
				price = vprice.GetDouble();

			if (data.HasMember("canceled-at"))
			{
				Value &vcanceledTime = data["canceled-at"];
				double fcanceledTime = vcanceledTime.GetDouble();
				sprintf(canceledTime, "%2d:%2d:%2d", fcanceledTime, fcanceledTime, fcanceledTime, fcanceledTime, fcanceledTime, fcanceledTime);
			}

			Value &vcreateTime = data["created-at"];
			double fcreateTime = vcreateTime.GetDouble();
			sprintf(createTime, "%2d:%2d:%2d", fcreateTime, fcreateTime, fcreateTime, fcreateTime, fcreateTime, fcreateTime);

			Value &vtradeAmount = data["field-amount"];
			if (vtradeAmount.IsString())
				tradeAmount = stod(vtradeAmount.GetString());

			Value &vtradeMoney = data["field-cash-amount"];
			if (vtradeMoney.IsString())
				tradeMoney = stod(vtradeMoney.GetString());

			Value &vfees = data["field-fees"];
			if (vfees.IsString())
				fees = stod(vfees.GetString());

			if (data.HasMember("finished-at"))
			{
				Value &vfinishedTime = data["finished-at"];
				double fTime = vfinishedTime.GetDouble();
				sprintf(finishedTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
			}

			Value &vorder_id = data["id"];
			if (vorder_id.IsString())
				orderID = stoll(vorder_id.GetString());
			else
				orderID = vorder_id.GetInt64();

			Value &vclient_order_id = data["client_order_id"];
			if (vclient_order_id.IsString())
				clientOrderID = stoll(vclient_order_id.GetString());
			else
				clientOrderID = vclient_order_id.GetInt64();

			Value &vsource = data["source"];
			if (vsource.IsString())
				memcpy(source, vsource.GetString(), sizeof(vsource) - 1);

			Value &vtype = data["type"];
			if (vtype.IsString())
			{
				string ftype = vtype.GetString();
				if (strcmp(ftype.c_str(), "buy-market") == 0)
				{
					type = huob_buy_market;
				}
				else if (strcmp(ftype.c_str(), "sell-market") == 0)
				{
					type = huob_sell_market;
				}
				else if (strcmp(ftype.c_str(), "buy-limit") == 0)
				{
					type = huob_buy_limit;
				}
				else if (strcmp(ftype.c_str(), "sell-limit") == 0)
				{
					type = huob_sell_limit;
				}
			}
			Value &vstate = data["state"];
			if (vstate.IsString())
			{
				string state = vstate.GetString();
				if (strcmp(state.c_str(), "pre-submitted") == 0)
				{
					orderStatus = huob_pre_submitted;
				}
				if (strcmp(state.c_str(), "submitting") == 0)
				{
					orderStatus = huob_submitting;
				}
				if (strcmp(state.c_str(), "submitted") == 0)
				{
					orderStatus = huob_submitted;
				}
				if (strcmp(state.c_str(), "partial-filled") == 0)
				{
					orderStatus = huob_partial_filled;
				}
				if (strcmp(state.c_str(), "partial-canceled") == 0)
				{
					orderStatus = huob_partial_canceled;
				}
				if (strcmp(state.c_str(), "filled") == 0)
				{
					orderStatus = huob_filled;
				}
				if (strcmp(state.c_str(), "canceled") == 0)
				{
					orderStatus = huob_canceled;
				}
			}
		}
        return 0;
    }
};

class HuobOrderDetail
{
public:
	HuobOrderDetail()
	{
		memset(this, 0, sizeof(HuobOrderDetail));
	}
	char account_id[63 + 1];
	char instrument_id[30 + 1];
	double amount;
	double price;
	double tradeAmount;
	double tradeMoney;
	double averagePrice;
	double tickTradeAmount; 
	double tickTradePrice;
	double fees;
	char finishedTime[8 + 1];
	char canceledTime[8 + 1];
	char createTime[8 + 1];
	int orderStatus;
	uint64_t orderID;
	int type;
	int offset;
	char source[63 + 1];

	string to_string()
	{
		ostringstream os;
		os << "HuobOrderDetail";
		os << ",account_id:" << account_id;
		os << ",instrument_id:" << instrument_id;
		os << ",amount:" << amount;
		os << ",canceledTime:" << canceledTime;
		os << ",createTime:" << createTime;
		os << ",tradeAmount:" << tradeAmount;
		os << ",tradeMoney:" << tradeMoney;
		os << ",averagePrice:" << averagePrice;
		os << ",tickTradeAmount:" << tickTradeAmount;
		os << ",tickTradePrice:" << tickTradePrice;
		os << ",finishedTime:" << finishedTime;
		os << ",orderID:" << orderID;
		os << ",price:" << price;
		os << ",source:" << source;
		os << ",orderStatus:" << orderStatus;
		os << ",type:" << type;
		os << ",offset:" << offset;
		return os.str();
	}
	int decode(const char* json, bool isFuture)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		cout << "json:" << json << endl;

		if (doc.HasParseError())
		{
			LOG_WARN << "HuobOrderDetail::Parse error. result:" << json;
			return -1;
		}
		if (isFuture)
		{
			if (doc.HasMember("status"))
			{
				if (strcmp(doc["status"].GetString(), "ok") != 0)
				{
					LOG_WARN << "HuobOrderDetail::Request return error. result:" << json;
					return -1;
				}
				Value &data = doc["data"];
				//for (int i = 0; i < dataNode.Size(); i++)
				{
					//spotrapidjson::Value& data = dataNode[i];
					Value &vsymbol = data["contract_code"];
					if (vsymbol.IsString())
						memcpy(instrument_id, vsymbol.GetString(), sizeof(instrument_id) - 1);

					Value &vamount = data["volume"];
					if (vamount.IsString())
						amount = stod(vamount.GetString());
					else
						amount = vamount.GetDouble();

					Value &vprice = data["price"];
					if (vprice.IsString())
						price = stod(vprice.GetString());
					else
						price = vprice.GetDouble();

					Value &vfees = data["fee"];
					if (vfees.IsString())
						fees = stod(vfees.GetString());
					else
						fees = vfees.GetDouble();

					Value &faveragePrice = data["trade_avg_price"];
					if (vfees.IsString())
						averagePrice = stod(faveragePrice.GetString());
					else
						averagePrice = faveragePrice.GetDouble();


					Value &vfinishedTime = data["created_at"];
					double fTime = vfinishedTime.GetDouble();
					sprintf(finishedTime, "%d-%2d-%2d %2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
					sprintf(canceledTime, "%d-%2d-%2d %2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
					sprintf(createTime, "%d-%2d-%2d %2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);

					Value &vorder_id = data["order_id"];
					if (vorder_id.IsString())
						orderID = stoll(vorder_id.GetString());
					else
						orderID = vorder_id.GetInt64();

					Value &vsource = data["order_source"];
					if (vsource.IsString())
						memcpy(source, vsource.GetString(), sizeof(vsource) - 1);

					Value &vdirection = data["direction"];
					string direction = vdirection.GetString();
					Value &vtype = data["order_price_type"];
					string ftype = vtype.GetString();
					Value &voffset = data["offset"];
					string foffset = voffset.GetString();

					if (strcmp(direction.c_str(), "buy") == 0)
					{
						if (strcmp(ftype.c_str(), "market") == 0)
						{
							type = huob_buy_market;
						}
						else if (strcmp(ftype.c_str(), "limit") == 0)
						{
							type = huob_buy_limit;
						}

						if (strcmp(foffset.c_str(), "open") == 0)
						{
							offset = 1;
						}
						else if (strcmp(foffset.c_str(), "close") == 0)
						{
							offset = 3;
						}
					}
					else if (strcmp(direction.c_str(), "sell") == 0)
					{
						if (strcmp(ftype.c_str(), "limit") == 0)
						{
							type = huob_sell_limit;
						}
						else if (strcmp(ftype.c_str(), "market") == 0)
						{
							type = huob_sell_market;
						}

						if (strcmp(foffset.c_str(), "open") == 0)
						{
							offset = 2;
						}
						else if (strcmp(foffset.c_str(), "close") == 0)
						{
							offset = 4;
						}

					}

					Value &vstate = data["status"];
					if (vstate.IsInt())
					{
						if (vstate.GetInt() == 1)
						{
							orderStatus = huob_pre_submitted;
						}
						else if (vstate.GetInt() == 2)
						{
							orderStatus = huob_submitting;
						}
						else if (vstate.GetInt() == 3)
						{
							orderStatus = huob_submitted;
						}
						else if (vstate.GetInt() == 4)
						{
							orderStatus = huob_partial_filled;
						}
						else if (vstate.GetInt() == 5)
						{
							orderStatus = huob_partial_canceled;
						}
						else if (vstate.GetInt() == 6)
						{
							orderStatus = huob_filled;
						}
						else if (vstate.GetInt() == 7)
						{
							orderStatus = huob_canceled;
						}
						else if (vstate.GetInt() == 11)
						{
							orderStatus = huob_canceling;
						}
					}

					Value & vtrades = data["trades"];
					for (int i = 0; i < vtrades.Size(); i++)
					{
						Value &vtrade = vtrades[i];
						vtrade["trade_id"].GetInt64();
						tickTradeAmount = vtrade["trade_volume"].GetDouble();
						tradeAmount += tickTradeAmount;
						tickTradePrice = vtrade["trade_price"].GetDouble();
						
						double trade_fee= vtrade["trade_fee"].GetDouble();
						double trade_turnover = vtrade["trade_turnover"].GetDouble();
						//vtrade["created_at"].GetInt64();

					}
				}
			}
		}
		else
		{			
		}
		return 0;
	}
};


class HuobRspReqOrder
{
public:
	HuobRspReqOrder()
	{
		memset(this, 0, sizeof(HuobRspReqOrder));
	}
	uint64_t id;
	string to_string()
	{
		ostringstream os;
		os << "HuobRspReqOrder";
		os << ",id:" << id;
		return os.str();
	}
	int decode(const char* json,bool isFuture)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("status") )
		{
			if (doc["status"].IsNumber())
			{
				int status = doc["status"].GetInt();
				if (status == 404)
				{
					LOG_WARN << " HuobApi::ReqOrderInsert result:" << status;
					cout << " HuobApi::ReqOrderInsert result:" << status << endl;
				}
				return status;
			}
			if (isFuture)
			{
				if (strcmp(doc["status"].GetString(), "ok") != 0)
				{
					int errCode = doc["err_code"].GetInt();
					string errMsg = doc["err_msg"].GetString();
					LOG_WARN << " ReqOrderInsert result, errCode:" << errCode << ",errMsg:" << errMsg;
					cout << " ReqOrderInsert result, errCode:" << errCode << ",errMsg:" << errMsg << endl;
					return -1;
				}

			}
			else
			{
				if (strcmp(doc["status"].GetString(), "ok") != 0)
				{
					string errCode = doc["err-code"].GetString();
					string errMsg = doc["err-msg"].GetString();
					LOG_WARN << " ReqOrderInsert result, errCode:" << errCode << ",errMsg:" << errMsg;
					cout << " ReqOrderInsert result, errCode:" << errCode << ",errMsg:" << errMsg << endl;
					return -1;
				}

			}			
		}

		if (isFuture)
		{
			Value &dataNode = doc["data"];
			Value &vid = dataNode["order_id"];

			if (vid.IsString())
				id = stoll(vid.GetString());
			else
				id = vid.GetInt64();
		}
		else
		{
			Value &vid = doc["data"];
			if (vid.IsString())
				id = stoll(vid.GetString());
			else
				id = vid.GetUint64();
		}
		return 0;
	}
};
class HuobRspCancelOrder
{
public:
	HuobRspCancelOrder()
	{
		memset(this, 0, sizeof(HuobRspCancelOrder));
	}
	uint64_t id;
	string to_string()
	{
		ostringstream os;
		os << "HuobRspReqOrder";
		os << ",id:" << id;
		return os.str();
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "HuobApi::CancelOrder Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("status"))
		{
			if (doc["status"].IsNumber())
			{
				int status = doc["status"].GetInt();
				if (status == 404)
				{
					LOG_WARN << " HuobApi::CancelOrder result:" << status;
					cout << " HuobApi::CancelOrder result:" << status << endl;
				}
				return status;
			}

			if (strcmp(doc["status"].GetString(), "ok") != 0)
			{
				string errCode = doc["err-code"].GetString();
				string errMsg = doc["err-msg"].GetString();
				LOG_WARN << " HuobApi::CancelOrder result, errCode:" << errCode << ",errMsg:" << errMsg;
				cout << " HuobApi::CancelOrder result, errCode:" << errCode << ",errMsg:" << errMsg << endl;
				return -1;
			}
		}

		Value &vid = doc["data"];
		if (vid.IsString())
			id = stoll(vid.GetString());
		else
			id = vid.GetUint64();
		

		return 0;
	}
};

class HuobRspCancelFutureOrder
{
public:
	HuobRspCancelFutureOrder()
	{
		memset(this, 0, sizeof(HuobRspCancelFutureOrder));
	}
	uint64_t id;
	string to_string()
	{
		ostringstream os;
		os << "HuobRspCancelFutureOrder";
		os << ",id:" << id;
		return os.str();
	}
	int decode(const char* json)
	{
		Document doc;
		doc.Parse(json, strlen(json));
		if (doc.HasParseError())
		{
			LOG_WARN << "HuobApi::CancelFutureOrder Parse error. result:" << json;
			return -1;
		}

		if (doc.HasMember("status"))
		{
			if (doc["status"].IsNumber())
			{
				int status = doc["status"].GetInt();
				if (status == 404)
				{
					LOG_WARN << " HuobApi::CancelFutureOrder result:" << status;
					cout << " HuobApi::HuobRspCancelFutureOrder result:" << status << endl;
				}
				return status;
			}

			if (strcmp(doc["status"].GetString(), "ok") != 0)
			{
				int errCode = doc["err_code"].GetInt();
				string errMsg = doc["err_msg"].GetString();
				LOG_WARN << " HuobApi::CancelFutureOrder result, errCode:" << errCode << ",errMsg:" << errMsg;
				cout << " HuobApi::CancelFutureOrder result, errCode:" << errCode << ",errMsg:" << errMsg << endl;
				return -1;
			}
		}
		Value &dataNode = doc["data"];
		Value &errorNode = dataNode["errors"];
		Value &successNode = dataNode["successes"];

		for (int i = 0; i < errorNode.Size(); i++)
		{
			Value &error = errorNode[i];
			string order_id = error["order_id"].GetString();
			int err_code = error["err_code"].GetInt();
			string err_msg = error["err_msg"].GetString();

			LOG_INFO << "order_id:" << order_id << ",err_code:" << err_code << ",err_msg:" << err_msg;

			if (err_code == 1062 || err_code == 1063)
			{
				LOG_INFO << "Order Cancel request submitted sucessfully, result will follow later, orderid =  " << order_id;
				return 0;
			}
			return -1;
		}


		id = stoll(successNode.GetString());
		LOG_INFO << "order_id:" << id;
		return 0;
	}
};

#endif
