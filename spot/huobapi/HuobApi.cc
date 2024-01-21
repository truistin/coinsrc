#include "string.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include "spot/utility/Logging.h"
#include "spot/utility/TradingDay.h"
#include "spot/restful/UriFactory.h"
#include "spot/huobapi/HuobApi.h"
#include <openssl/hmac.h>
#include "spot/base/DataStruct.h"
#include "base64/base64.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/connect.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/read.hpp"
#include "boost/asio/read_until.hpp"
#include "boost/system/system_error.hpp"


using namespace boost::asio;
//现货接口
//=============================================================//
#define HUOB_DOMAIN "api.huobpro.com"
//行情接口方法
#define HUOB_Symbols_API "/v1/common/symbols"
#define HUOB_DepthMarket_API "/market/depth"
//用户资产API
#define HUOB_ACCOUNT_API "/v1/account/accounts"
#define HUOB_ACCOUNT_BALANCE_API "/v1/account/accounts/"
//交易接口方法
#define HUOB_USER_TRANSACTIONS_API "/v1/order/orders/"
#define HUOB_ORDER_API "/v1/order/orders/"
#define HUOB_OPEN_ORDERS_API "/v1/order/openOrders"
#define HUOB_ORDER_INSERT_API "/v1/order/orders/place"
#define HUOB_CANCEL_ORDER_API "/v1/order/orders/"


//=============================================================//
#define HUOB_FUTURE_DOMAIN "api.hbdm.com"
#define HUOB_FUTURE_MARKET_DOMAIN "172.18.6.16:8882"
//期货市场接口方法
#define HUOB_FUTURE_CONTRACT_API "/api/v1/contract_contract_info"
#define HUOB_FUTURE_CONTRACT_PRICE_LIMIT_API "/api/v1/contract_price_limit"
#define HUOB_FUTURE_CONTRACT_OPEN_INTEREST_API "/api/v1/contract_open_interest"

#define HUOB_FUTURE_CONTRACT_INDEX_API "/api/v1/contract_index"
#define HUOB_FUTURE_DEPTH_MARKET_API "/market/depth"
#define HUOB_FUTURE_TRADE_API "/market/trade"
#define HUOB_FUTURE_HISTORY_TRADE_API "/market/history/trade"
//期货账户资产接口方法
#define HUOB_FUTURE_ACCOUNT_API "/api/v1/contract_account_info"
#define HUOB_FUTURE_POSITION_API "/api/v1/contract_position_info"
//期货交易接口方法
#define HUOB_FUTURE_ORDER_API "/api/v1/contract_order_info"
#define HUOB_FUTURE_ORDER_DETAIL_API "/api/v1/contract_order_detail"
#define HUOB_FUTURE_OPEN_ORDER_API "/api/v1/contract_openorders"
#define HUOB_FUTURE_TRADE_ORDER_API "/api/v1/contract_tradeorders"

#define HUOB_FUTURE_ORDER_INSERT_API "/api/v1/contract_order"
#define HUOB_BATCH_FUTURE_ORDER_INSERT_API "/api/v1/contract_batchorder"
#define HUOB_FUTURE_ORDER_CANCEL_API "/api/v1/contract_cancel"

using namespace std;
using namespace std::chrono;
using namespace spot::base;

std::map<string, string> HuobApi::channelToInstrumentMap_;
std::map<string, string> HuobApi::futureChannelToLongInstrumentMap_;
std::map<string, string> HuobApi::futureChannelToShortInstrumentMap_;

HuobApi::HuobApi(string api_key, string secret_key, string customer_id)
{
	m_uri.protocol = HTTP_PROTOCOL_HTTPS;
	m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
	m_uri.domain = HUOB_DOMAIN;
	m_uri.method = METHOD_GET;
	m_uri.isinit = true;
	m_nonce = duration_cast<chrono::microseconds>(system_clock::now().time_since_epoch()).count();	
	m_api_key = api_key;
	m_secret_key = secret_key;
	m_customer_id = customer_id;
}
HuobApi::~HuobApi()
{
}
//Set api for uri before SetPrivateParams, since SetPrivateParams has to create sign using api 
void HuobApi::SetPrivateParams(int http_mode, bool isFuture, int body_format)
{
	if (body_format == JSON_TYPE)
	{
		m_uri.m_body_format = JSON_TYPE;
	}

	if (isFuture)
	{
		m_uri.protocol = HTTP_PROTOCOL_HTTP;
		m_uri.urlprotocolstr = URL_PROTOCOL_HTTP;
	}
	else
	{
		m_uri.protocol = HTTP_PROTOCOL_HTTPS;
		m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
	}

	if (http_mode == HTTP_GET)
	{
		m_uri.AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36\r\n");
		m_uri.AddHeader("Content-Type","application/x-www-form-urlencoded");
		m_uri.method = METHOD_GET;
	}
	else if(http_mode == HTTP_POST)
	{
		m_uri.AddHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36\r\n");
		m_uri.AddHeader("Accept-Language", ":zh-cn");
		m_uri.AddHeader("Accept", "application/json");
		m_uri.AddHeader("Content-Type: application/json; charset=UTF-8");

		m_uri.method = METHOD_POST;
	}
	
	////添加参数，参数进行URL编码
	m_uri.AddParam(UrlEncode("AccessKeyId"), UrlEncode(m_api_key));
	m_uri.AddParam(UrlEncode("SignatureMethod"), UrlEncode("HmacSHA256"));
	m_uri.AddParam(UrlEncode("SignatureVersion"), UrlEncode("2"));
	string timeStamp = UrlEncode(getUTCTime());
	m_uri.AddParam(UrlEncode("Timestamp"), timeStamp);
	//m_uri.AddParam(UrlEncode("Timestamp"), UrlEncode("2018-08-27T08:23:55"));
	if (isFuture && http_mode == HTTP_GET)
	{
		return;
	}

	//按照签名要求构建消息体
	string sig_url = CreateSignature(http_mode, m_secret_key, "sha256");
	m_uri.AddParam(UrlEncode("Signature"), sig_url);

	if (http_mode == HTTP_POST)
	{
		m_uri.api += string("?") + m_uri.GetParamSet();
		m_uri.ClearParam();
	}
}


