include(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if(NOT HAVE_ACCEPT4)
  set_source_files_properties(SocketsOps.cc PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(net_SRCS
	EventLoop.h
	Poller.h
	Channel.h
	SocketsOps.h
	EventLoopThread.h
	EventLoop.cc
	Poller.cc
	Channel.cc
	WSAPollPoller.cc
	WSAPollPoller.h
	SocketsOps.cc
	EventLoopThread.cc
	Callbacks.h
	Buffer.h
	Buffer.cc
	InetAddress.cc
	InetAddress.h
	Endian.h
	Connector.h
	Connector.cc
	Timer.h
	Timer.cc
	TimerId.h
	TimerQueue.h
	TimerQueue.cc
	TcpConnection.h
	TcpConnection.cc
	Socket.h
	Socket.cc
	Acceptor.h
	Acceptor.cc
	TcpClient.h
	TcpClient.cc
	TcpServer.cc
	TcpServer.h
	EventLoopThreadPool.h
	EventLoopThreadPool.cc
	DefaultPoller.cc
	EPollPoller.cc
	EPollPoller.h
	PollPoller.cc
	PollPoller.h
	UdpMcastServer.cc
	UdpMcastServer.h
  )

add_library(spot_net ${net_SRCS})
target_link_libraries(spot_net spot_utility)
if(UNIX)
set_target_properties(spot_net PROPERTIES COMPILE_FLAGS "-std=c++17")
endif(UNIX)
#add_subdirectory(tests)




