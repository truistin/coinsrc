#ifndef SPOT_NET_UDPMCASTSERVER_H
#define SPOT_NET_UDPMCASTSERVER_H

#include <spot/utility/Types.h>
#include <spot/net/TcpConnection.h>
#include <spot/utility/NonCopyAble.h>
#include <spot/utility/Compatible.h>
#include <spot/utility/Logging.h>
#include <spot/net/SocketsOps.h>
#include <map>
#include <atomic>
#include <memory>
#include <functional>
#include <stdio.h>
namespace spot
{
	namespace net
	{
		class UdpMcastServer
		{
		public:
			UdpMcastServer(std::string multiCastIpPort, std::string interfaceIp);
			~UdpMcastServer();
			int sendMsg(const char* msg, int len);
		private:
			struct sockaddr_in addr_;
			int fd_;
		};

	}
}
#endif 