string HuobApi::CreateSignature(int http_mode, string secret_key,const char* encode_mode)
{
	string message = "";
	if (http_mode == HTTP_GET)
	{
		message = "GET\n" + string(m_uri.domain) + "\n" + m_uri.api + "\n" + m_uri.GetParamSet();
	}
	else if (http_mode == HTTP_POST)
	{
		message = "POST\n" + string(m_uri.domain) + "\n" + m_uri.api + "\n" + m_uri.GetParamSet();
	}
	/********************************签名 begin**********************************/
	
	//对消息体和密码进行UTF8编码
	string message_utf8 = string_To_UTF8(message);
	string secret_utf8 = string_To_UTF8(secret_key);
	//LOG_INFO << "message_utf8:" << message_utf8;

	////进行HMAC_SHA256加密
	unsigned char* output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	unsigned int output_length = 0;
	HmacEncode(encode_mode, secret_utf8.c_str(), message_utf8.c_str(), output, &output_length);
	////对HMAC_SHA256加密结果进行Base64编码
	string sig_base64 = websocketpp::base64_encode(output, output_length);

	//对Base64编码结果进行url编码
	string sig_url = UrlEncode(sig_base64);
	if (output)
	{
		free(output);
	}
	/********************************签名 end**********************************/
	return sig_url;
}

bool HuobApi::GetSymbols()
{
	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_Symbols_API;
	SetPrivateParams(HTTP_GET,false);

	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetSymbols parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		auto dataSize = dataNode.Size();
		for (unsigned int i = 0; i < dataSize; ++i)
		{
			spotrapidjson::Value& data = dataNode[i];

			string base_currency = data["base-currency"].GetString();
			string quote_currency = data["quote-currency"].GetString();

			LOG_INFO << "HuobApi::GetSymbols symbol:" << base_currency + quote_currency;
			cout << "HuobApi::GetSymbols symbol:" << base_currency + quote_currency << endl;
		}
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetSymbols result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetSymbols result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;

}
bool HuobApi::GetDepthMarket(string inst, int type)
{
	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;

	m_uri.api = HUOB_DepthMarket_API;
	string cp = GetCurrencyPair(inst);
	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(cp));

	switch (type)
	{
		case 0:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step0"));
			break;
		case 1:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step1"));
			break;
		case 2:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step2"));
			break;
		case 3:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step3"));
			break;
		case 4:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step4"));
			break;
		case 5:
			m_uri.AddParam(UrlEncode("type"), UrlEncode("step5"));
			break;
		default:
			break;
	}

	SetPrivateParams(HTTP_GET,false);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << " HuobApi::GetDepthMarket parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		strcpy(m_field.UpdateTime, getCurrentSystemTime().c_str());
		string channel = doc["ch"].GetString();
		string inst = HuobApi::GetInstrumentID(channel,true);
		strcpy(m_field.InstrumentID, inst.c_str());

		spotrapidjson::Value& tickNode = doc["tick"];
		spotrapidjson::Value& asks = tickNode["asks"];
		spotrapidjson::Value& bids = tickNode["bids"];
		auto askSize =  asks.Size();
		for (unsigned int i = 0; i < askSize; ++i)
		{
			spotrapidjson::Value& priceNode = asks[i][0];
			double price = priceNode.GetDouble();
			m_field.setAskPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = asks[i][1];
			double volume = volumeNode.GetDouble();
			m_field.setAskVolume(i + 1, volume);
		}
		auto bidSize = bids.Size();
		for (unsigned int i = 0; i < bidSize; ++i)
		{
			spotrapidjson::Value& priceNode = bids[i][0];
			double price = priceNode.GetDouble();
			m_field.setBidPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = bids[i][1];
			double volume = volumeNode.GetDouble();
			m_field.setBidVolume(i + 1, volume);
		}

		cout << "InstrumentID:"<< channel <<endl;
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetDepthMarket result,status:" << status << ",errCode:"<< errCode<<",errMsg:"<< errMsg;
		cout << " HuobApi::GetDepthMarket result,status:" << status << ",errCode:" << errCode<<",errMsg:" << errMsg<<endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;

}
bool HuobApi::GetUserTransactions(string orderID,list<HuobTransaction> &transaction_list)
{

	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_USER_TRANSACTIONS_API+ orderID+ "/matchresults";
	m_uri.AddParam(UrlEncode("order-id"), UrlEncode(orderID));

	SetPrivateParams(HTTP_GET,false);
	m_uri.Request();

	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetUserTransactions return empty." ;
		return false;
	}
	HuobTransaction obj;
	obj.decode(res.c_str());
	transaction_list.push_back(obj);
	return true;
}
bool HuobApi::GetOrder(string orderID, HuobOrder& order)
{

	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_ORDER_API+ orderID;
	m_uri.AddParam(UrlEncode("order-id"), UrlEncode(orderID));

	SetPrivateParams(HTTP_GET, false);
	m_uri.Request();

	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetUserTransactions return empty.";
		return false;
	}

	int ret = order.decode(res.c_str(),false);
	if (ret != 0)
	{
		LOG_WARN << "HuobApi::GetOrder decode failed";
		return false;
	}
	return true;
}

bool HuobApi::GetOpenOrders(string symbol, list<HuobOrder>& order_list)
{
	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_OPEN_ORDERS_API;
	m_uri.AddParam(UrlEncode("account-id"), UrlEncode(m_customer_id));
	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(symbol));

	SetPrivateParams(HTTP_GET, false);
	m_uri.Request();

	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetOpenOrders return empty.";
		return false;
	}

	Document doc;
	doc.Parse(res.c_str(), strlen(res.c_str()));
	if (doc.HasParseError())
	{
		return -1;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		HuobOrder order;

		spotrapidjson::Value& dataNodes = doc["data"];
		for (int i = 0; i < dataNodes.Size(); i++)
		{
			spotrapidjson::Value& data = dataNodes[i];
			Value &vamount = data["amount"];
			if (vamount.IsString())
				order.amount = stod(vamount.GetString());
			else
				order.amount = vamount.GetDouble();

			Value &vprice = data["price"];
			if (vprice.IsString())
				order.price = stod(vprice.GetString());
			else
				order.price = vprice.GetDouble();

			Value &vsymbol = data["symbol"];
			string symbol = vsymbol.GetString();
			memcpy(order.instrument_id, symbol.c_str(), sizeof(symbol) - 1);

			Value &vid = data["id"];
			if (vid.IsString())
				order.orderID = stoll(vid.GetString());
			else
				order.orderID = vid.GetUint64();

			order_list.push_back(order);
		}
	}
	return true;
}

