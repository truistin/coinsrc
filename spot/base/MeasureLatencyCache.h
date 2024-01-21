#ifndef SPOT_BASE_MEASURELATENCYCACHE_H
#define SPOT_BASE_MEASURELATENCYCACHE_H
#include<unordered_map>
#include<vector>
#include<memory>
#include<string>
#include<map>
#include<functional>
#include "spot/utility/SPSCQueue.h"
#include "spot/base/DataStruct.h"
#include "spot/cache/CacheAllocator.h"
using namespace spot::utility;
const static int MEASURE_COUNTS = 6;
namespace spot
{
	namespace base
	{
		class MeasureLatencyCache
		{
		public:
			MeasureLatencyCache(spsc_queue<QueueNode> *sendQueue);
			~MeasureLatencyCache();
			void addColumnName(string name);
		    void addMeasurePoint(uint16_t columnIndex, uint64_t epochTime);
			void addMeasurePoint(uint16_t columnIndex);
			void addMeasurePoint2(uint16_t columnIndex);
			void sendTick2OrderMetricData(int orderRef,const char* counterType, const char* exchangeID, int strategyID);
			void reset();
		private:
			void sendMetricData(int keyID, const char* counterType, const char* exchangeID, int strategyID, uint64_t epochTime, uint32_t epochTimeDiff, const char* currPoint, const char* lastPoint);
		private:
			spsc_queue<QueueNode> *sendQueue_;
			std::vector<std::string> columnNameVec_;
			uint64_t epochTimeVec_[MEASURE_COUNTS];
			QueueNode queueNode_;
		};		

	}
}
using namespace spot::base;
extern MeasureLatencyCache *gTickToOrderMeasure;
static void initTickToOrderMeasure(spsc_queue<QueueNode> *sendQueue)
{
	char *buff = CacheAllocator::instance()->allocate(sizeof(MeasureLatencyCache));
	gTickToOrderMeasure = new (buff) spot::base::MeasureLatencyCache(sendQueue);
	gTickToOrderMeasure->addColumnName("ReceiveMarketData");//0
	gTickToOrderMeasure->addColumnName("OnRtnInnerMarketData");	//1
	gTickToOrderMeasure->addColumnName("evaluateStrategy");//2
	gTickToOrderMeasure->addColumnName("StrategyInstrument::setOrder");//3
	gTickToOrderMeasure->addColumnName("DealOrdersApi::newOrder");//4
	gTickToOrderMeasure->addColumnName("OrderController::reqOrderBefore");//5
	//gTickToOrderMeasure->addColumnName("ReqOrderInsertBefore");//6
}
#endif