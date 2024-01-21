#ifndef SPOT_BASE_ORDERBOOK_H
#define SPOT_BASE_ORDERBOOK_H
#include "spot/base/DataStruct.h"

namespace spot
{
	namespace base
	{
		class OrderBook
		{
		public:
			OrderBook();
			~OrderBook();
			const InnerMarketData* getOrderBook() const { return &orderBook_; }
			InnerMarketData* setOrderBook(const InnerMarketData &md);
			InnerMarketData* setOrderBook(const InnerMarketTrade &mt);
			void setPreClosePrice(double preClosePrice);

			bool setMdData(const InnerMarketData& md);
			void setTickSize(double tickSize) { tickSize_ = tickSize; }

			// accessor price
			double bidPrice() { return orderBook_.BidPrice1; }
			double askPrice() { return orderBook_.AskPrice1; }
			double bidPrice(int level);
			double askPrice(int level);
			double midPrice() { return (bidPrice() + askPrice()) / 2.0; }
			double lastPrice() { return orderBook_.LastPrice; }

			// accessor size
			double bidSize() { return orderBook_.BidVolume1; }
			double askSize() { return orderBook_.AskVolume1; }
			int bidSizeInt() { return std::lround(orderBook_.BidVolume1); }
			int askSizeInt() { return std::lround(orderBook_.AskVolume1); }
			double bidSize(int level);
			double askSize(int level);

			double spreadBps();

			long long getLastUpdateTimestamp() { return lastUpdateTimestamp_; }
			void  updateLastUpdateTimestamp() { lastUpdateTimestamp_ = CURR_MSTIME_POINT; }

			bool isInit() { return isInit_; }
			void setIsInit(bool flag) {isInit_ = flag;}
			bool isConsistent();
			bool isOldMarketData(const InnerMarketData &md);
		private:
			InnerMarketData orderBook_;
			bool isInit_;
			long long lastUpdateTimestamp_;
			double tickSize_;
		};
	}
}
#endif