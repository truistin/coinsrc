#ifndef SPOT_BASE_TRADINGPERIOD_H
#define SPOT_BASE_TRADINGPERIOD_H
#include<iostream>
#include<chrono>
#include<unordered_map>
#include<stdlib.h>
#include<list>
#include<map>
#include<set>
#include<atomic>
#include "spot/base/DataStruct.h"
#include "boost/date_time.hpp"
#include "spot/utility/TradingTime.h"
using namespace std;
using namespace std::chrono;
using namespace spot::utility;
using namespace spot::base;

//#define CURR_STIME_POINT duration_cast<chrono::seconds>(system_clock::now().time_since_epoch()).count()

namespace spot
{
	namespace base
	{
		class TradingPeriod
		{
		public:
			TradingPeriod(string symbol, int deviationSecond);
			~TradingPeriod();

			void init();
			inline static void setMarketDataUpdateTime(const char* updateTime);
			static void setSimCurrTime(string localTimeStamp);
			static string getCurrTime();
			static string localTimeStamp();

		private:			
			void splitTime(int time, int &hour, int &minute, int &second);
			int genTime(const boost::posix_time::ptime &pTime);
			int timeToInt(long long time);
			long long timeToLong(int time);
			void enterTradingPeriod();
		private:
			int deviationSecond_;
			string symbolID_;
			unordered_map<int, long long> timeMap_;

			static string marketDataUpdateTime_;
			static string currSimTime_;
			static string localTimeStamp_;
			static long long epochTimeus_;
		};

		inline void TradingPeriod::setMarketDataUpdateTime(const char* updateTime)
		{
			marketDataUpdateTime_ = updateTime;
		}
	}
}
#endif