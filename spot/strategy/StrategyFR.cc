#include "StrategyFR.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Measure.h"
#include "spot/base/InitialData.h"
#include "spot/cache/CacheAllocator.h"
#include "spot/utility/TradingDay.h"

using namespace spot::strategy;
// using namespace spot::risk;

Strategy *StrategyFR::Create(int strategyID, StrategyParameter *params) {
    char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyFR));
    return new(buff) StrategyFR(strategyID, params);
}

StrategyFR::StrategyFR(int strategyID, StrategyParameter *params)
        : StrategyCircuit(strategyID, params) 
{
}

void StrategyFR::init() 
{
}

void StrategyFR::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}