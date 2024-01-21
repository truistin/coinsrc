#ifndef SPOT_NET_EVENTLOOPTHREAD_H
#define SPOT_NET_EVENTLOOPTHREAD_H

#include <spot/utility/Condition.h>
#include <spot/utility/Mutex.h>
#include <spot/utility/Thread.h>
#include <spot/utility/NonCopyAble.h>


#include<functional>
namespace spot
{
	namespace net
	{
		class EventLoop;
		class EventLoopThread : spot::utility::NonCopyable
		{
		public:
			typedef std::function<void(EventLoop*)> ThreadInitCallback;

			EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
				const string& name = string());
			~EventLoopThread();
			EventLoop* startLoop();

			void join();
			void stop();

		private:
			void threadFunc();

			EventLoop* loop_;
			bool exiting_;
			Thread thread_;
			MutexLock mutex_;
			Condition cond_;
			ThreadInitCallback callback_;
		};

	}
}
#endif