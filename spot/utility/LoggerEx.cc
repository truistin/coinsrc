#include <spot/utility/LoggerEx.h>

using namespace spot;
using namespace spot::utility;

LoggerEx::LoggerExCallback LoggerEx::cb_ = nullptr;

void LoggerEx::setCallback(LoggerExCallback cb)
{
	cb_ = cb;
}

LoggerEx::LoggerEx(spot::Logger::SourceFile file, int line, spot::Logger::LogLevel level)
	:logger_(file, line, level)
{
}

LoggerEx::~LoggerEx()
{
	if (cb_) {
		cb_(logger_.stream().buffer().data());
	}
}

LogStream& LoggerEx::stream()
{
	return logger_.stream();
}
