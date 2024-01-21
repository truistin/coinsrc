#include "spot/utility/FileWriter.h"
#include <cassert>
#include <stdio.h>
using namespace spot::utility;
using namespace std;
FileWriter::FileWriter(string filename, string mode, int bufferSize)
{
	fp_ = ::fopen(filename.c_str(), mode.c_str());
#ifdef __LINUX__
	if (bufferSize > 0) {
		buffer_ = new char[bufferSize];
		::setbuffer(fp_, buffer_, bufferSize);
	}
	else {
		buffer_ = nullptr;
	}
#endif
}

FileWriter::~FileWriter()
{
	if (fp_) {
		::fclose(fp_);
	}
	if (buffer_) {
		delete []buffer_;
		buffer_ = nullptr;
	}
}
bool FileWriter::isFileExist(string filename)
{
	auto fp = ::fopen(filename.c_str(), "r");
	return (fp != nullptr);
}
int FileWriter::writeFile(const std::string &msg)
{
	return writeFile(msg.c_str(), msg.length());
}
int FileWriter::writeFile(const char* data, size_t length)
{
	size_t writeLen = write(data, length);
	size_t remainLen = length - writeLen;
	int ret = 0;
	while (remainLen > 0)
	{
		size_t writeLen2 = write(data + writeLen, remainLen);
		if (writeLen2 == 0)
		{
			ret = ferror(fp_);
			if (ret)
			{
				fprintf(stderr, "writeFile failed %s\n", strerror_tl(ret));
			}
			break;
		}
		writeLen += writeLen2;
	}
	return ret;
}

void FileWriter::flush()
{
	::fflush(fp_);
}

size_t FileWriter::write(const char* data, size_t length)
{
	// #undef fwrite_unlocked
#ifdef __LINUX__
	return ::fwrite_unlocked(data, 1, length, fp_); // not thread safe
#endif

#ifdef WIN32
	return ::fwrite(data, 1, length, fp_);
#endif
}
