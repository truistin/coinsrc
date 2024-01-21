#include "spot/utility/TradingDay.h"

using namespace std;
using namespace std::chrono;
using namespace spot::utility;
std::string TradingDay::currTradingDay_;
typedef std::chrono::duration<int, std::ratio<60 * 60>> HoursType;
void TradingDay::init(int startTime)
{
		time_point<system_clock> tpNow = system_clock::now();
		time_point<system_clock> tpTradingDay = tpNow;
		auto ttNow = system_clock::to_time_t(tpNow);
		struct tm* ptm = localtime(&ttNow);
		int currTime = TradingTime::convertTime(ptm);
		if (!(currTime >= startTime && ptm->tm_wday == 5))
		{
			//tm_wday->num: 6->1;0->2;1->3;2->4;3->5;4->6;5->7
			int num;
			if (ptm->tm_wday == 6)
				num = 1;
			else
				num = ptm->tm_wday + 2;

			HoursType days(24 * num);
			tpTradingDay = tpNow - days;
		}

		auto ttTradingDay = system_clock::to_time_t(tpTradingDay);
		ptm = localtime(&ttTradingDay);
		char day[11] = { 0 };
		SNPRINTF(day, sizeof(day), "%d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
		currTradingDay_ = string(day);
}
std::string TradingDay::getToday()
{
	time_point<system_clock> tpNow = system_clock::now();
	auto ttNow = system_clock::to_time_t(tpNow);
	struct tm* ptm = localtime(&ttNow);

	char day[11] = { 0 };
	SNPRINTF(day, sizeof(day), "%d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);

	return day;
}
