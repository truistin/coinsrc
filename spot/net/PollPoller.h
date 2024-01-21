#ifndef SPOT_NET_POLLER_POLLPOLLER_H
#define SPOT_NET_POLLER_POLLPOLLER_H
#ifdef __LINUX__
#include <spot/net/Poller.h>
#include <vector>
struct pollfd;
namespace spot
{
	namespace net
	{
		class PollPoller : public Poller
		{
		public:
			PollPoller(EventLoop* loop);
			virtual ~PollPoller();
			virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
			virtual void updateChannel(Channel* channel);
			virtual void removeChannel(Channel* channel);
		private:
			void fillActiveChannels(int numEvents,
			ChannelList* activeChannels) const;
			typedef std::vector<struct pollfd> PollFdList;
			PollFdList pollfds_;
		};
	}
}
#endif  
#endif
