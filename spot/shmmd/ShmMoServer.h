#ifndef SPOT_SHMMD_SHMMOSERVER_H
#define SPOT_SHMMD_SHMMOSERVER_H

#include <spot/base/DataStruct.h>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_map>

namespace spot
{
	namespace shmmd
	{
		struct ShmMoUpdateInfo
		{
			int index;
			spot::base::InnerMarketOrder* order;
		};

		class ShmMoServer
		{
		public:
			static ShmMoServer& instance();
			virtual ~ShmMoServer();

			virtual void addInstruments(const std::vector<std::string>& instruments);
			virtual void addInstruments(const std::list<std::string>& instruments);
			virtual int size();
			virtual bool isInit();
			virtual bool init(const std::string& key);			
			virtual bool update(const spot::base::InnerMarketOrder* source);

		protected:
			ShmMoServer();
			void detachShm();

		protected:
			std::string key_;
			char* shmPtr_;
			std::set<std::string> instruments_;
			std::unordered_map<std::string, ShmMoUpdateInfo> instrumentIndexs_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMOSERVER_H
