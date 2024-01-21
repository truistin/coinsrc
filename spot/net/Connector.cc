// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <spot/net/Connector.h>

#include <spot/utility/Logging.h>
#include <spot/net/Channel.h>
#include <spot/net/EventLoop.h>
#include <spot/net/SocketsOps.h>
#include <functional>
#include <errno.h>

using namespace spot;
using namespace spot::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
	: loop_(loop),
	serverAddr_(serverAddr),
	connect_(false),
	retryDelayMs_(kInitRetryDelayMs)
{
	state_.store(kDisconnected);
}

Connector::~Connector()
{
	assert(!channel_);
}

void Connector::start()
{
	connect_ = true;
	loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
	loop_->assertInLoopThread();
	assert(state_.load() == kDisconnected);
	if (connect_)
	{
		connect();
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void Connector::stop()
{
	connect_ = false;
	loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
	// FIXME: cancel timer
}

void Connector::stopInLoop()
{
	loop_->assertInLoopThread();
	if (state_.load() == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		retry(sockfd);
	}
}

int Connector::connectRetriable(int e, int fd)
{
#ifdef __LINUX__
	if ((e) == EINTR || (e) == EINPROGRESS || (e) == 0)
	{
		return 0;
	}
	else if ((e) == ECONNREFUSED || (e) == ENETUNREACH || (e) == EAGAIN)
	{
		return 1;
	}
	else
	{
		return -1;
	}
#endif
#ifdef WIN32
	if (e == WSAEWOULDBLOCK)
	{
		fd_set Write, Err;
		TIMEVAL Timeout;
		int TimeoutSec = 10; // timeout after 10 seconds
		FD_ZERO(&Write);
		FD_ZERO(&Err);
		FD_SET(fd, &Write);
		FD_SET(fd, &Err);
		Timeout.tv_sec = TimeoutSec;
		Timeout.tv_usec = 0;
		int iResult = select(0, NULL, &Write, &Err, &Timeout);
		if (iResult == 0)
		{
			return 1;
		}
		else
		{
			if (FD_ISSET(fd, &Write))
			{
				return 0;
			}
			if (FD_ISSET(fd, &Err))
			{
				return 1;
			}
		}
	}
	if ((e) == 0 ||
		(e) == WSAEINTR || \
		(e) == WSAEINPROGRESS || \
		(e) == WSAEINVAL)
	{
		return 0;
	}
	else if ((e) == WSAECONNREFUSED || (e) == WSAENETUNREACH || (e) == EAGAIN)
	{
		return 1;
	}
	else
	{
		return -1;
	}
#endif
}
bool Connector::connectRefused(int e)
{
#ifdef __LINUX__
	if ((e) == ECONNREFUSED || (e) == ENETUNREACH || (e) == EAGAIN)
	{
		return true;
	}
	else
	{
		return false;
	}
#endif
#ifdef WIN32
	if ((e) == WSAECONNREFUSED || (e) == WSAENETUNREACH || (e) == EAGAIN || (e) == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
#endif
}
void Connector::connect()
{
	int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
	if (sockfd < 0)
	{
		return;
	}

	//»º³åÇø²éÑ¯
	int snd_size = 0;
	int rev_size = 0;
	int optLen = sizeof(int);
	int err = ::getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&snd_size, (socklen_t*)&optLen);
	err = ::getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&rev_size, (socklen_t*)&optLen);
	//std::cout << "socket created, sendBuf size:" << snd_size << ", revcBuf size:" << rev_size << std::endl;

	int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());

	int savedErrno = (ret == 0) ? 0 : sockets::socketGetError(sockfd);
	int connres = connectRetriable(savedErrno, sockfd);
	if (connres == 0)
	{
		connecting(sockfd);
		return;
	}
	else if (connres == 1)
	{
		retry(sockfd);
		return;
	}
	LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
	sockets::socketClose(sockfd);

}

void Connector::restart()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->setWriteCallback(
		std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
	channel_->setErrorCallback(
		std::bind(&Connector::handleError, this)); // FIXME: unsafe

// channel_->tie(shared_from_this()); is not working,
// as channel_ is not managed by shared_ptr
	channel_->enableWriting();

}

int Connector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove();
	int sockfd = channel_->fd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
	return sockfd;
}

void Connector::resetChannel()
{
	channel_.reset();
}

void Connector::handleWrite()
{
	LOG_TRACE << "Connector::handleWrite " << state_.load();

	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::socketGetError(sockfd);
		if (err)
		{
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
				<< err << " " << strerror_tl(err);
			retry(sockfd);
		}
		else if (sockets::isSelfConnect(sockfd))
		{
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd);
		}
		else
		{
			setState(kConnected);
			if (connect_)
			{
				newConnectionCallback_(sockfd);
			}
			else
			{
				sockets::socketClose(sockfd);
			}
		}
	}
	else
	{
		// what happened?
		assert(state_.load() == kDisconnected);
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" << state_.load();
	if (state_.load() == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::socketGetError(sockfd);
		LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
		retry(sockfd);
	}
}

void Connector::retry(int sockfd)
{
	connectionRetryCallback_(retryDelayMs_);
	sockets::socketClose(sockfd);
	setState(kDisconnected);
	if (connect_)
	{
		LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
			<< " in " << retryDelayMs_ << " milliseconds. ";
		loop_->runAfter(retryDelayMs_ / 1000.0,
			std::bind(&Connector::startInLoop, shared_from_this()));
		retryDelayMs_ = MIN((retryDelayMs_ * 2), kMaxRetryDelayMs);
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

