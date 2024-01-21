#ifndef SPOT_DIST_SERVER_H
#define SPOT_DIST_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <thread>
#include <spot/base/DataStruct.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/distributor/DistDef.h>

namespace spot
{
	namespace distributor
	{
		class DistServer
		{
		public:
			static DistServer& instance();
			virtual ~DistServer();

			virtual void addInstruments(const std::vector<std::string>& instruments);
			virtual void addInstruments(const std::list<std::string>& instruments);
			virtual int size();
			virtual bool isInit();
			virtual bool init(const std::string& key);
			virtual void start();
			virtual void stop();
			virtual void update(const spot::base::InnerMarketData* innerMktData);

		protected:
			DistServer();
			int setNonblocking(int sockfd);
			void addClient(int fd, int state);
			void deleteClient(int fd, int state);
			int sendMsg(int fd, void* buf, int size);
			int connectClient(struct sockaddr_in* servaddr);
			void acceptClient(int fd);

			void dataProcess();
			void dataProcess(InnerMarketData& data);
			void msgProcess(int fd);
			void ConnReqMsgProcess(int fd, const DistConnReqMsg* msg);
			void SubReqMsgProcess(int fd, const DistSubReqMsg* msg);
			void UnsubReqMsgProcess(int fd, const DistUnsubReqMsg* msg);
			void run();

		protected:
			std::string ip_;
			int port_;
			int epollfd_;
			std::map<int, int> clients_;
			moodycamel::ConcurrentQueue<InnerMarketData>* msgQueue_;
			std::thread* thread_;
			bool isRun_;
			std::set<std::string> instruments_;
		};
	}
}

#endif //SPOT_DIST_SERVER_H
