#ifndef SPOT_GTWAY_BYBIT_MDSPI_H
#define SPOT_GTWAY_BYBIT_MDSPI_H

#include <spot/base/DataStruct.h>
#include "spot/base/Instrument.h"
#include "spot/rapidjson/document.h"
#include "spot/rapidjson/rapidjson.h"
#include "spot/websocket/websocketapi.h"
#include <spot/gateway/MktDataStoreCallBack.h>

using namespace spot;
using namespace spot::base;
using namespace spot::gtway;
namespace spot
{
	namespace gtway
	{
		class BybitMdSpi : public WebSocketApi
		{
		public:
			BybitMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);

			~BybitMdSpi();

			void init();  

			void run();

			void AddSubInst(const char *inst);

			void com_callbak_open();

			void com_callbak_close();

			void com_callbak_message(const char *message);

		private:
			void subscribe(const std::string& channel, string deep, const char* instrID);

			void onErrResponse(spotrapidjson::Document& jsonDoc);

			void onTrade(const std::string& channel, const std::string& instId, spotrapidjson::Document& jsonDoc);

			void onDepthData(const std::string& channel, const std::string& instId, spotrapidjson::Document& jsonDoc);

			void onTickerData(const std::string& channel, const std::string& instId, spotrapidjson::Document& jsonDoc);

			bool verfiyChannelInstrument(const std::string &channel, const std::string &instId);
			
			void onData(InnerMarketData *innerMktData);
			void onData(InnerMarketTrade* innerMktTrade);

		private:
			std::mutex mutex_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;

			// std::vector<Instrument*> subInstVec_;

			WebSocketApi *comapi_;

			MktDataStoreCallBack *queueStore_;

			// InnerMarketData field_;

			// InnerMarketTrade trade_;

//			std::map<string, Instrument*> channelInstrumentMap_;

			std::map<string, std::vector<const char*> > channelInstrumentMap_;
			std::vector<const char*> instVec_;
			// std::vector<string> instVec_;

			atomic<bool> isBybitMDConnected_;
			atomic<uint64_t> count_;

			//////////////////////////////////////////////////////////////////
			string bybit_swap_depth5_channel_name = "orderbook";
			string bybit_swap_depth_channel_name = "orderbook";
			string bybit_swap_trade_channel_name = "publicTrade";
			string bybit_swap_ticker_channel_name = "tickers";

			uint64_t lastTradeTime_;
			uint64_t lastTickTime_;
			uint64_t lastDepthTime_;
			//////////////////////////////////////////////////////////////////
		};

	}
}

#endif