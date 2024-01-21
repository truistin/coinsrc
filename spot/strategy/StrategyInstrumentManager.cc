#include "spot/strategy/StrategyInstrumentManager.h"
#include "spot/base/InstrumentManager.h"
using namespace spot::base;

map<Instrument*, vector<StrategyInstrument*>> StrategyInstrumentManager::instStrategyInstMap_;
map<Instrument*, vector<StrategyInstrument*>> StrategyInstrumentManager::refInstStrategyInstMap_;
map<int, map<string, set<string>>> StrategyInstrumentManager::strategyInstRefInstMap_;
vector<StrategyInstrument*> StrategyInstrumentManager::emptyList_;
StrategyInstrumentManager::StrategyInstrumentManager()
{
}
StrategyInstrumentManager::~StrategyInstrumentManager()
{
}
void StrategyInstrumentManager::addRefInstrumentId(int strategyId, string instrumentId, string refInstrument)
{
	auto sIter = strategyInstRefInstMap_.find(strategyId);
	if (sIter != strategyInstRefInstMap_.end())
	{
		auto iIter = sIter->second.find(instrumentId);
		if (iIter != sIter->second.end())
		{
			iIter->second.insert(refInstrument);
		}
		else
		{
			set<string> refInstList;
			refInstList.insert(refInstrument);
			sIter->second[instrumentId] = refInstList;
		}
	}
	else
	{
		set<string> refInstList;
		refInstList.insert(refInstrument);
		map<string, set<string>> strategyInstMap;
		strategyInstMap[instrumentId] = refInstList;
		strategyInstRefInstMap_[strategyId] = strategyInstMap;
	}
}
void StrategyInstrumentManager::addStrategyInstrument(Instrument* instrument, StrategyInstrument* strategyInstrument)
{
	LOG_DEBUG << "addStrategyInstrument instrumentId:" << instrument->getInstrumentID() << " StrategyId:" << strategyInstrument->getStrategyID();
	//add instrument to StrategyInstrument map;
	auto iter = instStrategyInstMap_.find(instrument);
	if (iter == instStrategyInstMap_.end())
	{
		vector<StrategyInstrument*> vec;
		vec.push_back(strategyInstrument);
		instStrategyInstMap_[instrument] = vec;
	}
	else
	{
		iter->second.push_back(strategyInstrument);
	}
	//add refInstrument to StrategyInstrument map;

	auto sIter = strategyInstRefInstMap_.find(strategyInstrument->getStrategyID());
	if(sIter != strategyInstRefInstMap_.end())
	{
		auto iIter = sIter->second.find(instrument->getInstrumentID());
		if(iIter != sIter->second.end())
		{
			for (auto refIter : iIter->second)
			{
				auto refInstrument = InstrumentManager::getInstrument(refIter);
				if (refInstrument)
					addRefStrategyInstrument(refInstrument, strategyInstrument);
				else
					LOG_WARN << "cann't find instrument:" << refIter;
			}
		}
	}
}
void StrategyInstrumentManager::addRefStrategyInstrument(Instrument* instrument, StrategyInstrument* strategyInstrument)
{
	LOG_DEBUG << "addRefStrategyInstrument refInstrumentId:" << instrument->getInstrumentID() << " StrategyId:" << strategyInstrument->getStrategyID();
	auto iter = refInstStrategyInstMap_.find(instrument);
	if (iter == refInstStrategyInstMap_.end())
	{
		vector<StrategyInstrument*> vec;
		vec.push_back(strategyInstrument);
		refInstStrategyInstMap_[instrument] = vec;
	}
	else
	{
		iter->second.push_back(strategyInstrument);
	}
}
vector<StrategyInstrument*>& StrategyInstrumentManager::getStrategyInstrumentList(Instrument* instrument)
{
	auto iter = instStrategyInstMap_.find(instrument);
	if (iter == instStrategyInstMap_.end())
	{
		return emptyList_;
	}
	else
	{		
		return iter->second;
	}
}
vector<StrategyInstrument*>& StrategyInstrumentManager::getStrategyInstrumentListByRef(Instrument* instrument)
{
	auto iter = refInstStrategyInstMap_.find(instrument);
	if (iter == refInstStrategyInstMap_.end())
	{
		return emptyList_;
	}
	else
	{
		return iter->second;
	}
}
map<Instrument*, vector<StrategyInstrument*>>& StrategyInstrumentManager::strategyInstrumentMap()
{
	return instStrategyInstMap_;
}
void StrategyInstrumentManager::print()
{
	for (auto iIter : instStrategyInstMap_)
	{
		for (auto sIter : iIter.second)
		{
			LOG_DEBUG << "key:" << iIter.first->getInstrumentID() << " strategyId:"<< sIter->getStrategyID() << " instrumentId:" << sIter->getInstrumentID() << " ptr:" << sIter;
		}
	}
	for (auto iIter : refInstStrategyInstMap_)
	{
		for (auto sIter : iIter.second)
		{
			LOG_DEBUG << "ref key:" << iIter.first->getInstrumentID() << " strategyId:" << sIter->getStrategyID() << " instrumentId:" << sIter->getInstrumentID() << " ptr:" << sIter;
		}
	}

	for (auto iIter : strategyInstRefInstMap_)
	{
		for (auto sIter : iIter.second)
		{
			for (auto rIter : sIter.second)
			{
				LOG_DEBUG << "StrategyId:" << iIter.first << " InstrumentID:" << sIter.first << " RefInstrumentID: " << rIter;
			}
		}
	}

}
