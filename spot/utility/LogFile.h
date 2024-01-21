#ifndef SPOT_UTILITY_LOGFILE_H
#define SPOT_UTILITY_LOGFILE_H

#include <mutex>
#include <memory>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Types.h>
namespace spot
{
namespace ProcessInfo
{
	uint64_t pid();
	string hostname();
}
namespace FileUtil
{
class AppendFile;
}

class LogFile : utility::NonCopyable
{
 public:
  LogFile(const string& basename,
          size_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();
  static string getSpotLogFileName(int spotid, const string& basename);

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;
  const size_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  std::unique_ptr<std::mutex> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
	std::unique_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};

}
#endif  // MUDUO_BASE_LOGFILE_H
