#ifndef SPOT_GATEWAY_MKTDATAQUESTORE_H
#define SPOT_GATEWAY_MKTDATAQUESTORE_H
#include <spot/utility/SPSCQueue.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/base/DataStruct.h>
#include <spot/gateway/MktDataStoreCallBack.h>

using namespace spot::utility;
using namespace spot::base;
using namespace moodycamel;
using namespace spot::gtway;
namespace spot
{
	namespace gtway
	{
		class MktDataQueStore : public MktDataStoreCallBack
		{
		public:

			MktDataQueStore(ConcurrentQueue<InnerMarketData> *mdQueue, ConcurrentQueue<InnerMarketTrade> *mtQueue, ConcurrentQueue<QueueNode> *mdLastPriceQueue
				,int writeQ = 0) 
				: mdQueue_(mdQueue), mtQueue_(mtQueue), mdLastPriceQueue_(mdLastPriceQueue), writeQ_(writeQ)
			{
				assert(mdQueue_ != nullptr);
			}
			virtual void storeHandle(InnerMarketData *innerMktData);
			virtual void storeHandle(InnerMarketTrade *innerMktTrade);
			virtual void storeHandle(InnerMarketOrder *innerMktOrder);

		private:
			ConcurrentQueue<InnerMarketData> *mdQueue_;
			ConcurrentQueue<InnerMarketTrade> *mtQueue_;
			ConcurrentQueue<InnerMarketOrder> *moQueue_;
			ConcurrentQueue<QueueNode> *mdLastPriceQueue_;
			int writeQ_;

		};
	}
}
#endif