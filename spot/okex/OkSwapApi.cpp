#include "OkSwapApi.h"
#include "spot/utility/algo_hmac.h"
#include "base64/base64.hpp"
#include "spot/base/DataStruct.h"
#include "spot/base/SpotInitConfigTable.h"

using namespace std;
using namespace spot::base;
using namespace spot::utility;

#define OK_SWAP_CANCEL_BATCH_INFO "/api/v5/trade/cancel-batch-orders"

vector<string> OkSwapApi::okTickerToInstrumentMap_;

OkSwapApi::OkSwapApi(string api_key, string secret_key,string passphrase, AdapterCrypto* adapt)
{
	m_api_key = api_key;
	m_secret_key = secret_key;
	m_passphrase = passphrase;
	adapter_ = adapt;
	cancelAll = false;
	urlprotocol.InitApi(HTTP_SERVER_TYPE_COM);
}

OkSwapApi::~OkSwapApi() {}

string OkSwapApi::CreateSignature(string& timestamp, string& method, string& requestPath, string& body, const char* encode_mode)
{

	string signStr = timestamp + method + requestPath + body;

	unsigned char* output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	unsigned int output_length = 0;
	HmacEncode(encode_mode,  m_secret_key.c_str(), signStr.c_str(), output, &output_length);
	string sig_base64 = websocketpp::base64_encode(output, output_length);

	if (output)
	{
		free(output);
	}
	return sig_base64.c_str();
}

int OkSwapApi::SetPrivateParams(std::shared_ptr<Uri> uri)
{
	string timestamp = getUTCMSTime();
	string method = "";
	string requestPath = uri->api;
	string body = "";

	if (uri->method == METHOD_GET)
	{
		uri->m_body_format = FORM_TYPE;
		method = "GET";
		body = uri->GetParamSet();
		if (!body.empty())
		{
			body = "?" + body;
		}
	}
	else if (uri->method == METHOD_POST)
	{
		uri->m_body_format = JSON_TYPE;  // 实盘只用json
		method = "POST";
		body = uri->GetParamJson();
		if(strcmp(body.c_str(), "{}") == 0)
		{
			body = "";
		}

		LOG_DEBUG << "SetPrivateParams post body json: " << body;
	}

	string sign = CreateSignature(timestamp, method, requestPath, body, "sha256");

	uri->AddHeader(("OK-ACCESS-KEY:" + m_api_key).c_str());
	uri->AddHeader(("OK-ACCESS-SIGN:" + sign).c_str());
	uri->AddHeader(("OK-ACCESS-TIMESTAMP:" + timestamp).c_str());
	uri->AddHeader(("OK-ACCESS-PASSPHRASE:" + m_passphrase).c_str());
	uri->AddHeader("Accept:application/json");
	uri->AddHeader("Cookie:locale=en_US");
	uri->AddHeader("Content-Type: application/json; charset=UTF-8");
	uri->AddHeader(("Host:" + string(uri->domain)).c_str());
	//todo okex - testnet

#ifdef __TESTMODE__
	uri->AddHeader(("x-simulated-trading:1"));  // test need, pro needn't
#endif
	return 0;
}

void OkSwapApi::ConvertQuantityAndPrice(const Order* order, std::shared_ptr<Uri> m_uri) {
    if(strcmp(order->InstrumentID, "BTC-USDT-SWAP") == 0) {
        char buf[64];
        SNPRINTF(buf, sizeof(buf), "%.1f", order->LimitPrice);
        m_uri->AddParam(UrlEncode("price"), UrlEncode(buf));
    } else if(strcmp(order->InstrumentID, "ETH-USDT-SWAP") == 0) {
        char buf[64];
        SNPRINTF(buf, sizeof(buf), "%.2f", order->LimitPrice);
        m_uri->AddParam(UrlEncode("price"), UrlEncode(buf));
    } else {
        LOG_FATAL << "OkSwapApi::ConvertQuantityAndPrice error " << order->InstrumentID;
    }
}

void OkSwapApi::uriReqCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynReqCallback(message, clientId);
}

void OkSwapApi::uriCanCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynCancelCallback(message, clientId);
}

