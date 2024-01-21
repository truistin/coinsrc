#ifndef SPOT_STRATEGY_STRATEGYCIRCUIT_H
#define SPOT_STRATEGY_STRATEGYCIRCUIT_H

#include "spot/base/DataStruct.h"
#include "spot/strategy/Strategy.h"

namespace spot
{
	namespace strategy
	{
		class StrategyCircuit :public Strategy
		{
		public:
			StrategyCircuit(int strategyID, StrategyParameter *params);
			virtual void OnRtnInnerMarketData(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument);
			virtual void OnRtnInnerMarketTrade(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument);
			virtual void OnNewOrder(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			virtual void OnNewReject(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			virtual void OnPartiallyFilled(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			virtual void OnFilled(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			virtual void OnCanceled(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			virtual void OnCancelReject(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			//virtual void OnEndBinner(const BinInfo &binInfo);
			virtual void OnError();
			virtual bool turnOnStrategy();
			virtual bool turnOffStrategy();
			virtual bool turnOnInstrument(string instrumentID);
			virtual bool turnOffInstrument(string instrumentID);
			// Trading Logic
			virtual void OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnNewOrderTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument) {};
			virtual void OnNewRejectTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument) {};
			virtual void OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument) = 0;
			virtual void OnTimerTradingLogic() = 0;
			virtual void OnTimer();
			virtual void OnCheckCancelOrder();
			virtual int getCancelCount(StrategyInstrument* strategyInstrument) { return *cancelCountMap_[strategyInstrument]; }

			bool isOrderBookInitialised(StrategyInstrument* strategyInstrument) { return isOrderBookInitialised_[strategyInstrument]; }

			void checkTradeability();
			
			bool isSetOrderIntervalReady(StrategyInstrument *strategyInstrument, char direction);
			// bool isSetOrderIntervalReady(StrategyInstrument *strategyInstrument, double price, double quantity);
			// OrderFlag 0 req 1 cancel
			bool judgeBianReqCancelExceedCheck(StrategyInstrument *strategyInstrument, tm* ptm, bool OrderFlag, char direction, uint32_t& orderNum);
			bool judgeOkexReqCancelExceedCheck(StrategyInstrument *strategyInstrument, tm *ptm, bool OrderFlag,
                                                  char direction, uint32_t& orderNum);
			bool setOrder(StrategyInstrument* strategyInstrument, char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t operationSize = 0);

			void cancelOrdersByInstrument(StrategyInstrument* strategyInstrument);
			void cancelAllOrders();
			void turnOffInstrument(StrategyInstrument* strategyInstrument);

			void initCount();

			void checkOrderBookInitialised(StrategyInstrument* strategyInstrument);

			bool checkMarketDataConsistency(StrategyInstrument* strategyInstrument);

			void checkPendingCancelTimeout(StrategyInstrument* strategyInstrument);

			void checkPendingOrderTimeout(StrategyInstrument* strategyInstrument);
			bool checkOrderBookStale(StrategyInstrument* strategyInstrument);

			bool checkMaxPosition(StrategyInstrument* strategyInstrument, double coinOrderSize);

			void checkTradingStopTime();
			bool checkLiveOrderRestrictions();

			void checkMaxTrades(StrategyInstrument* strategyInstrument);
//			void checkMaxCancels(StrategyInstrument* strategyInstrument);
//			void checkMaxRejects(StrategyInstrument* strategyInstrument);

		//	bool checkOrderSelfCrossing(StrategyInstrument* strategyInstrument, char direction, double orderPrice);
			void checkCancelRejects();
			bool calcCancelTimesByOkBian(StrategyInstrument *strategyInstrument, tm *ptm, bool OrderFlag,
                                                  char direction, uint64_t currentEpochMs, uint32_t& cancelSize);

		private:
			bool isCountInit_;
			std::mutex bianMtx_; // bian order times calc
			std::mutex okexMtx_; // bian order times calc

			map<StrategyInstrument*, int*> cancelCountMap_;
			map<StrategyInstrument*, int*> reduceOnlyMap_;

			map<StrategyInstrument*, bool> isOrderBookInitialised_;

			map<StrategyInstrument*, long long> setBuyOrderEpochMs_;
			map<StrategyInstrument*, long long> setSellOrderEpochMs_;
			map<StrategyInstrument*, long long> lastOrderTimeStampMs_;

			map<StrategyInstrument*, int> tradeCount_;
			map<StrategyInstrument*, int> cancelCount_;
			map<StrategyInstrument*, int> rejectCount_;

			map<const Order*, StrategyInstrument*> cancelRejectsMap_;

		};
	}
}
#endif
