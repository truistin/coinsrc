#include <inttypes.h>
#include <spot/utility/LogFile.h>
#include <spot/utility/FileUtil.h>
#include <spot/utility/Compatible.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace spot;

string ProcessInfo::hostname()
{
	// HOST_NAME_MAX 64
	// _POSIX_HOST_NAME_MAX 255
	char buf[256];
	if (gethostname(buf, sizeof buf) == 0)
	{
		buf[sizeof(buf)-1] = '\0';
		return buf;
	}
	else
	{
		return "unknownhost";
	}
}

PID_T ProcessInfo::pid()
{
	return ::getpid();
}


LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new std::mutex : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  
	//assert(basename.find(DIRDELIMIER) != string::npos);
  rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
  if (mutex_)
  {
    std::lock_guard<std::mutex> lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
		std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ >= flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

bool LogFile::rollFile()
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new FileUtil::AppendFile(filename.c_str()));
    return true;
  }
  return false;
}
string LogFile::getSpotLogFileName(int spotid, const string& basename)
{
	time_t now = 0;
	string filename;
	filename.reserve(basename.size() + 64);
	filename = basename + "." + std::to_string(spotid);;

	char timebuf[32];
	now = time(NULL);
#ifdef __LINUX__
	struct tm tm;
	localtime_r(&now, &tm);
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
#endif

#ifdef WIN32
	tm *ptm = localtime(&now);
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", ptm);
#endif
	filename += timebuf;

	filename += ProcessInfo::hostname();

	char pidbuf[32];
	SNPRINTF(pidbuf, sizeof pidbuf, ".%lld", ProcessInfo::pid());
	filename += pidbuf;

	filename += ".log";

	return filename;
}
string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];	
	*now = time(NULL);
#ifdef __LINUX__
	struct tm tm;
  localtime_r(now, &tm);
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
#endif

#ifdef WIN32
	tm *ptm = localtime(now);
	strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", ptm);
#endif
  filename += timebuf;

  filename += ProcessInfo::hostname();

  char pidbuf[32];
	SNPRINTF(pidbuf, sizeof pidbuf, ".%lld", ProcessInfo::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}

