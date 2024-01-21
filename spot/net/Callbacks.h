// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef SPOT_NET_CALLBACKS_H
#define SPOT_NET_CALLBACKS_H

#include <functional>
#include <memory>

#include <spot/utility/Timestamp.h>

namespace spot
{

	// Adapted from google-protobuf stubs/common.h
	// see License in ftmd/base/Types.h
	template<typename To, typename From>
	inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f)
	{
		if (false)
		{
			implicit_cast<From*, To*>(0);
		}

#ifndef NDEBUG
		assert(f == NULL || dynamic_cast<To*>(f.get()) != NULL);
#endif
		return ::std::static_pointer_cast<To>(f);
	}

	namespace net
	{

		// All client visible callbacks go here.

		class Buffer;
		class TcpConnection;
		typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
		typedef std::function<void()> TimerCallback;
		typedef std::function<void(int)> ConnectionRetryCallback;
		typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
		typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
		typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
		typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

		// the data has been read to (buf, len)
		typedef std::function<void(const TcpConnectionPtr&,
			Buffer*,
			Timestamp)> MessageCallback;

		void defaultConnectionRetryCallback(int timeLapse);
		void defaultConnectionCallback(const TcpConnectionPtr& conn);
		void defaultMessageCallback(const TcpConnectionPtr& conn,
			Buffer* buffer,
			Timestamp receiveTime);

	}
}

#endif  // SPOT_NET_CALLBACKS_H