bool HuobApi::GetAccounts(list<string>* accountList)
{
	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_ACCOUNT_API;
	SetPrivateParams(HTTP_GET, false);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetAccounts return empty.";
		return false;
	}
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetAccounts parse error,result:" << res;
		return false;
	}

	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		auto dataSize = dataNode.Size();
		for (unsigned int i = 0; i < dataSize; ++i)
		{
			spotrapidjson::Value& data = dataNode[i];

			int accoutID = data["id"].GetInt();
			string type = data["type"].GetString();
			string state = data["state"].GetString();

			cout << "accoutID:"<< accoutID<<endl;
			char accout_id[63+1];
			sprintf(accout_id, "%d", accoutID);
			accountList->push_back(accout_id);
		}
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetAccounts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetAccounts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/

	return true;
}
bool HuobApi::GetAccountBalance(string accountID)
{
	m_uri.clear();
	m_uri.domain = HUOB_DOMAIN;
	m_uri.api = HUOB_ACCOUNT_BALANCE_API + accountID + "/balance";
	m_uri.AddParam(UrlEncode("account-id"), UrlEncode(accountID));
	SetPrivateParams(HTTP_GET, false);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetAccountBalanceparse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];

		int64_t accountID = dataNode["id"].GetInt64();
		string type = dataNode["type"].GetString();
		string state = dataNode["state"].GetString();

		spotrapidjson::Value& listNode = dataNode["list"];
		auto listSize = listNode.Size();
		for (unsigned int i = 0; i < listSize; ++i)
		{
			spotrapidjson::Value& data = listNode[i];
			string currency = data["currency"].GetString();
			string type = data["type"].GetString();
			string balance = data["balance"].GetString();
		}
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetAccountBalance result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetAccountBalance result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
uint64_t HuobApi::ReqOrderInsert(HuobOrder &order)
{
	list<string> accountList;
	GetAccounts(&accountList);

	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_DOMAIN;
		m_uri.api = HUOB_ORDER_INSERT_API;
		SetPrivateParams(HTTP_POST, false, JSON_TYPE);
		
		m_uri.AddParam(UrlEncode("account-id"), UrlEncode(order.account_id));

		char buf[64];
		SNPRINTF(buf, sizeof(buf), "%.4f", order.amount);
		m_uri.AddParam(UrlEncode("amount"), UrlEncode(buf));

		SNPRINTF(buf, sizeof(buf), "%.4f", order.price);
		m_uri.AddParam(UrlEncode("price"), UrlEncode(buf));

		string cp = GetCurrencyPair(order.instrument_id);
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(cp));
		if (order.type == huob_buy_limit)
		{
			m_uri.AddParam(UrlEncode("type"), UrlEncode("buy-limit"));
		}
		else if (order.type == huob_sell_limit)
		{
			m_uri.AddParam(UrlEncode("type"), UrlEncode("sell-limit"));
		}
		m_uri.AddParam(UrlEncode("source"), UrlEncode("api"));

		m_uri.Request();
		string &res = m_uri.result;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::ReqOrderInsert return empty.";
			return 0;
		}
		HuobRspReqOrder rspOrder;
		int ret = rspOrder.decode(res.c_str(),false);
		LOG_INFO <<"HuobApi::ReqOrderInsert result:" << rspOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::ReqOrderInsert decode failed";
			return 0;
		}
		return rspOrder.id;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::ReqOrderInsert exception occur:" << ex.what();
		return 0;
	}
}
bool HuobApi::CancelOrder(string &order_id)
{
	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_DOMAIN;
		m_uri.api = HUOB_CANCEL_ORDER_API + order_id + "/submitcancel";
		SetPrivateParams(HTTP_POST, false, JSON_TYPE);

		m_uri.AddParam(UrlEncode("id"), UrlEncode(order_id));
		m_uri.Request();

		string &res = m_uri.result;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::CancelOrder return empty. orderid:" << order_id;
			return false;
		}
		HuobRspCancelOrder rspCancleOrder;
		int ret = rspCancleOrder.decode(res.c_str());
		LOG_INFO << "HuobApi::CancelOrder result:" << rspCancleOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::CancelOrder decode failed";
			return false;
		}
		return true;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::CancelOrder exception occur:" << ex.what();
		return false;
	}
}

void HuobApi::AddInstrument(Instrument *instrument, bool isFuture)
{
	string channel = GetDepthChannel(instrument, isFuture);
	if (isFuture)
	{
		if (instrument->isCryptoFutureLongInstrument())
		{
			futureChannelToLongInstrumentMap_[channel] = instrument->getInstrumentID();
		}
		else
		{
			futureChannelToShortInstrumentMap_[channel] = instrument->getInstrumentID();
		}
	}
	else
	{
		channelToInstrumentMap_[channel] = instrument->getInstrumentID();
	}

	channel = GetTradeChannel(instrument, isFuture);
	if (isFuture)
	{
		if (instrument->isCryptoFutureLongInstrument())
		{
			futureChannelToLongInstrumentMap_[channel] = instrument->getInstrumentID();
		}
		else
		{
			futureChannelToShortInstrumentMap_[channel] = instrument->getInstrumentID();
		}
	}
	else
	{
		channelToInstrumentMap_[channel] = instrument->getInstrumentID();
	}
}
string HuobApi::GetDepthChannel(Instrument *inst, bool isFuture)
{
	string cp = "";
	if (isFuture)
	{
		//BTC0910_l_hb --> BTC_CW
		cp = GetFutureCurrencyPair(inst);
	}
	else
	{
		//btc_usdt_h --> btcusdt
		string instrumentID = inst->getInstrumentID();
		cp = GetCurrencyPair(instrumentID);
	}

	//convert to channel
	return "market." + cp + ".depth.step0";
}
string HuobApi::GetTradeChannel(Instrument *inst, bool isFuture)
{
	string cp = "";
	if (isFuture)
	{
		//BTC0910_l_hb --> BTC_CW
		cp = GetFutureCurrencyPair(inst);
	}
	else
	{
		//btc_usdt_h --> btcusdt
		string instrumentID = inst->getInstrumentID();
		cp = GetCurrencyPair(instrumentID);
	}

	//convert to channel
	return "market." + cp + ".trade.detail";
}
string HuobApi::GetInstrumentID(string channel, bool isLong)
{
	auto iter = channelToInstrumentMap_.find(channel);
	if (iter != channelToInstrumentMap_.end())
	{
		return iter->second;
	}

	if (isLong)
	{
		auto fiter = futureChannelToLongInstrumentMap_.find(channel);
		if (fiter != futureChannelToLongInstrumentMap_.end())
		{
			return fiter->second;
		}
	}
	else
	{
		auto fiter = futureChannelToShortInstrumentMap_.find(channel);
		if (fiter != futureChannelToShortInstrumentMap_.end())
		{
			return fiter->second;
		}
	}
	
	
	return "";
}
std::map<string, string> HuobApi::GetChannelMap()
{
	return channelToInstrumentMap_;
}
string HuobApi::GetCurrencyPair(string inst)
{
	//btc_usdt_h --> btcusdt
	string cp = inst.substr(0, inst.find_last_of('_'));
	cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
	return cp;
}
uint64_t HuobApi::GetCurrentNonce()
{
	return m_nonce++;
}

