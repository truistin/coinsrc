#ifdef __WINDOWS__
#include <spot/net/WSAPollPoller.h>
#include <spot/utility/Logging.h>
#include <spot/net/Channel.h>
#include <spot/utility/Compatible.h>

using namespace spot;
using namespace spot::net;

WSAPollPoller::WSAPollPoller(EventLoop* loop)
	: Poller(loop)
{
}

WSAPollPoller::~WSAPollPoller()
{
}

Timestamp WSAPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	Timestamp now(Timestamp::now());
	if (pollfds_.size() > 0)
	{
		int numEvents = WSAPoll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
		int savedErrno = WSAGetLastError();

		if (numEvents > 0)
		{
			LOG_TRACE << numEvents << " events happended";
			fillActiveChannels(numEvents, activeChannels);
		}
		else if (numEvents == 0)
		{
			LOG_TRACE << " nothing happended";
		}
		else
		{
			if (savedErrno != EINTR)
			{
				errno = savedErrno;
				LOG_SYSERR << "PollPoller::poll()";
			}
		}
	}
	return now;
}

void WSAPollPoller::fillActiveChannels(int numEvents,
	ChannelList* activeChannels) const
{
	for (PollFdList::const_iterator pfd = pollfds_.begin();
	pfd != pollfds_.end() && numEvents > 0; ++pfd)
	{
		if (pfd->revents > 0)
		{
			--numEvents;
			ChannelMap::const_iterator ch = channels_.find(pfd->fd);
			assert(ch != channels_.end());
			Channel* channel = ch->second;
			assert(channel->fd() == pfd->fd);
			channel->set_revents(pfd->revents);
			// pfd->revents = 0;
			activeChannels->push_back(channel);
		}
	}
}

void WSAPollPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
	if (channel->index() < 0)
	{
		// a new one, add to pollfds_
		assert(channels_.find(channel->fd()) == channels_.end());
		struct pollfd pfd;
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;
		pollfds_.push_back(pfd);
		int idx = static_cast<int>(pollfds_.size()) - 1;
		channel->set_index(idx);
		channels_[pfd.fd] = channel;
	}
	else
	{
		// update existing one
		assert(channels_.find(channel->fd()) != channels_.end());
		assert(channels_[channel->fd()] == channel);
		int idx = channel->index();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
		struct pollfd& pfd = pollfds_[idx];
		assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;
		if (channel->isNoneEvent())
		{
			// ignore this pollfd
			pfd.fd = -channel->fd() - 1;
		}
	}
}

void WSAPollPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd();
	assert(channels_.find(channel->fd()) != channels_.end());
	assert(channels_[channel->fd()] == channel);
	assert(channel->isNoneEvent());
	int idx = channel->index();
	assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
	const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
	assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
	size_t n = channels_.erase(channel->fd());
	assert(n == 1); (void)n;
	if (implicit_cast<size_t>(idx) == pollfds_.size() - 1)
	{
		pollfds_.pop_back();
	}
	else
	{
		int channelAtEnd = pollfds_.back().fd;
		iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
		if (channelAtEnd < 0)
		{
			channelAtEnd = -channelAtEnd - 1;
		}
		channels_[channelAtEnd]->set_index(idx);
		pollfds_.pop_back();
	}
}
#endif