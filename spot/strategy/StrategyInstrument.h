#ifndef SPOT_STRATEGY_INSTRUMENT_H
#define SPOT_STRATEGY_INSTRUMENT_H
#include<list>
#include "spot/utility/Utility.h"
#include "spot/base/DataStruct.h"
#include "spot/strategy/InstrumentOrder.h"
#include "spot/base/Instrument.h"
#include "spot/strategy/Position.h"
#include "spot/base/Adapter.h"
using namespace std;
using namespace spot;
using namespace spot::base;
using namespace spot::utility;
namespace spot
{
	namespace strategy
	{
		class Strategy;
		class InstrumentOrder;
		class StrategyInstrument
		{
		public:
			StrategyInstrument(Strategy *strategy, string instrumentID,Adapter *adapter);
			~StrategyInstrument();
			void initPosition();
			inline string getInstrumentID();	
			inline int getStrategyID();
			inline Strategy* getStrategy();
			inline Position& position();
			inline OrderByPriceMap* buyOrders();
			inline OrderByPriceMap* sellOrders();
            inline InstrumentOrder* getInstrumentOrder();
			
			void setOrder(char direction, double price, double quantity,SetOrderOptions setOrderOptions, uint32_t cancelSize = 0);
			void checkSetOrder(char direction);
			Instrument* instrument();
			void OnRtnOrder(const Order &rtnOrder);

			bool isTradeable() { return tradeable_; };
			void tradeable(bool tradeable) { tradeable_ = tradeable; };
			double getTopPrice(char direction);
			double getBottomPrice(char direction);
			void cancelOrder(const Order &order);
			int query(const Order &order);
			bool hasUnfinishedOrder();
			int getOrdersCount(char direction, double price);
			double getOrdersSize(char direction, double price = NAN);

		private:
			void updatePosition(const Order &rtnOrder);
		private:
			string instrumentID_;
			InstrumentOrder *instrumentOrder_;
			Instrument *instrument_;			
			bool tradeable_;
			Position position_;
			Adapter *adapter_;
			int strategyID_;
			Strategy *strategy_;
		};
		inline Strategy* StrategyInstrument::getStrategy()
		{
			return strategy_;
		}
		inline string StrategyInstrument::getInstrumentID()
		{
			return instrumentID_;
		}
		inline int StrategyInstrument::getStrategyID()
		{
			return strategyID_;
		}
		inline Position& StrategyInstrument::position()
		{
			return position_;
		}
		inline OrderByPriceMap* StrategyInstrument::buyOrders()
		{
			return instrumentOrder_->getPendingOrders(INNER_DIRECTION_Buy);
		}
		inline OrderByPriceMap* StrategyInstrument::sellOrders()
		{
			return instrumentOrder_->getPendingOrders(INNER_DIRECTION_Sell);
		}

        inline InstrumentOrder* StrategyInstrument::getInstrumentOrder()
        {
            return instrumentOrder_;
        }
	}
}
#endif
