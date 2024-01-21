#ifndef SPOT_BASE_INSTRUMENTMANAGER_H
#define SPOT_BASE_INSTRUMENTMANAGER_H
#include "spot/base/Instrument.h"
#include<map>
#include "spot/strategy/Strategy.h"

namespace spot
{
	namespace base
	{
		class InstrumentManager
		{
		public:
			InstrumentManager();
			~InstrumentManager();
			static Instrument* getInstrument(string instrumentID);
			static Instrument* addInstrument(string instrumentID, const SymbolInfo &symbolInfo);
			static map<string, Instrument*> getInstrumentMap();
			static vector<string> getInstrumentList(list<string> exchangeList);
			static list<string> getAllInstruments();
		private:
			static map<string, Instrument*> instrumentMap_;
			static map<string, list<SymbolInstrument>> optionTradeableInstrumentMap_;

		};
	}
}
#endif