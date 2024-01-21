#ifndef SPOT_STRATEGY_INSTRUMENTORDER_H
#define SPOT_STRATEGY_INSTRUMENTORDER_H
#include<iostream>
#include<string>
#include<map>
#include<queue>
#include<list>
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include "spot/base/DataStruct.h"
#include "spot/utility/Logging.h"
#include "spot/base/Adapter.h"
#include "spot/strategy/DealOrders.h"
using namespace std;
using namespace spot;
using namespace spot::base;
using namespace spot::utility;
using namespace spot::strategy;

namespace spot
{
	namespace strategy
	{
		typedef unordered_map<int, Order> RtnOrderMap;

		class InstrumentOrder
		{
		public:
			InstrumentOrder(const StrategyInstrumentPNLDaily &position, Adapter *adapter);
			~InstrumentOrder();			
			void setOrder(char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t cancelSize = 0);			
			void updateOrderByPrice(const Order &order);
			void checkSetOrder(char direction);
			inline OrderByPriceMap* getPendingOrders(char direction);
			double getTopPrice(char direction);
			double getBottomPrice(char direction);
			void cancelOrder(const Order &order);
			int query(const Order &order);
			int getCancelOrderSize(char direction);
            inline int getOrderSize(char direction);
		private:
			inline int getOrderType(char direction);
			void initSetOrderInfoMap(Order order);
			void initSetOrderInfo(SetOrderInfo *setOrderInfo);

		private:
			map<char, SetOrderInfo*>* setOrderInfoMap_;
			DealOrders *pendingOrders_;
		};
		inline int InstrumentOrder::getOrderSize(char direction)
		{
			return pendingOrders_->getOrderSize(direction);
		}
		inline OrderByPriceMap* InstrumentOrder::getPendingOrders(char direction)
		{
			return pendingOrders_->orderMap(direction);
		}
	}
}
#endif
