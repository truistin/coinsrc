#include <spot/gateway/OKxV5MdSpi.h>
#include <spot/utility/Logging.h>
#include "spot/utility/TradingDay.h"
#include "spot/base/Instrument.h"
#include "spot/okex/OkSwapApi.h"
#include <spot/utility/MeasureFunc.h>
#include <spot/shmmd/ShmMdServer.h>
#include <spot/distributor/DistServer.h>

#ifdef _LINUX_
#include <pthread.h>
#endif

using namespace spotrapidjson;
using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spot::shmmd;
using namespace spot::distributor;

//�����ַ��wss://ws.okx.com:8443/ws/v5/public
//post��params: {"op":"subscribe","args":[{"channel":"tickers","instId":"BTC-USDT-SWAP"}]}

void OKxV5MdSpi::com_callbak_message(const char *message) {
    //-//std::cout << message << std::endl;
    // MeasureFunc f(4);
    spotrapidjson::Document jsonDoc;
    //jsonDoc.Parse<0>(message);
    jsonDoc.Parse(message, strlen(message));
    if (jsonDoc.HasParseError()) {
        if (strcmp(message, "pong") == 0) {
         //   LOG_INFO << "recv heartbeat: " << message;
            return;
        }
        LOG_WARN << "OKxV5MdSpi::Parse error. result:" << message;
        return;
    }

    if (jsonDoc.HasMember("event")) {
        spotrapidjson::Value &eventNode = jsonDoc["event"];
        string recString = eventNode.GetString();
        LOG_DEBUG << "OKxV5MdSpi event: " << recString << " received";

        if (recString == "error") {
            onErrResponse(jsonDoc);
        }
    } else if (jsonDoc.HasMember("arg")) {
        // {"arg":{"channel":"trades","instId":"BTC-USD-SWAP"},
        spotrapidjson::Value &argNode = jsonDoc["arg"];
        if (argNode.HasMember("channel") && argNode.HasMember("instId")) {
            string channel_str = argNode["channel"].GetString();
            string instId_str = argNode["instId"].GetString();
            if (channel_str == "trades") {
                onTrade(channel_str, instId_str, jsonDoc);
            } else if (channel_str == "books5" || channel_str == "bbo-tbt") {
                onDepthData(channel_str, instId_str, jsonDoc);
            } else if (channel_str == "tickers") {
                onTickerData(channel_str, instId_str, jsonDoc);
            } else {
                LOG_ERROR << "OKxV5MdSpi::channel name: " << channel_str << " received";
            }
        }
    }
}

// {"op":"subscribe","args":{"channel":"books","instId":"BTC-USDT-SWAP"}};
void OKxV5MdSpi::com_callbak_open() {
    for (auto it : instVec_) {
        LOG_INFO << "OKEx Connected.." << ", instVec_ size: " << instVec_.size() << ", it: " << it;
    }

    if (comapi_ != 0) {
        isOKEXMDConnected_ = true;
    } else {
        LOG_FATAL << "com_callbak_open fail";
    }
    // std::vector<const char*>::iterator it;
    for (auto it = instVec_.begin(); it != instVec_.end(); it++) {
        const char* ch = *it;
        LOG_INFO << "OKEx instid " << ch;
        subscribe(okex_V5_swap_depth1_channel_name, ch);

    //   subscribe(okex_V5_swap_depth5_channel_name, ch);
    //  subscribe(okex_V5_swap_trade_channel_name, ch);
    //  subscribe(okex_V5_swap_ticker_channel_name, ch);
    }
}

void OKxV5MdSpi::com_callbak_close() {
    LOG_ERROR << "OKEx Callback Closed......";
    isOKEXMDConnected_ = false;
}

OKxV5MdSpi::OKxV5MdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queueStore) {
    apiInfo_ = apiInfo;
    queueStore_ = queueStore;
    count_ = 1;
    LOG_INFO << "OKxV5MdSpi levelType:" << apiInfo_->mdLevelType_;
}

OKxV5MdSpi::~OKxV5MdSpi() {
}

