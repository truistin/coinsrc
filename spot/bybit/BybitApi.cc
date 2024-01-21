#include "string.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include "spot/utility/Logging.h"
#include "spot/restful/UriFactory.h"
#include "spot/bybit/BybitApi.h"
#include <openssl/hmac.h>
#include "base64/base64.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/connect.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/read.hpp"
#include "boost/asio/read_until.hpp"
#include "boost/system/system_error.hpp"

#include <spot/utility/gzipwrapper.h>
#include <spot/utility/Utility.h>
#include <spot/base/MeasureLatencyCache.h>

#include <spot/base/DataStruct.h>
#include <spot/restful/CurlMultiCurl.h>

using namespace boost::asio;
using namespace spot;
using namespace spot::utility;

#define Bybit_UPDATE_ACCOUNT "/v5/account/upgrade-to-uta"
#define Bybit_SERVER_TIME_API "/api/v1/time"
#define Bybit_DepthMarket_API "/api/v1/depth"

#define Bybit_FUTURE_ORDER_INSERT_API "/v5/order/create"
#define Bybit_FUTURE_CANCEL_ORDER_API "/v5/order/cancel"
#define Bybit_POSITION_RISK_API "/v5/position/list"
#define Bybit_MARKPRICE_API "/v5/market/tickers"
#define Bybit_CANCEL_ALL_ORDER_API "/v5/order/cancel-all"
#define Bybit_QUERY_ORDER_API "/v5/order/realtime"

using namespace std;
using namespace std::chrono;
using namespace spot::base;

std::map<string, string> BybitApi::depthToInstrumentMap_;
std::map<string, string> BybitApi::tradeToInstrumentMap_;
std::map<string, string> BybitApi::tickerToInstrumentMap_;
std::map<string, string> BybitApi::originSymbolToSpotSymbol_;
std::vector<string> BybitApi::categoryVec_;


BybitApi::BybitApi(string api_key, string secret_key, string passphrase, AdapterCrypto* adapt) {
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    m_uri.domain = Bybit_DOMAIN;
    m_uri.method = METHOD_GET;
    m_uri.isinit = true;
    m_nonce = duration_cast<chrono::microseconds>(system_clock::now().time_since_epoch()).count();
    m_api_key = api_key;
    m_secret_key = secret_key;
    m_passphrase = passphrase;
    adapter_ = adapt;
    cancelAll = false;
    
    LOG_INFO << "BybitApi apikey:" << m_api_key << ", secret_key: " << m_secret_key << " passwd:" << m_passphrase << " skey-len:" << m_secret_key.length();

}

BybitApi::~BybitApi() {
}

void BybitApi::Init() {
    GetServerTime();
}

