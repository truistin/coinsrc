#ifndef SPOT_BASE_JSONINI_H
#define SPOT_BASE_JSONINI_H
#include "spot/base/DataStruct.h"
#include "spot/utility/SPSCQueue.h"
#include "spot/utility/FileWriter.h"
#include "spot/restful/uri.h"
#include "spot/restful/curlprotocol.h"
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/rapidjson.h>
using namespace std;
using namespace spot::base;
using namespace spot::utility;


namespace spot
{
	namespace strategy
	{
		class BackTestInit
		{
		public:
			BackTestInit(string urlBase,int spotID,string tradingDay, string initFilePath, spsc_queue<QueueNode> *initQueue, string sourcePath);
			BackTestInit(int spotID, spsc_queue<QueueNode> *initQueue);
			~BackTestInit();
			void getInitData();
			void getSpotConfig();
			void getFactorValue();
			void getSpotInitConfig();			
			void getStrategyInstrumentPNLDaily();		
			void getStrategyInfoList();
			std::list<StrategySymbol>* getStrategySymbolList();
			void getStrategyParamList();
			void getStrategyParamExtList();

			//name??.json
			bool hasJson(const char* name);
			void getJson(const char* name);
			void getJson_SignalSymbol(std::list<SignalSymbol>& signalSymbolList);

			void getSymbolInfoList(const std::list<StrategySymbol>& strategySymbolList, int strategyID);
			void getRefSymbolInfoList(const std::list<SignalSymbol>& signalSymbolList);
			void getSymbolTradingFeeList(const std::list<StrategySymbol>& strategySymbolList, int strategyID);			
			void getRefSymbolTradingFeeList(const std::list<SignalSymbol>& signalSymbolList);
			void getSymbolTradingPeriodList(const std::list<StrategySymbol>& strategySymbolList, int strategyID);
			void getRefSymbolTradingPeriodList(const std::list<SignalSymbol>& signalSymbolList);
			void getTradeableInstrumentList(const std::list<StrategySymbol>& strategySymbolList, int strategyID);
			void getOptionInstrumentList(const std::list<StrategySymbol>& strategySymbolList, int strategyID);
			void getRefTradeableInstrumentList(const std::list<SignalSymbol>& signalSymbolList);
			void getStrategyInstrumentPNLDailyList(int strategyID);

			void getSignalList(const std::list<StrategySignal>& strategySignalList, int strategyID);
			std::list<StrategySignal>* getStrategySignalList();
			void getSignalParamList(const std::list<StrategySignal>& strategySignalList, int strategyID);
			std::list<SignalSymbol>* getSignalSymbolList(const std::list<StrategySignal>& strategySignalList, const std::list<StrategySymbol>& strategySymbolList, int strategyID);

			void getSignalList(const std::list<StrategyInstrumentSignal>& strategyInstrumentSignalList, int strategyID);			
			void getSignalParamList(const std::list<StrategyInstrumentSignal>& strategyInstrumentSignalList, int strategyID);
			std::list<SignalSymbol>* getSignalSymbolList(const std::list<StrategyInstrumentSignal>& strategyInstrumentSignalList, const std::list<StrategySymbol>& strategySymbolList, int strategyID);
			std::list<StrategyInstrumentSignal>* getStrategyInstrumentSignalList();

			bool isStrategyInstrumentSignalUsed();

			void getStrategySymbolTargetList(int strategyID);
			void getStrategyIndexList(int strategyID);
			void getIndexInfo(int strategyID);
			map<string, list<string>> getTdInfo();
			map<string, list<string>> getMdInfo();
			map<string, string> getReplayParam();
			void sendFinished();
		private:
			map<string, list<string>> decodeTdInfo(spotrapidjson::Value &arrayValue);
			map<string, list<string>> decodeMdInfo(spotrapidjson::Value &arrayValue);
			map<string, string> decodeReplayParam(spotrapidjson::Value &arrayValue);
			string getAdjustTradingDay(const string& symbol);
			SymbolInfo* getSymbolInfo(const string& symbol);
		private:
			string urlBase_;
			string tradingDay_;
			string initFilePath_;
			int spotID_;
			list<int> strategyList_;
			spsc_queue<QueueNode> *initQueue_;
			string sourcePath_;
			map<string, SymbolInfo> symbolInfos_;
		};
	}

/*
		BackTestInit backtestInit(restfulAddr, spotid, TradingDay::getString(), "../config/", spinner.getRmqReadQueue(), sourcePath);
		backtestInit.getInitData();
*/

}

