#include <spot/gateway/BybitMdSpi.h>
#include <spot/utility/Logging.h>
#include "spot/utility/TradingDay.h"
#include "spot/base/Instrument.h"
#include <spot/utility/MeasureFunc.h>
#include <spot/shmmd/ShmMdServer.h>
#include <spot/shmmd/ShmMtServer.h>
#include <spot/distributor/DistServer.h>
#include "spot/bybit/BybitApi.h"

#ifdef _LINUX_
#include <pthread.h>
#endif

using namespace spotrapidjson;
using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spot::shmmd;
using namespace spot::distributor;

//wss://wss://stream.bybit.com/v5/private
/*postparams: {
    "req_id": "test", // 可選
    "op": "subscribe",
    "args": [
        "tickers.1.BTCUSDT",
        "publicTrade.BTCUSDT",
        "orderbook.1.ETHUSDT"
    ]
}*/

void BybitMdSpi::com_callbak_message(const char *message) {
    LOG_DEBUG << "BybitMdSpi com_callbak_message: " << message;
    //-//std::cout << message << std::endl;
    // MeasureFunc f(4);
    spotrapidjson::Document jsonDoc;
    //jsonDoc.Parse<0>(message);
    jsonDoc.Parse(message, strlen(message));
    if (jsonDoc.HasParseError()) {
        if (strcmp(message, "pong") == 0) {
           LOG_INFO << "recv heartbeat: " << message;
            return;
        }
        LOG_WARN << "BybitMdSpi::Parse error. result:" << message;
        return;
    }

    if (jsonDoc.HasMember("event")) {
        spotrapidjson::Value &eventNode = jsonDoc["event"];
        string recString = eventNode.GetString();
        LOG_DEBUG << "BybitMdSpi event: " << recString << " received";

        if (recString == "error") {
            onErrResponse(jsonDoc);
        }
    } else if (jsonDoc.HasMember("topic") && jsonDoc.HasMember("type")) {
        // {"arg":{"channel":"trades","instId":"BTC-USD-SWAP"},
        //     "topic": "tickers.BTCUSDT", "type": "snapshot",
        string topic_str = jsonDoc["topic"].GetString();
        std::vector<std::string> instId_vec  = utility::split(topic_str, ".");
        if (instId_vec.size() < 2) {
            LOG_FATAL << "BybitMdSpi com_callbak_message instId_vec.size < 2";
        }
        string channel = instId_vec[0];
        string instId_str;
        string type_str = jsonDoc["type"].GetString();

        if (type_str == "snapshot") {
            if (channel == "publicTrade") {
                instId_str = instId_vec[1];
                onTrade(channel, instId_str, jsonDoc);
            } else if (channel == "orderbook") {
                instId_str = instId_vec[2];
                onDepthData(channel, instId_str, jsonDoc);
            } else if (channel == "tickers") {
                instId_str = instId_vec[1];
                onTickerData(channel, instId_str, jsonDoc);
                LOG_INFO << "snapshot tickers: " << message;
            } else {
                LOG_ERROR << "BybitMdSpi::snapshot name: " << message;
            }
        } else if (type_str == "delta"){
            // LOG_WARN << "BybitMdSpi delta: " << message;
            if (channel == "trades") {
            } else if (channel == "orderbook") {
                instId_str = instId_vec[2];
                onDepthData(channel, instId_str, jsonDoc);
            } else if (channel == "tickers") {
                instId_str = instId_vec[1];
                onTickerData(channel, instId_str, jsonDoc);
            } else {
                LOG_ERROR << "BybitMdSpi::delta name: " << message;
            }
        } else {
            LOG_WARN << "BybitMdSpi ERROR: " << message;
            cout << "BybitMdSpi ERROR: " << message << endl;
        }
    }
}

// {"op":"subscribe","args":{"channel":"books","instId":"BTC-USDT-SWAP"}};
void BybitMdSpi::com_callbak_open() {
    // LOG_INFO << "Bybit Connected.." << ", instVec_ size: " << instVec_.size() << ", instVec_[0]" << 
    //     instVec_[0] << ", instVec_[1]: "<< instVec_[1];

    if (comapi_ != 0) {
        isBybitMDConnected_ = true;
    } else {
        LOG_FATAL << "com_callbak_open fail";
    }
    // std::vector<const char*>::iterator it;
    for (auto it = instVec_.begin(); it != instVec_.end(); it++) {
        const char* ch = *it;
        LOG_INFO << "BybitMd instid: " << ch;
        subscribe(bybit_swap_ticker_channel_name, "" ,ch);

        // subscribe(bybit_swap_depth_channel_name, "1", ch);
        // subscribe(bybit_swap_trade_channel_name, "", ch);
    //  subscribe(bybit_swap_ticker_channel_name, ch);
    }
}

