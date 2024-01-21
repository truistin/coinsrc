#ifndef SPOT_UTILITY_LOGGING_H
#define SPOT_UTILITY_LOGGING_H

#include <spot/utility/LogStream.h>
#include <spot/utility/Timestamp.h>
#include <spot/utility/Compatible.h>
#define INIT_WITHOUT_RMQ
namespace spot
{
class Logger
{
 public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    THROW,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile
  {
   public:
    template<int N>
    inline SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {
			const char* slash = strrchr(data_, DIRDELIMIER); // builtin function
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : data_(filename)
    {
			const char* slash = strrchr(filename, DIRDELIMIER);
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void(*FatalCallback)(const char* msg);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setFatalCallback(FatalCallback);
 private:

class Impl
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  void finish();
  LogStream stream_;
  LogLevel level_;
  int line_;
  SourceFile basename_;
};

  Impl impl_;

};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE if (spot::Logger::logLevel() <= spot::Logger::TRACE) \
	spot::Logger(__FILE__, __LINE__, spot::Logger::TRACE, __FUNCTION_NAME__).stream()
#define LOG_DEBUG if (spot::Logger::logLevel() <= spot::Logger::DEBUG) \
	spot::Logger(__FILE__, __LINE__, spot::Logger::DEBUG, __FUNCTION_NAME__).stream()
#define LOG_INFO if (spot::Logger::logLevel() <= spot::Logger::INFO) \
  spot::Logger(__FILE__, __LINE__).stream()
#define LOG_SYSERR spot::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL spot::Logger(__FILE__, __LINE__, true).stream()

#define LOG_WARN spot::Logger(__FILE__, __LINE__, spot::Logger::WARN).stream()
#define LOG_ERROR spot::Logger(__FILE__, __LINE__, spot::Logger::ERR).stream()
#define LOG_FATAL spot::Logger(__FILE__, __LINE__, spot::Logger::FATAL).stream()

#ifdef __STOCK__
#define LOG_STOCK LOG_DEBUG
#else
#define LOG_STOCK LOG_INFO
#endif
const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::spot::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == NULL)
  {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}

#include <spot/utility/LoggerEx.h>

#endif  // MUDUO_BASE_LOGGING_H
