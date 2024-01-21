#ifndef SPOT_UTILITY_TRADINGTIME_H
#define SPOT_UTILITY_TRADINGTIME_H
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include<iostream>
#include<unordered_map>
#include<vector>
#include<chrono>
#include<stdlib.h>
#include<atomic>
const int defaultStartTime = 160000;
const int nightTradingEndTime = 30000;
const int dailyTradingStartTime = 85500;
namespace spot
{
	namespace utility
	{
		class TradingTime
		{
		public:
			static void init(int startTime = defaultStartTime);
			static long long getEpochTime(int time);
			static long long getEpochTime(const char* time);
			static long long getEpochTimeFromString(string dateTimeStr);
			static int convertTime(std::chrono::system_clock::time_point &timePoint);
			static int convertTime(tm* ptm);
			static int startTime();
			static void checkIsBetweenNightAndDailyTrading();
			inline static bool isBetweenNightAndDailyTrading(); //是否在夜盘和日盘之间03::00:00<-->08:55:00
		private:
			static std::chrono::system_clock::time_point getStartTimePoint(int startTime);
			static long long nightTradingEndEpochTime_;
			static long long dailyTradingStartEpochTime_;
		private:
			static std::unordered_map<int, long long> timeMap_;
			static int startTime_;
			static std::atomic<bool> isBetweenNightAndDailyTrading_;
		};
		inline bool TradingTime::isBetweenNightAndDailyTrading()
		{
			return isBetweenNightAndDailyTrading_;
		}
	}
};
#endif
