#ifndef SPOT_BASE_PARAMETERSMANAGER_H
#define SPOT_BASE_PARAMETERSMANAGER_H
#include "spot/base/StrategyParameter.h"
#include<map>
#include "spot/strategy/Strategy.h"

namespace spot
{
	namespace base
	{
		class ParametersManager
		{
		public:
			ParametersManager();
			~ParametersManager();
			static StrategyParameter* getStrategyParameters(int strategyID);

			static void addStrategyParameter(const StrategyParam &params);

			static void initParameters();
			static map<int, StrategyParameter*>& strategyParametersMap();

		private:
			static map<int, StrategyParameter*> strategyParametersMap_;
		};
	}
}
#endif