#ifndef SPOT_GATEWAY_ORDERQUESTORE_H
#define SPOT_GATEWAY_ORDERQUESTORE_H
#include <spot/utility/SPSCQueue.h>
#include <spot/base/DataStruct.h>
#include <spot/gateway/OrderStoreCallBack.h>
#include "spot/utility/concurrentqueue.h"

using namespace spot::utility;
using namespace spot::base;
using namespace moodycamel;
namespace spot
{
namespace gtway
{
class OrderQueStore : public OrderStoreCallBack
{
public:
  OrderQueStore(ConcurrentQueue<Order> *queue) :queue_(queue) {}
  void storeHandle(Order *innerOrder);
  void storeHandle(Order&& innerOrder);
private:
	ConcurrentQueue<Order> *queue_;
};
}
}


#endif