#ifndef SPOT_BASE_INITIALDATA_H
#define SPOT_BASE_INITIALDATA_H
#include<map>
#include<list>
#include<set>
#include<string>
#include "spot/base/MqStruct.h"
using namespace std;
namespace spot
{
	namespace base
	{
		enum 
		{
			TradeType = 0,
			MdType = 1
		};
		class InitialData
		{
		public:
			InitialData() {};
			~InitialData() {};
			//static void addSpotConfig(SpotConfig &spotConfig);
			static void addSpotProductInfo(SpotProductInfo &spotProductInfo);
			static void addSpotTdInfo(SpotTdInfo &spotTdInfo);
			static void addSpotMdInfo(SpotMdInfo &spotMdInfo);
			static void addStrategyInfo(StrategyInfo &strategyInfo);
			static void addStrategyId(int strategyID);
			static void addStrategySymbol(StrategySymbol &strategySymbol);
			static void addSymbolInfo(SymbolInfo &symbolInfo);
			static void addSpotInitConfig(list<SpotInitConfig> &configList);
			static void addPnlDaily(StrategyInstrumentPNLDaily &pnl);
			static bool inSymbolList(const char* symbol, const list<StrategySymbol>& symbolList);
			static bool inStrategySymbol(const char* symbol);

			//static map<char, SpotConfig>& getInterfaceSpotConfigMap();
			static map<int, SpotProductInfo>& getSpotProductInfoMap();
			static map<string, map<int,list<SpotTdInfo>>>&  getInterfaceSpotTdInfoListMap();
			static map<string, map<int,list<SpotMdInfo>>>& getInterfaceSpotMdInfoListMap();

			static string getProductName(int spotID);
			static string getInvestorID(const char* interfaceType,  int type, int interfaceID = 0);
			static string getUserID(const char* interfaceType, int type, int interfaceID = 0);
			static map<int, StrategyInfo>& strategyInfoMap() { return strategyInfoMap_; };
			static set<int>& strategyList() { return strategyList_; };
			static map<int, list<StrategySymbol>>& strategySymbolMap() { return strategySymbolMap_; };
			static map<string, SpotInitConfig>& spotInitConfigMap() { return spotInitConfigMap_; };
			static int getTimerInterval() { return timerInterval_; };
			static int getForceCloseTimerInterval() { return timerForceCloseInterval_; };
			static int getBianListernKeyInterval() { return BianListenKeyInterval_; };
			static int getBybitQryInterval() { return BybitQryInterval_; };
			static int getOkPingPongInterval() { return OkPingPongInterval_; };
			static int getByPingPongInterval() { return ByPingPongInterval_; };
			static int getLastPriceDeviation() { return lastPriceDeviation_; };
			static int getL5Bid1Ask1Deviation() { return l5Bid1Ask1Deviation_; };
			static int getOrderQueryIntervalMili() { return orderQueryIntervalMili_; }

			static int getTimerStartSec() { return timerStartSec_; }
			static int getMqRecvTimeOut() { return mqRecvTimeOut_; }

			static int getSpotID() { return spotID_; }
			static void setSpotID(int id) { spotID_ = id; }
			static StrategyInstrumentPNLDaily* getPNLDaily(int strategyID, string instrumentID);
			static map<string, SymbolInfo> symbolInfoMap();
			static SymbolInfo* getSymbolInfo(const string& symbol);
		private:
			//static map<char, SpotConfig> interfaceSpotConfigMap_;

			static map<int, SpotProductInfo> spotProductInfoMap_;
			static map<string, map<int,list<SpotTdInfo>>> interfaceSpotTdInfoListMap_;
			static map<string, map<int,list<SpotMdInfo>>> interfaceSpotMdInfoListMap_;
			static map<int, StrategyInfo> strategyInfoMap_;
			static set<int> strategyList_;
			static map<int, list<StrategySymbol>> strategySymbolMap_;
			static map<string, SymbolInfo> symbolInfoMap_;
			static map<string, SpotInitConfig> spotInitConfigMap_;
			static map<int, map<string, StrategyInstrumentPNLDaily>> strategyInstPNLMap_;
			static int timerInterval_;
			static int timerForceCloseInterval_;
			static int coloTimeDeviation_;
			static int BianListenKeyInterval_;
			static int BybitQryInterval_;
			static int OkPingPongInterval_;
			static int ByPingPongInterval_;
			static int lastPriceDeviation_;
			static int l5Bid1Ask1Deviation_;
			static int orderQueryIntervalMili_;
			static int spotID_;

			static int timerStartSec_;
			static int mqRecvTimeOut_;
		};
	}
}
#endif