#ifndef SPOT_STRATEGY_STRATEGY_H
#define SPOT_STRATEGY_STRATEGY_H
#include<list>
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include "spot/base/DataStruct.h"
#include "spot/base/StrategyParameter.h"
#include "spot/strategy/StrategyInstrument.h"
#include "spot/base/Adapter.h" 
#include "spot/base/ParametersManager.h"
#include "spot/cache/CacheAllocator.h"

using namespace std;
using namespace spot;
using namespace spot::base;
using namespace spot::utility;
namespace spot
{
	namespace strategy
	{
		class StrategyInstrument;
		class Strategy
		{
		public:
			Strategy(int strategyID, StrategyParameter *params);
			virtual ~Strategy();
			virtual void init() = 0;
			virtual void OnRtnInnerMarketData(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnRtnInnerMarketTrade(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument) = 0;

			virtual void OnNewOrder(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnNewReject(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnPartiallyFilled(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnFilled(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnCanceled(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnCancelReject(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnError() = 0;
			virtual bool turnOffStrategy() = 0;
			virtual bool turnOffInstrument(string instrumentID) = 0;
			virtual bool turnOnStrategy() = 0;
			virtual bool turnOnInstrument(string instrumentID) = 0;
			virtual bool setOrder(StrategyInstrument *strategyInstrument, char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t operationSize = 0);
			virtual void OnTimer() = 0;
			virtual void OnForceCloseTimerInterval() {};
			virtual void OnCheckCancelOrder() {};
			virtual int getCancelCount(StrategyInstrument* strategyInstrument) = 0;

			int getStrategyID();
			StrategyParameter* parameters();
			const list<StrategyInstrument*>& strategyInstrumentList();
			void addStrategyInstrument(const char* instrumentID, Adapter *adapter);
			void OnRtnOrder(const Order &order);
			bool isTradeable() { return isTradeable_; }
			void setTradeable(bool isTradeable) { isTradeable_ = isTradeable; }
			void setStrategyName(const string& strategyName) { strategyName_ = strategyName; }
			const string& getStrategyName() { return strategyName_; }

			typedef map<string, double> DOUBLEMAP;

			map<Instrument*,StrategyInstrument*> instrumentStrategyInstrumentMap_;

			bool hasUnfinishedOrder();
			StrategyInstrument* getStrategyInstrument(Instrument* instrument);

			bool JudgeBianOrdersCountbyDurationOneMin(tm* ptm, int orderLimitCount, uint32_t& orderNum);
			bool JudgeBianOrdersCountbyDurationTenSec(int orderLimitCount, int bianTimeStep, int secondCount, uint32_t& orderNum);

			bool JudgeOkCancelCountbyDuration2s(tm* ptm, int orderLimitCount, int okTimeStep, uint32_t& cancelSize);
			bool JudgeOkReqCountbyDuration2s(tm* ptm, int orderLimitCount, int okTimeStep, int reqSize);

			void getCurrentPtm(tm& ptm);
		
		public:
			int bianOrdersPerMin_; // orders sum per min
			int bianOrdersPerTenSec_; // orders sum consective 10 secs

			uint32_t bianOrderOneMinSum_;
			uint32_t lastBaMin_;
			//vector<int> *bianOrderTenSecVec_; // sub: sec, value: orderCount


    		int bianOrderSecStep_; //10s
			int okexOrdersPer2kMsec_;
			int okexOrder2kMsecStep_;

			int lastSecIndex_;
			uint32_t bian10SecOrders_[6];

			// 60/2s, firstOrder : last firstOrder timestamp, second : 2s right threshold timeStamp, third : current Oders num
			vector<uint64_t> okReqLimitVec_; 
			vector<uint64_t> okCancelLimitVec_; 

		private:
			void actionOrder(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
		private:
			std::mutex okMtx_; // ok cancle order times calc
			int strategyID_;
			string strategyName_;
			bool isTradeable_;
			StrategyParameter *parameters_;
			list<StrategyInstrument*> strategyInstrumentList_;
			map<std::string, double> symbolTargetQtyMap_;
			tm* lastPtm_;
		};

	}
}
#endif
