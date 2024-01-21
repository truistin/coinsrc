#ifndef SPOT_GATEWAY_ORDERSTORECALLBACK_H
#define SPOT_GATEWAY_ORDERSTORECALLBACK_H
#include <spot/base/DataStruct.h>
using namespace spot::base;
namespace spot
{
	namespace gtway
	{
		class OrderStoreCallBack
		{
		public:
			virtual void storeHandle(Order *innerOrder) {};
			virtual void storeHandle(Order&& innerOrder) {};
			virtual ~OrderStoreCallBack() {};
		};
	}
}
#endif