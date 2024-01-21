#ifndef SPOT_SHMMD_SHMMTBASE_H
#define SPOT_SHMMD_SHMMTBASE_H

#include <spot/base/MqStruct.h>

/*  
+ ---------------- + ------------ +
|  ShmBufferSize   |  totalSize of share memory |
+ ---------------- + ------------ +
| InstrumentCount  |   nums of instrument	  |
+ ---------------- + ------------ +
|      Multiple    |   InnerMarketTrade nums of each instrument |
+ ---------------- + ------------ +
|    ShmMtIndex    |  loc beigin, and it contains instrumentid and  InnerMarketTrade's offset location of InnerMarketTrade |
+ ---------------- + ------------ +
|     ShmMtIndex location and  nums (InstrumentCount-1)    |
+ ---------------- + ------------ +
| InnerMarketTrade |   the begin trade data loc (len = Multiple)   |
+ ---------------- + ------------ +
|     InnerMarketTrade location and nums [len = (InstrumentCount-1) * Multiple]    |
+ ---------------- + ------------ +
*/

#define SPOT_SHMT_MULTIPLE 2
#define SPOT_SHMO_MULTIPLE 100

namespace spot
{
	namespace shmmd
	{
		struct ShmMtIndex
		{
			char InstrumentID[spot::base::InstrumentIDLen + 1];
			int Offset; //�����ڴ��׵�ַ �� InnerMarketData ��ƫ��
		};
		struct ShmMoIndex
		{
			char InstrumentID[spot::base::InstrumentIDLen + 1];
			int Offset; //�����ڴ��׵�ַ �� InnerMarketData ��ƫ��
		};
	}
}

#endif //SPOT_SHMMD_SHMMTBASE_H
