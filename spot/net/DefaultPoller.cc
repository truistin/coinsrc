// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <spot/net/Poller.h>
#include <spot/net/PollPoller.h>
#include <spot/net/EPollPoller.h>
#include <spot/net/WSAPollPoller.h>
#include <stdlib.h>

using namespace spot::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
#ifdef __LINUX__
	if (::getenv("MUDUO_USE_POLL"))
	{
		return new PollPoller(loop);
	}
	else
	{
		return new EPollPoller(loop);
	}
#else
	return new WSAPollPoller(loop);
#endif
}
