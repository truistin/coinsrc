#ifndef SPOT_SHMMD_SHMCLIENT_H
#define SPOT_SHMMD_SHMCLIENT_H

namespace spot
{
	namespace shmmd
	{
		class ShmClient
		{
		public:
			virtual bool run() = 0;
		};
	}
}

#endif //SPOT_SHMMD_SHMCLIENT_H