void BybitMdSpi::com_callbak_close() {
    LOG_ERROR << "Bybit Callback Closed......";
    isBybitMDConnected_ = false;
}

BybitMdSpi::BybitMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queueStore) {
    apiInfo_ = apiInfo;
    queueStore_ = queueStore;
    count_ = 1;
    LOG_INFO << "BybitMdSpi levelType:" << apiInfo_->frontMdAddr_;
}

BybitMdSpi::~BybitMdSpi() {
}

void BybitMdSpi::init() {
    isBybitMDConnected_ = false;
    // MeasureFunc::addMeasureData(4, "BybitMdSpi::dealOkMdData", 100000);
    std::string apiKey = apiInfo_->investorId_;
    std::string secret_key = apiInfo_->passwd_;
    std::string url = apiInfo_->frontMdAddr_;
    LOG_INFO << "apiInfo_->frontMdAddr_: " << apiInfo_->frontMdAddr_;

	lastTradeTime_ = 0;
	lastTickTime_ = 0;
	lastDepthTime_ = 0;

    comapi_ = new WebSocketApi();
	comapi_->SetUri(apiInfo_->frontMdAddr_);

    // comapi_->SetOkCompress(true);
    comapi_->SetCallBackOpen(std::bind(&BybitMdSpi::com_callbak_open, this));
    comapi_->SetCallBackMessage(std::bind(&BybitMdSpi::com_callbak_message, this, std::placeholders::_1));
    comapi_->SetCallBackClose(std::bind(&BybitMdSpi::com_callbak_close, this));

    std::thread BybitMdSpiSocket(std::bind(&WebSocketApi::Run, comapi_));
    pthread_setname_np(BybitMdSpiSocket.native_handle(), "BybitMdSocket");
    BybitMdSpiSocket.detach();

    std::thread BybitMdRun(&BybitMdSpi::run, this);
    pthread_setname_np(BybitMdRun.native_handle(), "BybitMdRun");
    BybitMdRun.detach();
}

void BybitMdSpi::run() {
    while (true) {
        // SLEEP(20000);
        std::this_thread::sleep_for(std::chrono::milliseconds(20000));
        if (isBybitMDConnected_) {
        	string heartbeat = "{ \"op\": \"ping\"}";
            LOG_DEBUG << "send ping";
	        comapi_->send(heartbeat);
        }
    }
}

void BybitMdSpi::AddSubInst(const char *inst) {
    BybitApi::AddInstrument(inst);
    instVec_.push_back(inst);
}