//期货接口
bool HuobApi::GetFutureConstracts()
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_CONTRACT_API;
	SetPrivateParams(HTTP_GET, true);

	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetFutureConstracts parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		auto dataSize = dataNode.Size();
		for (unsigned int i = 0; i < dataSize; ++i)
		{
			spotrapidjson::Value& data = dataNode[i];

			string symbol = data["symbol"].GetString();
			string contract_code = data["contract_code"].GetString();
			string contract_type = data["contract_type"].GetString();
			double contract_size = data["contract_size"].GetDouble();
			double price_tick = data["price_tick"].GetDouble();
			string delivery_date = data["delivery_date"].GetString();
			string create_date = data["create_date"].GetString();
			int contract_status = data["contract_status"].GetInt();

			LOG_INFO << "HuobApi::GetFutureConstracts symbol:" << symbol <<", contract_code:"<< contract_code
				<< ", contract_type:" << contract_type << ", contract_size:" << contract_size
				<< ", price_tick:" << price_tick<< ", delivery_date:" << delivery_date 
				<< ", create_date:" << create_date<< ", contract_status:" << contract_status;
			cout << "HuobApi::GetFutureConstracts symbol:" << symbol << ", contract_code:" << contract_code
				<< ", contract_type:" << contract_type << ", contract_size:" << contract_size
				<< ", price_tick:" << price_tick << ", delivery_date:" << delivery_date
				<< ", create_date:" << create_date << ", contract_status:" << contract_status << endl;
		}
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureConstracts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureConstracts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
bool HuobApi::GetFutureConstractIndex(string inst)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_CONTRACT_INDEX_API;

	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	SetPrivateParams(HTTP_GET, true);

	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetFutureConstractIndex parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		for (int i=0; i < dataNode.Size(); i++)
		{
			spotrapidjson::Value& indexNode = dataNode[i];
			string symbol = indexNode["symbol"].GetString();
			if (indexNode.HasMember("current_index"))
			{
				Type current_index = indexNode["current_index""_index"].GetType();
			}

			//LOG_INFO << "HuobApi::GetFutureConstractIndex symbol:" << symbol << ", current_index:" << current_index;
			//cout << "HuobApi::GetFutureConstractIndex symbol:" << symbol << ", current_index:" << current_index << endl;
		}
		
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureConstractIndex result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureConstractIndex result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
bool HuobApi::GetFutureContractPriceLimit(string inst)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_CONTRACT_PRICE_LIMIT_API;

	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	SetPrivateParams(HTTP_GET, true);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetFutureContractPriceLimit parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		for (int i = 0; i < dataNode.Size(); i++)
		{
			spotrapidjson::Value& limitNode = dataNode[i];
			string symbol = limitNode["symbol"].GetString();

			double high_limit = limitNode["high_limit"].GetDouble();
			double low_limit = limitNode["low_limit"].GetDouble();
			string contract_code = limitNode["contract_code"].GetString();
			string contract_type = limitNode["contract_type"].GetString();
			
			LOG_INFO << "HuobApi::GetFutureContractPriceLimit symbol:" << symbol 
				<< ", high_limit:" << high_limit << ", low_limit:" << low_limit
				<< ", contract_code:" << contract_code << ", contract_type:" << contract_type;
			cout << "HuobApi::GetFutureContractPriceLimit symbol:" << symbol
				<< ", high_limit:" << high_limit << ", low_limit:" << low_limit
				<< ", contract_code:" << contract_code << ", contract_type:" << contract_type << endl;
		}

	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureContractPriceLimit result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureContractPriceLimit result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
bool HuobApi::GetFutureContractOpenInterest(string inst)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_CONTRACT_OPEN_INTEREST_API;

	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	SetPrivateParams(HTTP_GET, true);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << "HuobApi::GetFutureContractOpenInterest parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		for (int i = 0; i < dataNode.Size(); i++)
		{
			spotrapidjson::Value& limitNode = dataNode[i];
			string symbol = limitNode["symbol"].GetString();

			double volume = limitNode["volume"].GetDouble();
			double amount = limitNode["amount"].GetDouble();
			string contract_code = limitNode["contract_code"].GetString();
			string contract_type = limitNode["contract_type"].GetString();

			LOG_INFO << "HuobApi::GetFutureContractOpenInterest symbol:" << symbol
				<< ", volume:" << volume << ", amount:" << amount
				<< ", contract_code:" << contract_code << ", contract_type:" << contract_type;
			cout << "HuobApi::GetFutureContractOpenInterest symbol:" << symbol
				<< ", volume:" << volume << ", amount:" << amount
				<< ", contract_code:" << contract_code << ", contract_type:" << contract_type << endl;
		}

	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureContractOpenInterest result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureContractOpenInterest result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}

bool HuobApi::GetFutureHistoryTrade(string inst, list<HuobTransaction> &transaction_list){return true;}

