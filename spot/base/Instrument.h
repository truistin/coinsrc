#ifndef SPOT_BASE_INSTRUMENT_H
#define SPOT_BASE_INSTRUMENT_H
#include "spot/base/DataStruct.h"
#include "spot/base/OrderBook.h"
using namespace spot::base;
namespace spot
{
	namespace base
	{
		class Instrument
		{
		public:			
			Instrument(const string &instrumentID,const SymbolInfo &instInfo);
			~Instrument();
			InnerMarketData* setOrderBook(const InnerMarketData &md);
			InnerMarketData* setOrderBook(const InnerMarketTrade &mt);
			bool isOldMarketData(const InnerMarketData &md);
			void addTradingFee(const SymbolTradingFee &tradingFee);
			void addInstrumentTradingFee(InstrumentTradingFee *tradingFee);
			inline const string& getInstrumentID();
			inline const SymbolInfo& getSymbolInfo();
			inline const char* getSymbol();
			inline const char* getExchange();
			inline int getMultiplier();
			inline double getMultiplierForTurnover();
			inline double getTickSize();
			double getTickSize(double orderPrice);
			inline double getMargin(); // 保证金比例
			inline const char* getType();
			inline bool isCrypto();

			inline bool isFuture();

			inline bool isCryptoFutureThisWeek();
			inline bool isCryptoFutureNextWeek();
			inline bool isCryptoFutureQuarter();
			inline bool isCryptoFuturePerpetual();
			inline bool isCryptoFuture();
			inline bool isCryptoDb();
			inline bool isOKExIndex();

			inline void setTradeableType(const string& tradeableType);
			inline void setIsCryptoFuture(bool flag);
			inline void setIsCryptoFuturePerpetual(bool flag);

			inline void setIsCryptoDb(bool flag);

			void setEndTradingDay(const string& endTradingDay);
			void setStartTradingDay(const string& startTradingDay);

			inline const string& getTradeableType();
			inline OrderBook* orderBook();
			const SymbolTradingFee& getMakerFee();
			const SymbolTradingFee& getTakerFee();
			inline InstrumentTradingFee* getInstrumentMakerFee();
			inline InstrumentTradingFee* getInstrumentTakerFee();
			const string& getEndTradingDay();
			const string& getStartTradingDay();

		private:
			string instrumentID_;
			string tradeableType_;
			SymbolInfo symbolInfo_;
			double multiplierForTurnover_;
			OrderBook *orderBook_;
			SymbolTradingFee makerFee_;
			SymbolTradingFee takerFee_;
			InstrumentTradingFee *instMakerFee_;
			InstrumentTradingFee *instTakerFee_;
			void setMultiplierForTurnover();
			void setCryptoFlags();
			bool isCryptoFuture_;
			bool isCryptoDb_;
			bool isCryptoFuturePerpetual_;
			string endTradingDay_;
			string startTradingDay_;
		};

		inline OrderBook* Instrument::orderBook()
		{
			return orderBook_;
		}
		inline const string& Instrument::getInstrumentID()
		{
			return instrumentID_;
		}
		inline const SymbolInfo& Instrument::getSymbolInfo()
		{
			return symbolInfo_;
		}
		inline const char* Instrument::getSymbol()
		{
			return symbolInfo_.Symbol;
		}
		inline const char* Instrument::getExchange()
		{
   		return symbolInfo_.ExchangeCode;
		}
		inline int Instrument::getMultiplier()
		{
			return symbolInfo_.Multiplier;
		}
		inline double Instrument::getMultiplierForTurnover()
		{
			return multiplierForTurnover_;
		}
		inline double Instrument::getTickSize()
		{
			return symbolInfo_.TickSize;
		}
		inline double Instrument::getMargin()
		{
			return symbolInfo_.Margin;
		}	
		inline const char* Instrument::getType()
		{
			return symbolInfo_.Type;
		}
		inline void Instrument::setTradeableType(const string& tradeableType)
		{
			tradeableType_ = tradeableType;
		}
		inline void Instrument::setIsCryptoFuture(bool flag)
		{
			isCryptoFuture_ = flag;
		}
		inline void Instrument::setIsCryptoFuturePerpetual(bool flag)
		{
			isCryptoFuturePerpetual_ = flag;
		}
		inline void Instrument::setIsCryptoDb(bool flag)
		{
			isCryptoDb_ = flag;
		}
		inline const string& Instrument::getTradeableType()
		{
			return tradeableType_;
		}
		inline bool Instrument::isCryptoFuturePerpetual()
		{
			return isCryptoFuturePerpetual_;
		}
		inline bool Instrument::isCryptoFuture()
		{
			return isCryptoFuture_;
		}
		inline bool Instrument::isCryptoDb()
		{
			return isCryptoDb_;
		}
		inline bool Instrument::isCrypto()
		{
			return AssetType_Crypto.compare(getType()) == 0;
		}
		inline bool Instrument::isCryptoFutureThisWeek()
		{
			return Crypto_Future_This_Week.compare(getTradeableType()) == 0;
		}
		inline bool Instrument::isCryptoFutureNextWeek()
		{
			return Crypto_Future_Next_Week.compare(getTradeableType()) == 0;
		}
		inline bool Instrument::isCryptoFutureQuarter()
		{
			return Crypto_Future_Quarter.compare(getTradeableType()) == 0;
		}
		inline bool Instrument::isOKExIndex()
		{
			return AssetType_OKExIndex.compare(getType()) == 0;
		}
		inline void Instrument::setEndTradingDay(const string& endTradingDay)
		{
			endTradingDay_ = endTradingDay;
		}
		inline const string& Instrument::getEndTradingDay()
		{
			return endTradingDay_;
		}
		inline void Instrument::setStartTradingDay(const string& startTradingDay)
		{
			startTradingDay_ = startTradingDay;
		}
		inline const string& Instrument::getStartTradingDay()
		{
			return startTradingDay_;
		}

		inline InstrumentTradingFee* Instrument::getInstrumentMakerFee()
		{
			return instMakerFee_;
		}
		inline InstrumentTradingFee* Instrument::getInstrumentTakerFee()
		{
			return instTakerFee_;
		}
		inline bool Instrument::isFuture()
		{
			return AssetType_Future.compare(getType()) == 0;
		}

	}
}
#endif