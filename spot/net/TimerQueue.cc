// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <spot/net/TimerQueue.h>
#include <spot/utility/Timestamp.h>
#include <spot/utility/Logging.h>
#include <spot/net/EventLoop.h>
#include <spot/net/Timer.h>
#include <spot/net/TimerId.h>
#include <spot/net/SocketsOps.h>
#include <functional>
#include <assert.h>
#include <iterator>

#ifdef __LINUX__
#include <sys/timerfd.h>
#endif

namespace spot
{
	namespace net
	{
		namespace detail
		{

			int createTimerfd()
			{
#ifdef __LINUX__
				int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
					TFD_NONBLOCK | TFD_CLOEXEC);
#else
				WSWakeUpfd wakeUpfd = createWakeUpfd();
				int timerfd = wakeUpfd.wakeupFd_;
#endif
				if (timerfd < 0)
				{
					LOG_SYSFATAL << "Failed in timerfd_create";
				}
				return timerfd;
			}
			struct timespec howMuchTimeFromNow(Timestamp when)
			{
				int64_t microseconds = when.microSecondsSinceEpoch()
					- Timestamp::now().microSecondsSinceEpoch();
				if (microseconds < 100)
				{
					microseconds = 100;
				}
				struct timespec ts;
				ts.tv_sec = static_cast<time_t>(
					microseconds / Timestamp::kMicroSecondsPerSecond);
				ts.tv_nsec = static_cast<long>(
					(microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
				return ts;
			}

			void readTimerfd(int timerfd, Timestamp now)
			{
				uint64_t howmany;
				SSIZE_T32 n = sockets::read(timerfd, &howmany, sizeof howmany);
				LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
				if (n != sizeof howmany)
				{
					LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
				}
			}

			void resetTimerfd(int timerfd, Timestamp expiration)
			{
				// wake up loop by timerfd_settime()
#ifdef __LINUX__
				struct itimerspec newValue;
				struct itimerspec oldValue;
				memset(&newValue, 0, sizeof newValue);
				memset(&oldValue, 0, sizeof oldValue);
				newValue.it_value = howMuchTimeFromNow(expiration);
				int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
				if (ret)
				{
					LOG_SYSERR << "timerfd_settime()";
				}
#endif
			}

		}
	}
}

using namespace spot;
using namespace spot::net;

TimerQueue::TimerQueue(EventLoop* loop)
	: loop_(loop),
	timerfd_(detail::createTimerfd()),
	timerfdChannel_(loop, timerfd_),
	timers_(),
	callingExpiredTimers_(false)
{
	timerfdChannel_.setReadCallback(
		std::bind(&TimerQueue::handleRead, this));
	// we are always reading the timerfd, we disarm it with timerfd_settime.
	timerfdChannel_.enableReading();

#ifdef WIN32
	loop->insertTimerChannel(&timerfdChannel_);
#endif
}

TimerQueue::~TimerQueue()
{
	timerfdChannel_.disableAll();
	timerfdChannel_.remove();
	sockets::socketClose(timerfd_);
	// do not remove channel, since we're in EventLoop::dtor();
	for (TimerList::iterator it = timers_.begin();
	it != timers_.end(); ++it)
	{
		delete it->second;
	}
}

TimerId TimerQueue::addTimer(TimerCallback&& cb,
	Timestamp when,
	double interval)
{
	Timer* timer = new Timer(std::move(cb), when, interval);
	loop_->runInLoop(
		std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}


void TimerQueue::cancel(TimerId timerId)
{
	loop_->runInLoop(
		std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();
	bool earliestChanged = insert(timer);

	if (earliestChanged)
	{
		detail::resetTimerfd(timerfd_, timer->expiration());
	}
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	ActiveTimer timer(timerId.timer_, timerId.sequence_);
	ActiveTimerSet::iterator it = activeTimers_.find(timer);
	if (it != activeTimers_.end())
	{
		size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
		assert(n == 1); (void)n;
		delete it->first; // FIXME: no delete please
		activeTimers_.erase(it);
	}
	else if (callingExpiredTimers_)
	{
		cancelingTimers_.insert(timer);
	}
	assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead()
{
	loop_->assertInLoopThread();
	Timestamp now(Timestamp::now());
#ifdef __LINUX__
	detail::readTimerfd(timerfd_, now);
#endif
	std::vector<Entry> expired = getExpired(now);

	callingExpiredTimers_ = true;
	cancelingTimers_.clear();
	// safe to callback outside critical section
	for (std::vector<Entry>::iterator it = expired.begin();
	it != expired.end(); ++it)
	{
		it->second->run();
	}
	callingExpiredTimers_ = false;

	reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
	assert(timers_.size() == activeTimers_.size());
	std::vector<Entry> expired;
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	TimerList::iterator end = timers_.lower_bound(sentry);
	//  assert(end == timers_.end() || now < end->first);
	std::copy(timers_.begin(), end, std::back_inserter(expired));
	timers_.erase(timers_.begin(), end);

	for (std::vector<Entry>::iterator it = expired.begin();
	it != expired.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		size_t n = activeTimers_.erase(timer);
		assert(n == 1); (void)n;
	}

	assert(timers_.size() == activeTimers_.size());
	return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
	Timestamp nextExpire;

	for (std::vector<Entry>::const_iterator it = expired.begin();
	it != expired.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		if (it->second->repeat()
			&& cancelingTimers_.find(timer) == cancelingTimers_.end())
		{
			it->second->restart(now);
			insert(it->second);
		}
		else
		{
			// FIXME move to a free list
			delete it->second; // FIXME: no delete please
		}
	}

	if (!timers_.empty())
	{
		nextExpire = timers_.begin()->second->expiration();
	}

	if (nextExpire.valid())
	{
		detail::resetTimerfd(timerfd_, nextExpire);
	}
}

bool TimerQueue::insert(Timer* timer)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	bool earliestChanged = false;
	const Timestamp when = timer->expiration();
	TimerList::iterator it = timers_.begin();
	if (it == timers_.end() || when < it->first)
	{
		earliestChanged = true;
	}
	{
		std::pair<TimerList::iterator, bool> result
			= timers_.insert(Entry(when, timer));
		assert(result.second); (void)result;
	}
	{
		std::pair<ActiveTimerSet::iterator, bool> result
			= activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
		assert(result.second); (void)result;
	}

	assert(timers_.size() == activeTimers_.size());
	return earliestChanged;
}