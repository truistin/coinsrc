#ifndef SPOT_BASE_TRADINGPERIODMANAGER_H
#define SPOT_BASE_TRADINGPERIODMANAGER_H
#include "spot/base/TradingPeriod.h"
#include<unordered_map>

namespace spot
{
	namespace base
	{
		class TradingPeriodManager
		{
		public:
			TradingPeriodManager();
			~TradingPeriodManager();

//			static void checkTradingPeriod();
//			static void setIsOutTradingPeriodFlag(bool flag);
//			static TradingPeriod* getTradingPeriod(string symbol);
		private:
			static unordered_map<string, TradingPeriod*> tradingPeriodMap_;
		};
	}
}
#endif