bool HuobApi::GetFutureDepthMarket(string inst, int type)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_MARKET_DOMAIN;
	m_uri.api = HUOB_FUTURE_DEPTH_MARKET_API;

	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	switch (type)
	{
	case 0:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step0"));
		break;
	case 1:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step1"));
		break;
	case 2:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step2"));
		break;
	case 3:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step3"));
		break;
	case 4:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step4"));
		break;
	case 5:
		m_uri.AddParam(UrlEncode("type"), UrlEncode("step5"));
		break;
	default:
		break;
	}

	SetPrivateParams(HTTP_GET, true);
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << " HuobApi::GetFutureDepthMarket parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		strcpy(m_field.UpdateTime, getCurrentSystemTime().c_str());
		string channel = doc["ch"].GetString();
		string inst = HuobApi::GetInstrumentID(channel,true);
		strcpy(m_field.InstrumentID, inst.c_str());

		spotrapidjson::Value& tickNode = doc["tick"];
		spotrapidjson::Value& asks = tickNode["asks"];
		spotrapidjson::Value& bids = tickNode["bids"];
		auto askSize = asks.Size();
		for (unsigned int i = 0; i < askSize; ++i)
		{
			spotrapidjson::Value& priceNode = asks[i][0];
			double price = priceNode.GetDouble();
			m_field.setAskPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = asks[i][1];
			double volume = volumeNode.GetDouble();
			m_field.setAskVolume(i + 1, volume);
		}
		auto bidSize = bids.Size();
		for (unsigned int i = 0; i < bidSize; ++i)
		{
			spotrapidjson::Value& priceNode = bids[i][0];
			double price = priceNode.GetDouble();
			m_field.setBidPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = bids[i][1];
			double volume = volumeNode.GetDouble();
			m_field.setBidVolume(i + 1, volume);
		}

		LOG_INFO << "HuobApi::GetFutureDepthMarket channel:" << channel;
		cout << "HuobApi::GetFutureDepthMarket channel:" << channel << endl;
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureDepthMarket result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureDepthMarket result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
bool HuobApi::GetFutureAccounts(string inst)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_ACCOUNT_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (strcmp(inst.c_str(), "") != 0)
	{
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	}

	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << " HuobApi::GetFutureAccounts parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];
		for (int i = 0; i < dataNode.Size(); i++)
		{
			spotrapidjson::Value& data = dataNode[i];

			string channel = data["symbol"].GetString();
			double margin_balance = data["margin_balance"].GetDouble();
			double margin_position = data["margin_position"].GetDouble();
			double margin_frozen = data["margin_frozen"].GetDouble();
			double margin_available = data["margin_available"].GetDouble();
			double profit_real = data["profit_real"].GetDouble();
			double profit_unreal = data["profit_unreal"].GetDouble();
			double risk_rate = data["risk_rate"].GetDouble();
			double liquidation_price = data["liquidation_price"].GetDouble();

			LOG_INFO << "HuobApi::GetFutureAccounts channel:" << channel
				<< ", margin_balance:" << margin_balance << ", margin_position:" << margin_position
				<< ", margin_frozen:" << margin_frozen << ", margin_available:" << margin_available
				<< ", profit_real:" << profit_real << ", profit_unreal:" << profit_unreal
				<< ", risk_rate:" << risk_rate
				<< ", liquidation_price:" << liquidation_price;
			cout << "HuobApi::GetFutureAccounts channel:" << channel
				<< ", margin_balance:" << margin_balance << ", margin_position:" << margin_position
				<< ", margin_frozen:" << margin_frozen << ", margin_available:" << margin_available
				<< ", profit_real:" << profit_real << ", profit_unreal:" << profit_unreal
				<<  ", risk_rate:" << risk_rate
				<< ", liquidation_price:" << liquidation_price << endl;
		}

	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFutureAccounts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFutureAccounts result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}
bool HuobApi::GetFuturePosition(string inst)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_POSITION_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (strcmp(inst.c_str(), "") != 0)
	{
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	}
	
	m_uri.Request();

	/******************************解析请求结构 begin*****************************/
	string &res = m_uri.result;
	Document doc;
	doc.Parse(res.c_str(), res.size());
	if (doc.HasParseError())
	{
		LOG_WARN << " HuobApi::GetFuturePosition parse error,result:" << res;
		return false;
	}
	spotrapidjson::Value& statusNode = doc["status"];
	string status = statusNode.GetString();
	if (status == "ok")
	{
		spotrapidjson::Value& dataNode = doc["data"];

		for (int i = 0; i < dataNode.Size(); i++)
		{
			spotrapidjson::Value& data = dataNode[i];


			string channel = data["contract_code"].GetString();
			double volume = data["volume"].GetDouble();
			double available = data["available"].GetDouble();
			double frozen = data["frozen"].GetDouble();
			double cost_open = data["cost_open"].GetDouble();
			double cost_hold = data["cost_hold"].GetDouble();
			double profit_unreal = data["profit_unreal"].GetDouble();
			double profit_rate = data["profit_rate"].GetDouble();
			double profit = data["profit"].GetDouble();
			double position_margin = data["position_margin"].GetDouble();
			int lever_rate = data["lever_rate"].GetInt();
			string direction = data["direction"].GetString();

			LOG_INFO << "HuobApi::GetFuturePosition channel:" << channel
				<<", volume:" << volume << ", available:" << available
				<< ", frozen:" << frozen << ", cost_open:" << cost_open
				<< ", cost_hold:" << cost_hold << ", profit_unreal:" << profit_unreal
				<< ", profit_rate:" << profit_rate << ", profit:" << profit
				<< ", position_margin:" << position_margin << ", lever_rate:" << lever_rate
				<< ", direction:" << direction;
			cout << "HuobApi::GetFuturePosition channel:" << channel
				<< ", volume:" << volume << ", available:" << available
				<< ", frozen:" << frozen << ", cost_open:" << cost_open
				<< ", cost_hold:" << cost_hold << ", profit_unreal:" << profit_unreal
				<< ", profit_rate:" << profit_rate << ", profit:" << profit
				<< ", position_margin:" << position_margin << ", lever_rate:" << lever_rate
				<< ", direction:" << direction<< endl;
		}
		
	}
	else
	{
		int errCode = doc["err_code"].GetInt();
		string errMsg = doc["err_msg"].GetString();
		LOG_WARN << " HuobApi::GetFuturePosition result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg;
		cout << " HuobApi::GetFuturePosition result,status:" << status << ",errCode:" << errCode << ",errMsg:" << errMsg << endl;
		return false;
	}
	/******************************解析请求结构 end*****************************/
	return true;
}

bool HuobApi::GetFutureOrderDetail(string inst, string orderID, HuobOrderDetail& order)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_ORDER_DETAIL_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (strcmp(inst.c_str(), "") != 0)
	{
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	}
	m_uri.AddParam(UrlEncode("order_id"), UrlEncode(orderID.c_str()));

	m_uri.Request();
	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetFutureOrderDetail return empty.";
		return false;
	}

	int ret = order.decode(res.c_str(), true);
	if (ret != 0)
	{
		LOG_WARN << "HuobApi::GetFutureOrderDetail decode failed";
		return false;
	}
	return true;
}

