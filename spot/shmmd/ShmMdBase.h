#ifndef SPOT_SHMMD_SHMMDBASE_H
#define SPOT_SHMMD_SHMMDBASE_H

#include <spot/base/MqStruct.h>

/*         共享内存结构说明
+ ---------------- + ------------ +
|  ShmBufferSize   |  共享内存大小 |
+ ---------------- + ------------ +
| InstrumentCount  |   合约个数	  |
+ ---------------- + ------------ +
|    ShmMdIndex    |   行情索引    |
+ ---------------- + ------------ +
|     省略(InstrumentCount-1个)    |
+ ---------------- + ------------ +
|  InnerMarketData |   行情数据    |
+ ---------------- + ------------ +
|     省略(InstrumentCount-1个)    |
+ ---------------- + ------------ +
*/

namespace spot
{
	namespace shmmd
	{
		struct ShmMdIndex
		{
			char InstrumentID[spot::base::InstrumentIDLen + 1];
			int Offset; //共享内存首地址 到 InnerMarketData 的偏移
		};

	}
}

#endif //SPOT_SHMMD_SHMMDBASE_H
