#ifndef SPOT_STRATEGY_DEAL_ORDERS_API_H
#define SPOT_STRATEGY_DEAL_ORDERS_API_H
#include "spot/base/MqStruct.h"
#include "spot/base/DataStruct.h"
#include "spot/base/Adapter.h"
#include "spot/base/Instrument.h"
using namespace spot::base;
namespace spot
{
	namespace strategy
	{
		class DealOrdersApi
		{
		public:
			DealOrdersApi(const StrategyInstrumentPNLDaily &position,Adapter *adapter);
			~DealOrdersApi();
			void newOrder(double closeTodayVolume, SetOrderInfo *setOrderInfo);
			void splitBuyOrders(Order &order, double closeToday);
			void checkBuyCloseTAndCloseYCost(Order &order, double closeToday);
			void splitSellOrders(Order &order, double closeToday);
			void checkSellCloseTAndCloseYCost(Order &order, double closeToday);
			inline void openOrder(Order &order);
			inline void closeTodayOrder(Order &order);
			int reqOrder(Order &order);
			
			void initOffsetPriority();
			bool isReqOrderIntervalReady(char direction);
			bool isReqOrderIntervalReadyWithDir();
		private:
			const StrategyInstrumentPNLDaily &position_;
			Adapter *adapterPendingOrder_;
			Instrument *instrument_;
			SetOrderInfo *setOrderInfo_;
			map<char, long long> reqOrderEpochMsMap_;
			const int* cbMinSetOrderIntervalMs_;
			long long orderTimeStamp_;
		};

	}
}
#endif
