#include "string.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include "spot/utility/Logging.h"
#include "spot/restful/UriFactory.h"
#include "spot/bian/BnApi.h"
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
using namespace std;

#define BN_MARKPRICE_FUNDRATE_API "/fapi/v1/premiumIndex"
#define BN_SERVER_TIME_API "/api/v1/time"
#define BN_FUTURE_SERVER_TIME_API "/fapi/v1/time"

#define BN_UM_ORDER_API "/papi/v1/um/order"
#define BN_UM_ALL_ORDERS_API "/papi/v1/um/allOrders"

#define BN_UM_CANCEL_ALL_ORDER_API "/papi/v1/um/allOpenOrders"
#define BN_UM_POSITION_RISK_API "/papi/v1/um/positionRisk"

#define BN_CM_ORDER_API "/papi/v1/cm/order"
#define BN_CM_ALL_ORDERS_API "/papi/v1/cm/allOrders"

#define BN_CM_CANCEL_ALL_ORDER_API "/papi/v1/cm/allOpenOrders"
#define BN_CM_POSITION_RISK_API "/papi/v1/cm/positionRisk"

#define BN_LEVERAGE_ORDER_API "/papi/v1/margin/order"

#define BN_UM_BRACKET_API "/papi/v1/um/leverageBracket"
#define BN_CM_BRACKET_API "/papi/v1/cm/leverageBracket"
#define BN_COLLATERALRATE_API "/sapi/v1/portfolio/collateralRate"

#define BN_CM_ACCOUNT "/papi/v1/cm/account"
#define BN_UM_ACCOUNT "/papi/v1/um/account"
#define BN_SPOT_BALANCE "/papi/v1/balance"

using namespace std;
using namespace std::chrono;
using namespace spot::base;

std::map<string, string> BnApi::depthToInstrumentMap_;
std::map<string, string> BnApi::tradeToInstrumentMap_;
std::map<string, string> BnApi::tickerToInstrumentMap_;

std::map<string, string> BnApi::spot_depthToInstrumentMap_;
std::map<string, string> BnApi::spot_tradeToInstrumentMap_;
std::map<string, string> BnApi::spot_tickerToInstrumentMap_;

std::map<string, string> BnApi::originSymbolToSpotSymbol_;

std::map<string, BnSpotAssetInfo> BnApi::BalMap_;

BnCmAccount* BnApi::CmAcc_;
BnUmAccount* BnApi::UmAcc_;

BnApi::BnApi(string api_key, string secret_key, string passphrase, AdapterCrypto* adapt) {
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    m_uri.domain = Bn_DOMAIN;
    m_uri.method = METHOD_GET;
    m_uri.isinit = true;
    m_nonce = duration_cast<chrono::microseconds>(system_clock::now().time_since_epoch()).count();
    m_api_key = api_key;
    m_secret_key = secret_key;
    m_passphrase = passphrase;
    adapter_ = adapt;
    cancelAll = false;

    CmAcc_ = new BnCmAccount;
    UmAcc_ = new BnUmAccount;
    
    LOG_INFO << "BnApi apikey:" << m_api_key << ", secret_key: " << m_secret_key << " passwd:" << m_passphrase << " skey-len:" << m_secret_key.length();

}

BnApi::~BnApi() {
}

void BnApi::Init() {
    GetServerTime();
}



//Set api for uri before SetPrivateParams, since SetPrivateParams has to create sign using api
void BnApi::SetPrivateParams(int http_mode, Uri& m_uri, int encode_mode) {
    string sig_url = CreateSignature(http_mode, m_secret_key, m_uri, "sha256");
    m_uri.AddParam(UrlEncode("signature"), sig_url);
    // if (encode_mode == ENCODING_GZIP) {
    //     m_uri.encoding = GZIP_ENCODING;
    // }
    if (http_mode == HTTP_GET) {
        m_uri.method = METHOD_GET;
        // m_uri.m_body_format = FORM_TYPE;
    } else if (http_mode == HTTP_POST) {
        m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        m_uri.m_body_format = FORM_TYPE; 
        m_uri.method = METHOD_POST;
        // m_uri.AddHeader("Content-Type: application/json");
        // m_uri.m_body_format = JSON_TYPE;     
    } else if (http_mode == HTTP_DELETE) {
        m_uri.AddHeader("Content-Type: application/x-www-form-urlencoded");
        m_uri.m_body_format = FORM_TYPE;  
        m_uri.method = METHOD_DELETE;
        // m_uri.AddHeader("Content-Type: application/json"); 
        // m_uri.m_body_format = JSON_TYPE;    // 瀹炵洏鐢╦son
    }
    string header = "X-MBX-APIKEY: " + m_api_key;
    m_uri.AddHeader(header.c_str());
}

string BnApi::CreateSignature(int http_mode, string secret_key, Uri& m_uri, const char *encode_mode) {
    string message = m_uri.GetParamSet();
    //cout << message << endl;
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

    string sig_url = UrlEncode(dst.c_str());
    if (output) {
        free(output);
    }
    return sig_url;
}

