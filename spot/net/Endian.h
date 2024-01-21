// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/ftmd/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef SPOT_NET_ENDIAN_H
#define SPOT_NET_ENDIAN_H

#include <spot/utility/Compatible.h>
#include <stdint.h>

namespace spot
{
	namespace net
	{
		namespace sockets
		{

			// the inline assembler code makes type blur,
			// so we disable warnings for a while.
#ifdef __LINUX__
#if defined(__clang__) || __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

			inline uint64_t hostToNetwork64(uint64_t host64)
			{
#ifdef __LINUX__
				return htobe64(host64);
#else
				return ntohll(host64);
#endif
			}
			inline uint32_t hostToNetwork32(uint32_t host32)
			{
#ifdef __LINUX__
				return htobe32(host32);
#else
				return ntohl(host32);
#endif
			}

			inline uint16_t hostToNetwork16(uint16_t host16)
			{
#ifdef __LINUX__
				return htobe16(host16);
#else
				return ntohs(host16);
#endif
			}

			inline uint64_t networkToHost64(uint64_t net64)
			{
#ifdef __LINUX__
				return be64toh(net64);
#else
				return ntohll(net64);
#endif
			}

			inline uint32_t networkToHost32(uint32_t net32)
			{
#ifdef __LINUX__
				return be32toh(net32);
#else
				return ntohl(net32);
#endif
			}

			inline uint16_t networkToHost16(uint16_t net16)
			{
#ifdef __LINUX__
				return be16toh(net16);
#else
				return ntohs(net16);
#endif
			}

#define ntohd(x) SwapDoubleEndian(&(x))

			inline double SwapDoubleEndian(double* net64)
			{
				uint64_t llVal = 0;
#ifdef __LINUX__
				llVal = be64toh(*((uint64_t*)net64));
#else
				llVal = ntohll(*((uint64_t*)net64));
#endif
				return *((double*)&llVal);
			}
		}
	}
}

#endif  //SPOT_NET_ENDIAN_H
