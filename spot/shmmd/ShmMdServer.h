#ifndef SPOT_SHMMD_SHMMDSERVER_H
#define SPOT_SHMMD_SHMMDSERVER_H

#include <spot/base/DataStruct.h>
#include <spot/utility/Utility.h>

#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_map>
using namespace spot::utility;
namespace spot
{
	namespace shmmd
	{
		class ShmMdServer
		{
		public:
			static ShmMdServer& instance();
			virtual ~ShmMdServer();

			virtual void addInstruments(const std::vector<std::string>& instruments);
			virtual void addInstruments(const std::list<std::string>& instruments);
			virtual int size();
			virtual bool isInit();
			virtual bool init(const std::string& key);
			virtual bool update(const spot::base::InnerMarketData* source);
		protected:
			ShmMdServer();
			void detachShm();

		protected:
			std::string key_;
			char* shmPtr_;
			std::set<std::string> instruments_;
			std::map<const char*, spot::base::InnerMarketData*, CStringLess> instrumentIndexs_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMDSERVER_H
