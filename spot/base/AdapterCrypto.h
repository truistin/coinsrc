#ifndef SPOT_BASE_ADAPTERCRYPTO_H
#define SPOT_BASE_ADAPTERCRYPTO_H

namespace spot
{
	namespace base
	{
		class AdapterCrypto
		{
		public:
			virtual ~AdapterCrypto() {};
			virtual int httpAsynReqCallback(char * result, uint64_t clientId) = 0;
			virtual int httpAsynCancelCallback(char * result, uint64_t clientId) = 0;
			virtual int httpQryOrderCallback(char * result, uint64_t clientId) = 0;

		};
	}
}
#endif