bool HuobApi::GetFutureOpenOrder(string inst, HuobOrder& order)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_OPEN_ORDER_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (strcmp(inst.c_str(), "") != 0)
	{
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	}

	m_uri.Request();
	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetFutureOpenOrder return empty.";
		return false;
	}

	int ret = order.decode(res.c_str(), true);
	if (ret != 0)
	{
		LOG_WARN << "HuobApi::GetFutureOpenOrder decode failed";
		return false;
	}
	return true;
}
bool HuobApi::GetFutureTradeOrder(string inst, HuobOrder& order)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_TRADE_ORDER_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (strcmp(inst.c_str(), "") != 0)
	{
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	}

	m_uri.Request();

	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetFutureTradeOrder return empty.";
		return false;
	}

	int ret = order.decode(res.c_str(), true);
	if (ret != 0)
	{
		LOG_WARN << "HuobApi::GetFutureTradeOrder decode failed";
		return false;
	}
	return true;
}

bool HuobApi::GetFutureOrder(string orderID, HuobOrder& order, bool isByOrderId)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_ORDER_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	if (isByOrderId)
	{
		m_uri.AddParam(UrlEncode("order_id"), UrlEncode(orderID));
	}
	else
	{
		m_uri.AddParam(UrlEncode("client_order_id"), UrlEncode(orderID));
	}
	m_uri.Request();
	LOG_DEBUG << "HuobApi::GetFutureOrder Request:"<< m_uri.GetParamJson();
	string &res = m_uri.result;

	LOG_DEBUG << "HuobApi::GetFutureOrder Request result:" << res;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetFutureOrder return empty.";
		return false;
	}

	int ret = order.decode(res.c_str(),true);
	if (ret != 0)
	{
		LOG_WARN << "HuobApi::GetFutureOrder GetFutureOrder failed";
		return false;
	}
	LOG_DEBUG << "HuobApi::GetFutureOrder decode result:" << order.to_string();
	return true;
}
bool HuobApi::GetFutureOrderList(const std::string& symbol, list<string> orderIDList, list<HuobOrder>& orderList, bool isByOrderId)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_DOMAIN;
	m_uri.api = HUOB_FUTURE_ORDER_API;
	SetPrivateParams(HTTP_POST, true, JSON_TYPE);

	string orderIDListParms = "";
	for (list<string>::iterator iter = orderIDList.begin(); iter != orderIDList.end(); iter++)
	{
		string orderID = *iter;
		if (iter != orderIDList.begin())
		{
			orderIDListParms += ",";
		}
		orderIDListParms += orderID;
	}

	if (isByOrderId)
	{
		m_uri.AddParam(UrlEncode("order_id"), orderIDListParms);
	}
	else
	{
		m_uri.AddParam(UrlEncode("client_order_id"), orderIDListParms);
	}
	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(symbol));

	m_uri.Request();
	LOG_INFO << "HuobApi::GetFutureOrderList Request:" << m_uri.GetParamJson();
	string &res = m_uri.result;

	LOG_INFO << "HuobApi::GetFutureOrderList Request result:" << res;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetFutureOrderList return empty.";
		return false;
	}
	Document doc;
	doc.Parse(res.c_str(), strlen(res.c_str()));
	if (doc.HasParseError())
	{
		LOG_WARN << "GetFutureOrderList::Parse error. result:" << res;
		return false;
	}
	if (doc.HasMember("status"))
	{
		if (strcmp(doc["status"].GetString(), "ok") != 0)
		{
			LOG_WARN << "HuobFutureOrder::Request return error. result:" << res;
			return false;
		}

		Value &dataNode = doc["data"];
		for (int i = 0; i < dataNode.Size(); i++)
		{
			HuobOrder order;
			memset(&order, 0 ,sizeof(HuobOrder));
			spotrapidjson::Value& data = dataNode[i];
			Value &vsymbol = data["contract_code"];
			if (vsymbol.IsString())
				memcpy(order.instrument_id, vsymbol.GetString(), sizeof(order.instrument_id) - 1);

			Value &vamount = data["volume"];
			if (vamount.IsString())
				order.amount = stod(vamount.GetString());
			else
				order.amount = vamount.GetDouble();

			Value &vprice = data["trade_avg_price"];
			if (vprice.IsString())
				order.price = stod(vprice.GetString());
			else
				order.price = vprice.GetDouble();

			Value &vtradeAmount = data["trade_volume"];
			if (vtradeAmount.IsString())
				order.tradeAmount = stod(vtradeAmount.GetString());
			else
				order.tradeAmount = vtradeAmount.GetDouble();

			Value &vtradeMoney = data["trade_turnover"];
			if (vtradeMoney.IsString())
				order.tradeMoney = stod(vtradeMoney.GetString());
			else
				order.tradeMoney = vtradeMoney.GetDouble();

			Value &vfees = data["fee"];
			if (vfees.IsString())
				order.fees = stod(vfees.GetString());
			else
				order.fees = vfees.GetDouble();

			Value &vfinishedTime = data["created_at"];
			double fTime = vfinishedTime.GetDouble();
			sprintf(order.finishedTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
			sprintf(order.canceledTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);
			sprintf(order.createTime, "%2d:%2d:%2d", fTime, fTime, fTime, fTime, fTime, fTime);

			Value &vorder_id = data["order_id"];
			if (vorder_id.IsString())
				order.orderID = stoll(vorder_id.GetString());
			else
				order.orderID = vorder_id.GetInt64();

			Value &v_client_order_id = data["client_order_id"];
			if (v_client_order_id.IsString())
				order.clientOrderID = stoll(v_client_order_id.GetString());
			else
				order.clientOrderID = v_client_order_id.GetInt64();

			Value &vsource = data["order_source"];
			if (vsource.IsString())
				memcpy(order.source, vsource.GetString(), sizeof(vsource) - 1);

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
					order.type = huob_buy_market;
				}
				else if (strcmp(ftype.c_str(), "limit") == 0)
				{
					order.type = huob_buy_limit;
				}

				if (strcmp(foffset.c_str(), "open") == 0)
				{
					order.offset = 1;
				}
				else if (strcmp(foffset.c_str(), "close") == 0)
				{
					order.offset = 3;
				}
			}
			else if (strcmp(direction.c_str(), "sell") == 0)
			{
				if (strcmp(ftype.c_str(), "limit") == 0)
				{
					order.type = huob_sell_limit;
				}
				else if (strcmp(ftype.c_str(), "market") == 0)
				{
					order.type = huob_sell_market;
				}

				if (strcmp(foffset.c_str(), "open") == 0)
				{
					order.offset = 2;
				}
				else if (strcmp(foffset.c_str(), "close") == 0)
				{
					order.offset = 4;
				}
			}

			Value &vstate = data["status"];
			if (vstate.IsInt())
			{
				if (vstate.GetInt() == 1)
				{
					order.orderStatus = huob_pre_submitted;
				}
				else if (vstate.GetInt() == 2)
				{
					order.orderStatus = huob_submitting;
				}
				else if (vstate.GetInt() == 3)
				{
					order.orderStatus = huob_submitted;
				}
				else if (vstate.GetInt() == 4)
				{
					order.orderStatus = huob_partial_filled;
				}
				else if (vstate.GetInt() == 5)
				{
					order.orderStatus = huob_partial_canceled;
				}
				else if (vstate.GetInt() == 6)
				{
					order.orderStatus = huob_filled;
				}
				else if (vstate.GetInt() == 7)
				{
					order.orderStatus = huob_canceled;
				}
				else if (vstate.GetInt() == 11)
				{
					order.orderStatus = huob_canceling;
				}
			}

			orderList.push_back(order);
		}
	}
	return true;
}

