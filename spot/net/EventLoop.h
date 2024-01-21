#ifndef SPOT_NET_EVENTLOOP_H
#define SPOT_NET_EVENTLOOP_H
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Compatible.h>
#include <spot/utility/CurrentThread.h>
#include <spot/utility/Timestamp.h>
#include <spot/utility/Mutex.h>
#include <spot/net/Callbacks.h>
#include <spot/net/TimerId.h>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include<iostream>

namespace spot
{
	namespace net
	{
		typedef std::function<void()> Functor;
		class Channel;
		class Poller;
		class TimerQueue;

		struct WSWakeUpfd
		{
			void reset()
			{
				wakeupFd_ = 0;
				notifyFd_ = 0;
			}
			int wakeupFd_;
			int notifyFd_;
		};

		WSWakeUpfd createWakeUpfd();
		class EventLoop : spot::utility::NonCopyable
		{
		public:
			EventLoop();
			~EventLoop();
			///一直循环
			///必须在一个线程内执行
			void loop();

			void quit();
			/// Runs callback immediately in the loop thread.
			/// It wakes up the loop, and run the cb.
			/// If in the same loop thread, cb is run within the function.
			/// Safe to call from other threads.
			void runInLoop(const Functor& cb);
			/// Queues callback in the loop thread.
			/// Runs after finish pooling.
			/// Safe to call from other threads.
			void queueInLoop(const Functor& cb);

			TimerId runAt(const Timestamp& time, TimerCallback&& cb);
			TimerId runAfter(double delay, TimerCallback&& cb);
			TimerId runEvery(double interval, TimerCallback&& cb);
			void cancel(TimerId timerId);
			void wakeup();
			void updateChannel(Channel* channel);
			void removeChannel(Channel* channel);
			bool hasChannel(Channel* channel);
			bool isInLoopThread() const;
			void insertTimerChannel(Channel *channel);
			void assertInLoopThread();
			uint64_t threadId();
			std::vector<Functor>& pendingFunctors();
		private:
			void abortNotInLoopThread();
			void handleRead();
			void doPendingFunctors();
			std::atomic<bool> looping_; /* atomic */
			bool quit_;
			std::atomic<bool> eventHandling_; /* atomic */
			std::atomic<bool> callingPendingFunctors_; /* atomic */
			int64_t iteration_;
			uint64_t threadId_;
			Timestamp pollReturnTime_;

			WSWakeUpfd wakeupFd_;
			std::unique_ptr<Poller> poller_;
			typedef std::vector<Channel*> ChannelList;
			ChannelList timerChannels_;
			std::unique_ptr<TimerQueue> timerQueue_;
			std::unique_ptr<Channel> wakeupChannel_;

			ChannelList activeChannels_;
			Channel* currentActiveChannel_;

			MutexLock mutex_;
			std::vector<Functor> pendingFunctors_; //Guarded by mutex_
		};

	}
}

#endif