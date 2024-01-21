#ifndef SPOT_NET_SOCKETSOPS_H
#define SPOT_NET_SOCKETSOPS_H
#include <spot/utility/Compatible.h>
#include <spot/utility/Types.h>
namespace spot
{
	namespace net
	{
		namespace sockets
		{
			int createNonblockingOrDie(ADDRESS_FAMILY family);
			int  connect(int sockfd, const struct sockaddr* addr);
			void bindOrDie(int sockfd, const struct sockaddr* addr);
			void listenOrDie(int sockfd);
			void socketClose(int sockfd);
			int socketGetError(int sockfd);
			SSIZE_T32 write(int sockfd, const void *buf, size_t count);
			SSIZE_T32 read(int sockfd, void *buf, size_t count);
			void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
			void toIp(char* buf, size_t size, const struct sockaddr* addr);
			void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
			struct sockaddr_in getLocalAddr(int sockfd);
			struct sockaddr_in getPeerAddr(int sockfd);
			bool isSelfConnect(int sockfd);
			const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
			int accept(int sockfd, struct sockaddr_in* addr);
			void shutdownWrite(int sockfd);
		}
	}
}
#endif