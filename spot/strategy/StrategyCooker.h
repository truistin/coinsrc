#ifndef SPOT_STRATEGY_STRATEGYCOOKER_H
#define SPOT_STRATEGY_STRATEGYCOOKER_H
#include<atomic>
#include<unordered_map>
#include<string>
#include "spot/utility/Compatible.h"
#include "spot/utility/SPSCQueue.h"
#include "spot/base/Adapter.h"
#include "spot/base/DataStruct.h"
#include "spot/strategy/Strategy.h"
#include "spot/strategy/OrderController.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/strategy/Initializer.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/gateway/GateWay.h"
using namespace std;
using namespace spot::base;
using namespace spot::utility;

namespace spot
{
	namespace strategy
	{
		class StrategyCooker
		{
		public:
			StrategyCooker();
			~StrategyCooker();
			void setGatewayQueue(ConcurrentQueue<InnerMarketData> *mdQueue, ConcurrentQueue<InnerMarketTrade> *mtQueue, ConcurrentQueue <Order>* orderQueue);
			void run();
			void setAllStrategyTradeable(bool tradeable);
			void updateStrategyParam(const QueueNode& node);
			bool checkStrategyParams(list<StrategyParam>* paramList);


			void updateMarketData(const QueueNode& node);

			void updateStrategySwitch(const QueueNode& node);
			bool setStrategyTradeable(int strategyID, bool tradeable);
			bool setStrategyInstrumentTradeable(int strategyID, string instrumentID, bool tradeable);
			void turnOffAllStrategy();
			inline ConcurrentQueue<InnerMarketData>* getMdQueue();
			inline ConcurrentQueue<InnerMarketTrade>* getMtQueue();
			inline ConcurrentQueue<Order>* getRtnOrderQueue();
			inline spsc_queue<QueueNode>*     getRmqReadQueue();
			void setAdapter(Adapter *adapter);
			bool initStrategyInstrument();

			void convertStrategyParam(const StrategyParam &updateParam, StrategyParam &param);
			void sendStrategyInstrumentTradeable();
			void handleRmqData(QueueNode& node);
		private:
			void runStrategy();
			void handleMarketData();
			void handleMarketData(InnerMarketData& md);
			void handleMarketTrade(InnerMarketTrade& mt);
			void handleRtnOrder(Order& rtnOrder);
			
			void sendInitFinished();
			void handleTimerEvent();
			void handleShmData();
			void handleCircuitBreakCheck();
			void handleForceCloseTimerInterval();
			void handleCheckCancelOrder();
		private:
			ConcurrentQueue<InnerMarketData>* mdQueue_;
			ConcurrentQueue<InnerMarketTrade>* mtQueue_;
			ConcurrentQueue<Order>* orderQueue_;
			spsc_queue<QueueNode>* rmqReadQueue_;
			OrderController *odrCtrler_;
			//MarketDataManager *MdDataManager_;
			Initializer *initializer_;
		};

		inline ConcurrentQueue<InnerMarketData>* StrategyCooker::getMdQueue()
		{
			return mdQueue_;
		}
		inline ConcurrentQueue<InnerMarketTrade>* StrategyCooker::getMtQueue()
		{
			return mtQueue_;
		}
		inline ConcurrentQueue<Order>* StrategyCooker::getRtnOrderQueue()
		{
			return orderQueue_;
		}
		inline spsc_queue<QueueNode>* StrategyCooker::getRmqReadQueue()
		{
			return rmqReadQueue_;
		}
	}
}

#endif