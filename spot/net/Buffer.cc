// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <spot/net/Buffer.h>

#include <spot/net/SocketsOps.h>

#include <errno.h>

#ifdef __LINUX__
#include <sys/uio.h>
#endif
using namespace spot;
using namespace spot::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

SSIZE_T32 Buffer::readFd(int fd, int* savedErrno)
{
	// saved an ioctl()/FIONREAD call to tell how much to read

	const SIZE_T32 writable = writableBytes();

	// when there is enough space in this buffer, don't read into extrabuf.
	// when extrabuf is used, we read 128k-1 bytes at most.
	const SSIZE_T32 n = sockets::read(fd, begin() + writerIndex_, writable);
	if (n < 0)
	{
		*savedErrno = errno;
	}
	else if (implicit_cast<size_t>(n) <= writable)
	{
		writerIndex_ += n;
	}

	return n;
}

