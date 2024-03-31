#include "spot/strategy/MarketDataManager.h"
#include "spot/strategy/StrategyFactory.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/TradingPeriod.h"
#include "spot/base/InitialData.h"
using namespace spot;
using namespace spot::base;
using namespace spot::strategy;
using namespace spot::utility;

MarketDataManager::MarketDataManager()
{
}
MarketDataManager::~MarketDataManager()
{
}
MarketDataManager& MarketDataManager::instance()
{
	static MarketDataManager instance;
	return instance;
}

void MarketDataManager::OnRtnInnerMarketData(InnerMarketData &marketData)
{
	LOG_DEBUG << "MarketDataManager OnRtnInnerMarketData BidPrice1: "
	 	<< marketData.BidPrice1
        << ", AskPrice1: " << marketData.AskPrice1
		<< ", InstrumentID: " << marketData.InstrumentID;
	// gTickToOrderMeasure->addMeasurePoint(0, marketData.UpdateMillisec);
	// gTickToOrderMeasure->addMeasurePoint(1);
	auto instrument = InstrumentManager::getInstrument(marketData.InstrumentID);
	if (!instrument)
	{
		LOG_WARN << "invalid market data. instrumentID:" << marketData.InstrumentID;
		return;
	}

	auto newMarketData = instrument->setOrderBook(marketData);

	// LOG_INFO << "MarketDataManager ::OnRtnInnerMarketData "
	//         << ", marketData.BidPrice1: " << newMarketData.BidPrice1
    //     << ", marketData.AskPrice1: " << newMarketData.AskPrice1
	// 	<< newMarketData.InstrumentID;

	evaluateStrategy(*newMarketData,instrument);
}

void MarketDataManager::OnRtnInnerMarketTrade(InnerMarketTrade &marketTrade)
{
	LOG_DEBUG << "MarketDataManager OnRtnInnerMarketTrade Price: " << marketTrade.Price
		<< ", inst: " << marketTrade.InstrumentID;
	auto instrument = InstrumentManager::getInstrument(marketTrade.InstrumentID);
	if (!instrument)
	{
		LOG_WARN << "invalid market trade. instrumentID:" << marketTrade.InstrumentID;
		return;
	}

	// if (!validateMarketTrade(marketTrade, instrument))
	// 	return;

	auto newMarketData = instrument->setOrderBook(marketTrade);

	evaluateStrategy(marketTrade, instrument);
}

void MarketDataManager::evaluateStrategy(const InnerMarketData &marketData, Instrument *instrument)
{
	// gTickToOrderMeasure->addMeasurePoint(2);
	auto strategyMap = StrategyFactory::strategyMap();

	for (auto strategyIter : strategyMap)
	{
		//if not found in instrumentStrategyInstrumentMap_ this instrument must be a ref symbol market data
		//iter->second is a strategyinstrument
		auto iter = strategyIter.second->instrumentStrategyInstrumentMap_.find(instrument);
		if (iter != strategyIter.second->instrumentStrategyInstrumentMap_.end())
		{
			strategyIter.second->OnRtnInnerMarketData(marketData, iter->second);
		}

	}
}

void MarketDataManager::evaluateStrategy(const InnerMarketTrade &marketTrade, Instrument *instrument)
{
	//gTickToOrderMeasure->addMeasurePoint(3);
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		//if not found in instrumentStrategyInstrumentMap_ this instrument must be a ref symbol market trade
		//iter->second is a strategyinstrument
		auto iter = strategyIter.second->instrumentStrategyInstrumentMap_.find(instrument);
		if (iter != strategyIter.second->instrumentStrategyInstrumentMap_.end())
		{
			strategyIter.second->OnRtnInnerMarketTrade(marketTrade, iter->second);
		}

	}
}

bool MarketDataManager::validateMarketData(InnerMarketData& marketData, Instrument *instrument)
{
	return true;
}

bool MarketDataManager::validateMarketTrade(InnerMarketTrade& marketTrade, Instrument *instrument)
{
	return true;
}
