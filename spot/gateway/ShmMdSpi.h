#ifndef SPOT_GTWAY_SHM_MD_SPI_H
#define SPOT_GTWAY_SHM_MD_SPI_H

#include <spot/base/DataStruct.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <spot/shmmd/ShmMdClient.h>
#include <spot/net/UdpMcastServer.h>

#include <unordered_map>
#include <memory>

using namespace spot;
using namespace spot::base;
using namespace spot::shmmd;
using namespace spot::net;

namespace spot
{
	namespace gtway
	{
		class ShmMdSpi: public ShmMdClient
		{
		public:
			ShmMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);
			virtual ~ShmMdSpi();
			void Init();
			void AddSubInst(const char * inst);

			virtual void onInnerMarketData(const spot::base::InnerMarketData& md);
			void setReckMarketDataCallback(RecvMarketDataCallback marketDataCallback);
		private:
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			MktDataStoreCallBack *queueStore_;
			std::unordered_map<string, string> subInstMap_;
			UdpMcastServer* pServer_;
			RecvMarketDataCallback marketDataCallback_;
		};
	}
}

#endif //SPOT_GTWAY_SHM_MD_SPI_H
