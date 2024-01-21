#ifndef SPOT_UTILITY_LOGGINGEX_H
#define SPOT_UTILITY_LOGGINGEX_H

#include <spot/utility/Logging.h>
#include <string>

namespace spot
{
	class LoggerEx
	{
	public:
		typedef void(*LoggerExCallback)(const std::string& fatalMsg);
		static void setCallback(LoggerExCallback cb);

		LoggerEx(spot::Logger::SourceFile file, int line, spot::Logger::LogLevel level);
		~LoggerEx();
		LogStream& stream();

	private:
		static LoggerExCallback cb_;
		spot::Logger logger_;
	};
}

#ifdef LOG_FATAL
#undef LOG_FATAL
#define LOG_FATAL spot::LoggerEx(__FILE__, __LINE__, spot::Logger::FATAL).stream()
#endif

#endif //SPOT_UTILITY_LOGGINGEX_H
