#ifndef SPOT_UTILITY_ASYNCLOGGING_H
#define SPOT_UTILITY_ASYNCLOGGING_H
#include <spot/utility/Thread.h>
#include <spot/utility/LogStream.h>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Condition.h>
#include <spot/utility/CountDownLatch.h>
#include <spot/utility/Mutex.h>
#include <spot/utility/LogFile.h>
#include <spot/utility/Logging.h>
#include <spot/comm/MqDispatch.h>
#include <memory>
#include <vector>
#include <amqp.h>
namespace spot
{
	class AsyncLogging : utility::NonCopyable
	{
	public:
		AsyncLogging(const string& basename,
			size_t rollSize, int logLevel,
			int mqLevel = 0,
			int flushInterval = 3,
			bool usedmq = true);
		~AsyncLogging()
		{
			if (running_)
			{
				stop();
			}
		}
		int connectMq(MqParam mqParam);
		void append(const char* logline, int len);
		void start()
		{
			running_ = true;
			thread_.start();
			latch_.wait();
		}
		void stop()
		{
			running_ = false;
			cond_.notify();
			thread_.join();
		}
		int lastBufferSize();
		uint64_t lastFlushTime();
		spot::Thread thread_;

	private:
		// declare but not define, prevent compiler-synthesized functions
		AsyncLogging(const AsyncLogging&);  // ptr_container
		void operator=(const AsyncLogging&);  // ptr_container

		void threadFunc();
		int publishMeseage(const char *msg, int length) const;
		int logTypeToLevel(const char *strType) const;
		typedef spot::detail::FixedBuffer<spot::detail::kLargeBuffer> Buffer;
		typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
		typedef std::unique_ptr<Buffer> BufferPtr;

		const int flushInterval_;
		bool running_;
		string basename_;
		size_t rollSize_;
		spot::CountDownLatch latch_;
		MutexLock  mutex_;
		spot::Condition cond_;
		BufferPtr currentBuffer_;
		BufferPtr nextBuffer_;
		BufferVector buffers_;
		bool connectMQ_;
		string sendQueName_;
		amqp_connection_state_t conn_;
		amqp_basic_properties_t props_;
		bool usedmq_;
		int mqLevel_;
		int lastBufferSize_;
		uint64_t lastFlushTime_;
	};
}
extern spot::AsyncLogging* g_asyncLog;
#endif  // MUDUO_BASE_ASYNCLOGGING_H
