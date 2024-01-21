#include <spot/gateway/OrderQueStore.h>
#include <spot/base/DataStruct.h>
using namespace spot;
using namespace spot::gtway;

void OrderQueStore::storeHandle(Order *innerOrder)
{
  queue_->enqueue(*innerOrder);
  return;
}

void OrderQueStore::storeHandle(Order&& innerOrder)
{
  queue_->enqueue(std::move(innerOrder));
  //queue_->enqueue(innerOrder);
  return;
}

