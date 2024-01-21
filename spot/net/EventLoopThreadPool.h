// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_EVENTLOOPTHREADPOOL_H
#define SPOT_NET_EVENTLOOPTHREADPOOL_H

#include <spot/utility/Types.h>
#include <spot/utility/NonCopyAble.h>
#include <vector>
#include <functional>
#include <memory>

namespace spot
{
	namespace net
	{
		class EventLoop;
		class EventLoopThread;

		class EventLoopThreadPool : spot::utility::NonCopyable
		{
		public:
			typedef std::function<void(EventLoop*)> ThreadInitCallback;

			EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
			~EventLoopThreadPool();
			void setThreadNum(int numThreads) { numThreads_ = numThreads; }
			void start(const ThreadInitCallback& cb = ThreadInitCallback());

			// valid after calling start()
			/// round-robin
			EventLoop* getNextLoop();

			/// with the same hash code, it will always return the same EventLoop
			EventLoop* getLoopForHash(size_t hashCode);

			std::vector<EventLoop*> getAllLoops();

			bool started() const
			{
				return started_;
			}

			const string& name() const
			{
				return name_;
			}

		private:

			EventLoop* baseLoop_;
			string name_;
			bool started_;
			int numThreads_;
			int next_;
			typedef std::vector<std::unique_ptr<EventLoopThread>> ThreadVector;
			ThreadVector threads_;
			std::vector<EventLoop*> loops_;
		};

	}
}

#endif  // SPOT_NET_EVENTLOOPTHREADPOOL_H
