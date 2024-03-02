#include <spot/gateway/BianMdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <spot/utility/Compatible.h>
#include <functional>
#include <spot/utility/gzipwrapper.h>
#include "spot/bian/BnApi.h"
#include <chrono>
#include <cinttypes>
#include <ctime>
#ifdef __LINUX__
#include <time.h>
#endif
#include <iomanip>
#include <spot/utility/MeasureFunc.h>
#include <spot/shmmd/ShmMdServer.h>
#include <spot/shmmd/ShmMtServer.h>
#include <spot/distributor/DistServer.h>

using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spotrapidjson;
using namespace std::chrono;
using namespace spot::shmmd;
using namespace spot::distributor;
//#define BINANCE_WSS   "wss://stream.binance.com:9443"
#define BINANCE_WSS	  "wss://fstream.binance.com/stream"
// "wss://fstream.binance.com/stream?streams=ethusdt@depth5/btcusdt@trade/btcusdt@bookTicker"
// "wss://dstream.binance.com/stream?streams=ethusdt@depth5/btcusdt@trade/btcusdt@bookTicker"
const unsigned int OrderBookLevelMax = 5;
BianMdSpi::BianMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack* queueStore)
{
	apiInfo_ = apiInfo;
	queueStore_ = queueStore;
	count_ = 1;
	symbolStreams_ ="/stream?streams=";
	symbolStreams1_ ="/stream?streams=";
	lastTradeTime_ = 0;
	lastTickTime_ = 0;
	lastDepthTime_ = 0;
}

