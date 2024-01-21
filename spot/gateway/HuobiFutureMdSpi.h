#ifndef SPOT_GTWAY_HUOBFUTUREMDSPI_H
#define SPOT_GTWAY_HUOBFUTUREMDSPI_H
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
		class HuobFutureMdSpi
		{
		public:
			HuobFutureMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);

			~HuobFutureMdSpi();
			void Init();
			void Run();

			void AddSubInst(Instrument *inst);
			void com_callbak_open();
			void com_callbak_close();
			void com_callbak_message(const char *message);
			void subscribe();
			void subscribe(string channel);
		private:
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			MktDataStoreCallBack *queueStore_;
			//InnerMarketData field_;
			//InnerMarketTrade trade_;
			WebSocketApi *websocketApi_;
			std::string depth_channel_name = ".depth.";
			std::string trade_channel_name = ".trade.";

			atomic<bool> isHuobFutureMDConnected_;
		};
	}
}

#endif