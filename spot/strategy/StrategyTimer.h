#ifndef SPOT_STRATEGY_STRATEGYTIMER_H
#define SPOT_STRATEGY_STRATEGYTIMER_H

class StrategyTimer
{
public:
	StrategyTimer();
	void start();
	void stop();
	void checkCircuitBreakTimer();
	void checkForceCloseTimerInterval();
	void checkSecondInterval();
	void checkCancelOrderTimer();
	void checkPassCurrentDay();
	void checkbianListenKeyInterval();
	void checkOkPingPongInterval();
	void checkByPingPongInterval();
	void checkBybitQryInterval();

protected:
	int64_t nextTimeoutEPTime(int timerInterval);
	bool isTimeout(int64_t EPTime);

private:
	int circuitBreakInterval_;
	int bianListenKeyInterval_;
	int bybitQryInterval_;
	int okPingPongInterval_;
	int byPingPongInterval_;
	int64_t circuitBreakEPTimeout_;

	int timerForceCloseInterval_;
	int secondInterval_;
	int64_t secondTimeout_;//1 times per second
	tm* currentDayPtm_; // record currentday

	int64_t bianListenKeyTimeout_;
	int64_t bybitQryTimeout_;
	int64_t okPingPongTimeout_;
	int64_t byPingPongTimeout_;
	int64_t forceCloseTimeout_;
	bool running_;
};
#endif