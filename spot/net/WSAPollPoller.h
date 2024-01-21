#ifndef SPOT_NET_WSAPOLLER_POLLPOLLER_H
#define SPOT_NET_WSAPOLLER_POLLPOLLER_H
#ifdef __WINDOWS__
#include <spot/net/Poller.h>
#include <vector>
namespace spot
{
	namespace net
	{
		class WSAPollPoller : public Poller
		{
		public:
			WSAPollPoller(EventLoop* loop);
			virtual ~WSAPollPoller();
			virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
			virtual void updateChannel(Channel* channel);
			virtual void removeChannel(Channel* channel);
		private:
			void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
			typedef std::vector<WSAPOLLFD> PollFdList;
			PollFdList pollfds_;
		};
	}
}
#endif 
#endif