bool BnApi::GetServerTime(int type) {
    m_uri.clear();

    m_uri.domain = Bn_DOMAIN;
    if (type == 0) {
        m_uri.api = BN_SERVER_TIME_API;
    } else if (type == 1) {
        m_uri.api = BN_FUTURE_SERVER_TIME_API;
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
        LOG_WARN << " BnApi::GetServerTime parse error,result:" << res;
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

void BnApi::GetSpotAsset()
{
    m_uri.clear();
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.domain = Bn_DOMAIN;
    m_uri.api = BN_SPOT_BALANCE;

    uint64_t EpochTime = CURR_MSTIME_POINT;

    m_uri.AddParam(("timestamp"), std::to_string(EpochTime));
    SetPrivateParams(HTTP_GET, m_uri);
    m_uri.Request();

    string &res = m_uri.result;
    // cout << "GetSpotAsset result: " << res;
    if (res.empty()) {
        LOG_ERROR << "BnApi GetSpotAsset decode failed res: " << res;
        return;
    }

    BnSpotAsset assetInfo;
    int ret = assetInfo.decode(res.c_str());
    if (ret != 0) {
        LOG_ERROR << "BnApi GetSpotAsset ERROR: " << res;
        return;
    }
/*
	char		asset[20];
	double		crossMarginFree;
	double 		crossMarginLocked;
	double		crossMarginBorrowed;
	double		crossMarginInterest;
*/
    for (auto& it : assetInfo.info_) {
        BalMap_[it.asset] = it;
        LOG_INFO << "GetSpotAsset asset: " << it.asset << ", crossMarginFree: " << it.crossMarginFree
            << ", crossMarginLocked: " << it.crossMarginLocked << ", crossMarginBorrowed: " << it.crossMarginBorrowed
            << ", crossMarginInterest: " << it.crossMarginInterest << ", crossMarginAsset: " << it.crossMarginAsset;
    }
    // BalMap_["crossMarginFree"] = assetInfo.crossMarginFree;
    // BalMap_["crossMarginLocked"] = assetInfo.crossMarginLocked;
}

void BnApi::GetUm_Cm_Account()
{
    m_uri.clear();
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.domain = Bn_DOMAIN;
    m_uri.api = BN_UM_ACCOUNT;

    uint64_t EpochTime = CURR_MSTIME_POINT;

    m_uri.AddParam(("timestamp"), std::to_string(EpochTime));
    SetPrivateParams(HTTP_GET, m_uri);
    m_uri.Request();

    string &res = m_uri.result;
    cout << "GetUm_Cm_Account result: " << res;
    if (res.empty()) {
        LOG_ERROR << "BnApi GetUm_Cm_Account decode failed res: " << res;
        return;
    }

    int ret = UmAcc_->decode(res.c_str());
    if (ret != 0) {
        LOG_ERROR << "BnApi GetUm_Cm_Account ERROR: " << res;
    }

    m_uri.clear();
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.domain = Bn_DOMAIN;
    m_uri.api = BN_CM_ACCOUNT;

    EpochTime = CURR_MSTIME_POINT;

    m_uri.AddParam(("timestamp"), std::to_string(EpochTime));
    SetPrivateParams(HTTP_GET, m_uri);
    m_uri.Request();

    string &res1 = m_uri.result;
    if (res1.empty()) {
        LOG_ERROR << "BnApi GetUm_Cm_Account decode failed res: " << res1;
        return;
    }

    ret = CmAcc_->decode(res1.c_str());
    if (ret != 0) {
        LOG_ERROR << "BnApi GetUm_Cm_Account ERROR: " << res1;
        return;
    }

    // for (auto& it : UmAcc_.info_) {
    //     LOG_INFO << "BnApi umAccInfo: " << it.asset << ", crossWalletBalance: " <<
    //         it.crossWalletBalance << ", crossUnPnl: " << it.crossUnPnl;
    // }

    // for (auto& it : cmAccInfo.info_) {
    //     LOG_INFO << "BnApi cmAccInfo: " << it.asset << ", crossWalletBalance: " <<
    //         it.crossWalletBalance << ", crossUnPnl: " << it.crossUnPnl;
    // }


}

void BnApi::GetLeverageBracket()
{
    for (auto it : originSymbolToSpotSymbol_) {
        m_uri.clear();
        m_uri.protocol = HTTP_PROTOCOL_HTTPS;
        m_uri.domain = Bn_DOMAIN;
        if (it.first.find("PERP") != std::string::npos) {
            m_uri.api = BN_CM_BRACKET_API;
        } else {
            m_uri.api = BN_UM_BRACKET_API;
        }
        
        uint64_t EpochTime = CURR_MSTIME_POINT;
        m_uri.AddParam(("symbol"), (it.first));
        m_uri.AddParam(("timestamp"), std::to_string(EpochTime));
        SetPrivateParams(HTTP_GET, m_uri);
        m_uri.Request();
        
        string &res = m_uri.result;
        // LOG_INFO << "GetLeverageBracket res: " << res << ", symbol: " << it.first;

        if (res.empty()) {
            LOG_FATAL << "BnApi GetLeverageBracket decode failed res: " << res << ", symbol: " << it.first;
            return;
        }

        Document doc;
        doc.Parse(res.c_str(), res.size());
        if (doc.HasParseError())
        {
            LOG_WARN << "BianApi GetLeverageBracket Parse error. result:" << res << ", symbol: " << it.first;
            return;
        }

        spotrapidjson::Value dataNodes = doc.GetArray();
        for (int i = 0; i < dataNodes.Capacity(); ++i) {
            spotrapidjson::Value &dataNode = dataNodes[i];
            for (auto it : mmr_table) {
                if (dataNode["symbol"].GetString() == it.table_name) {
                    int size = stoi(dataNode["notionalCoef"].GetString());
                    if (size != it.rows) {
                        LOG_WARN << "GetLeverageBracket ERROR: " << it.table_name;
                    }
                    spotrapidjson::Value bracketsArray = dataNode["brackets"].GetArray();
                    for (int j = 0; j < bracketsArray.Capacity(); j++) {
                        spotrapidjson::Value &Node = bracketsArray[j];
                        LOG_INFO << "mmr table symbol: " << it.table_name << ", notionalFloor: " << it.data[j][0]
                            << ", notionalCap: " << it.data[j][1] << ", initialLeverage: " << it.data[j][2]
                            << ", maintMarginRatio: " << it.data[j][3] << ", cum: " << it.data[j][4];

                        int64_t initialLeverage = Node["initialLeverage"].GetInt64();
                        it.data[j][2] = initialLeverage;

                        int64_t  notionalCap = Node["notionalCap"].GetInt64();
                        it.data[j][1] = notionalCap;

                        int64_t notionalFloor = Node["notionalFloor"].GetInt64();
                        it.data[j][0] = notionalFloor;

                        double maintMarginRatio = Node["maintMarginRatio"].GetDouble();
                        it.data[j][3] = maintMarginRatio;

                        double cum = Node["cum"].GetDouble();
                        it.data[j][4] = cum;

                        LOG_INFO << "GetLeverageBracket symbol: " << it.table_name << ", notionalFloor: " << it.data[j][0]
                            << ", notionalCap: " << it.data[j][1] << ", initialLeverage: " << it.data[j][2]
                            << ", maintMarginRatio: " << it.data[j][3] << ", cum: " << it.data[j][4];
                    }
                }
                continue;
            }
        }

    }

}

void BnApi::GetCollateralRate()
{
    m_uri.clear();
    m_uri.protocol = HTTP_PROTOCOL_HTTPS;
    m_uri.urlprotocolstr = URL_PROTOCOL_HTTPS;
    m_uri.method = METHOD_GET;
    m_uri.domain = "www.binancezh.info";
    m_uri.api = "/bapi/margin/v1/public/margin/portfolio/collateral-rate";

    m_uri.Request();
    string &res = m_uri.result;
    // cout << "GetCollateralRate res: " << res;
    // LOG_INFO << "GetCollateralRate res: " << res.c_str() << ", url: " << m_uri.GetUrl() << ", errorcode: " << m_uri.errcode 
        // << ", res size: " << res.size();
    if (res.size() < 10) {
        LOG_FATAL << "BnApi::GetCollateralRateHttp decode failed res: " << res;
        return;
    }

    Document doc;
    doc.Parse(res.c_str(), res.size());
    if (doc.HasParseError())
    {
        LOG_WARN << "BianApi::CancelOrder Parse error. result:" << res;
        return;
    }

    if (doc.HasMember("code"))
    {
        if (doc["code"].IsString())
        {
            string errorCode = doc["code"].GetString();
            if (errorCode != "000000")
            {
                LOG_FATAL << " BnApi::GetCollateralRate result:" << res;
            }
        }
    }

    spotrapidjson::Value &dataNodes = doc["data"];

    if (!dataNodes.IsArray()) {
        cout << "GetCollateralRate decode failed: " << endl;
        LOG_FATAL << "GetCollateralRate decode failed: " << res;
    }

    if (dataNodes.Size() == 0) {
        cout << "GetCollateralRate decode failed: " << endl;
        LOG_FATAL << "GetCollateralRate decode failed: " << res;
    }
/*
{"code":"000000","message":null,"messageDetail":null,"data":[{"asset":"USDC","collateralRate":"0.9999"},{"asset":"USDT","collateralRate":"0.9999"},{"asset":"BTC","collateralRate":"0.9500"},{"asset":"ETH","collateralRate":"0.9500"},{"asset":"BNB","collateralRate":"0.9500"},{"asset":"DOGE","collateralRate":"0.9500"}]

*/
    for (int i = 0; i < dataNodes.Size(); i++) {
        spotrapidjson::Value& dataNode = dataNodes[i];
        string str = dataNode["asset"].GetString();
        double d = stod(dataNode["collateralRate"].GetString());
        auto it = collateralRateMap.find(str);
        if (it == collateralRateMap.end()) {
            collateralRateMap.insert({str, d});
        } else {
            it->second = d;
        }
    }
}

void BnApi::uriQryMarkPCallbackOnHttp(char* result, uint64_t clientId)
{
    if (result == nullptr)
	{
		LOG_WARN << "BnApi::uriQryMarkPCallbackOnHttp result false";
		return;
	}

    BianMrkQryInfo qryInfo;
    int ret = qryInfo.decode(result);
    auto it = originSymbolToSpotSymbol_.find(qryInfo.markPxNode_.symbol);
    if (it == originSymbolToSpotSymbol_.end()) {
        LOG_ERROR << "BnApi::uriQryMarkPCallbackOnHttp decode symbol failed res: " << result;
        return;
    }

    if (ret != 0) {
        LOG_WARN << "BnApi::uriQryMarkPCallbackOnHttp decode failed";
        gIsMarkPxQryInfoSuccess[it->second] = false;
        int failedCount = ++queryFailedCountMap[it->second];
        if (failedCount >= 3) {
            LOG_ERROR << "BnApi::uriQryMarkPCallbackOnHttp instId [" << it->second << "]" << "failedCount:" << failedCount
                        << " res:" << result << "  ret:" << ret;
        }
        return;
    }

    memcpy(qryInfo.markPxNode_.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
    gQryMarkPxInfo[it->second] = qryInfo.markPxNode_;
    gIsMarkPxQryInfoSuccess[it->second] = true;
    LOG_DEBUG << "BnApi uriQryMarkPCallbackOnHttp: " << result << "gQryMarkPxInfo size: " << gQryMarkPxInfo.size()
        << ", gIsMarkPxQryInfoSuccess: " << gIsMarkPxQryInfoSuccess[it->second];
    queryFailedCountMap[it->second] = 0;
}

void BnApi::uriQryPosiCallbackOnHttp(char* result, uint64_t clientId)
{
	if (result == nullptr)
	{
		LOG_WARN << "BnApi::uriQryPosiCallbackOnHttp result false";
		return;
	}

    BianQryPositionInfo qryInfo;
    int ret = qryInfo.decode(result);
    if (ret == 2) {
        qryInfo.posiNode_.updatedTime = CURR_MSTIME_POINT;
        LOG_DEBUG << "NO BnApi uriQryPosiCallbackOnHttp: " << qryInfo.posiNode_.updatedTime;
		return;
	} else if (ret != 0) {
        auto it = originSymbolToSpotSymbol_.find(qryInfo.posiNode_.symbol);
        if (it == originSymbolToSpotSymbol_.end()) {
            LOG_ERROR << "BnApi No qryInfo.symbol uriQryPosiCallbackOnHttp: " << result;
            return;
        }
        cout << "BnApi::uriQryPosiCallbackOnHttp decode failed res: " << result;
        LOG_WARN << "BnApi::uriQryPosiCallbackOnHttp decode failed res: " << result;

        gIsPosiQryInfoSuccess[it->second] = false;
        int failedCount = ++posiQueryFailedCountMap[it->second];
        if (failedCount >= 3) {
            cout << "BnApi::uriQryPosiCallbackOnHttp instId [" << qryInfo.posiNode_.symbol << "]" << "failedCount:" << failedCount
                        << "  ret:" << ret;
            LOG_ERROR << "BnApi::uriQryPosiCallbackOnHttp instId [" << qryInfo.posiNode_.symbol << "]" << "failedCount:" << failedCount
                        << "  ret:" << ret;
        }
        return;
    }
    auto it = originSymbolToSpotSymbol_.find(qryInfo.posiNode_.symbol);
    if (it == originSymbolToSpotSymbol_.end()) {
        LOG_ERROR << "BnApi::uriQryPosiCallbackOnHttp decode symbol failed res: " << result;
        return;
    } 
    memcpy(qryInfo.posiNode_.symbol, (it->second).c_str(), min(InstrumentIDLen, (uint16_t)((it->second).size())));
    gIsPosiQryInfoSuccess[qryInfo.posiNode_.symbol] = true;
    gQryPosiInfo[qryInfo.posiNode_.symbol] = qryInfo.posiNode_;
    LOG_DEBUG << "BnApi uriQryPosiCallbackOnHttp: " << result << "gQryPosiInfo size: " << gQryPosiInfo.size()
        << ", gIsPosiQryInfoSuccess: " << gIsPosiQryInfoSuccess[qryInfo.posiNode_.symbol];
    posiQueryFailedCountMap[qryInfo.posiNode_.symbol] = 0;
    return;
}

void BnApi::uriQryOrderOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpQryOrderCallback(message, clientId);
}

int BnApi::QryAsynOrder(const Order& order) {
    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
    uri->method = METHOD_GET;
    uri->domain = Bn_DOMAIN;
    uri->exchangeCode = Exchange_BINANCE;

    string symbol;
    if (strcmp(order.Category, LINEAR.c_str()) == 0) {
        uri->api = BN_UM_ORDER_API;
        symbol = GetUMCurrencyPair((order.InstrumentID));
    } else if (strcmp(order.Category, INVERSE.c_str()) == 0) {
        uri->api = BN_CM_ORDER_API;
        symbol = GetCMCurrencyPair((order.InstrumentID));
    } else {
        // ReqOrderInsert_spot(order);
        // ReqOrderInsert_cm_spot(order);
        LOG_FATAL << "BnApi ReqOrderInsert: " << order.OrderRef;
    }

    uint64_t time = CURR_MSTIME_POINT;
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    uri->AddParam(("symbol"), symbol);
    uri->AddParam(("origClientOrderId"), std::to_string(order.OrderRef));
    uri->AddParam(("timestamp"), std::to_string(time));
    uri->setUriClientOrdId(order.OrderRef);

    SetPrivateParams(HTTP_GET, *(uri.get()));
    
    uri->setResponseCallback(std::bind(&BnApi::uriQryOrderOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    LOG_INFO << "BnApi QryAsynOrder: " << ", url: " << uri->GetUrl() 
                << ", param: " << uri->GetParamSet();
    
    return 0;
}

void BnApi::QryAsynInfo() {
    for (auto it : tickerToInstrumentMap_) {
        // position qry
        std::shared_ptr<Uri> uri = make_shared<Uri>();
        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bn_DOMAIN;
        uri->exchangeCode = Exchange_BINANCE;
        //uri->method = METHOD_GET;
        uint64_t EpochTime = CURR_MSTIME_POINT;
        uri->AddParam(("timestamp"), std::to_string(EpochTime));
        uri->AddParam(("recvWindow"), ("3000"));

        string symbol;
        if (it.second.find("swap") != it.second.npos) {
            uri->api = BN_UM_POSITION_RISK_API;
            symbol = GetUMCurrencyPair((it.second));
        } else if (it.second.find("perp") != it.second.npos) {
            uri->api = BN_CM_POSITION_RISK_API;
            symbol = GetCMCurrencyPair((it.second));
        }
        
        transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
        uri->AddParam(("symbol"), (symbol));
        SetPrivateParams(HTTP_GET, *(uri.get()));
        uri->setResponseCallback(std::bind(&BnApi::uriQryPosiCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);

        // markprice qry
        std::shared_ptr<Uri> mark_uri = make_shared<Uri>();
        mark_uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        mark_uri->domain = Bn_DOMAIN;
        mark_uri->exchangeCode = Exchange_BINANCE;
        mark_uri->AddParam(("symbol"), (symbol));
        mark_uri->api = BN_MARKPRICE_FUNDRATE_API;
        mark_uri->setResponseCallback(std::bind(&BnApi::uriQryMarkPCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(mark_uri);
    }
}

int BnApi::QryPosiBySymbol(const Order &order) {
    m_uri.clear();
    m_uri.domain = Bn_DOMAIN;
    uint64_t EpochTime = CURR_MSTIME_POINT;
    // m_uri.AddParam(UrlEncode("recvWindow"), UrlEncode("3000"));

    string symbol;
    if (strcmp(order.Category, LINEAR.c_str()) == 0) {
        m_uri.api = BN_UM_POSITION_RISK_API;
        symbol = GetUMCurrencyPair((order.InstrumentID));
    } else if (strcmp(order.Category, INVERSE.c_str()) == 0) {
        m_uri.api = BN_CM_POSITION_RISK_API;
        symbol = GetCMCurrencyPair((order.InstrumentID));
    } else {
        LOG_FATAL << "BnApi QryAsynInfo: " << order.InstrumentID << ", symbol: " << symbol
            << ", Category: " << order.Category;
    }
    transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    m_uri.AddParam(("symbol"), (symbol));
    m_uri.AddParam(("timestamp"), std::to_string(EpochTime));

    string sig_url = CreateSignature(HTTP_GET, m_secret_key, m_uri, "sha256");
    m_uri.method = METHOD_GET;
    string header = "X-MBX-APIKEY: " + m_api_key;
    m_uri.AddHeader(header.c_str());

    m_uri.api = m_uri.api +"?symbol="+symbol+"&timestamp="+std::to_string(EpochTime)+"&signature="+sig_url;
    LOG_INFO << "BnApi QryPosiBySymbol: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
        << ", param: " << m_uri.GetParamSet();
    m_uri.ClearParam();
    m_uri.Request();
    
    string &res = m_uri.result;
    if (res.empty()) {
        LOG_FATAL << "BnApi::QryPosiBySymbol decode failed res: " << res;
        return -1;
    }
    BianQryPositionInfo qryInfo;
    int ret = qryInfo.decode(res.c_str());

    LOG_INFO << "BnApi QryPosiBySymbol rst: " << res;
    if (ret == 2) return 0;

    if (ret != 0) {
        LOG_FATAL << "BnApi::QryPosiBySymbol decode failed res: " << res;
        return -1;
    }

    if (IS_DOUBLE_LESS(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "BOTH") == 0)) {
        memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
        string str("SELL");
        memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
    } else if (IS_DOUBLE_GREATER_EQUAL(qryInfo.posiNode_.size, 0) && (strcmp(qryInfo.posiNode_.side, "BOTH") == 0)) {
        memset(qryInfo.posiNode_.side, 0, sizeof(qryInfo.posiNode_.side));
        string str("BUY");
        memcpy(qryInfo.posiNode_.side, str.c_str(), min(uint16_t(str.size()), DateLen));
    } else {
        LOG_FATAL << "BnApi QryPosiBySymbol ERROR: " << qryInfo.posiNode_.side<< ", size: " << qryInfo.posiNode_.size;
    }

    qryInfo.posiNode_.size = abs(qryInfo.posiNode_.size);
    memcpy(qryInfo.posiNode_.symbol, order.InstrumentID, InstrumentIDLen);
    
    gIsPosiQryInfoSuccess[order.InstrumentID] = true;
    gQryPosiInfo[order.InstrumentID] = qryInfo.posiNode_;
    return 0;

}

void BnApi::ConvertQuantity(const Order &order, Uri& m_uri) {
    if(strcmp(order.InstrumentID, "btc_usdt_binance_swap") == 0) {
        char buf2[64] = {0};
        SNPRINTF(buf2, sizeof(buf2), "%.5f", order.VolumeTotalOriginal);
        m_uri.AddParam(("quantity"), (buf2));
    } else if(strcmp(order.InstrumentID, "btc_usd_binance_perp") == 0) {
        char buf2[64] = {0};
        SNPRINTF(buf2, sizeof(buf2), "%d", int(order.VolumeTotalOriginal));
        m_uri.AddParam(("quantity"), (buf2));    
    } else if(strcmp(order.InstrumentID, "eth_usdt_binance_swap") == 0) {
        char buf2[64] = {0};
        SNPRINTF(buf2, sizeof(buf2), "%.4f", order.VolumeTotalOriginal);
        m_uri.AddParam(("quantity"), (buf2));
    } else if(strcmp(order.InstrumentID, "eth_usd_binance_perp") == 0) {
        char buf2[64] = {0};
        SNPRINTF(buf2, sizeof(buf2), "%d", int(order.VolumeTotalOriginal));
        m_uri.AddParam(("quantity"), (buf2));
    } else {
        LOG_FATAL << "BnApi::ConvertQuantity error " << order.InstrumentID;
    }
}

void BnApi::ConvertPrice(const Order &order, Uri& m_uri) {
    if(strcmp(order.InstrumentID, "btc_usdt_binance_swap") == 0
        || strcmp(order.InstrumentID, "btc_usd_binance_perp") == 0) {
        if (order.Direction == INNER_DIRECTION_Buy) {
            double price = order.LimitPrice * 10;
            price = std::floor(price);
            price = price/10;
            char buf[64] = {0};
            SNPRINTF(buf, sizeof(buf), "%.1f", price);
            m_uri.AddParam(("price"), (buf));
        } else if (order.Direction == INNER_DIRECTION_Sell) {
            double price = order.LimitPrice * 10;
            price = std::ceil(price);
            price = price/10;
            char buf[64] = {0};
            SNPRINTF(buf, sizeof(buf), "%.1f", price);
            m_uri.AddParam(("price"), (buf));
        } else {
            LOG_FATAL << "Order direction wr";
        }

    } else if(strcmp(order.InstrumentID, "eth_usdt_binance_swap") == 0
        || strcmp(order.InstrumentID, "eth_usd_binance_perp") == 0) {

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

    } else {
        LOG_FATAL << "BnApi::ConvertPrice error " << order.InstrumentID;
    }
}

void BnApi::uriReqCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynReqCallback(message, clientId);
}

void BnApi::uriCanCallbackOnHttp(char* message, uint64_t clientId)
{
	adapter_->httpAsynCancelCallback(message, clientId);
}

uint64_t BnApi::ReqOrderInsert_swap(const Order& order) { // u swap
    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;

    uri->domain = Bn_DOMAIN;
    uri->api = BN_UM_ORDER_API;
    uri->exchangeCode = Exchange_BINANCE;

    string cp = GetUMCurrencyPair(order.InstrumentID);
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

    uri->setResponseCallback(std::bind(&BnApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    return 0;
}

uint64_t BnApi::ReqOrderInsert_lever(const Order& order) {
    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;

    uri->domain = Bn_DOMAIN;
    uri->api = BN_LEVERAGE_ORDER_API;
    uri->exchangeCode = Exchange_BINANCE;

    string cp = GetUMCurrencyPair(order.InstrumentID);
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

    uri->setResponseCallback(std::bind(&BnApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    return 0;
}

uint64_t BnApi::ReqOrderInsert_perp(const Order& order) {

    std::shared_ptr<Uri> uri = make_shared<Uri>();
    uri->protocol = HTTP_PROTOCOL_HTTPS;
    uri->urlprotocolstr = URL_PROTOCOL_HTTPS;

    uri->domain = Bn_DOMAIN;
    uri->api = BN_CM_ORDER_API;
    uri->exchangeCode = Exchange_BINANCE;

    string cp = GetCMCurrencyPair(order.InstrumentID);
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
    m_uri.AddParam(("quantity"), std::to_string(order.VolumeTotalOriginal));
    uint64_t EpochTime = CURR_MSTIME_POINT;

    uri->AddParam(("timestamp"), (std::to_string(EpochTime)));

    SetPrivateParams(HTTP_POST, *(uri.get()), ENCODING_GZIP);

    uri->setResponseCallback(std::bind(&BnApi::uriReqCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
    CurlMultiCurl::instance().addUri(uri);

    return 0;
}

/**
 * @param order
 * @return
 */
uint64_t BnApi::ReqOrderInsert(const Order& order) {
    int ret;
    if (strcmp(order.Category, LINEAR.c_str()) == 0) {
        ret = ReqOrderInsert_swap(order);
    } else if (strcmp(order.Category, INVERSE.c_str()) == 0) {
        ret = ReqOrderInsert_perp(order);
    }  else if (strcmp(order.Category, LEVERAGE.c_str()) == 0) {
        ret = ReqOrderInsert_lever(order);
    } else {
        // ReqOrderInsert_spot(order);
        // ReqOrderInsert_cm_spot(order);
        LOG_FATAL << "BnApi ReqOrderInsert: " << order.OrderRef;
    }
    return ret;
}

int BnApi::CancelOrder(const Order& order, int type) {
    // try {
        std::shared_ptr<Uri> uri = make_shared<Uri>();

        uri->protocol = HTTP_PROTOCOL_HTTPS;
        uri->urlprotocolstr = URL_PROTOCOL_HTTPS;
        uri->domain = Bn_DOMAIN;
        uri->exchangeCode = Exchange_BINANCE;
      //  uri->isinit = true;

        string cp;
        if (strcmp(order.Category, LINEAR.c_str()) == 0) {
            cp = GetUMCurrencyPair(order.InstrumentID);
            uri->api = BN_UM_ORDER_API;
        } else if (strcmp(order.Category, INVERSE.c_str()) == 0) {
            cp = GetCMCurrencyPair(order.InstrumentID);
            uri->api = BN_CM_ORDER_API;
        } else {
            // ReqOrderInsert_spot(order);
            // ReqOrderInsert_cm_spot(order);
            LOG_FATAL<<"GetCMCurrencyPair FATAL: " << order.InstrumentID;
        }

        transform(cp.begin(), cp.end(), cp.begin(), ::toupper);

        uri->setUriClientOrdId(order.OrderRef);
        uri->AddParam(("symbol"), (cp));

        uri->AddParam(("origClientOrderId"), std::to_string(order.OrderRef));
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

        uri->setResponseCallback(std::bind(&BnApi::uriCanCallbackOnHttp,this,std::placeholders::_1,std::placeholders::_2));
        CurlMultiCurl::instance().addUri(uri);
        return 0;
    // }
    // catch (std::exception ex) {
    //     LOG_WARN << "BnApi::CancelOrder exception occur:" << ex.what();
    //     return -1;
    // }
}

int BnApi::CancelAllOrders() {
    if (cancelAll) return 1;
    try {
        for (auto it : originSymbolToSpotSymbol_) {
            string symbol = it.second;
            m_uri.clear();
            m_uri.domain = Bn_DOMAIN;
            m_uri.api = BN_UM_CANCEL_ALL_ORDER_API;

            string cp;
            if (symbol.find("swap") != symbol.npos) {
                //btc_usdt_b_swap --> btcusdt
                m_uri.api = BN_UM_CANCEL_ALL_ORDER_API;
                cp = GetUMCurrencyPair(symbol);
            } else if (symbol.find("perp") != symbol.npos) {
                //btc_usdt_b_perp --> btcusdt_perp
                m_uri.api = BN_CM_CANCEL_ALL_ORDER_API;
                cp = GetCMCurrencyPair(symbol);
            } else {
                // ReqOrderInsert_spot(order);
                // ReqOrderInsert_cm_spot(order);
                LOG_FATAL<<"GetCMCurrencyPair FATAL: " << symbol;
            }

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
                LOG_FATAL << "BnApi CancelAllOrder fail result:" << rstCode << ", res: " << res;
                return -1;
            }
            if (res.empty()) {
                LOG_FATAL << "BnApi::CanCancelAllOrdercelOrder return empty. symbol:" << symbol;
                return -1;
            }
            LOG_INFO << "BnApi CancelAllOrders: " << m_uri.retcode << ", method: " << m_uri.method << ", url: " << m_uri.GetUrl() 
                << ", param: " << m_uri.GetParamSet() << ", res: " << res;

        }
        cancelAll = true;
        return 0;
    }
    catch (std::exception ex) {
        LOG_WARN << "BnApi::CancelOrder exception occur:" << ex.what();
        return -1;
    }
}

void BnApi::AddInstrument(const char *instrument) {
    string inst(instrument);
    string channelDepth;
    string channelTrade;
    string channelTicker;

    if (inst.find("swap") != inst.npos || inst.find("perp") != inst.npos) {
        channelDepth = GetDepthChannel(instrument);//btcusdt@depth5
        depthToInstrumentMap_[channelDepth] = instrument;

        channelTrade = GetTradeChannel(instrument);
        tradeToInstrumentMap_[channelTrade] = instrument;

        channelTicker = GetTickerChannel(instrument);
        tickerToInstrumentMap_[channelTicker] = instrument;
    } else {
        channelDepth = GetDepthChannel(instrument);//btcusdt@depth5
        spot_depthToInstrumentMap_[channelDepth] = instrument;

        channelTrade = GetTradeChannel(instrument);
        spot_tradeToInstrumentMap_[channelTrade] = instrument;

        channelTicker = GetTickerChannel(instrument);
        spot_tickerToInstrumentMap_[channelTicker] = instrument;
    }


    string cp;
    if (inst.find("swap") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
    } else if (inst.find("perp") != inst.npos) {
        //btc_usdt_b_perp --> btcusdt_perp
        cp = GetCMCurrencyPair(inst);
    } else if (inst.find("spot") != inst.npos) {
        //btc_usdt_b_perp --> btcusdt_perp
        cp = GetUMCurrencyPair(inst);
    }
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

    LOG_INFO << "BnApi::AddInstrument channelDepth: " << channelDepth << ", channelTrade: " << channelTrade
             << ", channelTicker: " << channelTicker << ", cp: " << cp << ", instrument: " << instrument;
}

string BnApi::GetDepthChannel(string inst) {
    string cp;
    if (inst.find("swap") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@depth5";
    } else if (inst.find("perp") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetCMCurrencyPair(inst);
        //convert to channel
        cp += "@depth5";
    } else if (inst.find("spot") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@depth5";
    }
    return cp;  //btcusdt@depth5
}

string BnApi::GetTickerChannel(string inst) {
    //btc_usdt_b --> btcusdt
    string cp;
    if (inst.find("swap") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@bookTicker";
    } else if (inst.find("perp") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetCMCurrencyPair(inst);
        //convert to channel
        cp += "@bookTicker";
    } else if (inst.find("spot") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@bookTicker";
    }
    return cp;  //btcusdt@bookTicker
}

string BnApi::GetTradeChannel(string inst) {
    string cp;
    if (inst.find("swap") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@trade";
    } else if (inst.find("perp") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetCMCurrencyPair(inst);
        //convert to channel
        cp += "@trade";
    } else if (inst.find("spot") != inst.npos) {
        //btc_usdt_b_swap --> btcusdt
        cp = GetUMCurrencyPair(inst);
        //convert to channel
        cp += "@trade";
    }
    return cp;
}

string BnApi::GetSpotDepthInstrumentID(string channel) {
    auto iter = spot_depthToInstrumentMap_.find(channel);
    if (iter != spot_depthToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BnApi::GetSpotTickerInstrumentID(string channel) {
    auto iter = spot_tickerToInstrumentMap_.find(channel);
    if (iter != spot_tickerToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BnApi::GetSpotTradeInstrumentID(string channel) {
    auto iter = spot_tradeToInstrumentMap_.find(channel);
    if (iter != spot_tradeToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BnApi::GetDepthInstrumentID(string channel) {
    auto iter = depthToInstrumentMap_.find(channel);
    if (iter != depthToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BnApi::GetTickerInstrumentID(string channel) {
    auto iter = tickerToInstrumentMap_.find(channel);
    if (iter != tickerToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

string BnApi::GetTradeInstrumentID(string channel) {
    auto iter = tradeToInstrumentMap_.find(channel);
    if (iter != tradeToInstrumentMap_.end()) {
        return iter->second;
    }
    return "";
}

std::map<string, string>* BnApi::GetSpotDepthInstrumentMap() {
    return &spot_depthToInstrumentMap_;
}

std::map<string, string>* BnApi::GetSpotTickerInstrumentMap() {
    return &spot_tickerToInstrumentMap_;
}

std::map<string, string>* BnApi::GetSpotTradeInstrumentMap() {
    return &spot_tradeToInstrumentMap_;
}

std::map<string, string>* BnApi::GetDepthInstrumentMap() {
    return &depthToInstrumentMap_;
}

std::map<string, string>* BnApi::GetTickerInstrumentMap() {
    return &tickerToInstrumentMap_;
}

std::map<string, string>* BnApi::GetTradeInstrumentMap() {
    return &tradeToInstrumentMap_;
}

string BnApi::GetUMCurrencyPair(string inst) {
    //btc_usdt_binance_swap --> btcusdt
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    return cp;
}

string BnApi::GetCMCurrencyPair(string inst) {
    //btc_usd_cm_h --> btcusdt
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    cp = cp  + "_perp";
    return cp;
}

uint64_t BnApi::GetCurrentNonce() {
    return m_nonce;
}