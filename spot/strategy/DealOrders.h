#ifndef SPOT_STRATEGY_DEAL_ORDERS_H
#define SPOT_STRATEGY_DEAL_ORDERS_H
#include "spot/base/MqStruct.h"
#include "spot/base/DataStruct.h"
#include "spot/strategy/DealOrdersApi.h"
#include "spot/base/Adapter.h"
#include<string>
#include<map>
#include<list>
#include<spot/utility/Utility.h>

using namespace spot::base;
using namespace spot::utility;
namespace spot
{
	namespace strategy
	{
		struct OrderByPrice
		{
			double Volume;  // 挂单量
			double VolumeTotal;
			double VolumePendingCancel;
			double VolumeCloseToday; // 今仓挂单量
			double Price;
			list<Order> OrderList;
			OrderByPrice()
			{
				Volume = 0.0;
				VolumeTotal = 0.0;
				VolumePendingCancel = 0.0;
				VolumeCloseToday = 0.0;
				Price = 0.0;
			}
		};
		typedef map<double, OrderByPrice*, DoubleLess> OrderByPriceMap;
		class DealOrders : Adapter
		{
		public:
			DealOrders(const StrategyInstrumentPNLDaily &position, Adapter *adapter);
			~DealOrders();
			void setOrder(SetOrderInfo *setOrderInfo);
			void cancelAllNonPendingOrders(char direction, uint32_t cancelSize);			
			void newOrders();

			bool cancelOrdersBySize(OrderByPrice *orderByPrice, uint32_t& cancelSize);
			bool cancelOrders(OrderByPrice *orderByPrice);

			void insertOrderByPrice(const Order &order);
			void updateOrderByPrice(const Order &rtnOrder);
			void removeOrderByPrice(const Order &rtnOrder,bool &setOrderFinished);
			void updateVolumes(char direction,double price);
			
			int reqOrder(Order& order);
			int cancelOrder(Order &order);
			int query(const Order &order);

			OrderByPrice* getOrderByPrice(char direction, double price);
			inline OrderByPriceMap* orderMap(char direction);
			double getTopPrice(char direction);
			double getBottomPrice(char direction);
			inline int getOrderSize(char direction);
			int getCancelOrderSize(char direction);
			inline double maxPosition();
			inline double targetPrice();
			inline double targetQuantity();
			inline char targetDirection();
		private:
			void getCloseVolumes(double& closeToday);			
			void tracePendingOrders(char direction);
			bool exceedMaxPosition(double appendVolume);
			
		private:			
			int roundLot_;
			double tickSize_;

			const int* orderSpacing_;
			const double* maxPosition_;
			Instrument *instrument_;
			OrderByPriceMap *buyOrders_;
			OrderByPriceMap *sellOrders_;
			DealOrdersApi *splitOrders_;
			SetOrderInfo *currSetOrder_;			
			Adapter *adapterOM_;
			const StrategyInstrumentPNLDaily &position_;
		};

		inline double DealOrders::maxPosition()
		{
			return *maxPosition_;
		}
		inline OrderByPriceMap* DealOrders::orderMap(char direction)
		{
			return (direction == INNER_DIRECTION_Buy) ? buyOrders_ : sellOrders_;
		}
		inline int DealOrders::getOrderSize(char direction)
		{
			return (direction == INNER_DIRECTION_Buy) ? buyOrders_->size() : sellOrders_->size();
		}

		inline double DealOrders::targetPrice()
		{
			return currSetOrder_->Price;
		}
		inline double DealOrders::targetQuantity()
		{
			return currSetOrder_->Quantity;
		}
		inline char DealOrders::targetDirection()
		{
			return currSetOrder_->Direction;
		}

	}
}
#endif