// channel = spot/depth5
// instrID = BTC-USD-SWAP
//wss://ws.okx.com:8443/ws/v5/public
/*postparams: {
    "req_id": "test", // 可選
    "op": "subscribe",
    "args": [
        "orderbook.1.BTCUSDT",
        "publicTrade.BTCUSDT",
        "orderbook.1.ETHUSDT"
    ]
}*/ //post的params: {"op":"subscribe","args":[{"channel":"tickers","instId":"BTC-USDT-SWAP"}]}
void BybitMdSpi::subscribe(const std::string &channel, string deep, const char* instrID)
{ // tickers.BTCUSDT
    string str;
    if (deep == "") {
        str =
            "{ \"op\": \"subscribe\", \"args\" : [\"" + channel + "." + instrID + "\"] }";      
    } else {
        str =
            "{ \"op\": \"subscribe\", \"args\" : [\"" + channel + "." + deep + "." + instrID + "\"] }";
    }

    LOG_INFO << "BybitMd Subscribe: " << ", channel: " << channel
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

void BybitMdSpi::onErrResponse(spotrapidjson::Document &jsonDoc) {
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
    LOG_ERROR << "BybitMdSpi received err response. errCode: " << errCode << " errMsg: " << errMsg;
}

void BybitMdSpi::onTrade(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc)
{
    if (string::npos != channel.find(bybit_swap_trade_channel_name)) {
        spotrapidjson::Value &dataNodes = jsonDoc["data"];
        if (dataNodes.IsArray()) {
                for (int i = 0; i < dataNodes.Capacity(); ++i) {
                InnerMarketTrade trade;
                memset(&trade, 0, sizeof(trade));
                spotrapidjson::Value &dataNode = dataNodes[i];
                spotrapidjson::Value &time = dataNode["T"];

                uint64_t EpochTime = time.GetUint64();
                if (lastTradeTime_ > EpochTime) continue;;
                trade.EpochTime = EpochTime;
                lastTradeTime_ = EpochTime;

                trade.MessageID = getMessageID();
                verfiyChannelInstrument(channel, instId);

                unsigned int len = strlen(instId.c_str());
                memcpy(trade.InstrumentID, instId.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));

                spotrapidjson::Value &price = dataNode["p"];
                spotrapidjson::Value &type = dataNode["S"];
                spotrapidjson::Value &amount = dataNode["v"];

                trade.UpdateMillisec = CURR_MSTIME_POINT;
                
                string dayTime = getCurrSystemDate2();
                dayTime += " ";
                dayTime += getCurrentSystemTime();
                memcpy(trade.TradingDay, dayTime.c_str(), min(sizeof(trade.TradingDay) - 1, dayTime.size()));

                memcpy(trade.ExchangeID, Exchange_BYBIT.c_str(), min(sizeof(trade.ExchangeID) - 1, Exchange_BYBIT.size()));
                
                // trade.Tid = atoll(tid.GetString());
                spotrapidjson::Value &tradeID = dataNode["i"];
                string tid = tradeID.GetString();
				memcpy(trade.Tid, tid.c_str(), min(sizeof(trade.Tid) - 1, tid.size()));

                trade.Price = atof(price.GetString());
                trade.Volume = atof(amount.GetString());
                trade.Turnover = trade.Price * trade.Volume;

                if (0 == strcmp(type.GetString(), "Buy")) {
                    trade.Direction = INNER_DIRECTION_Buy;
                } else if (0 == strcmp(type.GetString(), "Sell")) {
                    trade.Direction = INNER_DIRECTION_Sell;
                }

                onData(&trade);
            }
        }
    } 
}
/*        "bid1Price": "17215.50",
        "bid1Size": "84.489",
        "ask1Price": "17216.00",
        "ask1Size": "83.020"*/
void BybitMdSpi::onTickerData(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc) {
    uint64_t cnt = 2 * count_ + 1;
	count_ = count_ + 1;
    InnerMarketData field;
    memset(&field, 0, sizeof(field));
    if (jsonDoc.HasMember("ts")) {
        spotrapidjson::Value &time = jsonDoc["ts"];
        uint64_t nowEpochTime = time.GetUint64();
        if (lastTickTime_ > nowEpochTime) return;
        field.EpochTime = nowEpochTime;
        lastTickTime_ = nowEpochTime;
    }

    field.MessageID = getMessageID();
    field.LevelType = MDLEVELTYPE_TICKER;

    memcpy(field.ExchangeID, Exchange_BYBIT.c_str(), min(sizeof(field.ExchangeID) - 1, Exchange_BYBIT.size()));

    verfiyChannelInstrument(channel, instId);

    memcpy(field.InstrumentID, instId.c_str(), min(InstrumentIDLen, uint16_t(instId.size())));

    spotrapidjson::Value &dataNode = jsonDoc["data"];

    double bid = 0; double ask = 0; double bidVol = 0; double askVol = 0; double lastPrice = 0;
    double lastVolume = 0;

    if (dataNode.HasMember("bid1Price")) {
        spotrapidjson::Value &bid1 = dataNode["bid1Price"];
        bid = atof(bid1.GetString());
        field.setBidPrice(1, bid); 
    }

    if (dataNode.HasMember("ask1Price")) {
        spotrapidjson::Value &ask1 = dataNode["ask1Price"];
        ask = atof(ask1.GetString());
        field.setAskPrice(1, ask);
    }

    if (dataNode.HasMember("bid1Size")) {
        spotrapidjson::Value &bid1Volume = dataNode["bid1Size"];
        bidVol = atof(bid1Volume.GetString());
        field.setBidVolume(1, bidVol);
    }

    if (dataNode.HasMember("ask1Size")) {
        spotrapidjson::Value &ask1Volume = dataNode["ask1Size"];
        askVol = atof(ask1Volume.GetString());
        field.setAskVolume(1, askVol);
    }

    if (dataNode.HasMember("lastPrice")) {
        spotrapidjson::Value &dealLastPrice = dataNode["lastPrice"];
        lastPrice = atof(dealLastPrice.GetString());
        field.LastPrice = lastPrice;
    }

    if (IS_DOUBLE_ZERO(bid) || IS_DOUBLE_ZERO(ask) ||
        IS_DOUBLE_ZERO(bidVol) || IS_DOUBLE_ZERO(askVol)){
        LOG_WARN << "BybitMdSpi::onTickerData price || volume invaild 0 bid: " << 
            bid << ", ask: " << ask << ", bidVol: " << bidVol << ", askVol: " << askVol
            << ", lastPrice: " << lastPrice;
        return;
    }

    field.UpdateMillisec = CURR_MSTIME_POINT;

    string dayTime = getCurrSystemDate2();
    dayTime += " ";
    dayTime += getCurrentSystemTime();
    memcpy(field.TradingDay, dayTime.c_str(), min(sizeof(field.TradingDay) - 1, dayTime.size()));
    onData(&field);
    if (cnt != (2 * (count_ - 1) + 1))	{
        LOG_FATAL << "thread wrong cnt: " << cnt << ", count_: " << count_;
    }
    
    return;
}