BianMdSpi::~BianMdSpi()
{

}
void BianMdSpi::Init()
{
	subscribe();
	LOG_INFO << "frontMdAddr_: " << apiInfo_->frontMdAddr_ 
		<< ", InterfaceAddr_: " << apiInfo_->InterfaceAddr_
		<< ", frontQueryAddr_: " << apiInfo_->frontQueryAddr_
		<< ", symbolStreams_: " << symbolStreams_ << ", symbolStreams1_: "
		<< symbolStreams1_ << ", symbolStreams2_: " << symbolStreams2_;
	// MeasureFunc::addMeasureData(3, "BianMdSpi::dealBianMdData", 1000);
	cout << "frontMdAddr_: " << apiInfo_->frontMdAddr_ 
		<< ", InterfaceAddr_: " << apiInfo_->InterfaceAddr_
		<< ", symbolStreams_: " << symbolStreams_ << ", symbolStreams1_: "
		<< symbolStreams1_;
	string str  = string(apiInfo_->frontMdAddr_) + symbolStreams_;
	websocketApi_ = new WebSocketApi();
//	websocketApi_->SetUri(BINANCE_WSS + symbolStreams_);
//	websocketApi_->SetUri("wss://fstream.binance.com/stream?streams=ethusdt@bookTicker/btcusdt@bookTicker");
	websocketApi_->SetUri(str);
	//websocketApi_->SetCompress(true);
	websocketApi_->SetCallBackOpen(std::bind(&BianMdSpi::com_callbak_open, this));
	websocketApi_->SetCallBackMessage(std::bind(&BianMdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi_->SetCallBackClose(std::bind(&BianMdSpi::com_callbak_close, this));

	std::thread BianMdSpiSocket(std::bind(&WebSocketApi::Run, websocketApi_));
	pthread_setname_np(BianMdSpiSocket.native_handle(), "BaMdSocket");
	BianMdSpiSocket.detach();

	string str1 = string(apiInfo_->InterfaceAddr_) + symbolStreams1_;
	websocketApi1_ = new WebSocketApi();
//	websocketApi_->SetUri(BINANCE_WSS + symbolStreams_);
//	websocketApi_->SetUri("wss://fstream.binance.com/stream?streams=ethusdt@bookTicker/btcusdt@bookTicker");
	websocketApi1_->SetUri(str1);
	//websocketApi_->SetCompress(true);
	websocketApi1_->SetCallBackOpen(std::bind(&BianMdSpi::com_callbak_open, this));
	websocketApi1_->SetCallBackMessage(std::bind(&BianMdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi1_->SetCallBackClose(std::bind(&BianMdSpi::com_callbak_close, this));

	std::thread BianMdSpiSocket1(std::bind(&WebSocketApi::Run, websocketApi1_));
	pthread_setname_np(BianMdSpiSocket1.native_handle(), "BaMdSocket_1");
	BianMdSpiSocket1.detach();

	string str2 = string(apiInfo_->InterfaceAddr_) + symbolStreams2_;
	websocketApi2_ = new WebSocketApi();
//	websocketApi_->SetUri(BINANCE_WSS + symbolStreams_);
//	websocketApi_->SetUri("wss://fstream.binance.com/stream?streams=ethusdt@bookTicker/btcusdt@bookTicker");
	websocketApi2_->SetUri(str2);
	//websocketApi_->SetCompress(true);
	websocketApi2_->SetCallBackOpen(std::bind(&BianMdSpi::com_callbak_open, this));
	websocketApi2_->SetCallBackMessage(std::bind(&BianMdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi2_->SetCallBackClose(std::bind(&BianMdSpi::com_callbak_close, this));

	std::thread BianMdSpiSocket2(std::bind(&WebSocketApi::Run, websocketApi2_));
	pthread_setname_np(BianMdSpiSocket2.native_handle(), "BaMdSocket_2");
	BianMdSpiSocket2.detach();
}

void BianMdSpi::com_callbak_open()
{
	std::cout << "Binance com_callbak_open" << std::endl;
	LOG_INFO << "Binance com_callbak_open";
}

void BianMdSpi::com_callbak_close()
{
	std::cout << "Binance com_callbak_close" << std::endl;
	LOG_INFO << "Binance com_callbak_close";
}
void BianMdSpi::subscribe()
{
	bool firstSymbol = true;
	bool firstSymbol1 = true;
	bool firstSymbol2 = true;
	map<string, string> tickerInstrumentMap = BnApi::GetTickerInstrumentMap();
	map<string, string> tradeInstrumentMap = BnApi::GetTradeInstrumentMap();
	
	for (auto iter : tickerInstrumentMap)
	{
		LOG_INFO << "subscribe channel:" << iter.first << "second: " << iter.second;
		if (iter.second.find("perp") != iter.second.npos) {
			subscribePerp(iter.first,firstSymbol1);
			firstSymbol1 = false;
		} else if (iter.second.find("usdt") != iter.second.npos) {
			subscribe(iter.first,firstSymbol);
			firstSymbol = false;
		} else {
			subscribeSpot(iter.first,firstSymbol2);
			firstSymbol2 = false;
		}
	}

	for (auto iter : tradeInstrumentMap)
	{
		LOG_INFO << "subscribe trade:" << iter.first << "second: " << iter.second;;
		if (iter.second.find("perp") != iter.second.npos) {
			subscribePerp(iter.first,firstSymbol1);
		} else if (iter.second.find("usdt") != iter.second.npos) {
			subscribe(iter.first, firstSymbol);
		} else {
			subscribeSpot(iter.first,firstSymbol2);
		}
	}
}
void BianMdSpi::subscribe(string channel,bool firstSymbol)
{
	if (firstSymbol)
	{
		symbolStreams_ = symbolStreams_ + channel;
	}
	else
	{
		symbolStreams_ = symbolStreams_ + "/" + channel ;
	}
}

void BianMdSpi::subscribePerp(string channel,bool firstSymbol)
{
	if (firstSymbol)
	{
		symbolStreams1_ = symbolStreams1_ + channel;
	}
	else
	{
		symbolStreams1_ = symbolStreams1_ + "/" + channel ;
	}
}

void BianMdSpi::subscribeSpot(string channel,bool firstSymbol)
{
	if (firstSymbol)
	{
		symbolStreams2_ = symbolStreams2_ + channel;
	}
	else
	{
		symbolStreams2_ = symbolStreams2_ + "/" + channel ;
	}
}


void BianMdSpi::fillDepthAskBidInfo(spotrapidjson::Value& tickNode, InnerMarketData& field_)
{
	if (tickNode.HasMember("a"))
	{
		spotrapidjson::Value& asks = tickNode["a"];
		auto askSize = min(OrderBookLevelMax, asks.Size());
		for (unsigned int i = 0; i < askSize; ++i)
		{
			spotrapidjson::Value& priceNode = asks[i][0];
			double price = atof(priceNode.GetString());
			field_.setAskPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = asks[i][1];
			double volume = atof(volumeNode.GetString());
			field_.setAskVolume(i + 1, volume);
		}
	}

	if (tickNode.HasMember("b"))
	{
		spotrapidjson::Value& bids = tickNode["b"];
		auto bidSize = min(OrderBookLevelMax, bids.Size());
		for (unsigned int i = 0; i < bidSize; ++i)
		{
			spotrapidjson::Value& priceNode = bids[i][0];
			double price = atof(priceNode.GetString());
			field_.setBidPrice(i + 1, price);
			spotrapidjson::Value& volumeNode = bids[i][1];
			double volume = atof(volumeNode.GetString());
			field_.setBidVolume(i + 1, volume);
		}
	}
	return;
}

bool BianMdSpi::fillTickerAskBidInfo(spotrapidjson::Value& tickNode, InnerMarketData& field_)
{
	spotrapidjson::Value& ask1 = tickNode["a"];
	double price = atof(ask1.GetString());
	field_.setAskPrice(1, price);

	spotrapidjson::Value& askVolume1 = tickNode["A"];
	double volume = atof(askVolume1.GetString());
	field_.setAskVolume(1, volume);

	if (IS_DOUBLE_ZERO(price) || IS_DOUBLE_ZERO(volume)) {
		LOG_WARN << "BianMdSpi: TICKER askprice || askvolume invaild 0";
		return false;
	}

	spotrapidjson::Value& bid1 = tickNode["b"];
	price = atof(bid1.GetString());
	field_.setBidPrice(1, price);

	spotrapidjson::Value& bidVolume1 = tickNode["B"];
	volume = atof(bidVolume1.GetString());
	field_.setBidVolume(1, volume);

	// spotrapidjson::Value& lastP = tickNode["c"];
	// field_.LastPrice = atof(lastP.GetString());
	
	// spotrapidjson::Value& lastV = tickNode["Q"];
	// field_.Volume = atof(lastV.GetString());
	if (IS_DOUBLE_ZERO(price) || IS_DOUBLE_ZERO(volume)) {
		LOG_WARN << "BianMdSpi: TICKER bidprice || bidvolume invaild 0";
		return false;
	}
	return true;
}

void BianMdSpi::com_callbak_message(const char *message)
{
	LOG_INFO << "BianMdSpi com_callbak_message: " << message;
	// uint64_t cnt = 2 * count_ + 1;
	// count_ = count_ + 1;
	// MeasureFunc f(3);
	//cout << message << endl;
	try
	{
		// string a=message;
    	// string b="ping";
		// string c="Ping";
		// string d="PING";


    	// if(a.find(b) != string::npos || a.find(c) != string::npos || a.find(d) != string::npos) {
		// 	LOG_INFO << "BianMdSpi message1: " << message;
		// }

		if (strcmp(message, "ping") == 0) {
			string s = "pong";
			websocketApi_->send(s);
			LOG_DEBUG << "BianMdSpi message: " << message;
			return;
		}
		//LOG_INFO << "BianMdSpi::com_callbak_message mess: " << message;
		spotrapidjson::Document documentCOM_;
		documentCOM_.Parse<0>(message);

		if (documentCOM_.HasParseError())
		{
			LOG_ERROR << "Parsing to document failure";
			return;
		}

		// if (documentCOM_.HasMember("ping"))
		// {
		// 	spotrapidjson::Value& eventNode = documentCOM_["ping"];
		// 	int64_t ping = eventNode.GetInt64();
		// 	std::ostringstream oss;
		// 	oss << ping; std:string pongAsString(oss.str());
		// 	pongAsString = "{\"pong\":" + pongAsString + "}";

		// 	LOG_INFO << "pongAsString: " << pongAsString;
		// 	websocketApi_->send(pongAsString);
		// 	return;
		// }

		InnerMarketData field;
		if (documentCOM_.HasMember("stream"))
		{
			spotrapidjson::Value& channelNode = documentCOM_["stream"];
			string channel = channelNode.GetString();

			// channel:ethusdt@bookTicker
			map<string, string> tickerInstrumentMap = BnApi::GetTickerInstrumentMap();
			if (tickerInstrumentMap.find(channel) != tickerInstrumentMap.end()) {
				if (documentCOM_.HasMember("data")) {
					memset(&field, 0, sizeof(field));
					spotrapidjson::Value& tickNode = documentCOM_["data"];
					uint64_t newEpochTime = tickNode["T"].GetUint64();
					if (lastTickTime_ > newEpochTime) return;
					field.EpochTime = newEpochTime;
					lastTickTime_ = newEpochTime;

					field.MessageID = getMessageID();
					field.LevelType = MDLEVELTYPE_DEFAULT;
					memcpy(field.ExchangeID, Exchange_BINANCE.c_str(), min(sizeof(field.ExchangeID) - 1, Exchange_BINANCE.size()));

					string dayTime = getCurrSystemDate2();
					dayTime += " ";
					dayTime += getCurrentSystemTime();
					memcpy(field.TradingDay, dayTime.c_str(), min(sizeof(field.TradingDay) - 1, dayTime.size()));	
					field.UpdateMillisec = CURR_MSTIME_POINT;

					string inst = BnApi::GetTickerInstrumentID(channel);
					memcpy(field.InstrumentID, inst.c_str(), min(sizeof(field.InstrumentID) - 1, inst.size()));


					if (!fillTickerAskBidInfo(tickNode, field)) {
						LOG_WARN << "BianMdSpi::com_callbak_message price || amount invaild 0";
						return;
					}
					
				} else {
					LOG_FATAL << "BianMdSpi com_callbak_message recv ticker err";
				}
				
				//queueStore_->storeHandle(&field);
				onData(&field);
				// if (cnt != (2 * (count_ - 1) + 1))	{
				// 	LOG_FATAL << "thread wrong cnt: " << cnt << ", count_: " << count_;
				// }
				return;
			}

			map<string, string> depthInstrumentMap = BnApi::GetDepthInstrumentMap();
			if (depthInstrumentMap.find(channel) != depthInstrumentMap.end()) {
				if (documentCOM_.HasMember("data")) {
					memset(&field, 0, sizeof(field));
					spotrapidjson::Value& tickNode = documentCOM_["data"];
					uint64_t newEpochTime = tickNode["T"].GetUint64();

					if (lastDepthTime_ > newEpochTime) return;
					field.EpochTime = newEpochTime;
					lastDepthTime_ = newEpochTime;
					
					field.MessageID = getMessageID();
					field.LevelType = MDLEVELTYPE_DEFAULT;
					memcpy(field.ExchangeID, Exchange_BINANCE.c_str(), min(sizeof(field.ExchangeID) - 1, Exchange_BINANCE.size()));

					string dayTime = getCurrSystemDate2();
					dayTime += " ";
					dayTime += getCurrentSystemTime();
					memcpy(field.TradingDay, dayTime.c_str(), min(sizeof(field.TradingDay) - 1, dayTime.size()));	
					field.UpdateMillisec = CURR_MSTIME_POINT;

					string inst = BnApi::GetDepthInstrumentID(channel);
					memcpy(field.InstrumentID, inst.c_str(), min(sizeof(field.InstrumentID) - 1, inst.size()));

					fillDepthAskBidInfo(tickNode, field);
				} else {
					LOG_FATAL << "BianMdSpi com_callbak_message recv depth err";
				}

				onData(&field);
				return;
			}

			map<string, string> tradeInstrumentMap = BnApi::GetTradeInstrumentMap();
			if (tradeInstrumentMap.find(channel) != tradeInstrumentMap.end()) {
				if (documentCOM_.HasMember("data"))
				{
					spotrapidjson::Value& dataNode = documentCOM_["data"];
					if (dataNode.HasMember("e"))
					{
						InnerMarketTrade trade;
						spotrapidjson::Value& eNode = dataNode["e"];
						if (strcmp(eNode.GetString(), "trade") == 0)
						{
							memset(&trade, 0, sizeof(trade));
							spotrapidjson::Value& tradeTime = dataNode["T"];
							uint64_t newEpochTime = tradeTime.GetUint64();
							if (lastTradeTime_ > newEpochTime) return;
							trade.EpochTime = newEpochTime;
							lastTradeTime_ = newEpochTime;

							spotrapidjson::Value& direction = dataNode["m"];
							spotrapidjson::Value& tradeID = dataNode["t"];
							spotrapidjson::Value& price = dataNode["p"];
							spotrapidjson::Value& amount = dataNode["q"];
							spotrapidjson::Value& market = dataNode["X"];

							if (strcmp(market.GetString(), "MARKET") != 0) {
								LOG_INFO << "ADL or INSURANCE_FUND data: " << message;
								return;
							}

							string dayTime = getCurrSystemDate2();
							dayTime += " ";
							dayTime += getCurrentSystemTime();
							memcpy(trade.TradingDay, dayTime.c_str(), min(sizeof(trade.TradingDay) - 1, dayTime.size()));						
						
							trade.UpdateMillisec = CURR_MSTIME_POINT;
							// trade.EpochTime = tradeTime.GetUint64();

						
							trade.MessageID = getMessageID();
							memcpy(trade.ExchangeID, Exchange_BINANCE.c_str(), min(sizeof(trade.ExchangeID) - 1, Exchange_BINANCE.size()));

							string instrumentID = BnApi::GetTradeInstrumentID(channel);
							unsigned int len = strlen(instrumentID.c_str());
							memcpy(trade.InstrumentID, instrumentID.c_str(), (len < InstrumentIDLen ? len : InstrumentIDLen));

							string tid = std::to_string(tradeID.GetUint64());

							memcpy(trade.Tid, tid.c_str(), min(sizeof(trade.Tid) - 1, tid.size()));
							trade.Price = atof(price.GetString());
							trade.Volume = atof(amount.GetString());
							trade.Turnover = trade.Price * trade.Volume;

							if (direction.GetBool())
							{
								trade.Direction = INNER_DIRECTION_Sell;
							}
							else
							{
								trade.Direction = INNER_DIRECTION_Buy;
							}
							onData(&trade);
						}
						else
						{
							LOG_FATAL << "BianMdSpi com_callbak_message recv TRADE err";
							return;
						}
					}
				}
				
			}

		}
	}
	catch (exception ex)
	{
		LOG_WARN << "BianMdSpi::com_callbak_message exception occur:" << ex.what();
	}
}
void BianMdSpi::AddSubInst(const char *inst)
{
	LOG_INFO << "bian addSubInst:" << inst;
	BnApi::AddInstrument(inst);

}

void BianMdSpi::onData(InnerMarketData *innerMktData)
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

void BianMdSpi::onData(InnerMarketTrade* innerMktTrade)
{
	if (ShmMtServer::instance().isInit()) {
		ShmMtServer::instance().update(innerMktTrade);
	}
	else {
		queueStore_->storeHandle(innerMktTrade);
	}
}