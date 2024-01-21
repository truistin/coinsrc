#include "spot/utility/FixedBuffer.h"

using namespace spot::utility;
FixedBuffer::FixedBuffer(size_t bufferSize)
	:readerIndex_(0),writerIndex_(0)
{
	bufferSize_ = bufferSize;
	data_ = (char*)malloc(bufferSize_);
}
FixedBuffer::~FixedBuffer()
{
	free(data_);
}
bool FixedBuffer::append(const void* data, size_t len)
{
	if(len > writableBytes())
		return false;
	memcpy(data_ + writerIndex_, static_cast<const char*>(data), len);
	writerIndex_ += len;
	return true;
}
bool FixedBuffer::read(char* buff, size_t &len)
{
	len = readableBytes();
	if (len == 0)
		return false;
	memcpy(buff, beginRead(), len);
	readerIndex_ += len;
	return true;
}
void FixedBuffer::retrieve()
{
	readerIndex_ = 0;
	writerIndex_ = 0;
}
void FixedBuffer::moveReadableData()
{
	if (readableBytes() > 0)
	{
		size_t readable = readableBytes();
		memcpy(data_, beginRead(), readableBytes());
		writerIndex_ = readableBytes();
		readerIndex_ = 0;
	}
}