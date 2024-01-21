// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <spot/net/TcpConnection.h>

#include <spot/utility/Logging.h>
#include <spot/utility/WeakCallback.h>
#include <spot/net/Channel.h>
#include <spot/net/EventLoop.h>
#include <spot/net/Socket.h>
#include <spot/net/SocketsOps.h>
#include <functional>
#include <utility>
#include <errno.h>

#ifdef __LINUX__
#include <sys/ioctl.h>
#include <net/if.h>
#endif

using namespace spot;
using namespace spot::net;
using namespace std::placeholders;

void spot::net::defaultConnectionRetryCallback(int timeLapse)
{
}

void spot::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LOG_TRACE << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
	// do not call conn->forceClose(), because some users want to register message callback only.
}

void spot::net::defaultMessageCallback(const TcpConnectionPtr&,
	Buffer* buf,
	Timestamp)
{
	buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
	const string& nameArg,
	int sockfd,
	const InetAddress& localAddr,
	const InetAddress& peerAddr)
	: loop_(CHECK_NOTNULL(loop)),
	name_(nameArg),
	state_(kConnecting),
	socket_(new Socket(sockfd)),
	channel_(new Channel(loop, sockfd)),
	localAddr_(localAddr),
	peerAddr_(peerAddr),
	highWaterMark_(64 * 1024 * 1024),
	reading_(true)
{
	channel_->setReadCallback(
		std::bind(&TcpConnection::handleRead, this, _1));
	channel_->setWriteCallback(
		std::bind(&TcpConnection::handleWrite, this));
	channel_->setCloseCallback(
		std::bind(&TcpConnection::handleClose, this));
	channel_->setErrorCallback(
		std::bind(&TcpConnection::handleError, this));
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
		<< " fd=" << sockfd;

	macAddr_ = getpeermac(sockfd);
	socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
		<< " fd=" << channel_->fd()
		<< " state=" << stateToString();
	assert(state_.load() == kDisconnected);
}

string TcpConnection::getpeermac(int sockfd)
{
	int ret = 0;
	char mac[32] = { 0 };
#ifdef __LINUX__
	struct ifreq ifr;
	struct ifconf ifc;
	ret = ::ioctl(sockfd, SIOCGIFHWADDR, &ifr);
	int i;
	for (i = 0; i < 6; ++i)
	{
		sprintf(mac + 3 * i, "%02x:", (unsigned char)ifr.ifr_hwaddr.sa_data[i]);
	}
#endif
#ifdef WIN32
	return "";
#endif

	return mac;
}
string TcpConnection::getTcpInfoString() const
{
	char buf[1024];
	buf[0] = '\0';
	socket_->getTcpInfoString(buf, sizeof buf);
	return buf;
}

