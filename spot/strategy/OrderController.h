#ifndef SPOT_STRATEGY_ORDERCONTROLER_H
#define SPOT_STRATEGY_ORDERCONTROLER_H
#include<iostream>
#include<string>
#include<map>
#include<list>
#include "spot/base/DataStruct.h"
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Logging.h"
#include "spot/strategy/InstrumentOrder.h"
#include "spot/base/Adapter.h"
#include "spot/strategy/Strategy.h"
using namespace std;
//using namespace spot;
using namespace spot::base;
using namespace spot::utility;
using namespace spot::strategy;
namespace spot
{
	namespace strategy
	{
		struct OrderStatusMapping
		{
			char currentStatus;
			char nextStatus;
		};
		static const OrderStatusMapping OrderStatusMappingArray[] =
		{
			{ PendingNew,New },
			{ PendingNew,NewRejected },
			{ PendingNew,Partiallyfilled },
			{ PendingNew,Filled },
			{ PendingNew,Cancelled },

			{ New,NewRejected },
			{ New,Partiallyfilled },
			{ New,Filled },
			{ New,Cancelled },

			{ Partiallyfilled,Filled },
			{ Partiallyfilled,Cancelled },
			{ Partiallyfilled,Partiallyfilled },

			{ PendingCancel,Cancelled },
			{ PendingCancel,CancelRejected },
			{ PendingCancel,Partiallyfilled },
			{ PendingCancel,Filled },

			{ CancelRejected,Partiallyfilled },
			{ CancelRejected,Filled },
			{ CancelRejected,Cancelled },
		};
		typedef map<pair<char, char>, char> OrderStatusMappingMap;
		class OrderController:public Adapter
		{
		public:
			OrderController();
			~OrderController();

			void setAdapter(Adapter *adapter);
			void initOrderStatusMappingMap();
			static int initOrderRef(int spotID, string tradingDay, int tradingSessionLength = 168);//168 hours = 7 days
			
			void insertOrder(const Order& order);
			int reqOrder(Order &order) final;
			int cancelOrder(Order &order) final;
			void OnRtnOrder(Order &rtnOrder);
			int query(const Order &order) final;

			static int getOrderRef();
			static const RtnOrderMap& orderMap();

			bool isInvalidOrderStateTransition(char currentOrderStatus, char newOrderStatus);
			void updateTrade(Order &rtnOrder, Order &cacheOrder);
			void updateCurrentTrade(Order &rtnOrder, Order &cacheOrder);
			void updateOrder(const Order &rtnOrder, Order &cacheOrder);
			bool isCanceledOrderBeforePartiallyfilled(Order &rtnOrder,Order &cacheOrder);
			bool checkCancelBeforePartiallyfilledOrder(Order &cacheOrder);
			void OnStrategyRtnOrder(Order &rtnOrder, Order &cacheOrder);
		private:
			static int orderRef_;
			static Adapter *adapterGateway_;
			static RtnOrderMap orderMap_;
			OrderStatusMappingMap orderStatusMappingMap_;
			RtnOrderMap canceledBeforePartiallyfilledOrderMap_;
		};
	}
}
#endif