void OKxV5MdSpi::init() {
    isOKEXMDConnected_ = false;
    // MeasureFunc::addMeasureData(4, "OKxV5MdSpi::dealOkMdData", 100000);
    std::string apiKey = apiInfo_->investorId_;
    std::string secret_key = apiInfo_->passwd_;
    std::string url = apiInfo_->frontMdAddr_;
    LOG_INFO << "apiInfo_->frontMdAddr_: " << apiInfo_->frontMdAddr_;
    cout << "apiInfo_->frontMdAddr_: " << apiInfo_->frontMdAddr_;
    lastTradeTime_ = 0;
    lastTickTime_ = 0;
    lastDepthTime_ = 0;
    comapi_ = new WebSocketApi();
	comapi_->SetUri(apiInfo_->frontMdAddr_);

    // comapi_->SetOkCompress(true);
    comapi_->SetCallBackOpen(std::bind(&OKxV5MdSpi::com_callbak_open, this));
    comapi_->SetCallBackMessage(std::bind(&OKxV5MdSpi::com_callbak_message, this, std::placeholders::_1));
    comapi_->SetCallBackClose(std::bind(&OKxV5MdSpi::com_callbak_close, this));

    std::thread OKxV5MdSpiSocket(std::bind(&WebSocketApi::Run, comapi_));
    pthread_setname_np(OKxV5MdSpiSocket.native_handle(), "OKxV5MdSocket");
    OKxV5MdSpiSocket.detach();

    std::thread OKxV5MdRun(&OKxV5MdSpi::run, this);
    pthread_setname_np(OKxV5MdRun.native_handle(), "OKxV5MdRun");
    OKxV5MdRun.detach();
}

void OKxV5MdSpi::run() {
    while (true) {
        // SLEEP(20000);
        std::this_thread::sleep_for(std::chrono::milliseconds(20000));
        if (isOKEXMDConnected_) {
        	string heartbeat = "ping";
	        comapi_->send(heartbeat);
        }
    }
}

void OKxV5MdSpi::AddSubInst(const char *inst) {
    OkSwapApi::AddInstrument(inst);
    instVec_.push_back(inst);
}

// channel = spot/depth5
// instrID = BTC-USD-SWAP
//�����ַ��wss://ws.okx.com:8443/ws/v5/public
//post��params: {"op":"subscribe","args":[{"channel":"tickers","instId":"BTC-USDT-SWAP"}]}
void OKxV5MdSpi::subscribe(const std::string &channel, const char* instrID) {

    string str =
        "{ \"op\": \"subscribe\", \"args\" : [{\"channel\":\"" + channel + "\",\"instId\":\"" + instrID + "\"}] }";

    LOG_INFO << "OKEx_V5 Subscribe channel : " << channel
             << ", instrID: " << instrID << ", str: " << str;
    comapi_->send(str);


    if (channelInstrumentMap_.find(channel) != channelInstrumentMap_.end()) {
        channelInstrumentMap_[channel].push_back(instrID);
    } else {
        std::vector<const char* > vec;
        vec.push_back(instrID);
        channelInstrumentMap_[channel] = vec;
    }

} 

void OKxV5MdSpi::onErrResponse(spotrapidjson::Document &jsonDoc) {
    int errCode = 0;
    string errMsg;
    if (jsonDoc.HasMember("errorCode")) {
        spotrapidjson::Value &node = jsonDoc["errorCode"];
        errCode = node.GetInt();
    }
    if (jsonDoc.HasMember("message")) {
        spotrapidjson::Value &node = jsonDoc["message"];
        errMsg = node.GetString();
    }
    LOG_ERROR << "OKxV5MdSpi received err response. errCode: " << errCode << " errMsg: " << errMsg;
}

