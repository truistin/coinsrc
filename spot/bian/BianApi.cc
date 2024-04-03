#include "string.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include "spot/utility/Logging.h"
#include "spot/restful/UriFactory.h"
#include "spot/bian/BianApi.h"
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

#include <spot/utility/gzipwrapper.h>
#include <spot/utility/Utility.h>
#include <spot/base/MeasureLatencyCache.h>

#include <spot/restful/CurlMultiCurl.h>

using namespace boost::asio;
using namespace spot;
using namespace spot::utility;


#define Bian_SERVER_TIME_API "/api/v1/time"
#define Bian_DepthMarket_API "/api/v1/depth"
#define Bian_ACCOUNT_API "/fapi/v1/account"
#define Bian_ALL_ORDERS_API "/fapi/v1/allOrders"
#define Bian_OPEN_ORDERS_API "/fapi/v1/openOrders"
#define Bian_QUERY_ORDER_API "/fapi/v1/order"
#define Bian_ORDER_INSERT_API "/fapi/v1/order"
#define Bian_CANCEL_ORDER_API "/fapi/v1/order"
#define Bian_ORDER_INSERT_TEST_API "/fapi/v1/order/test"

#define Bian_FUTURE_SERVER_TIME_API "/fapi/v1/time"
#define Bian_FUTURE_DEPTH_API "/fapi/v1/depth"
#define Bian_FUTURE_ACCOUNT_API "/fapi/v2/account"
#define Bian_FUTURE_ALL_ORDERS_API "/fapi/v1/allOrders"
#define Bian_FUTURE_OPEN_ORDERS_API "/fapi/v1/openOrders"
#define Bian_FUTURE_QUERY_ORDER_API "/fapi/v1/order"
#define Bian_FUTURE_ORDER_INSERT_API "/fapi/v1/order"
#define Bian_FUTURE_CANCEL_ORDER_API "/fapi/v1/order"
#define Bian_MARKPRICE_FUNDRATE_API "/fapi/v1/premiumIndex"
#define Bian_ORDER_INSERT_F_TEST_API "/fapi/v1/order/test"
#define Bian_POSITION_RISK_API "/fapi/v2/positionRisk"

#define Bian_CANCEL_ALL_ORDER_API "/fapi/v1/allOpenOrders"

using namespace std;
using namespace std::chrono;
using namespace spot::base;

std::map<string, string> BianApi::depthToInstrumentMap_;
std::map<string, string> BianApi::tradeToInstrumentMap_;
std::map<string, string> BianApi::tickerToInstrumentMap_;
std::map<string, string> BianApi::originSymbolToSpotSymbol_;


BianApi::BianApi(string api_key, string secret_key, string passphrase, AdapterCrypto* adapt) {
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    m_uri.domain = Bian_DOMAIN;
    m_uri.method = METHOD_GET;
    m_uri.isinit = true;
    m_nonce = duration_cast<chrono::microseconds>(system_clock::now().time_since_epoch()).count();
    m_api_key = api_key;
    m_secret_key = secret_key;
    m_passphrase = passphrase;
    adapter_ = adapt;
    cancelAll = false;
    
    LOG_INFO << "BianApi apikey:" << m_api_key << ", secret_key: " << m_secret_key << " passwd:" << m_passphrase << " skey-len:" << m_secret_key.length();

}

BianApi::~BianApi() {
}

void BianApi::Init() {
    GetServerTime();
}



