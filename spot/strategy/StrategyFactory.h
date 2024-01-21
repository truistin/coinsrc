#ifndef SPOT_STRATEGY_STRATEGYFACTORY_H
#define SPOT_STRATEGY_STRATEGYFACTORY_H
#include "spot/utility/Compatible.h"
#include "spot/strategy/Strategy.h"
using namespace spot;

namespace spot
{
	namespace strategy
	{
		typedef Strategy*(*StrategyCreateFunc)(int strategyID, StrategyParameter *params);
		class StrategyFactory
		{
		public:
			StrategyFactory();
		    ~StrategyFactory();
			static void registerStrategys();
			static bool registerStrategy(string strategyName, StrategyCreateFunc func);
			static bool addStrategy(int strategyID,string strategyName);
			static void initStrategy();
			static Strategy* getStrategy(int strategyID);
			static map<int, Strategy*>& strategyMap();
		private:
			static map<string, StrategyCreateFunc> regs_; //key:strategyName
			static map<int,Strategy*> strategyMap_; //key:strategyID
		};
	}
}
#endif