void OKxV5MdSpi::onTrade(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc)
{
    if (string::npos != channel.find(okex_V5_swap_trade_channel_name)) {
        spotrapidjson::Value &dataNodes = jsonDoc["data"];
        if (dataNodes.IsArray()) {
            for (int i = 0; i < dataNodes.Capacity(); ++i) {
                memset(&trade_, 0, sizeof(trade_));
                spotrapidjson::Value &dataNode = dataNodes[i];

                spotrapidjson::Value &time = dataNode["ts"];
                uint64_t EpochTime = atoll(time.GetString());                
                if (lastTradeTime_ > EpochTime) continue;;
                lastTradeTime_ = EpochTime;
                trade_.EpochTime = EpochTime;

                trade_.MessageID = getMessageID();
                verfiyChannelInstrument(channel, instId);

                unsigned int len = strlen(instId.c_str());
                memcpy(trade_.InstrumentID, instId.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));

                spotrapidjson::Value &price = dataNode["px"];
                spotrapidjson::Value &type = dataNode["side"];
                spotrapidjson::Value &amount = dataNode["sz"];
                spotrapidjson::Value &tradeID = dataNode["tradeId"];

                trade_.UpdateMillisec = CURR_MSTIME_POINT;

                string dayTime = getCurrSystemDate2();
                dayTime += " ";
                dayTime += getCurrentSystemTime();
                memcpy(trade_.TradingDay, dayTime.c_str(), min(sizeof(trade_.TradingDay) - 1, dayTime.size()));

                memcpy(trade_.ExchangeID, Exchange_OKEX.c_str(), min(sizeof(trade_.ExchangeID) - 1, Exchange_OKEX.size()));
                
                string tid = tradeID.GetString();
				memcpy(trade_.Tid, tid.c_str(), min(sizeof(trade_.Tid) - 1, tid.size()));
                
                trade_.Price = atof(price.GetString());
                trade_.Volume = atof(amount.GetString());
                trade_.Turnover = trade_.Price * trade_.Volume;

                if (0 == strcmp(type.GetString(), "buy")) {
                    trade_.Direction = INNER_DIRECTION_Buy;
                } else if (0 == strcmp(type.GetString(), "sell")) {
                    trade_.Direction = INNER_DIRECTION_Sell;
                }

                queueStore_->storeHandle(&trade_);
            }
        }
    } 
}

void OKxV5MdSpi::onTickerData(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc)
{
    spotrapidjson::Value &dataNodes = jsonDoc["data"];

    if (dataNodes.IsArray()) {
        for (int i = 0; i < dataNodes.Capacity(); ++i) {
            memset(&field_, 0, sizeof(field_));
            spotrapidjson::Value &dataNode = dataNodes[i];

            spotrapidjson::Value &time = dataNode["ts"];
            
            uint64_t nowEpochTime = atoll(time.GetString());
            if (lastTickTime_ > nowEpochTime) return;
            lastTickTime_ = nowEpochTime;
            field_.EpochTime = nowEpochTime;
            field_.MessageID = getMessageID();
        //    spotrapidjson::Value& tradeTime = dataNode["T"];
            field_.LevelType = MDLEVELTYPE_TICKER;

            memcpy(field_.ExchangeID, Exchange_OKEX.c_str(), min(sizeof(field_.ExchangeID) - 1, Exchange_OKEX.size()));

            verfiyChannelInstrument(channel, instId);

            unsigned int len = strlen(instId.c_str());
            memcpy(field_.InstrumentID, instId.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));
            
            double bid; double ask; double bidVol; double askVol; double lastPrice;
            double lastVolume;

            spotrapidjson::Value &bid1 = dataNode["bidPx"];
            bid = atof(bid1.GetString());
            field_.setBidPrice(1, bid);

            spotrapidjson::Value &ask1 = dataNode["askPx"];
            ask = atof(ask1.GetString());
            field_.setAskPrice(1, ask);

            spotrapidjson::Value &bid1Volume = dataNode["bidSz"];
            bidVol = atof(bid1Volume.GetString());
            field_.setBidVolume(1, bidVol);

            spotrapidjson::Value &ask1Volume = dataNode["askSz"];
            askVol = atof(ask1Volume.GetString());
            field_.setAskVolume(1, askVol);

            spotrapidjson::Value &dealLastPrice = dataNode["last"];
            lastPrice = atof(dealLastPrice.GetString());
            field_.LastPrice = lastPrice;

            spotrapidjson::Value &dealLastVol = dataNode["lastSz"];
            lastVolume = atof(dealLastVol.GetString());
            field_.Volume = lastVolume;

            if (IS_DOUBLE_ZERO(bid) || IS_DOUBLE_ZERO(ask) ||
                IS_DOUBLE_ZERO(bidVol) || IS_DOUBLE_ZERO(askVol) || IS_DOUBLE_ZERO(lastPrice)){
                LOG_WARN << "OKxV5MdSpi::onTickerData price || volume invaild 0 bid: " << 
                    bid << ", ask: " << ask << ", bidVol: " << bidVol << ", askVol: " << askVol
                    << ", lastPrice: " << lastPrice;
		        return;
	        }

            field_.UpdateMillisec = CURR_MSTIME_POINT;

            string dayTime = getCurrSystemDate2();
            dayTime += " ";
            dayTime += getCurrentSystemTime();
            memcpy(field_.TradingDay, dayTime.c_str(), min(sizeof(field_.TradingDay) - 1, dayTime.size()));
            onData(&field_);
        }
    }
    
    return;
}

