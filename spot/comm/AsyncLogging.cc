#include <spot/comm/AsyncLogging.h>
#include <spot/comm/MqUtils.h>
#include <spot/utility/LogFile.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Timestamp.h>
#include <spot/utility/Compatible.h>
#include <spot/base/MqStruct.h>
#include <spot/base/BoostGZip.h>
#include <functional>
#include <iostream>
using namespace spot;
using namespace spot::base;
using namespace std;
spot::AsyncLogging* g_asyncLog = NULL;
void asyncOutput(const char* msg, int len)
{
	g_asyncLog->append(msg, len);
}
AsyncLogging::AsyncLogging(const string& basename,
	size_t rollSize,
	int logLevel,
	int mqLevel,
	int flushInterval, bool usedmq)
	: flushInterval_(flushInterval),
	running_(false),
	basename_(basename),
	rollSize_(rollSize),
	mqLevel_(mqLevel),
	thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
	latch_(1),
	mutex_(),
	cond_(mutex_),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_(),
	connectMQ_(false),
	usedmq_(usedmq)
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);
	Logger::setOutput(asyncOutput);
	Logger::setLogLevel(Logger::LogLevel(logLevel));	
}
int AsyncLogging::connectMq(MqParam mqParam)
{
#ifndef INIT_WITHOUT_RMQ
	if (!usedmq_)
		return 0;	
	int res = init_rabbitmq(conn_, mqParam.hostname, mqParam.port, mqParam.user, mqParam.password);
	if (res)
	{
		fprintf(stderr, "init_rabbitmq error");
		LOG_ERROR << "init_rabbitmq error";
	}
	sendQueName_ = "SpotToServer" + std::to_string(mqParam.spotid);
	
	props_._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_PRIORITY_FLAG;
	props_.content_type = amqp_cstring_bytes("text/plain");
	props_.delivery_mode = 2; /* persistent delivery mode */
	props_.priority = 0;
#endif
	connectMQ_ = true;
	return 0;
}
void AsyncLogging::append(const char* logline, int len)
{
	MutexLockGuard lock(mutex_);
	if (currentBuffer_->avail() > len)
	{
		currentBuffer_->append(logline, len);
	}
	else
	{
		buffers_.push_back(std::unique_ptr<Buffer>(currentBuffer_.release()));
		if (nextBuffer_)
		{
			currentBuffer_ = std::move(nextBuffer_);
		}
		else
		{
			currentBuffer_.reset(new Buffer); // Rarely happens
		}
		currentBuffer_->append(logline, len);
		cond_.notify();
	}
}
int AsyncLogging::logTypeToLevel(const char *strType) const
{
	int level = INT16_MAX;
	if (strncmp(strType, "TRACE  ", 7) == 0)
	{
		level = 0;
	}
	else if (strncmp(strType, "DEBUG  ", 7) == 0)
	{
		level = 1;
	}
	else if (strncmp(strType, "INFO   ", 7) == 0)
	{
		level = 2;
	}
	else if (strncmp(strType, "WARN   ", 7) == 0)
	{
		level = 3;
	}
	else if (strncmp(strType, "ERR    ", 7) == 0)
	{
		level = 4;
	}
	else if (strncmp(strType, "THROW  ", 7) == 0)
	{
		level = 5;
	}
	else if (strncmp(strType, "FATAL  ", 7) == 0)
	{
		level = 6;
	}
	return level;
}
int AsyncLogging::publishMeseage(const char *msg, int length) const
{
#ifndef INIT_WITHOUT_RMQ
	string strMsg(msg, length);
	int res;
	base::LogInfo info;
	string strTime, strType, strEpoch, strText;
	long long epochTime;
	int level = 0;
	while (strMsg.length() > 30)
	{
		strType = strMsg.substr(17, 7);
		level = logTypeToLevel(strType.c_str());
		size_t endPosition = strMsg.find_first_of("\n");
		if (endPosition == string::npos)
		{
			endPosition = strMsg.length();
			strText = strMsg.substr(0, endPosition);
			strMsg = "";
		}
		else
		{
			strText = strMsg.substr(0, endPosition);
			strMsg = strMsg.substr(endPosition + 1);
		}

		if (level < mqLevel_)
		{
			continue;
		}
		res = amqp_basic_publish(conn_,
			1,
			amqp_cstring_bytes(""),
			amqp_cstring_bytes(sendQueName_.c_str()),
			0,
			0,
			&props_,
			amqp_cstring_bytes(strText.c_str()));
		if (res < 0)
		{
			LOG_ERROR << "publishMeseage error:" << amqp_error_string2(res);
		}
		if (level == 6) //FATAL
		{
			return 0;
		}
	}
#endif
	return 1;
}
void AsyncLogging::threadFunc()
{
	assert(running_ == true);
	latch_.countDown();
	LogFile output(basename_, rollSize_, false);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	if (usedmq_ && !connectMQ_)
	{
		char buf[256];
		strcpy(buf, "connectMQ is false");
		output.append(buf, static_cast<int>(strlen(buf)));
		output.flush();
		LOG_ERROR << buf;
	}
	while (true)
	{
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());
		{
			MutexLockGuard lock(mutex_);
			if (buffers_.empty())  // unusual usage!
			{
				cond_.waitForSeconds(flushInterval_);
			}
			buffers_.push_back(std::unique_ptr<Buffer>(currentBuffer_.release()));
			currentBuffer_ = std::move(newBuffer1);
			buffersToWrite.swap(buffers_);
			if (!nextBuffer_)
			{
				nextBuffer_ = std::move(newBuffer2);
			}
		}
		assert(!buffersToWrite.empty());
		if (buffersToWrite.size() > 10000)
		{
			char buf[256];
			SNPRINTF(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
				Timestamp::now().toFormattedString().c_str(),
				buffersToWrite.size() - 2);
			fputs(buf, stderr);
			output.append(buf, static_cast<int>(strlen(buf)));
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}
		lastBufferSize_ = buffersToWrite.size();
		auto startTime = CURR_USTIME_POINT;
		for (size_t i = 0; i < buffersToWrite.size(); ++i)
		{
			//当文件日志小于MQ日志的时候才记文件
			if (g_logLevel < mqLevel_ || mqLevel_ == 0)
			{
				output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
				output.flush();
			}
			if (buffersToWrite[i]->length() >0 && usedmq_)
			{
				 publishMeseage(buffersToWrite[i]->data(), buffersToWrite[i]->length());				
			}
		}
		if (buffersToWrite.size() > 2)
		{
			// drop non-bzero-ed buffers, avoid trashing
			buffersToWrite.resize(2);
		}
		if (!newBuffer1)
		{
			assert(!buffersToWrite.empty());
			newBuffer1 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer1->reset();
		}
		if (!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2 = std::move(buffersToWrite.back());
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}
		buffersToWrite.clear();
		output.flush();
		lastFlushTime_ = CURR_USTIME_POINT - startTime;
	}	
}
int AsyncLogging::lastBufferSize()
{
	return lastBufferSize_;
}
uint64_t AsyncLogging::lastFlushTime()
{
	return lastFlushTime_;
}