#include "spot/utility/TradingTime.h"
#include "spot/utility/Utility.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/TradingPeriod.h"
#include<time.h>
#include<iomanip>
using namespace std;
using namespace std::chrono;
using namespace spot::utility;

std::unordered_map<int, long long> TradingTime::timeMap_;

int TradingTime::startTime_ = defaultStartTime;
std::atomic<bool> TradingTime::isBetweenNightAndDailyTrading_;
long long TradingTime::nightTradingEndEpochTime_;
long long TradingTime::dailyTradingStartEpochTime_;

void TradingTime::init(int startTime)
{
	return;
}

long long TradingTime::getEpochTime(int time)
{
	char buf[10];
	sprintf(buf, "%06d", time);
	string date = getCurrSystemDate();
	string dateTimeStr = date + "_" + buf;
	return getEpochTimeFromString(dateTimeStr);
}

long long TradingTime::getEpochTime(const char* time)
{
	if (strlen(time) == 0)
		return 0;
	int itime = convertToTimeInt(time);
	return getEpochTime(itime);
}
long long TradingTime::getEpochTimeFromString(string dateTimeStr)
{
	//dateTimeStr:20180418_150000
	long long epoch = -1;
#ifdef __LINUX__
	struct tm t;
	strptime(dateTimeStr.c_str(), "%Y%m%d_%H%M%S", &t);
	epoch = std::mktime(&t);
#else
	std::tm t = {};
	dateTimeStr.insert(13, ":");
	dateTimeStr.insert(11, ":");
	dateTimeStr.insert(6, "-");
	dateTimeStr.insert(4, "-");
	dateTimeStr = dateTimeStr + "Z";
	std::istringstream ss(dateTimeStr);
	
	if (ss >> std::get_time(&t, "%Y-%m-%d_%H:%M:%S"))
	{
		epoch = std::mktime(&t);
	}	
#endif
	if (epoch > 0)
		return epoch;
	else
		return 0;
}

system_clock::time_point TradingTime::getStartTimePoint(int startTime)
{
	duration<int> one_second(1);
	auto currTimePoint = system_clock::now();
	if (convertTime(currTimePoint) == startTime)
	{
		//如果当前时间等于startTime，返回当前时间
		return currTimePoint;
	}
	auto nextpoint = currTimePoint - one_second;
	//遍历一天的每一秒
	for (auto i = 0; i < 60 * 60 * 24; i++)
	{
		if (convertTime(nextpoint) == startTime)
		{
			//找到startTime时间起点
			return nextpoint;
		}
		nextpoint = nextpoint  - one_second;
	}
	return currTimePoint;
}

int TradingTime::convertTime(std::chrono::system_clock::time_point &timePoint)
{
	auto time_t = system_clock::to_time_t(timePoint);
	auto ptm = localtime(&time_t);
	return convertTime(ptm);
}
int TradingTime::convertTime(tm* ptm)
{
	return ptm->tm_hour * 10000 + ptm->tm_min * 100 + ptm->tm_sec;
}
int TradingTime::startTime()
{
	return startTime_;
}
void TradingTime::checkIsBetweenNightAndDailyTrading()
{
	auto currEpochTime = CURR_STIME_POINT;
	if (currEpochTime < nightTradingEndEpochTime_ || currEpochTime > dailyTradingStartEpochTime_)
	{
		isBetweenNightAndDailyTrading_ = false;
	}
	else
	{
		isBetweenNightAndDailyTrading_ = true;
	}
}
