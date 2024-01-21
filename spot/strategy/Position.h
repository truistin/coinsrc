#ifndef SPOT_BASE_POSITION_H
#define SPOT_BASE_POSITION_H
#include "spot/base/DataStruct.h"
#include "spot/base/Risk.h"
#include "spot/base/Instrument.h"

namespace spot
{
	namespace base
	{
		class Position : public Risk
		{
		public:
			Position();
			virtual ~Position();
			void setInstrument(Instrument* instrument);
			void setPnlDaily(const StrategyInstrumentPNLDaily pnlDaily);
			inline double getNetPosition() const;
			inline int getNetPositionInt() const;
			inline double getTodayLong() const;
			inline double getTodayShort() const;

			inline double getBuyQuantity() const;
			inline double getSellQuantity() const;
			inline double getAvgBuyPrice() const;
			inline double getAvgSellPrice() const;
			inline double getTurnover() const;
			inline const StrategyInstrumentPNLDaily& pnlDaily();
			inline StrategyInstrumentPNLDaily& PublicPnlDaily();
			void initPosition(const StrategyInstrumentPNLDaily &position);
			void updatePosition(const Order &rtnOrder);
			void updatePnlInfo(const Order &rtnOrder);
			void updateUPnlInfo(const Order &rtnOrder);
			void updateCPnlInfo(const Order &rtnOrder);
			double getTransactionFee(const SymbolTradingFee& tradingFee, double quantity,double price);
			double getTransactionFee(InstrumentTradingFee *tradingFee, double quantity, double price);
			double aggregatedFee() override;
			StrategyInstrumentPNLDaily OnRtnOrder(const Order& rtnOrder);
			double avgPrice(double lastQuantity, double lastPrice, double newQuantity, double newPrice);
			double avgPriceOK(Instrument* instrument, char direction, double newQuantity, double newPrice);
			inline Instrument* getInstrument();
			void calcAggregateFeeByRate(const Order &rtnOrder);
			void calcAggregateFee(const Order &rtnOrder);
		private:
			void checkPnlInfo();
			void updatePositionBySingle(const Order &rtnOrder);
			void updatePositionByDouble(const Order &rtnOrder);
			StrategyInstrumentPNLDaily pnlDaily_;
			Instrument *instrument_;
		};
		inline Instrument* Position::getInstrument()
		{
			return instrument_;
		}
		inline StrategyInstrumentPNLDaily& Position::PublicPnlDaily()
		{
			return pnlDaily_;
		}
		inline const StrategyInstrumentPNLDaily& Position::pnlDaily()
		{
			return pnlDaily_;
		}
		inline double Position::getNetPosition() const
		{
			return pnlDaily_.NetPosition;
		}
		inline int Position::getNetPositionInt() const
		{
			return std::lround(pnlDaily_.NetPosition);
		}
		inline double Position::getTodayLong() const
		{
			return pnlDaily_.TodayLong;
		}
		inline double Position::getTodayShort() const
		{
			return pnlDaily_.TodayShort;
		}

		inline double Position::getBuyQuantity() const
		{
			return pnlDaily_.BuyQuantity;
		}
		inline double Position::getSellQuantity() const
		{
			return pnlDaily_.SellQuantity;
		}
		inline double Position::getAvgBuyPrice() const
		{
			return pnlDaily_.AvgBuyPrice;
		}
		inline double Position::getAvgSellPrice() const
		{
			return pnlDaily_.AvgSellPrice;
		}
		inline double Position::getTurnover() const
		{
			return pnlDaily_.Turnover;
		}
	}
}
#endif