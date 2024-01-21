#ifndef SPOT_SHMMD_SHMMDCLIENT_H
#define SPOT_SHMMD_SHMMDCLIENT_H

#include <spot/base/DataStruct.h>
#include <spot/shmmd/ShmClient.h>

#include <string>
#include <vector>
#include <unordered_map>
typedef std::function<void(spot::base::InnerMarketData& md)> RecvMarketDataCallback;
namespace spot
{
	namespace shmmd
	{
		class ShmMdClient: public ShmClient
		{
		public:
			ShmMdClient();
			virtual ~ShmMdClient();

			virtual void setSubAll(bool isSubAll);
			virtual bool init(const std::string& key, const std::vector<std::string>& instruments);
			virtual void start();
			virtual bool run();
			virtual void onInnerMarketData(const spot::base::InnerMarketData& md) = 0;
			virtual void stop();
			virtual std::string toString();

		protected:
			void detachShm();
			std::string toString(const spot::base::InnerMarketData& md);

		protected:
			std::string key_;
			char* shmPtr_;
			bool isSubAll_;
			std::vector<std::string> instruments_;
			//key: instrumentId, value: LastSeqNum
			std::unordered_map<spot::base::InnerMarketData*, uint64_t> innerMarketDatas_;
			bool isTerminated_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMDCLIENT_H
