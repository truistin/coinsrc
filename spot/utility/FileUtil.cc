// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <spot/utility/FileUtil.h>
#include <spot/utility/Logging.h> // strerror_tl
#include <spot/utility/Compatible.h>
#include <spot/utility/Types.h>
#include <type_traits>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

using namespace spot;

FileUtil::AppendFile::AppendFile(StringArg filename)
  : fp_(::fopen(filename.c_str(), "a")),  // 'e' for O_CLOEXEC 'e' windows not mode 
    writtenBytes_(0)
{
  assert(fp_);
#ifdef __LINUX__
  ::setbuffer(fp_, buffer_, sizeof buffer_);
#endif
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

FileUtil::AppendFile::~AppendFile()
{
  ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* logline, const size_t len)
{
  size_t n = write(logline, len);
  size_t remain = len - n;
  while (remain > 0)
  {
    size_t x = write(logline + n, remain);
    if (x == 0)
    {
      int err = ferror(fp_);
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    n += x;
    remain = len - n; // remain -= x
  }

  writtenBytes_ += len;
}

void FileUtil::AppendFile::flush()
{
  ::fflush(fp_);
}

size_t FileUtil::AppendFile::write(const char* logline, size_t len)
{
  // #undef fwrite_unlocked
#ifdef __LINUX__
  return ::fwrite_unlocked(logline, 1, len, fp_); // not thread safe
#endif

#ifdef WIN32
	return ::fwrite(logline, 1, len, fp_);
#endif
}

FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
: fp_(::fopen(filename.c_str(), "rb")),
    err_(0)
{
  buf_[0] = '\0';
  if (fp_ ==  nullptr)
  {
    err_ = errno;
  }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
  if (fp_)
  {
    ::fclose(fp_); // FIXME: check EINTR
  }
}



// return errno
template<typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String* content,
                                          int64_t* fileSize,
                                          int64_t* modifyTime,
                                          int64_t* createTime)
{
//  static_assert(sizeof(off_t) == 8,"");
  assert(content != NULL);
  int err = err_;
  if (fp_ )
  {
    content->clear();

    if (fileSize)
    {
      struct stat statbuf;
      if (::fstat(fileno(fp_), &statbuf) == 0)
      {
				if (S_ISREG(statbuf.st_mode))
        {
          *fileSize = statbuf.st_size;
					//std::min => (std::min) avoid windows.h conflict
          content->reserve(static_cast<int>((std::min)(implicit_cast<int64_t>(maxSize), *fileSize)));
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
          err = EISDIR;
        }
        if (modifyTime)
        {
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime)
        {
          *createTime = statbuf.st_ctime;
        }
      }
      else
      {
        err = errno;
      }
    }

    while (content->size() < implicit_cast<size_t>(maxSize))
    {
      size_t toRead = (std::min)(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
			size_t n = ::fread(buf_, 1, toRead, fp_ );
      if (n > 0)
      {
        content->append(buf_, n);
      }
      else
      {
        if (n < 0)
        {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int* size)
{
  int err = err_;
  if (fp_)
  {
    size_t n = ::fread(buf_, 1,sizeof(buf_)-1, fp_);
    if (n > 0)
    {
      if (size)
      {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    }
		if (n == 0)
    {
      err = errno;
    }
  }
  return err;
}


template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                string* content,
                                int64_t*, int64_t*, int64_t*);

template int FileUtil::ReadSmallFile::readToString(
    int maxSize,
    string* content,
    int64_t*, int64_t*, int64_t*);

