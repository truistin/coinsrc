#include <chrono>
#include<thread>
#include "spot/base/DataStruct.h"
#include "spot/strategy/StrategyTimer.h"
#include "spot/base/TradingPeriodManager.h"
#include "spot/utility/TradingTime.h"
#include "spot/base/InitialData.h"
#include "spot/strategy/RmqWriter.h"
using namespace spot::utility;
using namespace spot::base;
using namespace spot;

const int INTERVAL_TIMES = 1;
StrategyTimer::StrategyTimer()
	: circuitBreakInterval_(0)
	, bianListenKeyInterval_(0)
	, okPingPongInterval_(0)
	, byPingPongInterval_(0)
	, circuitBreakEPTimeout_(0)
	, timerForceCloseInterval_(0)
	, secondInterval_(0)
	, secondTimeout_(0)
	, bianListenKeyTimeout_(0)
	, bybitQryTimeout_(0)
	, okPingPongTimeout_(0)
	, forceCloseTimeout_(0)
	, running_(false)
{
	bianListenKeyInterval_ = InitialData::getBianListernKeyInterval();
	bybitQryInterval_ = InitialData::getBybitQryInterval();
	okPingPongInterval_ = InitialData::getOkPingPongInterval();
	byPingPongInterval_ = InitialData::getByPingPongInterval();
	circuitBreakInterval_ = InitialData::getTimerInterval();
	timerForceCloseInterval_ = InitialData::getForceCloseTimerInterval();
	secondInterval_ = INTERVAL_TIMES;

	time_point<system_clock> tpTradingDay = system_clock::now();
	auto tpNow = system_clock::to_time_t(tpTradingDay);
	currentDayPtm_ = localtime(&tpNow);

}

void StrategyTimer::start()
{
	forceCloseTimeout_ = nextTimeoutEPTime(timerForceCloseInterval_);
	circuitBreakEPTimeout_ = nextTimeoutEPTime(circuitBreakInterval_);
	secondTimeout_ = nextTimeoutEPTime(secondInterval_);
	bianListenKeyTimeout_ = nextTimeoutEPTime(bianListenKeyInterval_);
	bybitQryTimeout_ = nextTimeoutEPTime(bybitQryInterval_);
	okPingPongTimeout_ = nextTimeoutEPTime(okPingPongInterval_);
	byPingPongTimeout_ = nextTimeoutEPTime(byPingPongInterval_);
	running_ = true;
	while (running_) 
	{
		std::this_thread::sleep_for(std::chrono::microseconds(1));  // wait for a fix interval
		checkCircuitBreakTimer();
		checkForceCloseTimerInterval();
		//checkSecondInterval();
		checkbianListenKeyInterval();
		checkBybitQryInterval();
		checkOkPingPongInterval();
		checkByPingPongInterval();
	}
}
void StrategyTimer::stop()
{
	running_ = false;
}

void StrategyTimer::checkOkPingPongInterval()
{
	if (isTimeout(okPingPongTimeout_))
	{
		gOkPingPongFlag = true;
		okPingPongTimeout_ = nextTimeoutEPTime(okPingPongInterval_);
	}
}

void StrategyTimer::checkByPingPongInterval()
{
	if (isTimeout(byPingPongTimeout_))
	{
		gByPingPongFlag = true;
		byPingPongTimeout_ = nextTimeoutEPTime(byPingPongInterval_);
	}
}

void StrategyTimer::checkbianListenKeyInterval()
{
	if (isTimeout(bianListenKeyTimeout_))
	{
		gBianListenKeyFlag = true;
		bianListenKeyTimeout_ = nextTimeoutEPTime(bianListenKeyInterval_);
	}
}

void StrategyTimer::checkBybitQryInterval()
{
	if (isTimeout(bybitQryTimeout_))
	{
		gBybitQryFlag = true;
		bybitQryTimeout_ = nextTimeoutEPTime(bybitQryInterval_);
	}
}

void StrategyTimer::checkPassCurrentDay()
{
	time_point<system_clock> tpTradingDay = system_clock::now();
	auto tpNow = system_clock::to_time_t(tpTradingDay);
	tm* ptm = localtime(&tpNow);
	if (ptm->tm_mday != currentDayPtm_->tm_mday) {
		gResetTimeOrderLimit = true;
		currentDayPtm_ = ptm;
	}
}
void StrategyTimer::checkCircuitBreakTimer()
{
	if (isTimeout(circuitBreakEPTimeout_))
	{
		gCircuitBreakTimerActive = true;
		circuitBreakEPTimeout_ = nextTimeoutEPTime(circuitBreakInterval_);
	}
}
void StrategyTimer::checkForceCloseTimerInterval()
{
	if (timerForceCloseInterval_ == 0)
		return;

	if (isTimeout(forceCloseTimeout_))
	{
		gForceCloseTimeIntervalActive = true;
		forceCloseTimeout_ = nextTimeoutEPTime(timerForceCloseInterval_);
	}
}

void StrategyTimer::checkSecondInterval()
{
	if (isTimeout(secondTimeout_))
	{
		gCancelOrderActive = true;
		secondTimeout_ = nextTimeoutEPTime(secondInterval_);
	}
}
int64_t StrategyTimer::nextTimeoutEPTime(int timerInterval)
{
	return Timestamp::now().secondsSinceEpoch() + timerInterval;
}

bool StrategyTimer::isTimeout(int64_t EPTime)
{
	return  Timestamp::now().secondsSinceEpoch() >= EPTime;
}