//Set api for uri before SetPrivateParams, since SetPrivateParams has to create sign using api
void BybitApi::SetPrivateParams(int http_mode, Uri& m_uri, int encode_mode) {
    string message;
    if (http_mode == HTTP_GET) {
        message = m_uri.GetParamSet();
    } else if (http_mode == HTTP_POST) {
        message = m_uri.GetParamJson();
    } else if (http_mode == HTTP_DELETE) {
        message = m_uri.GetParamJson();
    }

    uint64_t EpochTime = CURR_MSTIME_POINT;
    string param_str = std::to_string(EpochTime) + m_api_key + "3000" + message;
    unsigned char *output = (unsigned char *) malloc(EVP_MAX_MD_SIZE);
    unsigned int output_length = 0;
    HmacEncode("sha256", m_secret_key.c_str(), param_str.c_str(), output, &output_length);

    char buf[64];
    std::string dst = "";
    for (int i = 0; i < 32; i++) {
        sprintf(buf, "%02x", output[i]);
        buf[0] = ::toupper(buf[0]);
        buf[1] = ::toupper(buf[1]);
        dst = dst + buf;
    }

    //�Լ��ܽ������url����
    string sig_url = UrlEncode(dst.c_str());
    if (output) {
        free(output);
    }

    string headersign = "X-BAPI-SIGN: " + sig_url;
    m_uri.AddHeader(headersign.c_str());
    // m_uri.AddParam(UrlEncode("X-BAPI-SIGN"), sig_url);

    // if (encode_mode == ENCODING_GZIP) {
    //     m_uri.encoding = GZIP_ENCODING;
    // }

    // 服务端目前支持的是urlencoded格式，所以头部content-type进行了格式的限定，目前m_body_format不支持json
    if (http_mode == HTTP_GET) {
        m_uri.method = METHOD_GET;
        //m_uri.m_body_format = FORM_TYPE;
    } else if (http_mode == HTTP_POST) {
        // the Bybit Unified Trading Account only support json
        m_uri.AddHeader("Content-Type: application/json"); // CreateSignature用的是param set,这里不能用json
        m_uri.m_body_format = JSON_TYPE;    // 实盘用json

        // m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        // m_uri.m_body_format = FORM_TYPE;
        m_uri.method = METHOD_POST;
    //    m_uri.m_body_format = JSON_TYPE;
    } else if (http_mode == HTTP_DELETE) {
        m_uri.AddHeader("Content-Type: application/json"); // CreateSignature用的是param set,这里不能用json
        m_uri.m_body_format = JSON_TYPE;    // 实盘用json

        //  m_uri.m_body_format = FORM_TYPE;
        // m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        m_uri.method = METHOD_DELETE;
       
        // m_uri.m_body_format = JSON_TYPE;
    }

    string header = "X-BAPI-API-KEY: " + m_api_key;
    m_uri.AddHeader(header.c_str());

    string headertime = "X-BAPI-TIMESTAMP: " + std::to_string(EpochTime);
    m_uri.AddHeader(headertime.c_str());

    string headerwindow = "X-BAPI-RECV-WINDOW: 3000";
    m_uri.AddHeader(headerwindow.c_str());
    
}

string BybitApi::CreateSignature(int http_mode, string secret_key, Uri& m_uri, const char *encode_mode)
{
    string message;
    if (http_mode == HTTP_GET) {
        message = m_uri.GetParamSet();
    } else if (http_mode == HTTP_POST) {
        message = m_uri.GetParamJson();
    } else if (http_mode == HTTP_DELETE) {
        message = m_uri.GetParamJson();
    }
    /********************************ǩ�� begin**********************************/
    ////����HMAC_SHA256����
    unsigned char *output = (unsigned char *) malloc(EVP_MAX_MD_SIZE);
    unsigned int output_length = 0;
    HmacEncode("sha256", secret_key.c_str(), message.c_str(), output, &output_length);

    char buf[64];
    std::string dst = "";
    for (int i = 0; i < 32; i++) {
        sprintf(buf, "%02x", output[i]);
        buf[0] = ::toupper(buf[0]);
        buf[1] = ::toupper(buf[1]);
        dst = dst + buf;
    }

    //�Լ��ܽ������url����
    string sig_url = UrlEncode(dst.c_str());
    if (output) {
        free(output);
    }
    /********************************ǩ�� end**********************************/
    return sig_url;
}

bool BybitApi::GetServerTime(int type) {
    m_uri.clear();

    m_uri.domain = Bybit_DOMAIN;
    if (type == 0) {
        m_uri.api = Bybit_SERVER_TIME_API;
    } else if (type == 1) {
        // m_uri.api = Bybit_FUTURE_SERVER_TIME_API;
    }
    m_uri.method = METHOD_GET;

    uint64_t currentTime = CURR_MSTIME_POINT;
    uint64_t lagencyTime = 0;
    uint64_t serverTime = 0;
    m_uri.Request();

    string &res = m_uri.result;
    Document doc;
    doc.Parse(res.c_str(), res.size());
    if (doc.HasParseError()) {
        LOG_WARN << " BybitApi::GetServerTime parse error,result:" << res;
        return false;
    }

    if (doc.HasMember("serverTime")) {
        spotrapidjson::Value &serverTimeNode = doc["serverTime"];
        if (serverTimeNode.IsUint64()) {
            serverTime = serverTimeNode.GetUint64();
        }
        if (serverTimeNode.IsString()) {
            serverTime = atoll(serverTimeNode.GetString());
        }
    }

    lagencyTime = (CURR_MSTIME_POINT - currentTime) / 2;
    currentTime = CURR_MSTIME_POINT;

    m_timeLagency = (serverTime - currentTime) + lagencyTime;
    m_timeLagency = (serverTime - currentTime);
    return true;

}