void OkSwapApi::OrderInsert(const Order* innerOrder, const string& uuid)
{
	std::shared_ptr<Uri> uri = make_shared<Uri>();
	urlprotocol.GetUrl(*(uri.get()), HTTP_API_TYPE_OKX_SWAP_FUTURE_ORDER_INSERT);
	uri->exchangeCode = Exchange_OKEX;

	string side = "buy";
	if (innerOrder->Direction == INNER_DIRECTION_Sell)
	{
        side = "sell";
	}

	string orderType = "limit";
	if (innerOrder->OrderType == ORDERTYPE_LIMIT_CROSS) {
        orderType = "limit";
	} else if (innerOrder->OrderType == ORDERTYPE_POST_ONLY) {
        orderType = "post_only";
	} else if (innerOrder->OrderType == ORDERTYPE_MARKET) {
		orderType = "market";
	} else {
	    return;
	}
	uri->setUriClientOrdId(innerOrder->OrderRef);
	//string instrumentCode = innerOrder->InstrumentID;
     // order postion
	int orderVolme = innerOrder->VolumeTotalOriginal;
    uri->AddParam("sz", to_string(orderVolme));
    // custom order id
    uri->AddParam("clOrdId", uuid);
	// buy or sell
	uri->AddParam("side", side);
	// price
	if (innerOrder->OrderType != ORDERTYPE_MARKET) {
		uri->AddParam("px", to_string(innerOrder->LimitPrice));
    	ConvertQuantityAndPrice(innerOrder, uri);
	}
	// instruemntId
	uri->AddParam("instId", innerOrder->InstrumentID);
	uri->AddParam("ordType", orderType);
	uri->AddParam("tdMode", "isolated");

	//set sign after api and params
	SetPrivateParams(uri);
	uri->setResponseCallback(std::bind(&OkSwapApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
	CurlMultiCurl::instance().addUri(uri);
	//uri->Request();
    //LOG_INFO << "OkSwapApi OrderInsert: res " << uri->result;
	return;
	//return uri->result;
}
string  OkSwapApi::OrderCancel(const string& instrument_id, const string& order_id)
{
	std::shared_ptr<Uri> uri = make_shared<Uri>();
	urlprotocol.GetUrl(*(uri.get()), HTTP_API_TYPE_OKX_SWAP_FUTURE_ORDER_CANCEL);
	uri->exchangeCode = Exchange_OKEX;
	//uri->api += "/" + instrument_id + "/" + order_id;
	uri->setUriClientOrdId(stoi(order_id));

	uri->AddParam("instId", instrument_id);
	uri->AddParam("clOrdId", order_id);

	//set sign after api and params
	SetPrivateParams(uri);
	uri->setResponseCallback(std::bind(&OkSwapApi::uriCanCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
	CurlMultiCurl::instance().addUri(uri);
    //LOG_INFO << "OkSwapApi OrderCancel: res " << uri->result;
	return uri->result;
}

int OkSwapApi::CancelAllOrders() {
    if (cancelAll) return 1;
    // try {
        for (auto it : okTickerToInstrumentMap_) {
            string symbol = it;
			std::shared_ptr<Uri> uri = make_shared<Uri>();
            uri->clear();
            uri->domain = OKX_DOMAIN;
            uri->api = OK_SWAP_CANCEL_BATCH_INFO;

            uri->AddParam(("instId"), symbol);
			uri->method = METHOD_POST;
            SetPrivateParams(uri);
            int rstCode = uri->Request();
            string &res = uri->result;
            if (rstCode != CURLE_OK) {
                string &res = uri->result;
                LOG_FATAL << "OkSwapApi CancelAllOrder fail result:" << rstCode << ", res: " << res;
                return -1;
            }
            if (res.empty()) {
                LOG_FATAL << "OkSwapApi::CanCancelAllOrdercelOrder return empty. symbol:" << symbol
					<< "retcode: " << uri->retcode << ", method: " << uri->method << ", url: " << uri->GetUrl() 
                	<< ", param: " << uri->GetParamSet() << ", res: " << res;
                continue;
            }
            LOG_INFO << "OkSwapApi CancelAllOrders: " << uri->retcode << ", method: " << uri->method << ", url: " << uri->GetUrl() 
                << ", param: " << uri->GetParamSet() << ", res: " << res;

        }
        cancelAll = true;
        return 0;
    // }
    // catch (std::exception ex) {
    //     LOG_WARN << "OkSwapApi::CancelOrder exception occur:" << ex.what();
    //     return -1;
    // }
}

int OkSwapApi::QryPosiBySymbol(const Order &order)
{
	for (auto instID : okTickerToInstrumentMap_) {
        OKexPosQryInfo qryInfo;
		std::shared_ptr<Uri> uri = make_shared<Uri>();;
		urlprotocol.GetUrl(*(uri.get()), HTTP_API_TYPE_OKX_POSITION_INFO);
		uri->api += "?instId=" + instID;
		SetPrivateParams(uri);

		uri->Request();
		string &res = uri->result;

		int ret = qryInfo.decode(res.c_str());
		if (ret == -2)
		{
			memcpy(qryInfo.posiNode_.symbol, order.InstrumentID, InstrumentIDLen);
			qryInfo.posiNode_.setSide("NONE");

			gIsPosiQryInfoSuccess[instID] = true;
			gQryPosiInfo[order.InstrumentID] = qryInfo.posiNode_;
            LOG_WARN << "OkSwapApi::QryPosiInfo empty return false instID [" << instID << "]";
			continue;
		}

		if (ret != 0) {
			LOG_FATAL << "OkSwapApi::QryPosiBySymbol decode failed res: " << res;
			return -1;
		}

		LOG_INFO << "OkSwapApi QryPosiBySymbol rst: " << res;

		if (IS_DOUBLE_GREATER_EQUAL(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "net") == 0)) {
			memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
			string str("BUY");
			memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
		} else if (IS_DOUBLE_LESS(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "net") == 0)) {
			memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
			string str("SELL");
			memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
		} else {
			LOG_FATAL << "okswap QryPosiBySymbol ERROR: " << qryInfo.posiNode_.side<< ", size: " << qryInfo.posiNode_.size;
		}

		qryInfo.posiNode_.size = abs(qryInfo.posiNode_.size);
		memcpy(qryInfo.posiNode_.symbol, order.InstrumentID, InstrumentIDLen);
		
		gIsPosiQryInfoSuccess[order.InstrumentID] = true;
		gQryPosiInfo[order.InstrumentID] = qryInfo.posiNode_;
		return 0;
	}

	return 0;
}

void OkSwapApi::AddInstrument(const char *instrument)
{
	okTickerToInstrumentMap_.push_back(std::string(instrument));

	gIsPosiQryInfoSuccess[instrument] = false;
	gIsMarkPxQryInfoSuccess[instrument] = false;

	LOG_INFO << "OkSwapApi::AddInstrument instrument: " << instrument;

}