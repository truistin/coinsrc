// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <spot/net/Socket.h>

#include <spot/utility/Logging.h>
#include <spot/net/InetAddress.h>
#include <spot/net/SocketsOps.h>
#include <spot/utility/Compatible.h>

#include <stdio.h>  // snprintf

using namespace spot;
using namespace spot::net;

Socket::~Socket()
{
	sockets::socketClose(sockfd_);
}


bool Socket::getTcpInfoString(char* buf, int len) const
{
#ifdef __LINUX__
	struct tcp_info tcpi;
	socklen_t size = sizeof(tcpi);
	memset(&tcpi, 0, sizeof(socklen_t));
	if (getsockopt(sockfd_, SOL_TCP, TCP_INFO, &tcpi, &size) != 0)
	{
		return false;
	}
	snprintf(buf, len, "unrecovered=%u rto=%u ato=%u snd_mss=%u rcv_mss=%u lost=%u retrans=%u rtt=%u rttvar=%u sshthresh=%u cwnd=%u total_retrans=%u",
		tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
		tcpi.tcpi_rto,          // Retransmit timeout in usec
		tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
		tcpi.tcpi_snd_mss,
		tcpi.tcpi_rcv_mss,
		tcpi.tcpi_lost,         // Lost packets
		tcpi.tcpi_retrans,      // Retransmitted packets out
		tcpi.tcpi_rtt,          // Smoothed round trip time in usec
		tcpi.tcpi_rttvar,       // Medium deviation
		tcpi.tcpi_snd_ssthresh,
		tcpi.tcpi_snd_cwnd,
		tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
#else
	CSADDR_INFO tcpinfo;
	int addrInfoLen = sizeof(CSADDR_INFO);
	if (getsockopt(sockfd_, SOL_SOCKET, SO_BSP_STATE, (char *)&tcpinfo, &addrInfoLen) != 0)
	{
		return false;
	}
	snprintf(buf, len, "protocol=%d ,sockettype=%d,localaddr=%s,remoteladdr=%s\n", tcpinfo.iProtocol, tcpinfo.iSocketType, tcpinfo.LocalAddr.lpSockaddr, tcpinfo.RemoteAddr.lpSockaddr);
#endif

	return true;
}

void Socket::bindAddress(const InetAddress& addr)
{
	sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen()
{
	sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	int connfd = sockets::accept(sockfd_, &addr);
	if (connfd >= 0)
	{
		peeraddr->setSockAddrInet(addr);
	}
	return connfd;
}

void Socket::shutdownWrite()
{
	sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
	char optval = on ? 1 : 0;
	::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
		(char *)&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}

void Socket::setReuseAddr(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
		(char *)&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
		&optval, static_cast<socklen_t>(sizeof optval));
	if (ret < 0 && on)
	{
		LOG_SYSERR << "SO_REUSEPORT failed.";
	}
#else
	if (on)
	{
		LOG_ERROR << "SO_REUSEPORT is not supported.";
	}
#endif
}

void Socket::setKeepAlive(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
		(char *)&optval, static_cast<socklen_t>(sizeof optval));
	// FIXME CHECK
}

