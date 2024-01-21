#ifndef SPOT_SHMMD_SHMMANAGEMENT_H
#define SPOT_SHMMD_SHMMANAGEMENT_H

#include <spot/shmmd/ShmClient.h>
#include <vector>
#include <thread>

namespace spot
{
	namespace shmmd
	{
		class ShmManagement
		{
		public:
			static ShmManagement& instance();
			virtual ~ShmManagement();
			void addClient(ShmClient* client);
			void start(bool isSubThreadLoop=true);
			void stop();
			void loop();
			bool isSubThreadLoop();

		protected:
			ShmManagement();
			void run();

			std::vector<ShmClient*> clients_;
			std::thread* thread_;
			bool isTerminated_;
			bool isSubThreadLoop_;
			int sleepIntervalus_;
		};
	}
}

#endif //SPOT_SHMMD_SHMMANAGEMENT_H
