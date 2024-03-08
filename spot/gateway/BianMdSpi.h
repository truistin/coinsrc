#ifndef SPOT_GTWAY_BIANMDSPI_H
#define SPOT_GTWAY_BIANMDSPI_H
#include <spot/base/DataStruct.h>
#include "spot/rapidjson/document.h"
#include "spot/rapidjson/rapidjson.h"
#include "spot/websocket/websocketapi.h"
#include "spot/base/Instrument.h"
#include <map>
#include <string>
using namespace spot;
using namespace spot::base;
namespace spot
{
	namespace gtway
	{
		class MktDataStoreCallBack;
		class BianMdSpi
		{
		public:
			BianMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);

			~BianMdSpi();
			void Init();
			
			void AddSubInst(const char *inst);
			void com_callbak_open();
			void com_callbak_close();
			void com_callbak_message(const char *message);
			void subscribe();
			void subscribe(string channel, bool firstSymbol);
			void subscribePerp(string channel,bool firstSymbol);

		private:
			void fillDepthAskBidInfo(spotrapidjson::Value& tickNode, InnerMarketData& field);
			bool fillTickerAskBidInfo(spotrapidjson::Value& tickNode, InnerMarketData& field);
			void onData(InnerMarketData *innerMktData);
			void onData(InnerMarketTrade* innerMktTrade);

		private:
			atomic<uint64_t> count_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			MktDataStoreCallBack *queueStore_;
			// InnerMarketData field_;
			InnerMarketTrade trade_;
			WebSocketApi *websocketApi_;
			WebSocketApi *websocketApi1_;
			WebSocketApi *websocketApi2_;
			std::string  symbolStreams_;
			std::string  symbolStreams1_;
			std::string  symbolStreams2_;
			uint64_t lastTradeTime_;
			uint64_t lastTickTime_;
			uint64_t lastDepthTime_;
		};
	}
}

#endif