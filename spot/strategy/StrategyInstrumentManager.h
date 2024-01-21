#ifndef SPOT_STRATEGY_STRATEGYINSTRUMENTMANAGER_H
#define SPOT_STRATEGY_STRATEGYINSTRUMENTMANAGER_H
#include "spot/base/Instrument.h"
#include<map>
#include<set>
#include "spot/strategy/Strategy.h"
#include "spot/base/TradingPeriod.h"
#include "spot/strategy/StrategyInstrument.h"
class StrategyInstrumentManager
{
public:
	StrategyInstrumentManager();
	~StrategyInstrumentManager();
	static void addRefInstrumentId(int strategyId, string instrumentId, string refInstrument);
	static void addStrategyInstrument(Instrument* instrument, StrategyInstrument* strategyInstrument);
	static void addRefStrategyInstrument(Instrument* instrument, StrategyInstrument* strategyInstrument);
	static vector<StrategyInstrument*>& getStrategyInstrumentList(Instrument* instrument);
	static vector<StrategyInstrument*>& getStrategyInstrumentListByRef(Instrument* instrument);
	static void print();
	static map<Instrument*, vector<StrategyInstrument*>>& strategyInstrumentMap();
private:
	static map<Instrument*, vector<StrategyInstrument*>> instStrategyInstMap_;
	static map<Instrument*, vector<StrategyInstrument*>> refInstStrategyInstMap_;
	static map<int, map<string, set<string>>> strategyInstRefInstMap_;
	static vector<StrategyInstrument*> emptyList_;
};
#endif