#include <iostream>
#include <string>
#include <utility>
#include <spot/net/EventLoop.h>
#include <spot/net/Channel.h>
#include <spot/net/WSAPollPoller.h>
#include <spot/net/SocketsOps.h>
#include <spot/utility/Logging.h>
#include <spot/net/TimerQueue.h>
#include <spot/net/Poller.h>
#include<spot/utility/Compatible.h>

using namespace spot;
using namespace spot::net;

const int kPollTimeMillSecond = 1;

namespace spot
{
	namespace net
	{
		WSWakeUpfd createWakeUpfd()
		{
			WSWakeUpfd wakeupFd;
#ifdef __LINUX__
			int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
			if (evtfd < 0)
			{
				LOG_SYSERR << "Failed in eventfd";
				abort();
			}
			wakeupFd.wakeupFd_ = evtfd;
			wakeupFd.notifyFd_ = evtfd;
			return wakeupFd;
#else
			WSADATA wsaData = { 0 };
			int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			SOCKET listener = INVALID_SOCKET;
			listener = socket(AF_INET, SOCK_STREAM, 0);
			if (listener == INVALID_SOCKET)
			{
				wakeupFd.reset();
				return wakeupFd;
			}
			BOOL so_reuseaddr = 1;
			int rc = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
				(char *)&so_reuseaddr, sizeof so_reuseaddr);
			BOOL tcp_nodelay = 1;
			rc = setsockopt(listener, IPPROTO_TCP, TCP_NODELAY,
				(char *)&tcp_nodelay, sizeof tcp_nodelay);

			struct sockaddr_in addr;
			memset(&addr, 0, sizeof addr);
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr.sin_port = htons(6905);


			wakeupFd.notifyFd_ = socket(AF_INET, SOCK_STREAM, 0);
			if (wakeupFd.notifyFd_ == INVALID_SOCKET)
			{
				wakeupFd.reset();
				return wakeupFd;
			}

			rc = setsockopt(wakeupFd.notifyFd_, IPPROTO_TCP, TCP_NODELAY,
				(char *)&tcp_nodelay, sizeof tcp_nodelay);

			rc = bind(listener, (const struct sockaddr *) &addr, sizeof addr);

			if (rc != SOCKET_ERROR)
				rc = listen(listener, 1);
			if (rc != SOCKET_ERROR)
				rc = connect(wakeupFd.notifyFd_, (struct sockaddr *) &addr, sizeof addr);
			if (rc != SOCKET_ERROR)
				wakeupFd.wakeupFd_ = accept(listener, NULL, NULL);

			int saved_errno = 0;
			if (wakeupFd.wakeupFd_ == INVALID_SOCKET)
				saved_errno = WSAGetLastError();
			rc = closesocket(listener);

			return wakeupFd;
#endif
		}
	}
}

EventLoop::EventLoop()
	: looping_(false),
	quit_(false),
	callingPendingFunctors_(false),
	currentActiveChannel_(NULL),
	wakeupFd_(createWakeUpfd()),
	wakeupChannel_(new Channel(this, wakeupFd_.wakeupFd_))
{
	poller_.reset(Poller::newDefaultPoller(this)),
		threadId_ = currentthread::tid();
	timerQueue_.reset(new TimerQueue(this));
	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	wakeupChannel_->disableAll();
	removeChannel(wakeupChannel_.get());
}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in threadId_ = " << threadId_
		<< ", current thread id = " << currentthread::tid();
}

void EventLoop::updateChannel(Channel* channel)
{
	//  assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	//  assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (eventHandling_)
	{
		assert(currentActiveChannel_ == channel ||
			std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
	}
	poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
	//  assert(channel->ownerLoop() == this);
	assertInLoopThread();
	return poller_->hasChannel(channel);
}
void EventLoop::loop()
{
	assert(!looping_);//避免重复调用loop
	assertInLoopThread();
	looping_ = true;

	quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
	LOG_TRACE << "EventLoop " << this << " start looping";

	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMillSecond, &activeChannels_);
		++iteration_;

		eventHandling_ = true;
		for (ChannelList::iterator it = activeChannels_.begin();
		it != activeChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();

#ifdef __WINDOWS__
		// 未考虑多线程的唤醒
		for (ChannelList::iterator it = timerChannels_.begin(); it != timerChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->set_revents(Channel::kReadEvent);
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
#endif
	}
}

void EventLoop::quit()
{
	quit_ = true;
	// There is a chance that loop() just executes while(!quit_) and exits,
	// then EventLoop destructs, then we are accessing an invalid object.
	// Can be fixed using mutex_ in both places.
	if (!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}
void EventLoop::wakeup()
{
	uint64_t one = 1;
	SSIZE_T32 n = sockets::write(wakeupFd_.notifyFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void EventLoop::handleRead()
{
	//std::cout << "handleRead" << std::endl;
	uint64_t one = 1;
	int32_t n = sockets::read(wakeupFd_.wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
	{
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of " << sizeof(one);
	}
}
void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

void EventLoop::insertTimerChannel(Channel *channel)
{
	timerChannels_.push_back(channel);
}

TimerId EventLoop::runAt(const Timestamp& time, TimerCallback&& cb)
{
	return timerQueue_->addTimer(std::move(cb), time, 0.0);
}
void EventLoop::cancel(TimerId timerId)
{
	timerQueue_->cancel(timerId);
}

TimerId EventLoop::runAfter(double delay, TimerCallback&& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback&& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(std::move(cb), time, interval);
}
void EventLoop::assertInLoopThread()
{
	if (!isInLoopThread())
	{
		abortNotInLoopThread();
	}
}
bool EventLoop::isInLoopThread() const
{
	return threadId_ == currentthread::tid();
}
uint64_t EventLoop::threadId()
{
	return threadId_;
}
std::vector<Functor>& EventLoop::pendingFunctors()
{
	return pendingFunctors_;
}