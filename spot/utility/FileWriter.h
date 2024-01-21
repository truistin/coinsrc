#ifndef SPOT_UTILITY_FILEWRITER_H
#define SPOT_UTILITY_FILEWRITER_H
#include "spot/utility/FileUtil.h"
#include "spot/utility/Logging.h"
namespace spot
{
	namespace utility
	{
		class FileWriter
		{
		public:
			FileWriter(string filename, string mode = "w", int bufferSize=0);
			~FileWriter();
			int writeFile(const char* data, size_t length);
			int writeFile(const std::string &msg);
			static bool isFileExist(string filename);
			void flush();
		private:
			size_t write(const char* data, size_t length);
		private:
			FILE* fp_;
			char* buffer_;
		};

	}

}

#endif  