bool HuobApi::GetUserFutureTransactions(string inst, list<HuobFutureTransaction> &transaction_list)
{
	m_uri.clear();
	m_uri.domain = HUOB_FUTURE_MARKET_DOMAIN;
	m_uri.api = HUOB_FUTURE_TRADE_API;
	SetPrivateParams(HTTP_GET, true);

	m_uri.AddParam(UrlEncode("symbol"), UrlEncode(inst.c_str()));
	//m_uri.AddParam(UrlEncode("size"), UrlEncode("10"));

	m_uri.Request();

	string &res = m_uri.result;
	if (res.empty())
	{
		LOG_WARN << "HuobApi::GetUserTransactions return empty.";
		return false;
	}
	HuobFutureTransaction obj;
	int ret = obj.decode(res.c_str());
	memcpy(obj.symbol, HuobApi::GetInstrumentID(obj.symbol, true).c_str(), sizeof(obj.symbol));

	if (ret != 0 )
	{
		LOG_INFO << "GetUserFutureTransactions decode error:" << ret;
	}

	transaction_list.push_back(obj);

	LOG_INFO<< "HuobFutureTransaction:" << obj.to_string();
	cout << obj.to_string() << endl;
	return true;
}

uint64_t HuobApi::ReqFutureOrderInsert(HuobOrder &order)
{
	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_FUTURE_DOMAIN;
		m_uri.api = HUOB_FUTURE_ORDER_INSERT_API;
		SetPrivateParams(HTTP_POST, true, JSON_TYPE);


		string clientID = to_string(GetCurrentNonce());
		m_uri.AddParam(UrlEncode("client_order_id"), UrlEncode(clientID.c_str()));

		string cp = GetFutureConstractCode(order.instrument_id);
		m_uri.AddParam(UrlEncode("contract_code"), UrlEncode(cp));

		if (order.type == huob_buy_limit)
		{
			m_uri.AddParam(UrlEncode("direction"), UrlEncode("buy"));
		}
		else if (order.type == huob_sell_limit)
		{
			m_uri.AddParam(UrlEncode("direction"), UrlEncode("sell"));
		}

		m_uri.AddParam(UrlEncode("lever_rate"), UrlEncode("20"));

		if (order.offset == 1 || order.offset == 2)
		{
			m_uri.AddParam(UrlEncode("offset"), UrlEncode("open"));
		}
		else if (order.offset == 3 || order.offset == 4)
		{
			m_uri.AddParam(UrlEncode("offset"), UrlEncode("close"));
		}
		//m_uri.AddParam(UrlEncode("offset"), UrlEncode("open"));
		if (order.type == huob_buy_limit)
		{
			m_uri.AddParam(UrlEncode("order_price_type"), UrlEncode("limit"));
		}
		else if (order.type == huob_sell_limit)
		{
			m_uri.AddParam(UrlEncode("order_price_type"), UrlEncode("limit"));
		}

		//m_uri.AddParam(UrlEncode("order_price_type"), UrlEncode("opponent"));
		char buf[64];
		SNPRINTF(buf, sizeof(buf), "%.2f", order.price);
		m_uri.AddParam(UrlEncode("price"), UrlEncode(buf));

		int amount = order.amount;
		SNPRINTF(buf, sizeof(buf), "%d", amount);
		cout << "buf:" << buf<< endl;
		m_uri.AddParam(UrlEncode("volume"), UrlEncode(buf));

		m_uri.Request(); 
		
		LOG_DEBUG << "ReqFutureOrderInsert Request: " << m_uri.GetParamJson();

		string &res = m_uri.result;
		LOG_DEBUG << "ReqFutureOrderInsert Request result: " << res;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::ReqFutureOrderInsert return empty.";
			return 0;
		}
		HuobRspReqOrder rspOrder;
		int ret = rspOrder.decode(res.c_str(),true);
		LOG_INFO << "HuobApi::ReqFutureOrderInsert decode result:" << rspOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::ReqFutureOrderInsert decode failed";
			return 0;
		}
		return rspOrder.id;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::ReqFutureOrderInsert exception occur:" << ex.what();
		return 0;
	}
}
bool HuobApi::CancelFutureOrder(const std::string &order_id, const std::string& symbol)
{
	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_FUTURE_DOMAIN;
		m_uri.api = HUOB_FUTURE_ORDER_CANCEL_API;
		SetPrivateParams(HTTP_POST, true, JSON_TYPE);

		m_uri.AddParam(UrlEncode("order_id"), UrlEncode(order_id));
		m_uri.AddParam(UrlEncode("symbol"), UrlEncode(symbol));
		m_uri.Request();

		LOG_DEBUG << "CancelFutureOrder Request: " << m_uri.GetParamJson();

		string &res = m_uri.result;
		LOG_DEBUG << "CancelFutureOrder Request result: "<<res;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::CancelFutureOrder return empty. orderid:" << order_id;
			return false;
		}
		HuobRspCancelFutureOrder rspCancleOrder;
		int ret = rspCancleOrder.decode(res.c_str());
		LOG_INFO << "HuobApi::CancelFutureOrder decode result:" << rspCancleOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::CancelFutureOrder cancel failed";
			return false;
		}
		return true;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::CancelOrder exception occur:" << ex.what();
		return false;
	}
}

