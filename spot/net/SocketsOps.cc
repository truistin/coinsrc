#include <spot/net/SocketsOps.h>
#include <spot/net/Endian.h>
#include <spot/utility/Logging.h>
#include <spot/net/InetAddress.h>
#include <cstdint>
using namespace spot;
using namespace spot::net;

SSIZE_T32 sockets::write(int sockfd, const void *buf, size_t count)
{
#ifdef WIN32
	return send(sockfd, (char *)buf, count, 0);
#endif
#ifdef __LINUX__
	return ::write(sockfd, buf, count);
#endif
}


SSIZE_T32 sockets::read(int sockfd, void *buf, size_t count)
{
#ifdef WIN32
	return recv(sockfd, (char *)buf, count, 0);
#endif
#ifdef __LINUX__
	return ::read(sockfd, buf, count);
#endif
}

const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}
struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in* addr)
{
	return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}
void sockets::toIpPort(char* buf, size_t size,
	const struct sockaddr* addr)
{
	toIp(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
	uint16_t port = sockets::networkToHost16(addr4->sin_port);
	assert(size > end);
	snprintf(buf + end, size - end, ":%u", port);
}

void sockets::toIp(char* buf, size_t size,
	const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET)
	{
		assert(size >= INET_ADDRSTRLEN);
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, (void *)&addr4->sin_addr, buf, static_cast<socklen_t>(size));
	}
}

void sockets::fromIpPort(const char* ip, uint16_t port,
struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
	{
		LOG_SYSERR << "sockets::fromIpPort";
	}
}

int setNonBlockAndCloseOnExec(int sockfd)
{
#ifdef WIN32
	unsigned long nonblocking = 1;
	if (ioctlsocket(sockfd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
		fprintf(stderr, "fcntl(%d, F_GETFL)", (int)sockfd);
		return -1;
	}

#else
	// non-block
	int flags = ::fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(sockfd, F_SETFL, flags);
	if (ret < 0)
		return ret;
	// FIXME check

	// close-on-exec
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(sockfd, F_SETFD, flags);
	if (ret < 0)
		return ret;
	// FIXME check

#endif
	return 0;
}
int sockets::createNonblockingOrDie(ADDRESS_FAMILY family)
{
	int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
	{
		LOG_SYSFATAL << "sockets::createNonblockingOrDie";
		return -1;
	}
	if (setNonBlockAndCloseOnExec(sockfd) < 0)
	{
		LOG_SYSFATAL << "sockets::setNonBlockAndCloseOnExec";
		socketClose(sockfd);
		return -1;
	}
	return sockfd;
}
int sockets::connect(int sockfd, const struct sockaddr* addr)
{
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}
void sockets::bindOrDie(int sockfd, const struct sockaddr* addr)
{
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
	if (ret < 0)
	{
		LOG_SYSFATAL << "sockets::bindOrDie " << socketGetError(sockfd);
		socketClose(sockfd);
	}
}
void sockets::listenOrDie(int sockfd)
{
	int ret = ::listen(sockfd, SOMAXCONN);
	if (ret < 0)
	{
		LOG_SYSFATAL << "sockets::listenOrDie " << socketGetError(sockfd);
	}
}
void sockets::socketClose(int sockfd)
{
#ifndef WIN32
	::close(sockfd);
#else
	closesocket(sockfd);
#endif
}

int sockets::socketGetError(int sockfd)
{
#ifdef WIN32
	int optval = 0;
	int optvallen = sizeof(optval);
	int err = WSAGetLastError();
	if (err == WSAEWOULDBLOCK && sockfd >= 0) {
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optvallen) < 0)
			return err;
		if (optval)
			return optval;
	}
	return err;
#else
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);

	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
	{
		return errno;
	}
	else
	{
		return optval;
	}
#endif
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
	struct  sockaddr_in localaddr;
	memset(&localaddr, 0, sizeof localaddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
	if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
	{
		LOG_SYSERR << "sockets::getLocalAddr";
	}
	return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
	struct sockaddr_in peeraddr;
	memset(&peeraddr, 0, sizeof peeraddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
	if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
	{
		LOG_SYSERR << "sockets::getPeerAddr";
	}
	return peeraddr;
}
bool sockets::isSelfConnect(int sockfd)
{
	struct sockaddr_in localaddr = getLocalAddr(sockfd);
	struct sockaddr_in peeraddr = getPeerAddr(sockfd);
	if (localaddr.sin_family == AF_INET)
	{
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port
			&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	}
	else
	{
		return false;
	}
}

int sockets::accept(int sockfd, struct sockaddr_in* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);

	int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
	setNonBlockAndCloseOnExec(connfd);

	//»º³åÇø²éÑ¯
	if (connfd >= 0)
	{
		int snd_size = 0;
		int rev_size = 0;
		int optLen = sizeof(int);
		int err = ::getsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (char*)&snd_size, (socklen_t*)&optLen);
		err = ::getsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (char*)&rev_size, (socklen_t*)&optLen);

		//std::cout << "socket accept, sendBuf size:" << snd_size << ", revcBuf size:" << rev_size << std::endl;
	}

	if (connfd < 0)
	{
		int savedErrno = socketGetError(connfd);
		LOG_SYSERR << "Socket::accept";
		switch (savedErrno)
		{
#ifdef __LINUX__
		case EAGAIN:
		case ECONNABORTED:
#if EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
#endif
		case EINTR:
		case EPERM:
		case EMFILE: // per-process lmit of open file desctiptor ???
			// expected errors
			errno = savedErrno;
			break;
#endif
#ifdef WIN32
		case WSAEINTR:
		case WSAEMFILE:
		case WSAEWOULDBLOCK:
		case WSAENOBUFS:
			errno = savedErrno;
			break;
#endif
		default:
			LOG_FATAL << "unknown error of ::accept " << savedErrno;
			break;

		}
	}
	return connfd;
}

void sockets::shutdownWrite(int sockfd)
{
	if (::shutdown(sockfd, 1) < 0)
	{
		LOG_SYSERR << "sockets::shutdownWrite";
	}
}