#include <spot/utility/Logging.h>
#include <spot/utility/CurrentThread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <exception>
#include "Utility.h"

namespace spot
{
	thread_local char t_errnobuf[512];
	thread_local char t_time[32];
	thread_local time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
#ifdef __LINUX__
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
#endif
#ifdef WIN32
	return strerror(savedErrno);
#endif
}

Logger::LogLevel initLogLevel()
{
  if (::getenv("SPOT_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("SPOT_LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE  ",
  "DEBUG  ",
  "INFO   ",
  "WARN   ",
  "ERR    ",
  "THROW  ",
  "FATAL  ",
};

// helper class for known string length at compile time
class T
{
 public:
  T(const char* str, unsigned len)
    :str_(str),
     len_(len)
  {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char* msg, int len)
{
  size_t n = fwrite(msg, 1, len, stdout);
  //FIXME check n
  (void)n;
}

void defaultFlush()
{
  fflush(stdout);
}
void defaultFtalCallback(const char* msg)
{
}
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
Logger::FatalCallback g_fatal_callback = defaultFtalCallback;

}

using namespace spot;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
	stream_ << CURR_USTIME_POINT << " ";
	currentthread::tid();
//	stream_ << LogLevelName[level];
	stream_ << T(LogLevelName[level], 7);
	//stream_ << T(currentthread::tidString(), currentthread::tidStringLength());
	stream_ << currentthread::tidString();
	stream_ << " ";
}

void Logger::Impl::finish()
{
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
  : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
  impl_.stream_ << func << ' ';
  if (impl_.level_ == THROW)
	{  
	  impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
		g_flush();
		fprintf(stderr, "exception:%s\n", func);
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		throw std::runtime_error(func);
	}
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : impl_(toAbort?FATAL:ERR, errno, file, line)
{
}

Logger::~Logger()
{
	impl_.finish();
	const LogStream::Buffer& buf(stream().buffer());
	if (impl_.level_ == FATAL)
	{
		g_fatal_callback(buf.data());		
	}
	g_output(buf.data(), buf.length());

	if (impl_.level_ == FATAL)
	{	
		g_flush();
		fprintf(stderr, "fatal:%s\n", buf.data());
		SLEEP(10000);
		abort();
	}
}

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
  g_flush = flush;
}
void Logger::setFatalCallback(FatalCallback fatal_callback)
{
	g_fatal_callback = fatal_callback;
}


