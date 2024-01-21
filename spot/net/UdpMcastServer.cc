#include <spot/net/UdpMcastServer.h>
#include <spot/utility/Utility.h>
using namespace spot;
using namespace spot::net;

UdpMcastServer::UdpMcastServer(std::string multiCastIpPort,std::string interfaceIp)
{
#ifdef __LINUX__
	struct ip_mreq mreq;
	struct in_addr if_req;
	if ((fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		LOG_FATAL << "socket error " << errno;
	}

	u_int yes = 1;
	if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		LOG_FATAL << "setsockopt reuseaddr  " << errno;
	}

	memset(&addr_, 0, sizeof(addr_));
	addr_.sin_family = AF_INET;
	int multiCastPort;
	string multiCastIp = spot::utility::parseAddPort((char *)multiCastIpPort.c_str(), multiCastPort);
	addr_.sin_addr.s_addr = inet_addr(multiCastIp.c_str());
	addr_.sin_port = htons(multiCastPort);
	LOG_INFO << "UdpMcastServer Init Start " << multiCastIpPort.c_str() << " LocalIp:" << interfaceIp;

	if (bind(fd_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0) 
	{
		LOG_FATAL << "setsockopt bind  error:" << errno;
	}
	inet_aton(interfaceIp.c_str(), &(if_req));

	if (setsockopt(fd_, SOL_IP, IP_MULTICAST_IF, &if_req, sizeof(struct in_addr)) < 0) 
	{
		LOG_FATAL << "setsockopt multicast_if  error:" << errno;
	}
	LOG_INFO << " Upd Init End ";
#endif
}
UdpMcastServer::~UdpMcastServer()
{

}
int UdpMcastServer::sendMsg(const char* msg, int len)
{
	int cnt = 0;
#ifdef __LINUX__
	cnt = sendto(fd_, msg, len, 0, (struct sockaddr *) &addr_, sizeof(addr_));
	if (cnt <0)
	{
		LOG_ERROR << "sendMsg send error:" << errno;
	}	
#endif
	return cnt;
}