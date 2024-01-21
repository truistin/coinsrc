#ifndef FTMD_NET_SELECTPOLLER_H
#define FTMD_NET_SELECTPOLLER_H
#include <ftmd/net/Poller.h>

struct fd_set;
namespace ftmd
{
namespace net
{
class SelectPoller : public Poller
{
 public:
  SelectPoller(EventLoop* loop);
  virtual ~SelectPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  void fillActiveChannels(fd_set readfds,fd_set writefds,
                          ChannelList* activeChannels) const;

  fd_set readfds, writefds;
  int maxId_;
};
}
}


#endif