// to do time * 1000000
void OKxV5MdSpi::onDepthData(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc)
{
    uint64_t cnt = 2 * count_ + 1;
	count_ = count_ + 1;
    spotrapidjson::Value &dataNodes = jsonDoc["data"];
    if (dataNodes.IsArray()) {
        for (int i = 0; i < dataNodes.Capacity(); ++i) {
            memset(&field_, 0, sizeof(field_));
            spotrapidjson::Value &dataNode = dataNodes[i];
            spotrapidjson::Value &time = dataNode["ts"];
            
            uint64_t nowEpochTime = atoll(time.GetString());
            if (lastDepthTime_ > nowEpochTime) return;
            lastDepthTime_ = nowEpochTime;
            field_.EpochTime = nowEpochTime;
            
            field_.MessageID = getMessageID();
            field_.LevelType = MDLEVELTYPE_DEFAULT;
            memcpy(field_.ExchangeID, Exchange_OKEX.c_str(), min(sizeof(field_.ExchangeID) - 1, Exchange_OKEX.size()));

            verfiyChannelInstrument(channel, instId);

            unsigned int len = strlen(instId.c_str());
            memcpy(field_.InstrumentID, instId.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));

            // spotrapidjson::Value &timestampNode = dataNode["ts"];
            // field_.EpochTime = atoll(timestampNode.GetString());  //2019-01-08T11:42:41.801Z



            field_.UpdateMillisec = CURR_MSTIME_POINT;

            string dayTime = getCurrSystemDate2();
            dayTime += " ";
            dayTime += getCurrentSystemTime();
            memcpy(field_.TradingDay, dayTime.c_str(), min(sizeof(field_.TradingDay) - 1, dayTime.size()));
 
            spotrapidjson::Value &asks = dataNode["asks"];
            spotrapidjson::Value &bids = dataNode["bids"];

            if (channel == okex_V5_swap_depth1_channel_name || channel == okex_V5_swap_depth5_channel_name) {
                for (size_t i = 0; i < asks.Size(); ++i) {
                    spotrapidjson::Value &priceNode = asks[i][0];
                    spotrapidjson::Value &amountNode = asks[i][1];
                    double price = atof(priceNode.GetString());    //different type
                    double amount = atof(amountNode.GetString());

                    field_.setAskPrice(i + 1, price);
                    field_.setAskVolume(i + 1, amount);
                }

                for (size_t i = 0; i < bids.Size(); ++i) {
                    spotrapidjson::Value &priceNode = bids[i][0];
                    spotrapidjson::Value &amountNode = bids[i][1];
                    double price = atof(priceNode.GetString());   //different type
                    double amount = atof(amountNode.GetString());

                    field_.setBidPrice(i + 1, price);
                    field_.setBidVolume(i + 1, amount);
                }
                onData(&field_);
				if (cnt != (2 * (count_ - 1) + 1))	{
					LOG_FATAL << "thread wrong cnt: " << cnt << ", count_: " << count_;
				}
            }
        }
    }
}

bool OKxV5MdSpi::verfiyChannelInstrument(const std::string &channel, const std::string &instId)
{
    if (channelInstrumentMap_.find(channel) != channelInstrumentMap_.end()) {
        bool flag = false;
        std::map<string, std::vector<const char*> >::iterator it = channelInstrumentMap_.find(channel);
        for (auto iter : it->second) {
            if (strcmp(iter, instId.c_str()) == 0) {
                flag = true;
                return flag;
            }
        }
        if (!flag) {
            LOG_FATAL << "OKEx onTrade channel not found flag: " << flag << ", channel: " << channel << ", instId: " << instId;
            return false;
        }
    } else {
        LOG_FATAL << "OKEx onTrade channel not found: " << channel << ", instId: " << instId;
        return false;
    }
    return true;
}

void OKxV5MdSpi::onData(InnerMarketData *innerMktData)
{
	if (ShmMdServer::instance().isInit())
	{
		ShmMdServer::instance().update(innerMktData);
	}
	if (DistServer::instance().isInit())
	{
		DistServer::instance().update(innerMktData);
	}
	if (!ShmMdServer::instance().isInit() && !DistServer::instance().isInit())
	{
		queueStore_->storeHandle(innerMktData);
	}
}