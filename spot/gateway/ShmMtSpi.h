#ifndef SPOT_GTWAY_SHM_MT_SPI_H
#define SPOT_GTWAY_SHM_MT_SPI_H

#include <spot/base/DataStruct.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <spot/shmmd/ShmMtClient.h>

#include <unordered_map>
#include <memory>

using namespace spot;
using namespace spot::base;
using namespace spot::shmmd;

namespace spot
{
	namespace gtway
	{
		class ShmMtSpi : public ShmMtClient
		{
		public:
			ShmMtSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);
			virtual ~ShmMtSpi();
			void Init();
			void AddSubInst(const char * inst);

			virtual void onInnerMarketTrade(const spot::base::InnerMarketTrade& mt);
			void setRecvMarketTradeCallback(RecvMarketTradeCallback marketTradeCallback);

		private:
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			MktDataStoreCallBack *queueStore_;
			std::unordered_map<string, string> subInstMap_;
			RecvMarketTradeCallback marketTradeCallback_;
		};
	}
}

#endif //SPOT_GTWAY_SHM_MT_SPI_H
