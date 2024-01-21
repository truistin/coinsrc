#include <ftmd/net/SelectPoller.h>
#include <ftmd/base/Logging.h>
#include <ftmd/base/Types.h>
#include <ftmd/net/Channel.h>

using namespace ftmd;
using namespace ftmd::net;


SelectPoller::SelectPoller(EventLoop* loop)
  : Poller(loop)
{
}

SelectPoller::~SelectPoller()
{
}

Timestamp SelectPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  Timestamp now(Timestamp::now());
  std::int32_t sTimeOut = 1000;
  struct timeval timeout;
	timeout.tv_sec = sTimeOut / 1000000;
	timeout.tv_usec = sTimeOut % 1000000;
  int ret = 0;
  ret = select(maxId_, &readfds, &writefds, NULL, &timeout);
  if (ret > 0)
  {
		LOG_TRACE << ret << " events happended";
    fillActiveChannels(readfds, writefds,activeChannels);
	}
  else
  {
    int savedErrno = errno;
    LOG_SYSERR << "SelectPoller::poll()";
  }
  return now;
}

void SelectPoller::fillActiveChannels(fd_set readfds,fd_set writefds,
                          ChannelList* activeChannels) const
{
  for(auto &iter : *activeChannels)
  {
    	int nReadID = 0;
      nReadID = iter->fd();
      if (nReadID<0 || (nReadID>0 && FD_ISSET(nReadID,&readfds)))
      {
        iter->set_revents(POLLIN);
        activeChannels->push_back(iter);
      }

      int nWriteID = 0;
      nWriteID = iter->fd();
      if (nWriteID<0 || (nWriteID>0 && FD_ISSET(nWriteID, &writefds)))
			{
				 iter->set_revents(POLLOUT);
        activeChannels->push_back(iter);
			}
  }
}
void SelectPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)
  {
    assert(channels_.find(channel->fd()) == channels_.end());

    FD_ZERO(&readfds);
	  FD_ZERO(&writefds);

    if (channel->isReading())
    {
      int readId = channel->fd();
      FD_SET(readId, &readfds);
			if(maxId_ < readId)
			{
				maxId_ = readId;
			}
      
    }
    if (channel->isWriting())
    {
      int writeId = channel->fd();
      FD_SET(writeId, &writefds);
			if(maxId_ < writeId)
			{
				maxId_ = writeId;
			}
      
    }
    int idx = -1;
    channel->set_index(idx);
    channels_[channel->fd()] = channel;
  }
}


void SelectPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());

  int idx = channel->index();
  size_t n = channels_.erase(channel->fd());
}