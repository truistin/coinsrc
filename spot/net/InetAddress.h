// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef SPOT_NET_INETADDRESS_H
#define SPOT_NET_INETADDRESS_H

#include <spot/utility/Copyable.h>
#include <spot/utility/StringPiece.h>
#include <spot/utility/Compatible.h>


namespace spot
{
	namespace net
	{
		namespace sockets
		{
			struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
			const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
		}

		///
		/// Wrapper of sockaddr_in.
		///
		/// This is an POD interface class.
		class InetAddress : public spot::Copyable
		{
		public:
			/// Constructs an endpoint with given port number.
			/// Mostly used in TcpServer listening.
			explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);

			/// Constructs an endpoint with given ip and port.
			/// @c ip should be "1.2.3.4"
			InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

			/// Constructs an endpoint with given struct @c sockaddr_in
			/// Mostly used when accepting new connections
			explicit InetAddress(const struct sockaddr_in& addr)
				: addr_(addr)
			{ }

			ADDRESS_FAMILY family() const { return addr_.sin_family; }
			string toIp() const;
			string toIpPort() const;
			uint16_t toPort() const;

			// default copy/assignment are Okay

			const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr_); }

			void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

			uint32_t ipNetEndian() const;
			uint16_t portNetEndian() const { return addr_.sin_port; }

			// resolve hostname to IP address, not changing port or sin_family
			// return true on success.
			// thread safe
			static bool resolve(StringArg hostname, InetAddress* result);
			// static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

		private:
			union
			{
				struct sockaddr_in addr_;
			};
		};

	}
}

#endif  // SPOT_NET_INETADDRESS_H
