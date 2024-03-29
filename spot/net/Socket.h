// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef SPOT_NET_SOCKET_H
#define SPOT_NET_SOCKET_H

#include <spot/utility/NonCopyAble.h>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace spot
{
	///
	/// TCP networking.
	///
	namespace net
	{

		class InetAddress;

		///
		/// Wrapper of socket file descriptor.
		///
		/// It closes the sockfd when desctructs.
		/// It's thread safe, all operations are delegated to OS.
		class Socket : spot::utility::NonCopyable
		{
		public:
			explicit Socket(int sockfd)
				: sockfd_(sockfd)
			{ }

			// Socket(Socket&&) // move constructor in C++11
			~Socket();

			int fd() const { return sockfd_; }

			bool getTcpInfoString(char* buf, int len) const;

			/// abort if address in use
			void bindAddress(const InetAddress& localaddr);
			/// abort if address in use
			void listen();

			/// On success, returns a non-negative integer that is
			/// a descriptor for the accepted socket, which has been
			/// set to non-blocking and close-on-exec. *peeraddr is assigned.
			/// On error, -1 is returned, and *peeraddr is untouched.
			int accept(InetAddress* peeraddr);

			void shutdownWrite();

			///
			/// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
			///
			void setTcpNoDelay(bool on);

			///
			/// Enable/disable SO_REUSEADDR
			///
			void setReuseAddr(bool on);

			///
			/// Enable/disable SO_REUSEPORT
			///
			void setReusePort(bool on);

			///
			/// Enable/disable SO_KEEPALIVE
			///
			void setKeepAlive(bool on);

		private:
			const int sockfd_;
		};

	}
}
#endif  // SPOT_NET_SOCKET_H
