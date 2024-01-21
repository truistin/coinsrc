#ifndef SPOT_BASE_ADAPTER_H
#define SPOT_BASE_ADAPTER_H
#include"spot/base/DataStruct.h"

namespace spot
{
	namespace base
	{
		class Adapter
		{
		public:
			virtual ~Adapter() {};
			virtual int reqOrder(Order &order) = 0;
			virtual int cancelOrder(Order &order) = 0;
			virtual int query(const Order &order) = 0;
		};
	}
}
#endif