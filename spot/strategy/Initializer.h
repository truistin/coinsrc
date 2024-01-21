#ifndef SPOT_STRATEGY_INITIALIZER_H
#define SPOT_STRATEGY_INITIALIZER_H
#include<vector>
#include<unordered_map>
#include "spot/utility/Utility.h"
#include "spot/base/DataStruct.h"
#include "spot/base/TradingPeriod.h"
#include "spot/base/Instrument.h"
#include "spot/utility/SPSCQueue.h"
using namespace std;
using namespace spot::base;
using namespace spot::utility;

namespace spot
{
	namespace strategy
	{
		class Initializer
		{
		public:
			Initializer();
			Initializer(int spotID);
			~Initializer();
			bool init(spsc_queue<QueueNode> *initQueue, bool isMarketData = false);
			bool getInitData(spsc_queue<QueueNode> *initQueue, bool isMarketData = false);
			void sendInitInfo();
			bool initAll();
			bool initSymbol(int strategyID);
			bool initInstrument(int strategyID, string symbol);
			//bool initOptionInstrument(int strategyID, string symbol);
			void initStrategyInstrument(int strategyID, string instrumentID);

			void saveSpotProductInfo(const QueueNode& node);
			void saveSpotTdInfo(const QueueNode& node);
			void saveSpotMdInfo(const QueueNode& node);
			void saveSpotInitConfig(list<SpotInitConfig>* spotInitConfigList);
			void saveSpotStrategy(list<SpotStrategy>* spotStrategyList);
			void saveStrategyInfo(list<StrategyInfo>* strategyList);
			void saveStrategySymbol(list<StrategySymbol>* strategySymbolList);
			void saveSymbolInfo(list<SymbolInfo>* symbolList);

			void saveSymbolInstrument(list<SymbolInstrument>* tradeableInstList);
			
			void saveStrategyParam(list<StrategyParam>* strategyParamList);
			void saveSymbolTradingFee(list<SymbolTradingFee>* symbolTradingFeeList);
			void saveStrategyInstrumentPNLDaily(list<StrategyInstrumentPNLDaily>* strategyInstPNLList);

			void saveInstrumentTradingFee(list<InstrumentTradingFee>* tradingFeeList);

			bool addTradingFees(const string& symbol, Instrument* instrument);
			bool addInstrument(int strategyID, string symbol, string instrumentID, string tradeableType);
			//void addTradeableOptionInstrument(string symbol, SymbolInstrument& tadeableInstr);

			vector<string>				getInstrumentList();
			vector<string>				getInstrumentList(list<string> exchangeList);
			//return the tradeable instrument id for a given symbol and type i.e. primary or secondary	
			string						getSymbolInstrumentID(string symbol, string type, string symbolType);
			list<string>				getOKExTradeableInstrumentID(string symbol, string type, string symbolType);
			static void					getInstrumentListByStrategyID(int strategyID, list<string>& instrumentList);
			static Instrument* getSymbolInstrument(string symbol, string tradeableType);

			//void validateConfigList(list<SpotConfig>& configList);
			void validateTdInfoList(list<SpotTdInfo>& tdInfoList);
			void validateMdInfoList(list<SpotMdInfo>& mdInfoList);

		private:
			static map<int, list<StrategySymbol>> strategySymbolMap_;
			static map<int, list<string>> strategyInstrumentMap_;
			static map<string, map<string, SymbolInstrument>> symbolInstrumentMap_;


			map<string, SymbolInfo> symbolInfoMap_;
			map<string, map<string, SymbolTradingFee>> symbolTradingFeeMap_;
			map<string, map<string, InstrumentTradingFee>> instTradingFeeMap_;

			bool isSPOTMD_ = false;
			static string strategyIDs_;
		};
	}
}
#endif
