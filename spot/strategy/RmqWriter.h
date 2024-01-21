#ifndef SPOT_STRATEGY_RMQWRITER_H
#define SPOT_STRATEGY_RMQWRITER_H
#include "spot/base/DataStruct.h"
#include "spot/utility/SPSCQueue.h"

using namespace std;
using namespace spot::base;
using namespace spot::utility;
namespace spot
{
	namespace strategy
	{
		class RmqWriter
		{
		public:
			RmqWriter();
			~RmqWriter();
			static spsc_queue<QueueNode> * getRmqWriterQueue();
			static void enqueue(const QueueNode &node);
			static void writeOrder(const Order &order);
			static void writePNLDaily(const StrategyInstrumentPNLDaily& pnlDaily);
			static void writeStrategyInstrumentTradeable(int strategyID, string instrumentID, bool tradeable);
			static void writeStrategyParam(const StrategyParam &strategyParam);
		private:
			static spsc_queue<QueueNode> *rmqWriterQueue_;
		};
	}
}
#endif
