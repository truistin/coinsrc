#ifndef SPOT_SHMMD_SHMMTCLIENT_H
#define SPOT_SHMMD_SHMMTCLIENT_H

#include <spot/base/DataStruct.h>
#include <spot/shmmd/ShmClient.h>

#include <string>
#include <vector>
#include <unordered_map>
typedef std::function<void(spot::base::InnerMarketTrade& mt)> RecvMarketTradeCallback;

namespace spot
{
	namespace shmmd
	{
		class ShmMtClient: public ShmClient
		{
		public:
			ShmMtClient();
			virtual ~ShmMtClient();

			virtual void setSubAll(bool isSubAll);
			virtual bool init(const std::string& key, const std::vector<std::string>& instruments);
			virtual void start();
			virtual bool run();
			virtual void onInnerMarketTrade(const spot::base::InnerMarketTrade& mt) = 0;
			virtual void stop();
			virtual std::string toString();

		protected:
			void detachShm();
			std::string toString(const spot::base::InnerMarketTrade& mt);

		protected:
			std::string key_;
			char* shmPtr_;
			bool isSubAll_;
			std::vector<std::string> instruments_;
			int multiple_;
			//key: instrumentId, value: Last index
			std::unordered_map<spot::base::InnerMarketTrade*, int> innerMarketTrades_;
			bool isTerminated_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMTCLIENT_H
