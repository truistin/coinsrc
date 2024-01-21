#ifndef SPOT_UTILITY_TRADINGDAY_H
#define SPOT_UTILITY_TRADINGDAY_H
#include "spot/utility/Compatible.h"
#include "spot/utility/TradingTime.h"
#include<iostream>
#include<unordered_map>
#include<chrono>
#include<stdlib.h>
namespace spot
{
	namespace utility
	{
		class TradingDay
		{
		public:
			static void init(int startTime = 160000);
			static const std::string& getString();	
			static void setString(const std::string& currTradingDay);
			static std::string getToday();
		private:
			static std::string currTradingDay_;
		};

		inline void TradingDay::setString(const std::string& currTradingDay)
		{
			currTradingDay_ = currTradingDay;
		}
		inline const std::string& TradingDay::getString()
		{
			return currTradingDay_;
		}
	}
};
#endif
