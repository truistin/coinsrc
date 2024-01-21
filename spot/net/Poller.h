#ifndef SPOT_NET_POLLER_H
#define SPOT_NET_POLLER_H

#include <map>
#include <vector>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Timestamp.h>
#include <spot/net/EventLoop.h>

namespace spot
{
	namespace net
	{

		class Channel;

#define FD_EVENT int
		///
		/// Base class for IO Multiplexing
		///
		/// This class doesn't own the Channel objects.
		class Poller : spot::utility::NonCopyable
		{
		public:
			typedef std::vector<Channel*> ChannelList;

			Poller(EventLoop* loop);
			virtual ~Poller();

			/// Polls the I/O events.
			/// Must be called in the loop thread.
			virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

			/// Changes the interested I/O events.
			/// Must be called in the loop thread.
			virtual void updateChannel(Channel* channel) = 0;

			/// Remove the channel, when it destructs.
			/// Must be called in the loop thread.
			virtual void removeChannel(Channel* channel);

			virtual bool hasChannel(Channel* channel) const;

			static Poller* newDefaultPoller(EventLoop* loop);

			void assertInLoopThread() const
			{
				ownerLoop_->assertInLoopThread();
			}

		protected:
			typedef std::map<int, Channel*> ChannelMap;
			ChannelMap channels_;

		private:
			EventLoop* ownerLoop_;
		};

	}
}
#endif  