std::map<string, string> HuobApi::GetFutureLongChannelMap()
{
	return futureChannelToLongInstrumentMap_;
}

std::map<string, string> HuobApi::GetFutureShortChannelMap()
{
	return futureChannelToShortInstrumentMap_;
}

string HuobApi::GetFutureCurrencyPair(Instrument *inst)
{
	string cp = inst->getInstrumentID().substr(0, 3);
	//BTC0910_l_hb --> BTC_CW
	if (inst->isCryptoFutureThisWeek())
	{		
		cp = cp + "_CW";
	}
	else if (inst->isCryptoFutureNextWeek())
	{
		cp = cp + "_NW";
	}
	else if (inst->isCryptoFutureQuarter())
	{
		cp = cp + "_CQ";
	}
	return cp;
}

string HuobApi::GetFutureConstractCode(string inst)
{
	//BTC0910_l_hb --> BTC0910
	//string cp = inst.substr(0, inst.find_first_of('_'));

	//20181214 --> 1214
	ostringstream os;
	string tradingDay = TradingDay::getToday();
	string year = tradingDay.substr(2, 2); //18
	tradingDay = tradingDay.substr(4, 4); //1214
										  //BTC0910_l_hb --> 0910
	string instrumentDay = inst.substr(3, 4); //0910
	if (atoi(instrumentDay.c_str()) < atoi(tradingDay.c_str())) { //下一年的合约
		os << atoi(year.c_str()) + 1;
		year = os.str();
	}

	string cp = inst.substr(0, 3);
	cp += year;
	cp += inst.substr(3, 4);
	return cp;
}

uint64_t HuobApi::ReqBatchFutureOrderInsert(list<HuobOrder> &order_list)
{
	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_FUTURE_DOMAIN;
		m_uri.api = HUOB_BATCH_FUTURE_ORDER_INSERT_API;
		SetPrivateParams(HTTP_POST, true, JSON_TYPE);

		string paramList = "[";
		for(auto iter = order_list.begin(); iter != order_list.end();iter++)
		{
			auto order = *iter;

			string clientID = to_string(GetCurrentNonce());
			m_uri.AddParam(UrlEncode("client_order_id"), UrlEncode(clientID.c_str()));

			string cp = GetFutureConstractCode(order.instrument_id);
			m_uri.AddParam(UrlEncode("contract_code"), UrlEncode(cp));

			if (order.type == huob_buy_limit)
			{
				m_uri.AddParam(UrlEncode("direction"), UrlEncode("buy"));
			}
			else if (order.type == huob_sell_limit)
			{
				m_uri.AddParam(UrlEncode("direction"), UrlEncode("sell"));
			}

			m_uri.AddParam(UrlEncode("lever_rate"), UrlEncode("20"));

			if (order.offset == 1 || order.offset == 2)
			{
				m_uri.AddParam(UrlEncode("offset"), UrlEncode("open"));
			}
			else if (order.offset == 3 || order.offset == 4)
			{
				m_uri.AddParam(UrlEncode("offset"), UrlEncode("close"));
			}

			if (order.type == huob_buy_limit)
			{
				m_uri.AddParam(UrlEncode("order_price_type"), UrlEncode("limit"));
			}
			else if (order.type == huob_sell_limit)
			{
				m_uri.AddParam(UrlEncode("order_price_type"), UrlEncode("limit"));
			}


			char buf[64];
			SNPRINTF(buf, sizeof(buf), "%.2f", order.price);
			m_uri.AddParam(UrlEncode("price"), UrlEncode(buf));

			SNPRINTF(buf, sizeof(buf), "%d", order.amount);
			m_uri.AddParam(UrlEncode("volume"), UrlEncode("2"));

			if (iter != order_list.begin())
			{
				paramList += ",";
			}
			paramList += m_uri.GetParamJson();
			m_uri.ClearParam();
		}
		
		paramList += "]";
		cout << "paramList:" << paramList<<endl;
		m_uri.AddParam(UrlEncode("orders_data"), paramList);

		m_uri.Request();
		string &res = m_uri.result;
		cout << "res:" << res << endl;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::ReqBatchFutureOrderInsert return empty.";
			return 0;
		}
		HuobRspReqOrder rspOrder;
		int ret = rspOrder.decode(res.c_str(), true);
		LOG_INFO << "HuobApi::ReqBatchFutureOrderInsert result:" << rspOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::ReqBatchFutureOrderInsert decode failed";
			return 0;
		}
		return rspOrder.id;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::ReqBatchFutureOrderInsert exception occur:" << ex.what();
		return 0;
	}
	return 0;
}
bool HuobApi::CancelBatchFutureOrder(list<string> &order_id_list)
{
	try
	{
		m_uri.clear();
		m_uri.domain = HUOB_FUTURE_DOMAIN;
		m_uri.api = HUOB_FUTURE_ORDER_CANCEL_API;
		SetPrivateParams(HTTP_POST, true, JSON_TYPE);

		string params = "";
		for (auto iter = order_id_list.begin(); iter != order_id_list.end(); iter++)
		{
			auto order_id = *iter;
			if (iter != order_id_list.begin())
			{
				params += ",";
			}
			params += order_id;
		}


		m_uri.AddParam(UrlEncode("order_id"), params);
		m_uri.Request();

		string &res = m_uri.result;
		cout << "res:" << res << endl;
		if (res.empty())
		{
			LOG_WARN << "HuobApi::CancelBatchFutureOrder return empty";
			return false;
		}
		HuobRspCancelFutureOrder rspCancleOrder;
		int ret = rspCancleOrder.decode(res.c_str());
		LOG_INFO << "HuobApi::CancelBatchFutureOrder result:" << rspCancleOrder.to_string();
		if (ret != 0)
		{
			LOG_WARN << "HuobApi::CancelBatchFutureOrder decode failed";
			return false;
		}
		return true;
	}
	catch (std::exception ex)
	{
		LOG_WARN << "HuobApi::CancelBatchFutureOrder exception occur:" << ex.what();
		return false;
	}

}