//Set api for uri before SetPrivateParams, since SetPrivateParams has to create sign using api
void BianApi::SetPrivateParams(int http_mode, Uri& m_uri, int encode_mode) {
    string sig_url = CreateSignature(http_mode, m_secret_key, m_uri, "sha256");
    m_uri.AddParam(UrlEncode("signature"), sig_url);
    // if (encode_mode == ENCODING_GZIP) {
    //     m_uri.encoding = GZIP_ENCODING;
    // }
    // 服务端目前支持的是urlencoded格式，所以头部content-type进行了格式的限定，目前m_body_format不支持json
    if (http_mode == HTTP_GET) {
        m_uri.method = METHOD_GET;
        // m_uri.m_body_format = FORM_TYPE;
    } else if (http_mode == HTTP_POST) {
        m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        m_uri.m_body_format = FORM_TYPE; 
        m_uri.method = METHOD_POST;
        // m_uri.AddHeader("Content-Type: application/json");// CreateSignature用的是param set,这里不能用json
        // m_uri.m_body_format = JSON_TYPE;     
    } else if (http_mode == HTTP_DELETE) {
        m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        m_uri.m_body_format = FORM_TYPE;  
        m_uri.method = METHOD_DELETE;
        // m_uri.AddHeader("Content-Type: application/json"); // CreateSignature用的是param set,这里不能用json
        // m_uri.m_body_format = JSON_TYPE;    // 实盘用json
    }
    string header = "X-MBX-APIKEY: " + m_api_key;
    m_uri.AddHeader(header.c_str());
}

