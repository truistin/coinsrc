#ifndef SPOT_NET_POLLER_EPOLLPOLLER_H
#define SPOT_NET_POLLER_EPOLLPOLLER_H
#ifdef __LINUX__
#include <spot/net/Poller.h>
#include <vector>
struct epoll_event;
namespace spot
{
	namespace net
	{
		class EPollPoller : public Poller
		{
		public:
			EPollPoller(EventLoop* loop);
			virtual ~EPollPoller();
			virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
			virtual void updateChannel(Channel* channel);
			virtual void removeChannel(Channel* channel);
		private:
			static const int kInitEventListSize = 16;
			static const char* operationToString(int op);
			void fillActiveChannels(int numEvents,
				ChannelList* activeChannels) const;
			void update(int operation, Channel* channel);
			typedef std::vector<struct epoll_event> EventList;
			int epollfd_;
			EventList events_;
		};
	}
}
#endif
#endif  