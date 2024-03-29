// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_CHANNEL_H
#define SPOT_NET_CHANNEL_H
#include <spot/utility/Timestamp.h>
#include <spot/utility/NonCopyAble.h>

#include <functional>
#include <memory>

namespace spot
{
	namespace net
	{

		class EventLoop;

		///
		/// A selectable I/O channel.
		///
		/// This class doesn't own the file descriptor.
		/// The file descriptor could be a socket,
		/// an eventfd, a timerfd, or a signalfd
		class Channel : spot::utility::NonCopyable
		{
		public:
			typedef std::function<void()> EventCallback;
			typedef std::function<void(Timestamp)> ReadEventCallback;

			Channel(EventLoop* loop, int fd);
			~Channel();

			void handleEvent(Timestamp receiveTime);
			void setReadCallback(const ReadEventCallback& cb)
			{
				readCallback_ = cb;
			}
			void setWriteCallback(const EventCallback& cb)
			{
				writeCallback_ = cb;
			}
			void setCloseCallback(const EventCallback& cb)
			{
				closeCallback_ = cb;
			}
			void setErrorCallback(const EventCallback& cb)
			{
				errorCallback_ = cb;
			}


			/// Tie this channel to the owner object managed by shared_ptr,
			/// prevent the owner object being destroyed in handleEvent.
			void tie(const std::shared_ptr<void>&);

			int fd() const { return fd_; }
			int events() const { return events_; }
			void set_revents(int revt) { revents_ = revt; } // used by pollers

			bool isNoneEvent() const { return events_ == kNoneEvent; }

			void enableReading() { events_ |= kReadEvent; update(); }
			void disableReading() { events_ &= ~kReadEvent; update(); }
			void enableWriting() { events_ |= kWriteEvent; update(); }
			void disableWriting() { events_ &= ~kWriteEvent; update(); }
			void disableAll() { events_ = kNoneEvent; update(); }
			bool isWriting() const { return (events_ & kWriteEvent) != 0; }
			bool isReading() const { return (events_ & kReadEvent) != 0; }

			// for Poller
			int index() { return index_; }
			void set_index(int idx) { index_ = idx; }

			// for debug
			string reventsToString() const;
			string eventsToString() const;

			void doNotLogHup() { logHup_ = false; }

			EventLoop* ownerLoop() { return loop_; }
			void remove();
			static const int kNoneEvent;
			static const int kReadEvent;
			static const int kWriteEvent;
		private:
			static string eventsToString(int fd, int ev);

			void update();
			void handleEventWithGuard(Timestamp receiveTime);
			EventLoop* loop_;
			const int  fd_;
			int        events_;
			int        revents_; // it's the received event types of epoll or poll
			int        index_; // used by Poller.
			bool       logHup_;

			std::weak_ptr<void> tie_;
			bool tied_;
			bool eventHandling_;
			bool addedToLoop_;
			ReadEventCallback readCallback_;
			EventCallback writeCallback_;
			EventCallback closeCallback_;
			EventCallback errorCallback_;
		};

	}
}
#endif  // SPOT_NET_CHANNEL_H
