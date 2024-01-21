#include <spot/net/Poller.h>
#include <spot/net/Channel.h>
using namespace spot;
using namespace spot::net;

Poller::Poller(EventLoop* loop)
	: ownerLoop_(loop)
{
}

Poller::~Poller()
{
}
void Poller::removeChannel(Channel* channel)
{
	assertInLoopThread();
	auto iter = channels_.find(channel->fd());
	if (iter != channels_.end())
	{
		channels_.erase(iter);
	}
}
bool Poller::hasChannel(Channel* channel) const
{
	assertInLoopThread();
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}

