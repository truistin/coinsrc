#ifndef SPOT_STRATEGY_MDDATAMANAGER_H
#define SPOT_STRATEGY_MDDATAMANAGER_H
#include<iostream>
#include<string>
#include<map>
#include<list>
#include "spot/base/DataStruct.h"
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Logging.h"
#include "spot/strategy/InstrumentOrder.h"
#include "spot/base/Adapter.h"
#include "spot/strategy/Strategy.h"
using namespace std;
using namespace spot;

namespace spot
{
	namespace strategy
	{

		class MarketDataManager
		{
		public:
			MarketDataManager();
			~MarketDataManager();
			static MarketDataManager& instance();
			void OnRtnInnerMarketData(InnerMarketData &marketData);
			void OnRtnInnerMarketTrade(InnerMarketTrade &marketTrade);
			void evaluateStrategy(const InnerMarketData &marketData, Instrument *instrument);
			void evaluateStrategy(const InnerMarketTrade &marketTrade, Instrument *instrument);
			bool validateMarketData(InnerMarketData& marketData, Instrument *instrument);
			bool validateMarketTrade(InnerMarketTrade& marketTrade, Instrument *instrument);

		private:
			map<Instrument*, bool> isLastPriceDeviated;
		};
	}
}
#endif
