// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_CONNECTOR_H
#define SPOT_NET_CONNECTOR_H

#include <spot/net/InetAddress.h>
#include <spot/utility/NonCopyAble.h>
#include <memory>
#include <functional>
#include<atomic>

//#include <std/enable_shared_from_this.hpp>
//#include <std/function.hpp>
//#include <std/noncopyable.hpp>
//#include <std/scoped_ptr.hpp>

namespace spot
{
	namespace net
	{

		class Channel;
		class EventLoop;

		class Connector : spot::utility::NonCopyable,
			public std::enable_shared_from_this<Connector>
		{
		public:
			typedef std::function<void(int sockfd)> NewConnectionCallback;
			typedef std::function<void(int timeLapse)> ConnectionRetryCallback;

			Connector(EventLoop* loop, const InetAddress& serverAddr);
			~Connector();

			void setNewConnectionCallback(const NewConnectionCallback& cb)
			{
				newConnectionCallback_ = cb;
			}

			void setConnectionRetryCallback(const ConnectionRetryCallback& cb)
			{
				connectionRetryCallback_ = cb;
			}

			void start();  // can be called in any thread
			void restart();  // must be called in loop thread
			void stop();  // can be called in any thread

			const InetAddress& serverAddress() const { return serverAddr_; }

		private:
			enum States { kDisconnected, kConnecting, kConnected };
			static const int kMaxRetryDelayMs = 30 * 1000;
			static const int kInitRetryDelayMs = 500;

			void setState(States s) { state_.store(s); }
			void startInLoop();
			void stopInLoop();
			void connect();
			void connecting(int sockfd);
			void handleWrite();
			void handleError();
			void retry(int sockfd);
			int removeAndResetChannel();
			void resetChannel();
			int connectRetriable(int err, int fd);
			bool connectRefused(int err);
			EventLoop* loop_;
			InetAddress serverAddr_;
			bool connect_; // atomic
			std::atomic<States> state_;  // FIXME: use atomic variable
			std::unique_ptr<Channel> channel_;
			NewConnectionCallback newConnectionCallback_;
			ConnectionRetryCallback connectionRetryCallback_;
			int retryDelayMs_;
		};

	}
}

#endif  // SPOT_NET_CONNECTOR_H