void TcpConnection::send(const void* data, int len)
{
	send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
	if (state_.load() == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			loop_->runInLoop(std::bind(&TcpConnection::bindSendInLoop, this, message.as_string()));

		}
	}
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
	if (state_.load() == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(buf->peek(), buf->readableBytes());
			buf->retrieveAll();
		}
		else
		{
			loop_->runInLoop(
				std::bind(&TcpConnection::bindSendInLoop,
					this,     // FIXME
					buf->retrieveAllAsString()));

		}
	}
}
void TcpConnection::bindSendInLoop(const string& message)
{
	sendInLoop(message.data(), message.size());
}
void TcpConnection::sendInLoop(const StringPiece& message)
{
	sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
	loop_->assertInLoopThread();
	SSIZE_T32 nwrote = 0;
	SIZE_T32 remaining = len;
	bool faultError = false;
	if (state_.load() == kDisconnected)
	{
		LOG_WARN << "disconnected, give up writing";
		return;
	}
	// if no thing in output queue, try writing directly
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
	{
		nwrote = sockets::write(channel_->fd(), data, len);
		if (nwrote >= 0)
		{
			remaining = len - nwrote;
			if (remaining == 0 && writeCompleteCallback_)
			{
				loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
			}
		}
		else // nwrote < 0
		{
			LOG_INFO << "TcpConnection::sendInLoop nwrote:" << nwrote;
			nwrote = 0;
			if (errno != EWOULDBLOCK)
			{
				int error_no = errno;
				LOG_SYSERR << "TcpConnection::sendInLoop errno:" << error_no;
				if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
				{
					faultError = true;
				}
			}
		}
	}

	assert(remaining <= len);
	if (!faultError && remaining > 0)
	{
		size_t oldLen = outputBuffer_.readableBytes();
		if (oldLen + remaining >= highWaterMark_
			&& oldLen < highWaterMark_
			&& highWaterMarkCallback_)
		{
			loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
		}
		outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
		if (!channel_->isWriting())
		{
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown()
{
	// FIXME: use compare and swap
	if (state_.load() == kConnected)
	{
		setState(kDisconnecting);
		// FIXME: shared_from_this()?
		loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop_->assertInLoopThread();
	if (!channel_->isWriting())
	{
		// we are not writing
		socket_->shutdownWrite();
	}
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose()
{
	// FIXME: use compare and swap
	if (state_.load() == kConnected || state_.load() == kDisconnecting)
	{
		setState(kDisconnecting);
		loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
	}
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
	if (state_.load() == kConnected || state_.load() == kDisconnecting)
	{
		setState(kDisconnecting);
		loop_->runAfter(
			seconds,
			makeWeakCallback(shared_from_this(),
				&TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
	}
}

void TcpConnection::forceCloseInLoop()
{
	loop_->assertInLoopThread();
	if (state_.load() == kConnected || state_.load() == kDisconnecting)
	{
		// as if we received 0 byte in handleRead();
		handleClose();
	}
}

const char* TcpConnection::stateToString() const
{
	switch (state_.load())
	{
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

void TcpConnection::setTcpNoDelay(bool on)
{
	socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
	loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
	loop_->assertInLoopThread();
	if (!reading_ || !channel_->isReading())
	{
		channel_->enableReading();
		reading_ = true;
	}
}

void TcpConnection::stopRead()
{
	loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
	loop_->assertInLoopThread();
	if (reading_ || channel_->isReading())
	{
		channel_->disableReading();
		reading_ = false;
	}
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	assert(state_.load() == kConnecting);
	setState(kConnected);
	channel_->tie(shared_from_this());
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	if (state_.load() == kConnected)
	{
		setState(kDisconnected);
		channel_->disableAll();

		connectionCallback_(shared_from_this());
	}
	channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
	loop_->assertInLoopThread();
	int savedErrno = 0;
	SSIZE_T32 n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0)
	{
		messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	}
	else if (n == 0)
	{
		LOG_INFO << "handleRead read len:" << n << " readableBytes:" << inputBuffer_.readableBytes()
			<< " writableBytes:" << inputBuffer_.writableBytes();
		handleClose();
	}
	else
	{
		errno = savedErrno;
		LOG_SYSERR << "TcpConnection::handleRead";
		handleError();
	}
}

void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		SSIZE_T32 n = sockets::write(channel_->fd(),
			outputBuffer_.peek(),
			outputBuffer_.readableBytes());
		if (n > 0)
		{
			outputBuffer_.retrieve(n);
			if (outputBuffer_.readableBytes() == 0)
			{
				channel_->disableWriting();
				if (writeCompleteCallback_)
				{
					loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
				}
				if (state_.load() == kDisconnecting)
				{
					LOG_INFO << "handleWrite state is kDisconnecting";
					shutdownInLoop();
				}
			}
		}
		else
		{
			LOG_SYSERR << "TcpConnection::handleWrite";
			// if (state_ == kDisconnecting)
			// {
			//   shutdownInLoop();
			// }
		}
	}
	else
	{
		LOG_TRACE << "Connection fd = " << channel_->fd()
			<< " is down, no more writing";
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	LOG_INFO << "fd = " << channel_->fd() << " state = " << stateToString();
	assert(state_.load() == kConnected || state_.load() == kDisconnecting);
	// we don't close fd, leave it to dtor, so we can find leaks easily.
	setState(kDisconnected);
	channel_->disableAll();

	TcpConnectionPtr guardThis(shared_from_this());
	connectionCallback_(guardThis);
	// must be the last line
	closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
	int err = sockets::socketGetError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError [" << name_
		<< "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

