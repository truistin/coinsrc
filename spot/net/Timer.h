// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_TIMER_H
#define SPOT_NET_TIMER_H

#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Timestamp.h>
#include <spot/net/Callbacks.h>
#include <atomic>
namespace spot
{
	namespace net
	{
		///
		/// Internal class for timer event.
		///
		class Timer : spot::utility::NonCopyable
		{
		public:
			Timer(TimerCallback&& cb, Timestamp when, double interval)
				: callback_(std::move(cb)),
				expiration_(when),
				interval_(interval),
				repeat_(interval > 0.0),
				sequence_(s_numCreated_.fetch_add(1))
			{ }

			void run() const
			{
				callback_();
			}

			Timestamp expiration() const { return expiration_; }
			bool repeat() const { return repeat_; }
			int64_t sequence() const { return sequence_; }

			void restart(Timestamp now);

			static int64_t numCreated() { return s_numCreated_.load(); }

		private:
			const TimerCallback callback_;
			Timestamp expiration_;
			const double interval_;
			const bool repeat_;
			const int64_t sequence_;

			static std::atomic<int64_t> s_numCreated_;
		};
	}
}
#endif  // SPOT_NET_TIMER_H
