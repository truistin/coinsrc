// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_TIMERQUEUE_H
#define SPOT_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Mutex.h>
#include <spot/utility/Timestamp.h>
#include <spot/net/Callbacks.h>
#include <spot/net/Channel.h>

namespace spot
{
	namespace net
	{

		class EventLoop;
		class Timer;
		class TimerId;

		///
		/// A best efforts timer queue.
		/// No guarantee that the callback will be on time.
		///
		class TimerQueue : spot::utility::NonCopyable
		{
		public:
			TimerQueue(EventLoop* loop);
			~TimerQueue();

			///
			/// Schedules the callback to be run at given time,
			/// repeats if @c interval > 0.0.
			///
			/// Must be thread safe. Usually be called from other threads.
			TimerId addTimer(TimerCallback&& cb,
				Timestamp when,
				double interval);


			void cancel(TimerId timerId);

		private:

			// FIXME: use unique_ptr<Timer> instead of raw pointers.
			typedef std::pair<Timestamp, Timer*> Entry;
			typedef std::set<Entry> TimerList;
			typedef std::pair<Timer*, int64_t> ActiveTimer;
			typedef std::set<ActiveTimer> ActiveTimerSet;

			void addTimerInLoop(Timer* timer);
			void cancelInLoop(TimerId timerId);
			// called when timerfd alarms
			void handleRead();
			// move out all expired timers
			std::vector<Entry> getExpired(Timestamp now);
			void reset(const std::vector<Entry>& expired, Timestamp now);

			bool insert(Timer* timer);

			EventLoop* loop_;
			const int timerfd_;
			Channel timerfdChannel_;
			// Timer list sorted by expiration
			TimerList timers_;

			// for cancel()
			ActiveTimerSet activeTimers_;
			bool callingExpiredTimers_; /* atomic */
			ActiveTimerSet cancelingTimers_;
		};

	}
}
#endif  // SPOT_NET_TIMERQUEUE_H
