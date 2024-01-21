#ifndef SPOT_GTWAY_DIST_MD_SPI_H
#define SPOT_GTWAY_DIST_MD_SPI_H

#include <spot/base/DataStruct.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <spot/distributor/DistDef.h>
#include <spot/distributor/DistClient.h>

#include <unordered_map>
#include <memory>

using namespace spot;
using namespace spot::base;
using namespace spot::distributor;

namespace spot
{
	namespace gtway
	{
		class DistMdSpi : public DistClient
		{
		public:
			DistMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore);
			virtual ~DistMdSpi();
			void Init();
			void AddSubInst(const char * inst);

			virtual void onMsg(const spot::base::InnerMarketData* innerMktData);

		private:
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			MktDataStoreCallBack *queueStore_;
			std::unordered_map<string, string> subInstMap_;
		};
	}
}

#endif //SPOT_GTWAY_DIST_MD_SPI_H
