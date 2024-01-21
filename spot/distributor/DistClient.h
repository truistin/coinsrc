#ifndef SPOT_DIST_CLIENT_H
#define SPOT_DIST_CLIENT_H

#include <string>
#include <spot/base/DataStruct.h>
#include <spot/distributor/DistDef.h>

namespace spot
{
	namespace distributor
	{
		class DistClient
		{
		public:
			DistClient();
			virtual ~DistClient();

			virtual void setSubAll(bool isSubAll);
			virtual bool init(const std::string& key, const std::vector<std::string>& instruments, const std::string& tag="");
			virtual void start();
			virtual void stop();
			virtual void onMsg(const spot::base::InnerMarketData* innerMktData);

		protected:
			void run();
			int setNonblocking(int sockfd);
			void checkConn();
			void msgProcess();
			void ConnRspMsgProcess(const DistConnRspMsg* msg);
			void SubRspMsgProcess(const DistSubRspMsg* msg);
			void unSubRspMsgProcess(const DistUnsubRspMsg* msg);
			void InnerMarketDataProcess(const DistInnerMarketData* msg);

		protected:
			bool isSubAll_;
			std::string tag_;
			std::string ip_;
			int port_;
			int sockfd_;
			std::thread* thread_;
			bool isRun_;
			bool isConnected_;
			struct addrinfo* servinfo_;
			uint64_t lastEpoch_;
			int aliveIntval_; //√Î
			std::vector<std::string> instruments_;
		};
	}
}
#endif //SPOT_DIST_CLIENT_H
