#ifndef SPOT_BASE_JSONINI_H
#define SPOT_BASE_JSONINI_H
#include "spot/base/DataStruct.h"
#include "spot/utility/SPSCQueue.h"
#include "spot/utility/FileWriter.h"
#include "spot/restful/Uri.h"
#include "spot/restful/CurlMultiCurl.h"
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/rapidjson.h>
using namespace std;
using namespace spot::base;
using namespace spot::utility;


namespace spot
{
	namespace strategy
	{
		class InitJsonFile
		{
		public:
			InitJsonFile(string initFilePath, spsc_queue<QueueNode> *initQueue);
			InitJsonFile(spsc_queue<QueueNode> *initQueue);
			~InitJsonFile();
			void getInitData();
		private:
			void getJson(const char* name);
			void sendFinished();
		private:
			string initFilePath_;
			vector<string> member_;
			spsc_queue<QueueNode> *initQueue_;
		};
	}

}
#endif

