#ifndef SPOT_UTILITY_FIXEDBUFFER_H
#define SPOT_UTILITY_FIXEDBUFFER_H
#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>

const size_t DefaultBufferSize = 8192;

namespace spot
{
	namespace utility
	{
		class FixedBuffer
		{
		public:
			FixedBuffer(size_t bufferSize = DefaultBufferSize);
			~FixedBuffer();
			bool append(const void* data, size_t len);
			bool read(char* buff, size_t &len);
			inline size_t readerIndex();
			inline size_t writerIndex();
			inline size_t readableBytes();
			inline size_t writableBytes();
			inline char* data();			
			inline char* beginWrite();
			inline char* beginRead();
			inline void unwrite(size_t len);
			inline void unread(size_t len);
			inline void setReaderIndex(size_t len);
			void retrieve();
			void moveReadableData();
			
		private:
			char *data_;
			size_t bufferSize_;
			size_t readerIndex_;
			size_t writerIndex_;		
		};
		inline char* FixedBuffer::data()
		{
			return data_;
		}
		inline size_t FixedBuffer::readerIndex()
		{
			return readerIndex_;
		}
		inline size_t FixedBuffer::writerIndex()
		{
			return writerIndex_;
		}
		inline size_t FixedBuffer::readableBytes()
		{
			return writerIndex_ - readerIndex_;
		}

		inline size_t FixedBuffer::writableBytes()
		{
			return bufferSize_ - writerIndex_;
		}

		inline char* FixedBuffer::beginWrite()
		{
			return data_ + writerIndex_;
		}
		inline char* FixedBuffer::beginRead()
		{
			return data_ + readerIndex_;
		}
		inline void FixedBuffer::unwrite(size_t len)
		{
			assert(len <= writerIndex_);
			writerIndex_ -= len;
		}
		inline void FixedBuffer::unread(size_t len)
		{
			assert(len <= readerIndex_);
			readerIndex_ -= len;
		}
		inline void FixedBuffer::setReaderIndex(size_t len)
		{
			readerIndex_ = len;
		}
	}
}

#endif  