void BybitApi::uriQryMarkPCallbackOnHttp(char* result, uint64_t clientId)
{
    if (result == nullptr)
	{
		LOG_WARN << "BybitApi::uriQryMarkPCallbackOnHttp result false";
		return;
	}

    BybitMrkStruct qryInfo;
    int ret = qryInfo.decode(result);

    if (ret != 0) {
        for (auto& qry : qryInfo.vecOrd_) {
            auto it = originSymbolToSpotSymbol_.find(qry.symbol);
            if (it == originSymbolToSpotSymbol_.end()) {
                LOG_ERROR << "BybitApi::uriQryMarkPCallbackOnHttp decode symbol failed res: " << result;
                continue;
            }
            LOG_WARN << "BybitApi::uriQryMarkPCallbackOnHttp decode failed";
            gIsMarkPxQryInfoSuccess[it->second] = false;
            int failedCount = ++queryFailedCountMap[it->second];
            if (failedCount >= 3) {
                LOG_ERROR << "BybitApi::uriQryMarkPCallbackOnHttp instId [" << it->second << "]" << "failedCount:" << failedCount
                            << " res:" << result << "  ret:" << ret;
            }
        }
    }

    for (auto& qry : qryInfo.vecOrd_) {
        auto it = originSymbolToSpotSymbol_.find(qry.symbol);
        if (it == originSymbolToSpotSymbol_.end()) {
            LOG_ERROR << "BybitApi::uriQryMarkPCallbackOnHttp decode symbol failed res: " << result;
            continue;
        }
        memcpy(qry.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
        gQryMarkPxInfo[it->second] = qry;
        gIsMarkPxQryInfoSuccess[it->second] = true;
        LOG_DEBUG << "BybitApi uriQryMarkPCallbackOnHttp: " << result << "gQryMarkPxInfo size: " << gQryMarkPxInfo.size()
            << ", gIsMarkPxQryInfoSuccess: " << gIsMarkPxQryInfoSuccess[it->second];
        queryFailedCountMap[it->second] = 0;
        
    }
    return;
}

void BybitApi::uriQryPosiCallbackOnHttp(char* result, uint64_t clientId)
{
	if (result == nullptr)
	{
		LOG_WARN << "BybitApi::uriQryPosiCallbackOnHttp result false";
		return;
	}

    BybitQryStruct qryInfo;
    int ret = qryInfo.decode(result);

    if (ret == 2) {
        for (auto& qry : qryInfo.vecOrd_) {
            auto it = originSymbolToSpotSymbol_.find(qry.symbol);
            if (it == originSymbolToSpotSymbol_.end()) {
                LOG_ERROR << "NO qry.symbol uriQryPosiCallbackOnHttp: " << result;
                continue;
            }
            qry.updatedTime = CURR_MSTIME_POINT;
            memcpy(qry.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
            gIsPosiQryInfoSuccess[qry.symbol] = false;
            gQryPosiInfo[qry.symbol] = qry;
            LOG_DEBUG << "NO BybitApi uriQryPosiCallbackOnHttp: " << qry.symbol << ", updateTime: " << qry.updatedTime;
        }
		return;
	} else if (ret != 0) {
        for (auto& qry : qryInfo.vecOrd_) {
            auto it = originSymbolToSpotSymbol_.find(qry.symbol);
            if (it == originSymbolToSpotSymbol_.end()) {
                LOG_ERROR << "BybitApi::uriQryPosiCallbackOnHttp decode symbol failed res: " << result;
                continue;
            }
            cout << "BybitApi::uriQryPosiCallbackOnHttp decode failed res: " << result;
            LOG_WARN << "BybitApi::uriQryPosiCallbackOnHttp decode failed res: " << result;

            gIsPosiQryInfoSuccess[it->second] = false;
            int failedCount = ++posiQueryFailedCountMap[it->second];
            if (failedCount >= 3) {
                cout << "BybitApi::uriQryPosiCallbackOnHttp instId [" << qry.symbol << "]" << "failedCount:" << failedCount
                            << "  ret:" << ret;
                LOG_ERROR << "BybitApi::uriQryPosiCallbackOnHttp instId [" << qry.symbol << "]" << "failedCount:" << failedCount
                            << "  ret:" << ret;
            }
        }
    }

    for (auto& qry : qryInfo.vecOrd_) {
        auto it = originSymbolToSpotSymbol_.find(qry.symbol);
        if (it == originSymbolToSpotSymbol_.end()) {
            LOG_ERROR << "BybitApi::uriQryPosiCallbackOnHttp decode symbol failed res: " << result;
            continue;
        }
        memcpy(qry.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
        
        if (strcmp(qry.side, "Buy") == 0) {
            qry.setSide("BUY");
        } else if (strcmp(qry.side, "Sell") == 0) {
            qry.setSide("SELL");
        }
        qry.size = abs(qry.size);
        gIsPosiQryInfoSuccess[qry.symbol] = true;
        gQryPosiInfo[qry.symbol] = qry;
        LOG_DEBUG << "BybitApi uriQryPosiCallbackOnHttp: " << result << "gQryPosiInfo size: " << gQryPosiInfo.size()
            << ", gIsPosiQryInfoSuccess: " << gIsPosiQryInfoSuccess[qry.symbol];
        posiQueryFailedCountMap[qry.symbol] = 0;
    }
    return;
}

void BybitApi::UpdateAccount() {
    m_uri.clear();
    m_uri.domain = Bybit_DOMAIN;
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    m_uri.api = Bybit_UPDATE_ACCOUNT;
    SetPrivateParams(HTTP_POST, m_uri);
    m_uri.Request();
    LOG_INFO << "BybitApi::UpdateAccount res: " << m_uri.result;
    cout << "BybitApi::UpdateAccount res: " << m_uri.result;
    return;
}

int BybitApi::QryPosiBySymbol(const Order &order) {
    m_uri.clear();
    m_uri.domain = Bybit_DOMAIN;
    string symbol = GetCurrencyPair((order.InstrumentID));
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    // m_uri.domain = Bybit_DOMAIN;
    //m_uri.method = METHOD_GET;
    m_uri.AddParam(("symbol"), (symbol));
    m_uri.AddParam(("category"), "linear");
    m_uri.api = Bybit_POSITION_RISK_API;
    SetPrivateParams(HTTP_GET, m_uri);
    LOG_INFO << "BybitApi QryPosiBySymbol: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
        << ", param: " << m_uri.GetParamSet();
    m_uri.Request();

    string &res = m_uri.result;
    if (res.empty()) {
        return -1;
    }

    BybitQryStruct qryInfo;
    int ret = qryInfo.decode(res.c_str());

    LOG_INFO << "BybitApi QryPosiBySymbol rst: " << res;
    if (ret == 2) {
		return 0;
	} else if (ret != 0) {
        return -1;
    }

    for (auto& qry : qryInfo.vecOrd_) {
        auto it = originSymbolToSpotSymbol_.find(qry.symbol);
        if (it == originSymbolToSpotSymbol_.end()) {
            LOG_ERROR << "QryPosiBySymbol ERROR: " << qry.symbol;
            return -1;
        }
        memcpy(qry.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
        if (strcmp(qry.side, "Buy") == 0) {
            qry.setSide("BUY");
        } else if (strcmp(qry.side, "Sell") == 0) {
            qry.setSide("SELL");
        } else {
            qry.setSide("NONE");
        }
        cout << "qry side: " << qry.side << ", symbol: " << qry.symbol << ", it->second: " << it->second << 
            ",size: " << qry.size << endl;
        gIsPosiQryInfoSuccess[qry.symbol] = true;
        gQryPosiInfo[qry.symbol] = qry;
    }
    return 0;
}

void BybitApi::uriQryOrderOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpQryOrderCallback(message, clientId);
}

int BybitApi::QryAsynOrder(const Order& order) {
    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
    uri->method = METHOD_GET;
    uri->domain = Bybit_DOMAIN;
    uri->exchangeCode = Exchange_BYBIT;
    uri->api = Bybit_QUERY_ORDER_API;

    string symbol = GetCurrencyPair((order.InstrumentID));
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

    uri->AddParam(("symbol"), (symbol));
    uri->AddParam(("category"), order.Category);
    uri->AddParam(("orderLinkId"), std::to_string(order.OrderRef));
    uri->setUriClientOrdId(order.OrderRef);
    SetPrivateParams(HTTP_GET, *(uri.get()));
    
    uri->setResponseCallback(std::bind(&BybitApi::uriQryOrderOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    LOG_INFO << "BybitApi QryAsynOrder: " << ", url: " << uri->GetUrl() 
                << ", param: " << uri->GetParamSet();    

    return 0;
}

void BybitApi::QryAsynInfo() {
    for (auto it : tickerToInstrumentMap_) {
        string instId = it.second;
        string symbol = GetCurrencyPair((instId));
        transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

        // position qry
        std::shared_ptr<Uri> uri = make_shared<Uri>();
        // uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bybit_DOMAIN;
        uri->exchangeCode = Exchange_BYBIT;
        //uri->method = METHOD_GET;
        uri->AddParam(("symbol"), (symbol));
        uri->AddParam(("category"), "linear");
        uri->api = Bybit_POSITION_RISK_API;
        //uri->api = api +"?symbol="+symbol+"&timestamp="+std::to_string(EpochTime)+"&signature="+sig_url;
        SetPrivateParams(HTTP_GET, *(uri.get()));
        uri->setResponseCallback(std::bind(&BybitApi::uriQryPosiCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        // markprice qry
        std::shared_ptr<Uri> mark_uri = make_shared<Uri>();
        mark_uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        mark_uri->domain = Bybit_DOMAIN;
        mark_uri->exchangeCode = Exchange_BYBIT;
        mark_uri->AddParam(("symbol"), (symbol));
        mark_uri->AddParam(("category"), ("linear"));
        mark_uri->api = Bybit_MARKPRICE_API;
        mark_uri->setResponseCallback(std::bind(&BybitApi::uriQryMarkPCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(mark_uri);
    }
}

void BybitApi::ConvertQuantityAndPrice(const Order &order, Uri& m_uri) {
    if(strcmp(order.InstrumentID, "BTCUSDT") == 0) {
        if (order.Direction == INNER_DIRECTION_Buy) {
            double price = order.LimitPrice * 10;
            price = std::floor(price);
            price = price/10;
            char buf[64];
            SNPRINTF(buf, sizeof(buf), "%.1f", price);
            m_uri.AddParam(("price"), (buf));
        } else if (order.Direction == INNER_DIRECTION_Sell) {
            double price = order.LimitPrice * 10;
            price = std::ceil(price);
            price = price/10;
            char buf[64];
            SNPRINTF(buf, sizeof(buf), "%.1f", price);
            m_uri.AddParam(("price"), (buf));
        } else {
            LOG_FATAL << "Order direction wr";
        }

        char buf2[64];
        SNPRINTF(buf2, sizeof(buf2), "%.5f", order.VolumeTotalOriginal);
        m_uri.AddParam(("qty"), (buf2));

    } else if(strcmp(order.InstrumentID, "ETHUSDT") == 0) {

        if (order.Direction == INNER_DIRECTION_Buy) {
            double price = order.LimitPrice * 100;
            price = std::floor(price);
            price = price/100;
            char buf[64];
            SNPRINTF(buf, sizeof(buf), "%.2f", price);
            m_uri.AddParam(("price"), (buf));
        } else if (order.Direction == INNER_DIRECTION_Sell) {
            double price = order.LimitPrice * 100;
            price = std::ceil(price);
            price = price/100;
            char buf[64];
          SNPRINTF(buf, sizeof(buf), "%.2f", price);
           m_uri.AddParam(("price"), (buf));
        } else {
            LOG_FATAL << "Order direction wr";
        }

        char buf2[64];
        SNPRINTF(buf2, sizeof(buf2), "%.4f", order.VolumeTotalOriginal);
        m_uri.AddParam(("qty"), (buf2));
    } else {
        LOG_FATAL << "BybitApi::ConvertQuantityAndPrice error " << order.InstrumentID;
    }
}

void BybitApi::uriReqCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynReqCallback(message, clientId);
}

void BybitApi::uriCanCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynCancelCallback(message, clientId);
}

/**
 * @param order
 * @return
 */
uint64_t BybitApi::ReqOrderInsert(const Order& order) {
    try {
        std::shared_ptr<Uri> uri = make_shared<Uri>();

       // uri->clear();
       // uri->isinit = true;
        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bybit_DOMAIN;
        uri->api = Bybit_FUTURE_ORDER_INSERT_API;
        uri->exchangeCode = Exchange_BYBIT;

        string cp = GetCurrencyPair(order.InstrumentID);
        transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
        uri->AddParam(("symbol"), (cp));
        uri->AddParam(("category"), (string(order.Category)));

        uri->AddParam(("orderLinkId"), std::to_string(order.OrderRef));
        uri->setUriClientOrdId(order.OrderRef);
        if (order.Direction == INNER_DIRECTION_Buy) {
            uri->AddParam(("side"), ("Buy"));
        } else if (order.Direction == INNER_DIRECTION_Sell) {
            uri->AddParam(("side"), ("Sell"));
        }

        if (order.OrderType == ORDERTYPE_LIMIT_CROSS) {
            uri->AddParam(("orderType"), ("Limit"));
        } else if (order.OrderType == ORDERTYPE_MARKET) {
            uri->AddParam(("orderType"), ("Market"));
        }

        uri->AddParam(("timeInForce"), (string(order.TimeInForce)));
        uri->AddParam(("orderFilter"), ("Order"));
        uri->AddParam(("isLeverage"), ("0"));
        uri->AddParam(("positionIdx"), ("0"));  // 单向持仓
        ConvertQuantityAndPrice(order, *(uri.get()));

        SetPrivateParams(HTTP_POST, *(uri.get()), ENCODING_GZIP);

        uri->setResponseCallback(std::bind(&BybitApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        return 0;
    }
    catch (std::exception ex) {
        LOG_WARN << "BybitApi::ReqOrderInsert exception occur:" << ex.what();
        return -1;
    }
}

int BybitApi::CancelAllOrders()
{
    if (cancelAll) return 1;
    try {
        categoryVec_.push_back("linear");
        categoryVec_.push_back("inverse");
        categoryVec_.push_back("option");
        for (auto iter : categoryVec_) {
            for (auto it : originSymbolToSpotSymbol_) {
                m_uri.clear();
                m_uri.domain = Bybit_DOMAIN;
                m_uri.api = Bybit_CANCEL_ALL_ORDER_API;

                string symbol = it.second;
                string cp = GetCurrencyPair(symbol);
                transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
                m_uri.AddParam(("symbol"), (cp));

                m_uri.AddParam(("category"), (iter));
                SetPrivateParams(HTTP_POST, m_uri, ENCODING_GZIP);

                int rstCode = m_uri.Request();
                string &res = m_uri.result;
                if (rstCode != CURLE_OK) {
                    string &res = m_uri.result;
                    LOG_FATAL << "BybitApi CancelAllOrder fail result:" << rstCode << ", res: " << res;
                    return -1;
                }
                if (res.empty()) {
                    LOG_FATAL << "BybitApi::CancelAllOrder return empty category:" << iter << ", symbol: " << symbol;
                    return -1;
                }
                LOG_INFO << "BybitApi CancelAllOrders: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
                    << ", param: " << m_uri.GetParamSet() << ", res: " << res;
            }
        }
        cancelAll = true;
        return 0;
    }
    catch (std::exception ex) {
        LOG_WARN << "BybitApi::CancelOrder exception occur:" << ex.what();
        return -1;
    }

}

int BybitApi::CancelOrder(const Order &innerOrder, int type) {
    try {
        std::shared_ptr<Uri> uri = make_shared<Uri>();

        uri->domain = Bybit_DOMAIN;
        uri->api = Bybit_FUTURE_CANCEL_ORDER_API;
        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->exchangeCode = Exchange_BYBIT;
      //  uri->isinit = true;
        string cp = GetCurrencyPair(string(innerOrder.InstrumentID));
        transform(cp.begin(), cp.end(), cp.begin(), ::toupper);

        uri->setUriClientOrdId(innerOrder.OrderRef);
        uri->AddParam(("symbol"), (cp));
        uri->AddParam(("category"), (string(innerOrder.Category)));
        uri->AddParam(("orderLinkId"), (to_string(innerOrder.OrderRef)));
        // uri.AddParam(UrlEncode("orderId"), UrlEncode(order_id));

        SetPrivateParams(HTTP_POST, *(uri.get()), ENCODING_GZIP);
        //SetPrivateParams(HTTP_DELETE);

        uri->setResponseCallback(std::bind(&BybitApi::uriCanCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        return 0;
    }
    catch (std::exception ex) {
        LOG_WARN << "BybitApi::CancelOrder exception occur:" << ex.what();
        return -1;
    }
}

void BybitApi::AddInstrument(const char *instrument) {
    string channelDepth = GetDepthChannel(instrument);//btcusdt@depth5
    depthToInstrumentMap_[channelDepth] = instrument;

    string channelTrade = GetTradeChannel(instrument);
    tradeToInstrumentMap_[channelTrade] = instrument;

    string channelTicker = GetTickerChannel(instrument);
    tickerToInstrumentMap_[channelTicker] = instrument;

    //btc_usdt_b --> btcusdt
    string cp = GetCurrencyPair(instrument);
    transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
    originSymbolToSpotSymbol_[cp] = string(instrument);

	gIsPosiQryInfoSuccess[instrument] = false;
	gIsMarkPxQryInfoSuccess[instrument] = false;

    LOG_INFO << "BybitApi::AddInstrument channelDepth: " << channelDepth << ", channelTrade: " << channelTrade
             << ", channelTicker: " << channelTicker << ", instrument: " << instrument << ", cp: " << cp;
}

string BybitApi::GetDepthChannel(string inst) {
    //btc_usdt_b --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to channel
    cp += "@depth5";
    return cp;  //btcusdt@depth5
}

string BybitApi::GetTickerChannel(string inst) {
    //btc_usdt_b --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to channel
    cp += "@bookTicker";
    return cp;  //btcusdt@bookTicker һ����������
}

string BybitApi::GetTradeChannel(string inst) {
    //btc_usdt_h --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to trade
    cp += "@trade";
    return cp;
}

string BybitApi::GetDepthInstrumentID(string channel) {
    auto iter = depthToInstrumentMap_.find(channel);
    if (iter != depthToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BybitApi::GetTickerInstrumentID(string channel) {
    auto iter = tickerToInstrumentMap_.find(channel);
    if (iter != tickerToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BybitApi::GetTradeInstrumentID(string channel) {
    auto iter = tradeToInstrumentMap_.find(channel);
    if (iter != tradeToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

std::map<string, string> BybitApi::GetDepthInstrumentMap() {
    return depthToInstrumentMap_;
}

std::map<string, string> BybitApi::GetTickerInstrumentMap() {
    return tickerToInstrumentMap_;
}

std::map<string, string> BybitApi::GetTradeInstrumentMap() {
    return tradeToInstrumentMap_;
}

string BybitApi::GetCurrencyPair(string inst) {
    //btc_usdt_h --> btcusdt
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    return cp;
}

uint64_t BybitApi::GetCurrentNonce() {
    return m_nonce;
}