string BianApi::CreateSignature(int http_mode, string secret_key, Uri& m_uri, const char *encode_mode) {
    string message = m_uri.GetParamSet();
    //cout << message << endl;
    /********************************ǩ�� begin**********************************/
    ////����HMAC_SHA256����
    unsigned char *output = (unsigned char *) malloc(EVP_MAX_MD_SIZE);
    unsigned int output_length = 0;
    HmacEncode(encode_mode, secret_key.c_str(), message.c_str(), output, &output_length);

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

bool BianApi::GetServerTime(int type) {
    m_uri.clear();

    m_uri.domain = Bian_DOMAIN;
    if (type == 0) {
        m_uri.api = Bian_SERVER_TIME_API;
    } else if (type == 1) {
        m_uri.api = Bian_FUTURE_SERVER_TIME_API;
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
        LOG_WARN << " BianApi::GetServerTime parse error,result:" << res;
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

void BianApi::uriQryMarkPCallbackOnHttp(char* result, uint64_t clientId)
{
    if (result == nullptr)
	{
		LOG_WARN << "BianApi::uriQryMarkPCallbackOnHttp result false";
		return;
	}

    BianMrkQryInfo qryInfo;
    int ret = qryInfo.decode(result);
    auto it = originSymbolToSpotSymbol_.find(qryInfo.markPxNode_.symbol);
    if (it == originSymbolToSpotSymbol_.end()) {
        LOG_ERROR << "BianApi::uriQryMarkPCallbackOnHttp decode symbol failed res: " << result;
        return;
    }

    if (ret != 0) {
        LOG_WARN << "BianApi::uriQryMarkPCallbackOnHttp decode failed";
        gIsMarkPxQryInfoSuccess[it->second] = false;
        int failedCount = ++queryFailedCountMap[it->second];
        if (failedCount >= 3) {
            LOG_ERROR << "BianApi::uriQryMarkPCallbackOnHttp instId [" << it->second << "]" << "failedCount:" << failedCount
                        << " res:" << result << "  ret:" << ret;
        }
        return;
    }

    memcpy(qryInfo.markPxNode_.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
    gQryMarkPxInfo[it->second] = qryInfo.markPxNode_;
    gIsMarkPxQryInfoSuccess[it->second] = true;
    LOG_DEBUG << "BianApi uriQryMarkPCallbackOnHttp: " << result << "gQryMarkPxInfo size: " << gQryMarkPxInfo.size()
        << ", gIsMarkPxQryInfoSuccess: " << gIsMarkPxQryInfoSuccess[it->second];
    queryFailedCountMap[it->second] = 0;
}

void BianApi::uriQryPosiCallbackOnHttp(char* result, uint64_t clientId)
{
	if (result == nullptr)
	{
		LOG_WARN << "BianApi::uriQryPosiCallbackOnHttp result false";
		return;
	}

    BianQryPositionInfo qryInfo;
    int ret = qryInfo.decode(result);
    if (ret == 2) {
        qryInfo.posiNode_.updatedTime = CURR_MSTIME_POINT;
        LOG_DEBUG << "NO BianApi uriQryPosiCallbackOnHttp: " << qryInfo.posiNode_.updatedTime;
		return;
	} else if (ret != 0) {
        auto it = originSymbolToSpotSymbol_.find(qryInfo.posiNode_.symbol);
        if (it == originSymbolToSpotSymbol_.end()) {
            LOG_ERROR << "BianApi No qryInfo.symbol uriQryPosiCallbackOnHttp: " << result;
            return;
        }
        cout << "BianApi::uriQryPosiCallbackOnHttp decode failed res: " << result;
        LOG_WARN << "BianApi::uriQryPosiCallbackOnHttp decode failed res: " << result;

        gIsPosiQryInfoSuccess[it->second] = false;
        int failedCount = ++posiQueryFailedCountMap[it->second];
        if (failedCount >= 3) {
            cout << "BianApi::uriQryPosiCallbackOnHttp instId [" << qryInfo.posiNode_.symbol << "]" << "failedCount:" << failedCount
                        << "  ret:" << ret;
            LOG_ERROR << "BianApi::uriQryPosiCallbackOnHttp instId [" << qryInfo.posiNode_.symbol << "]" << "failedCount:" << failedCount
                        << "  ret:" << ret;
        }
        return;
    }
    auto it = originSymbolToSpotSymbol_.find(qryInfo.posiNode_.symbol);
    if (it == originSymbolToSpotSymbol_.end()) {
        LOG_ERROR << "BianApi::uriQryPosiCallbackOnHttp decode symbol failed res: " << result;
        return;
    } 
    memcpy(qryInfo.posiNode_.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
    gIsPosiQryInfoSuccess[qryInfo.posiNode_.symbol] = true;
    gQryPosiInfo[qryInfo.posiNode_.symbol] = qryInfo.posiNode_;
    LOG_DEBUG << "BianApi uriQryPosiCallbackOnHttp: " << result << "gQryPosiInfo size: " << gQryPosiInfo.size()
        << ", gIsPosiQryInfoSuccess: " << gIsPosiQryInfoSuccess[qryInfo.posiNode_.symbol];
    posiQueryFailedCountMap[qryInfo.posiNode_.symbol] = 0;
    return;
}

void BianApi::uriQryOrderOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpQryOrderCallback(message, clientId);
}

int BianApi::QryAsynOrder(const Order& order) {
    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
    uri->method = METHOD_GET;
    uri->domain = Bian_DOMAIN;
    uri->exchangeCode = Exchange_BINANCE;
    uri->api = Bian_QUERY_ORDER_API;
    uint64_t time = CURR_MSTIME_POINT;

    string symbol = GetCurrencyPair((order.InstrumentID));
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

    uri->AddParam(("symbol"), symbol);
    uri->AddParam(("origClientOrderId"), std::to_string(order.OrderRef));
    uri->AddParam(("timestamp"), std::to_string(time));
    uri->setUriClientOrdId(order.OrderRef);

    SetPrivateParams(HTTP_GET, *(uri.get()));
    
    uri->setResponseCallback(std::bind(&BianApi::uriQryOrderOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    LOG_INFO << "BianApi QryAsynOrder: " << ", url: " << uri->GetUrl() 
                << ", param: " << uri->GetParamSet();
    
    return 0;
}

void BianApi::QryAsynInfo() {
    for (auto it : tickerToInstrumentMap_) {
        string instId = it.second;
        string symbol = GetCurrencyPair((instId));
        transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

        // position qry
        std::shared_ptr<Uri> uri = make_shared<Uri>();
        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bian_DOMAIN;
        uri->exchangeCode = Exchange_BINANCE;
        //uri->method = METHOD_GET;
        uri->AddParam(("symbol"), (symbol));
        uint64_t EpochTime = CURR_MSTIME_POINT;
        uri->AddParam(("timestamp"), std::to_string(EpochTime));
        uri->AddParam(("recvWindow"), ("3000"));
        uri->api = Bian_POSITION_RISK_API;
        //uri->api = api +"?symbol="+symbol+"&timestamp="+std::to_string(EpochTime)+"&signature="+sig_url;
        SetPrivateParams(HTTP_GET, *(uri.get()));
        uri->setResponseCallback(std::bind(&BianApi::uriQryPosiCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        // markprice qry
        std::shared_ptr<Uri> mark_uri = make_shared<Uri>();
        mark_uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        mark_uri->domain = Bian_DOMAIN;
        uri->exchangeCode = Exchange_BINANCE;
        mark_uri->AddParam(("symbol"), (symbol));
        mark_uri->api = Bian_MARKPRICE_FUNDRATE_API;
        mark_uri->setResponseCallback(std::bind(&BianApi::uriQryMarkPCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(mark_uri);
    }
}

int BianApi::QryPosiBySymbol(const Order &order) {
    m_uri.clear();
    m_uri.domain = Bian_DOMAIN;
    string symbol = GetCurrencyPair((order.InstrumentID));
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    m_uri.AddParam(("symbol"), (symbol));
    uint64_t EpochTime = CURR_MSTIME_POINT;
    m_uri.AddParam(("timestamp"), std::to_string(EpochTime));
    // m_uri.AddParam(UrlEncode("recvWindow"), UrlEncode("3000"));
    string sig_url = CreateSignature(HTTP_GET, m_secret_key, m_uri, "sha256");
    m_uri.method = METHOD_GET;
    string header = "X-MBX-APIKEY: " + m_api_key;
    m_uri.AddHeader(header.c_str());
    string api = Bian_POSITION_RISK_API;
    m_uri.api = api +"?symbol="+symbol+"&timestamp="+std::to_string(EpochTime)+"&signature="+sig_url;
    LOG_INFO << "BianApi QryPosiBySymbol: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
        << ", param: " << m_uri.GetParamSet();
    m_uri.ClearParam();
    m_uri.Request();
    
    string &res = m_uri.result;
    if (res.empty()) {
        LOG_FATAL << "BianApi::QryPosiBySymbol decode failed res: " << res;
        return -1;
    }
    BianQryPositionInfo qryInfo;
    int ret = qryInfo.decode(res.c_str());
    if (ret != 0) {
        LOG_FATAL << "BianApi::QryPosiBySymbol decode failed res: " << res;
        return -1;
    }

    LOG_INFO << "BianApi QryPosiBySymbol rst: " << res;
    if (ret == 2) return 0;

    if (IS_DOUBLE_LESS(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "BOTH") == 0)) {
        memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
        string str("SELL");
        memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
    } else if (IS_DOUBLE_GREATER_EQUAL(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "BOTH") == 0)) {
        memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
        string str("BUY");
        memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
    } else {
        LOG_FATAL << "BianApi QryPosiBySymbol ERROR: " << qryInfo.posiNode_.side<< ", size: " << qryInfo.posiNode_.size;
    }

    qryInfo.posiNode_.size = abs(qryInfo.posiNode_.size);
    memcpy(qryInfo.posiNode_.symbol, order.InstrumentID, InstrumentIDLen);
    
    gIsPosiQryInfoSuccess[order.InstrumentID] = true;
    gQryPosiInfo[order.InstrumentID] = qryInfo.posiNode_;
    return 0;

}

void BianApi::ConvertQuantity(const Order &order, Uri& m_uri) {
    auto it = orderFormMap.find(order.InstrumentID);
    if (it == orderFormMap.end()) {
        LOG_FATAL << "BianApi::ConvertQuantity error, not found InstrumentID: " << order.InstrumentID;
    }

    char buf2[64];
    char qtyformat[20]; 
    snprintf(qtyformat, sizeof(qtyformat), "%%.%df", it->second.qty_decimal); 
    SNPRINTF(buf2, sizeof(buf2), qtyformat, order.VolumeTotalOriginal);
    m_uri.AddParam(("quantity"), (buf2));
}

void BianApi::ConvertPrice(const Order &order, Uri& m_uri) {
    auto it = orderFormMap.find(order.InstrumentID);
    if (it == orderFormMap.end()) {
        LOG_FATAL << "BianApi::ConvertPrc error, not found InstrumentID: " << order.InstrumentID;
    }
    if (order.Direction == INNER_DIRECTION_Buy) {
        double price = order.LimitPrice * it->second.price_decimal;
        price = std::floor(price);
        price = price/it->second.price_decimal;

        char buf[64];
        char prcformat[20]; 
        snprintf(prcformat, sizeof(prcformat), "%%.%df", it->second.price_decimal); 
        SNPRINTF(buf, sizeof(buf), prcformat, price);

        m_uri.AddParam(("price"), (buf));

    } else if (order.Direction == INNER_DIRECTION_Sell) {
        double price = order.LimitPrice * it->second.price_decimal;
        price = std::ceil(price);
        price = price/it->second.price_decimal;

        char buf[64];
        char prcformat[20]; 
        snprintf(prcformat, sizeof(prcformat), "%%.%df", it->second.price_decimal); 
        SNPRINTF(buf, sizeof(buf), prcformat, price);

        m_uri.AddParam(("price"), (buf));

    } else {
        LOG_FATAL << "Order direction wr" << order.InstrumentID << ", side: " << order.Direction;
    }
}

// void BianApi::ConvertQuantity(const Order &order, Uri& m_uri) {
//     if(strcmp(order.InstrumentID, "btc_usdt_binance") == 0) {
//         char buf2[64];
//         SNPRINTF(buf2, sizeof(buf2), "%.5f", order.VolumeTotalOriginal);
//         m_uri.AddParam(("quantity"), (buf2));

//     } else if(strcmp(order.InstrumentID, "eth_usdt_binance") == 0) {
//         char buf2[64];
//         SNPRINTF(buf2, sizeof(buf2), "%.4f", order.VolumeTotalOriginal);
//         m_uri.AddParam(("quantity"), (buf2));
//     } else {
//         LOG_FATAL << "BianApi::ConvertQuantity error " << order.InstrumentID;
//     }
// }

// void BianApi::ConvertPrice(const Order &order, Uri& m_uri) {
//     if(strcmp(order.InstrumentID, "btc_usdt_binance") == 0) {
//         if (order.Direction == INNER_DIRECTION_Buy) {
//             double price = order.LimitPrice * 10;
//             price = std::floor(price);
//             price = price/10;
//             char buf[64];
//             SNPRINTF(buf, sizeof(buf), "%.1f", price);
//             m_uri.AddParam(("price"), (buf));
//         } else if (order.Direction == INNER_DIRECTION_Sell) {
//             double price = order.LimitPrice * 10;
//             price = std::ceil(price);
//             price = price/10;
//             char buf[64];
//             SNPRINTF(buf, sizeof(buf), "%.1f", price);
//             m_uri.AddParam(("price"), (buf));
//         } else {
//             LOG_FATAL << "Order direction wr";
//         }

//     } else if(strcmp(order.InstrumentID, "eth_usdt_binance") == 0) {

//         if (order.Direction == INNER_DIRECTION_Buy) {
//             double price = order.LimitPrice * 100;
//             price = std::floor(price);
//             price = price/100;
//             char buf[64];
//             SNPRINTF(buf, sizeof(buf), "%.2f", price);
//             m_uri.AddParam(("price"), (buf));
//         } else if (order.Direction == INNER_DIRECTION_Sell) {
//             double price = order.LimitPrice * 100;
//             price = std::ceil(price);
//             price = price/100;
//             char buf[64];
//           SNPRINTF(buf, sizeof(buf), "%.2f", price);
//            m_uri.AddParam(("price"), (buf));
//         } else {
//             LOG_FATAL << "Order direction wr";
//         }

//     } else {
//         LOG_FATAL << "BianApi::ConvertPrice error " << order.InstrumentID;
//     }
// }

void BianApi::uriReqCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynReqCallback(message, clientId);
}

void BianApi::uriCanCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynCancelCallback(message, clientId);
}

/**
 * @param order
 * @return
 */
uint64_t BianApi::ReqOrderInsert(const Order& order) {
    // try {
        std::shared_ptr<Uri> uri = make_shared<Uri>();
        
        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
       // uri->clear();
       // uri->isinit = true;
        uri->domain = Bian_DOMAIN;
        uri->api = Bian_FUTURE_ORDER_INSERT_API;
        uri->exchangeCode = Exchange_BINANCE;

        string cp = GetCurrencyPair(order.InstrumentID);
        transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
        uri->AddParam(("symbol"), (cp));

        uri->AddParam(("newClientOrderId"), std::to_string(order.OrderRef));
        uri->setUriClientOrdId(order.OrderRef);
        if (order.Direction == INNER_DIRECTION_Buy && order.OrderType == ORDERTYPE_LIMIT_CROSS) {
            uri->AddParam(("type"), ("LIMIT"));
            uri->AddParam(("side"), ("BUY"));
            ConvertPrice(order, *(uri.get()));
            uri->AddParam(("timeInForce"), (string(order.TimeInForce)));
        } else if (order.Direction == INNER_DIRECTION_Sell && order.OrderType == ORDERTYPE_LIMIT_CROSS) {
            uri->AddParam(("type"), ("LIMIT"));
            uri->AddParam(("side"), ("SELL"));
            ConvertPrice(order, *(uri.get()));
            uri->AddParam(("timeInForce"), (string(order.TimeInForce)));
        } else if (order.Direction == INNER_DIRECTION_Buy && order.OrderType == ORDERTYPE_MARKET) {
            uri->AddParam(("type"), ("MARKET"));
            uri->AddParam(("side"), ("BUY"));
        } else if (order.Direction == INNER_DIRECTION_Sell && order.OrderType == ORDERTYPE_MARKET) {
            uri->AddParam(("type"), ("MARKET"));
            uri->AddParam(("side"), ("SELL"));
        }

        uri->AddParam(("recvWindow"), ("3000"));
        ConvertQuantity(order, *(uri.get()));
        uint64_t EpochTime = CURR_MSTIME_POINT;

        uri->AddParam(("timestamp"), (std::to_string(EpochTime)));

        SetPrivateParams(HTTP_POST, *(uri.get()), ENCODING_GZIP);

        uri->setResponseCallback(std::bind(&BianApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        return 0;
    // }
    // catch (std::exception ex) {
    //     LOG_WARN << "BianApi::ReqOrderInsert exception occur:" << ex.what();
    //     return -1;
    // }
}

int BianApi::CancelOrder(string &symbol, string &order_id, int type) {
    // try {
        std::shared_ptr<Uri> uri = make_shared<Uri>();

        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bian_DOMAIN;
        uri->api = Bian_FUTURE_CANCEL_ORDER_API;
        uri->exchangeCode = Exchange_BINANCE;
      //  uri->isinit = true;
        string cp = GetCurrencyPair(symbol);
        transform(cp.begin(), cp.end(), cp.begin(), ::toupper);

        uri->setUriClientOrdId(stoi(order_id));
        uri->AddParam(("symbol"), (cp));
        uri->AddParam(("origClientOrderId"), (order_id));
//        m_uri.AddParam(UrlEncode("orderId"), UrlEncode(order_id));

        uint64_t EpochTime = CURR_MSTIME_POINT;
        //todo modify int
        std::stringstream ss;
        ss << EpochTime;
        uri->AddParam(("timestamp"), (ss.str()));
        //todo int
        uri->AddParam(("recvWindow"), ("30000"));

        SetPrivateParams(HTTP_DELETE, *(uri.get()), ENCODING_GZIP);
        //SetPrivateParams(HTTP_DELETE);

        uri->setResponseCallback(std::bind(&BianApi::uriCanCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);
        return 0;
    // }
    // catch (std::exception ex) {
    //     LOG_WARN << "BianApi::CancelOrder exception occur:" << ex.what();
    //     return -1;
    // }
}

int BianApi::CancelAllOrders() {
    if (cancelAll) return 1;
    try {
        for (auto it : originSymbolToSpotSymbol_) {
            string symbol = it.second;
            m_uri.clear();
            m_uri.domain = Bian_DOMAIN;
            m_uri.api = Bian_CANCEL_ALL_ORDER_API;
            string cp = GetCurrencyPair(symbol);
            transform(cp.begin(), cp.end(), cp.begin(), ::toupper);

            m_uri.AddParam(("symbol"), (cp));

            uint64_t EpochTime = CURR_MSTIME_POINT;
            //todo modify int
            // std::stringstream ss;
            // ss << EpochTime;
            // m_uri.AddParam(UrlEncode("timestamp"), UrlEncode(ss.str()));
            // m_uri.AddParam(UrlEncode("recvWindow"), UrlEncode("10000"));
            m_uri.AddParam(("timestamp"), std::to_string(EpochTime));

            SetPrivateParams(HTTP_DELETE, m_uri, ENCODING_GZIP);
            // m_uri.api = api +"?symbol="+symbol+"&timestamp="+std::to_string(EpochTime)+"&signature="+sig_url;
            int rstCode = m_uri.Request();
            string &res = m_uri.result;
            if (rstCode != CURLE_OK) {
                string &res = m_uri.result;
                LOG_FATAL << "BianApi CancelAllOrder fail result:" << rstCode << ", res: " << res;
                return -1;
            }
            if (res.empty()) {
                LOG_FATAL << "BianApi::CanCancelAllOrdercelOrder return empty. symbol:" << symbol;
                return -1;
            }
            LOG_INFO << "BianApi CancelAllOrders: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
                << ", param: " << m_uri.GetParamSet() << ", res: " << res;

        }
        cancelAll = true;
        return 0;
    }
    catch (std::exception ex) {
        LOG_WARN << "BianApi::CancelOrder exception occur:" << ex.what();
        return -1;
    }
}

void BianApi::AddInstrument(const char *instrument) {
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

    // for (auto it : originSymbolToSpotSymbol_) {
    //     cout << "origin symbol: " << it.first << ", spot symbol: " << it.second;
    // }

//    BianMrkQryInfo qryInfo;
//    gQryMarkPxInfo[instrument] = qryInfo;
//    gIsBianQryInfoSuccess[instrument] = false;

	gIsPosiQryInfoSuccess[instrument] = false;
	gIsMarkPxQryInfoSuccess[instrument] = false;

    LOG_INFO << "BianApi::AddInstrument channelDepth: " << channelDepth << ", channelTrade: " << channelTrade
             << ", channelTicker: " << channelTicker;
}

string BianApi::GetDepthChannel(string inst) {
    //btc_usdt_b --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to channel
    cp += "@depth5";
    return cp;  //btcusdt@depth5
}

string BianApi::GetTickerChannel(string inst) {
    //btc_usdt_b --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to channel
    cp += "@bookTicker";
    return cp;  //btcusdt@bookTicker һ����������
}

string BianApi::GetTradeChannel(string inst) {
    //btc_usdt_h --> btcusdt
    string cp = GetCurrencyPair(inst);
    //convert to trade
    cp += "@trade";
    return cp;
}

string BianApi::GetDepthInstrumentID(string channel) {
    auto iter = depthToInstrumentMap_.find(channel);
    if (iter != depthToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BianApi::GetTickerInstrumentID(string channel) {
    auto iter = tickerToInstrumentMap_.find(channel);
    if (iter != tickerToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BianApi::GetTradeInstrumentID(string channel) {
    auto iter = tradeToInstrumentMap_.find(channel);
    if (iter != tradeToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

std::map<string, string> BianApi::GetDepthInstrumentMap() {
    return depthToInstrumentMap_;
}

std::map<string, string> BianApi::GetTickerInstrumentMap() {
    return tickerToInstrumentMap_;
}

std::map<string, string> BianApi::GetTradeInstrumentMap() {
    return tradeToInstrumentMap_;
}

string BianApi::GetCurrencyPair(string inst) {
    //btc_usdt_h --> btcusdt
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    return cp;
}

uint64_t BianApi::GetCurrentNonce() {
    return m_nonce;
}