void BybitMdSpi::onDepthData(const std::string &channel, const std::string &instId, spotrapidjson::Document &jsonDoc)
{
    memset(&field, 0, sizeof(field));
    spotrapidjson::Value &timestampNode = jsonDoc["ts"];
    // field.EpochTime = timestampNode.GetUint64();  //2019-01-08T11:42:41.801Z

    uint64_t nowEpochTime = timestampNode.GetUint64();
    if (lastDepthTime_ > nowEpochTime) return;
    field.EpochTime = nowEpochTime;
    lastDepthTime_ = nowEpochTime;

    field.MessageID = getMessageID();
    field.LevelType = MDLEVELTYPE_DEFAULT;
    memcpy(field.ExchangeID, Exchange_BYBIT.c_str(), min(sizeof(field.ExchangeID) - 1, Exchange_BYBIT.size()));

    verfiyChannelInstrument(channel, instId);

    unsigned int len = strlen(instId.c_str());
    memcpy(field.InstrumentID, instId.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));

    field.UpdateMillisec = CURR_MSTIME_POINT;

    string dayTime = getCurrSystemDate2();
    dayTime += " ";
    dayTime += getCurrentSystemTime();
    memcpy(field.TradingDay, dayTime.c_str(), min(sizeof(field.TradingDay) - 1, dayTime.size()));

    spotrapidjson::Value &dataNode = jsonDoc["data"];
    spotrapidjson::Value &asks = dataNode["a"];
    spotrapidjson::Value &bids = dataNode["b"];

    if (channel == bybit_swap_depth_channel_name) {
        if (!asks.IsArray() || !bids.IsArray()) {
            LOG_FATAL << "onDepthData get error";
        }
        for (size_t i = 0; i < asks.Size(); ++i) {
            spotrapidjson::Value &priceNode = asks[i][0];
            spotrapidjson::Value &amountNode = asks[i][1];
            double price = atof(priceNode.GetString());    //different type
            double amount = atof(amountNode.GetString());
            if (IS_DOUBLE_ZERO(amount)) continue;

            field.setAskPrice(i + 1, price);
            field.setAskVolume(i + 1, amount);
        }

        for (size_t i = 0; i < bids.Size(); ++i) {
            spotrapidjson::Value &priceNode = bids[i][0];
            spotrapidjson::Value &amountNode = bids[i][1];
            double price = atof(priceNode.GetString());   //different type
            double amount = atof(amountNode.GetString());
            if (IS_DOUBLE_ZERO(amount)) continue;
            field.setBidPrice(i + 1, price);
            field.setBidVolume(i + 1, amount);
        }
        onData(&field);
    }
}

bool BybitMdSpi::verfiyChannelInstrument(const std::string &channel, const std::string &instId)
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
            LOG_FATAL << "Bybit onTrade channel not found flag: " << flag << ", channel: " << channel << ", instId: " << instId;
            return false;
        }
    } else {
        LOG_FATAL << "Bybit onTrade channel not found: " << channel << ", instId: " << instId;
        return false;
    }
    return true;
}


void BybitMdSpi::onData(InnerMarketData *innerMktData)
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

void BybitMdSpi::onData(InnerMarketTrade* innerMktTrade)
{
	if (ShmMtServer::instance().isInit()) {
		ShmMtServer::instance().update(innerMktTrade);
	}
	else {
		queueStore_->storeHandle(innerMktTrade);
	}
}