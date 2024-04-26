#include "spot/strategy/StrategyFactory.h"
#include "spot/base/ParametersManager.h"
#include "spot/strategy/Initializer.h"
#include "spot/strategy/StrategyUCArb.h"
#include "spot/strategy/StrategyUCArb1.h"
#include "spot/strategy/StrategyFR.h"


using namespace spot;
using namespace spot::strategy;

map<int, Strategy*> StrategyFactory::strategyMap_;
map<string, StrategyCreateFunc> StrategyFactory::regs_;
StrategyFactory::StrategyFactory()
{
}
StrategyFactory::~StrategyFactory()
{
	for (auto strategyIter : strategyMap_)
	{
		delete strategyIter.second;
	}
	strategyMap_.clear();
}
bool StrategyFactory::registerStrategy(string strategyName, StrategyCreateFunc func)
{
	auto it = regs_.find(strategyName);
	if (it != regs_.end())
	{
		LOG_WARN << "registerStrategy failed, strategyName:" << strategyName;
		return false;
	}

	regs_[strategyName] = func;
	LOG_DEBUG << "registerStrategy success, strategyName:" << strategyName;
	return true;
}
void StrategyFactory::initStrategy()
{
	for (auto strategyIter : strategyMap_) {
		strategyIter.second->init();
	}
}
void StrategyFactory::registerStrategys()
{
	// registerStrategy("FR", StrategyFR::Create);
	registerStrategy("arb1", StrategyUCArb1::Create); 
}

bool StrategyFactory::addStrategy(int strategyID, string strategyName)
{
	// return 1; // test
	auto parameters = ParametersManager::getStrategyParameters(strategyID);

	if (!parameters)
	{
		LOG_FATAL << "addStrategy cann't find strategy parameters. strategyID:" << strategyID;
		return false;
	}

	auto it = regs_.find(strategyName);
	if (it != regs_.end())
	{
		StrategyCreateFunc func = it->second;
		Strategy* strategy = func(strategyID, parameters);
		strategy->setStrategyName(strategyName);
		strategyMap_[strategyID] = strategy;
	}
	else
	{
		LOG_FATAL << "addStrategy unknown strategyName. strategyName:" << strategyName.c_str();
		return false;
	}
	return true;
}

Strategy* StrategyFactory::getStrategy(int strategyID)
{
	auto strategyIter = strategyMap_.find(strategyID);
	if (strategyIter != strategyMap_.end())
	{
		return strategyIter->second;
	}
	return nullptr;
}
map<int, Strategy*>& StrategyFactory::strategyMap()
{
	return strategyMap_;
}
