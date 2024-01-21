#ifndef SPOT_SHMMD_SHMMTSERVER_H
#define SPOT_SHMMD_SHMMTSERVER_H

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
		struct ShmMtUpdateInfo
		{
			int index;
			spot::base::InnerMarketTrade* trade;
		};

		class ShmMtServer
		{
		public:
			static ShmMtServer& instance();
			virtual ~ShmMtServer();

			virtual void addInstruments(const std::vector<std::string>& instruments);
			virtual void addInstruments(const std::list<std::string>& instruments);
			virtual int size();
			virtual bool isInit();
			virtual bool init(const std::string& key);

			virtual bool update(const spot::base::Order* source);
			virtual bool update(const spot::base::InnerMarketTrade* source);

		protected:
			ShmMtServer();
			void detachShm();

		protected:
			std::string key_;
			char* shmPtr_;
			std::set<std::string> instruments_;
			std::unordered_map<std::string, ShmMtUpdateInfo> instrumentIndexs_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMTSERVER_H
