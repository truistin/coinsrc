#include "spot/strategy/DealOrders.h"
#include "spot/base/ParametersManager.h"
#include "spot/base/InstrumentManager.h"
#include "spot/strategy/OrderController.h"
#include "spot/utility/Utility.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/strategy/StrategyInstrumentManager.h"


using namespace spot::base;
using namespace spot::strategy;
DealOrders::DealOrders(const StrategyInstrumentPNLDaily &position, Adapter *adapter)
	:position_(position)
{
	adapterOM_ = adapter;	
	auto strategyParams = ParametersManager::getStrategyParameters(position.StrategyID);
	instrument_ = InstrumentManager::getInstrument(position.InstrumentID);

	maxPosition_ = strategyParams->getTcMaxPosition();

	tickSize_ = instrument_->getTickSize();

	char *buff = CacheAllocator::instance()->allocate(sizeof(OrderByPriceMap));
	buyOrders_ = new (buff) OrderByPriceMap();
	buff = CacheAllocator::instance()->allocate(sizeof(OrderByPriceMap));
	sellOrders_ = new (buff) OrderByPriceMap();
	buff = CacheAllocator::instance()->allocate(sizeof(DealOrdersApi));
	splitOrders_ = new (buff) DealOrdersApi(position, this);
}
DealOrders::~DealOrders()
{
	delete buyOrders_;
	delete sellOrders_;
}
void DealOrders::setOrder(SetOrderInfo *setOrderInfo)
{	
	//make a local copy
	currSetOrder_ = setOrderInfo;

	auto size = getOrderSize(targetDirection());
	if (IS_DOUBLE_ZERO(targetPrice()) || IS_DOUBLE_ZERO(targetQuantity()))
	{
		if (size > 0)
		{
			LOG_INFO << "cancel all orders strategyID:" << position_.StrategyID << " instrumentID:" << position_.InstrumentID
				<< " direction:" << targetDirection() << " price:" << targetPrice() << " quantity:" << targetQuantity();
			//价格为0或者数量为0，撤掉所有买单或者卖单
			cancelAllNonPendingOrders(targetDirection(), setOrderInfo->cancelSize);
		}
		return;
	}

	// LOG_INFO << "setOrder newOrders strategyID:" << position_.StrategyID << "instrumentID:" << position_.InstrumentID
	// 	<< " direction:" << targetDirection() << " price:" << targetPrice() << " quantity:" << targetQuantity();

	//当前没有报单，根据OrderLevelMin决定下多少单
	newOrders();
}
void DealOrders::cancelAllNonPendingOrders(char direction, uint32_t cancelSize)
{
	if (cancelSize == 0) {
		for (auto orderByPriceIter : *orderMap(direction))
		{
			if (!cancelOrders(orderByPriceIter.second)) {
				return;
			}
			LOG_INFO << "orderByPriceIter->second->OrderList: " << orderByPriceIter.second->OrderList.size();
		}
	} else {
		for (auto orderByPriceIter : *orderMap(direction))
		{
			if (!cancelOrdersBySize(orderByPriceIter.second, cancelSize)) {
				return;
			}
			LOG_INFO << "orderByPriceIter->second->OrderList: " << orderByPriceIter.second->OrderList.size();
		}
	}
}
void DealOrders::newOrders()
{
	currSetOrder_->pOrder->VolumeTotalOriginal = targetQuantity();
	currSetOrder_->pOrder->LimitPrice = targetPrice();;
	double closeToday;
	// getCloseVolumes(closeToday);
/*	if (exceedMaxPosition(currSetOrder_->pOrder->VolumeTotalOriginal))
	{
		LOG_WARN << instrument_->getInstrumentID() << " exceed maxPositon " << maxPosition();
		return;
	}*/
	splitOrders_->newOrder(closeToday, currSetOrder_);
}

bool DealOrders::cancelOrdersBySize(OrderByPrice *orderByPrice, uint32_t& cancelSize)
{
	uint32_t cancelNum = 0;
	auto iter = orderByPrice->OrderList.begin();
	for (; iter != orderByPrice->OrderList.end(); ++iter)
	{
		if (iter->OrderStatus != PendingCancel)
		{
			if (cancelNum < cancelSize) {
				LOG_INFO << "DealOrders cancelOrder: " << iter->OrderRef;
				cancelOrder(*iter);
				cancelNum++;
			} else {
				return false;
			}
			
		}
		else
		{
			cancelSize = 0;
			LOG_WARN << "there is no need to cancel order.OrderRef:" << iter->OrderRef << " OrderStatus:" << iter->OrderStatus << " OrderType:" << iter->OrderType
				<< ", cancelSize: " << cancelSize;
		}
	}
	cancelSize = cancelSize - cancelNum;
	LOG_INFO << "DealOrders cancelOrdersBySize size: " << cancelSize << ", cancelNum: " << cancelNum;
	return true;
}

bool DealOrders::cancelOrders(OrderByPrice *orderByPrice)
{
	auto iter = orderByPrice->OrderList.begin();
	for (; iter != orderByPrice->OrderList.end(); ++iter)
	{
		if (iter->OrderStatus != PendingCancel)
		{
			LOG_INFO << "DealOrders cancelOrder: " << iter->OrderRef;
			cancelOrder(*iter);
		}
		else
		{
			LOG_WARN << "there is no need to cancel order.OrderRef:" << iter->OrderRef << " OrderStatus:" << iter->OrderStatus << " OrderType:" << iter->OrderType;
		}
	}
	return true;
}

int DealOrders::getCancelOrderSize(char direction)
{
	int sum = 0;
	for (auto orderByPriceIter : *orderMap(direction)) {
		sum = sum + orderByPriceIter.second->OrderList.size();
	}
	return sum;
}

OrderByPrice* DealOrders::getOrderByPrice(char direction, double price)
{
	if (direction == INNER_DIRECTION_Buy)
	{
		auto iter = buyOrders_->find(price);
		if (iter != buyOrders_->end())
		{
			return iter->second;
		}
		return nullptr;
	}
	else
	{
		auto iter = sellOrders_->find(price);
		if (iter != sellOrders_->end())
		{
			return iter->second;
		}
		return nullptr;
	}

}
double DealOrders::getTopPrice(char direction)
{
	if (direction == INNER_DIRECTION_Buy)
	{
		//map中Price最大的
		return buyOrders_->size() != 0 ? buyOrders_->rbegin()->first : NAN;
	}
	else
	{
		//map中Price最小的
		return sellOrders_->size() != 0 ? sellOrders_->begin()->first : NAN;
	}
}
double DealOrders::getBottomPrice(char direction)
{
	if (direction == INNER_DIRECTION_Buy)
	{
		//map中Price最小的
		return buyOrders_->size() != 0 ? buyOrders_->begin()->first : NAN;
	}
	else
	{
		//map中Price最大的
		return sellOrders_->size() != 0 ? sellOrders_->rbegin()->first : NAN;
	}
}
void DealOrders::updateVolumes(char direction,double price)
{
	OrderByPriceMap *map = (direction == INNER_DIRECTION_Buy) ? buyOrders_ : sellOrders_;
	auto orderByPrice = map->find(price);
	if (orderByPrice != map->end())
	{
		//reset to zero before recaculate
		orderByPrice->second->Volume = 0;
		orderByPrice->second->VolumeTotal = 0;
		orderByPrice->second->VolumePendingCancel = 0;
		orderByPrice->second->VolumeCloseToday = 0;
		for (auto order : orderByPrice->second->OrderList)
		{
			orderByPrice->second->Volume += order.VolumeRemained;
			orderByPrice->second->VolumeTotal += order.VolumeTotalOriginal;
			if (order.OrderStatus == PendingCancel || order.OrderStatus == CancelRejected)
			{
				orderByPrice->second->VolumePendingCancel += order.VolumeRemained;
			}
			if (order.Offset == INNER_OFFSET_CloseToday)
				orderByPrice->second->VolumeCloseToday += order.VolumeRemained;
		}
		LOG_DEBUG << "price:" << orderByPrice->second->Price << " volume:" << orderByPrice->second->Volume
			<< " volumePendingCancel:" << orderByPrice->second->VolumePendingCancel
			<< " volumeCloseT:" << orderByPrice->second->VolumeCloseToday;
		tracePendingOrders(direction);
	}	
}

int DealOrders::query(const Order &order)
{
	int ret = adapterOM_->query(order);
	return ret;
}

int DealOrders::reqOrder(Order& order)
{
/*	if (!splitOrders_->isReqOrderIntervalReady(targetDirection()))
	{
		return 1;
	}*/

	order.OrderRef = OrderController::getOrderRef();
	order.OrderStatus = PendingNew;
	order.TimeStamp = CURR_MSTIME_POINT;
	auto ret = adapterOM_->reqOrder(order);
	LOG_DEBUG << " Price:" << order.LimitPrice << " Volume:" << order.VolumeTotalOriginal << " OrderRef:" << order.OrderRef;
	if (ret == 0) {
		// can't insert here, it should insert when httpAsyn return
		insertOrderByPrice(order);
	}
	
	return ret;
}
int DealOrders::cancelOrder(Order &order)
{
	if (order.OrderStatus == PendingNew)
	{
		return 0;
	}
	// gTickToOrderMeasure->reset();
	order.OrderStatus = PendingCancel;
	order.TimeStamp = CURR_MSTIME_POINT;
	auto ret = adapterOM_->cancelOrder(order);
	LOG_INFO << "cancelOrder Price:" << order.LimitPrice << " Volume:" << order.VolumeTotalOriginal << " OrderRef:" << order.OrderRef;

	updateOrderByPrice(order);
	return ret;
}
void DealOrders::insertOrderByPrice(const Order &order)
{
	auto map = orderMap(order.Direction);
	auto orderByPriceIter = map->find(order.LimitPrice);
	if (orderByPriceIter == map->end())
	{
		OrderByPrice *orderByPrice = new OrderByPrice();
		orderByPrice->Volume = order.VolumeTotalOriginal;
		orderByPrice->Price = order.LimitPrice;
		orderByPrice->OrderList.push_back(order);
		map->insert(std::pair<double, OrderByPrice*>(orderByPrice->Price, orderByPrice));
		LOG_DEBUG << "insertOrderByPrice exist instrumentID:" << order.InstrumentID << ", orderRef: " << order.OrderRef << " direction:" << order.Direction << " status:" << order.OrderStatus
			<< " limitPrice:" << order.LimitPrice << " volumeTotal:" << order.VolumeTotalOriginal << " volumeRemain:" << order.VolumeRemained;

	}
	else
	{
		LOG_DEBUG << "insertOrderByPrice not exist instrumentID:" << order.InstrumentID << ", orderRef: " << order.OrderRef << " direction:" << order.Direction << " status:" << order.OrderStatus
			<< " limitPrice:" << order.LimitPrice << " volumeTotal:" << order.VolumeTotalOriginal << " volumeRemain:" << order.VolumeRemained;
		orderByPriceIter->second->OrderList.push_back(order);
	}
	updateVolumes(order.Direction,order.LimitPrice);
}
void DealOrders::updateOrderByPrice(const Order &rtnOrder)
{
	LOG_DEBUG << "updateOrderByPrice instrumentID:" << rtnOrder.InstrumentID << ", orderRef: " << rtnOrder.OrderRef << " direction:" << rtnOrder.Direction << " status:" << rtnOrder.OrderStatus
		<< " limitPrice:" << rtnOrder.LimitPrice << " volumeTotal:" << rtnOrder.VolumeTotalOriginal << " volumeRemain:" << rtnOrder.VolumeRemained;
	auto orderByPrice = getOrderByPrice(rtnOrder.Direction, rtnOrder.LimitPrice);
	if (orderByPrice == nullptr)
		return;
	auto orderIter = orderByPrice->OrderList.begin();
	for (; orderIter != orderByPrice->OrderList.end(); ++orderIter)
	{
		//根据OrderRef找到对应报单
		if (orderIter->OrderRef == rtnOrder.OrderRef)
		{
			if (rtnOrder.OrderStatus == Partiallyfilled && orderIter->OrderStatus == PendingCancel)
			{
				*orderIter = rtnOrder;
				LOG_DEBUG << "updateOrderByPrice orderRef: " << rtnOrder.OrderRef << "Partiallyfilled Volume:" << rtnOrder.Volume << " VolumeTotalOriginal:" << rtnOrder.VolumeTotalOriginal;
				//如果PendingCancel状态下收到Partiallyfilled，当前状态不能改变, 这是为了在updateVolumeAndPendingVolume 里可以重新计算
				//volume and pending volume
				orderIter->OrderStatus = PendingCancel;
			}
			else
			{
				*orderIter = rtnOrder;
			}
			break;
		}
	}
	updateVolumes(rtnOrder.Direction,rtnOrder.LimitPrice);
}
void DealOrders::removeOrderByPrice(const Order &rtnOrder, bool &setOrderFinished)
{
	auto map = orderMap(rtnOrder.Direction);
	auto orderByPriceIter = map->find(rtnOrder.LimitPrice);
	if (orderByPriceIter == map->end())
		return;
	LOG_INFO << "removeOrderByPrice OrderRef: " <<rtnOrder.OrderRef <<", instrumentID:" << rtnOrder.InstrumentID << ", direction:" << rtnOrder.Direction << ", status:" << rtnOrder.OrderStatus
		<< ", limitPrice:" << rtnOrder.LimitPrice << ", volumeTotal:" << rtnOrder.VolumeTotalOriginal << ", volumeRemain:" << rtnOrder.VolumeRemained
		<< ", orderByPriceIter->second->OrderList: " << orderByPriceIter->second->OrderList.size();
	auto orderIter = orderByPriceIter->second->OrderList.begin();
	for (; orderIter != orderByPriceIter->second->OrderList.end(); ++orderIter)
	{
		LOG_DEBUG << "removeOrderByPrice2 OrderRef: " <<orderIter->OrderRef
			<< ", orderByPriceIter->second->OrderList: " << orderByPriceIter->second->OrderList.size();

		if (orderIter->OrderRef == rtnOrder.OrderRef)
		{
			if (rtnOrder.OrderStatus == OrderStatus::Filled 
				&& (orderIter->OrderStatus == OrderStatus::PendingCancel || orderIter->OrderStatus == OrderStatus::CancelRejected))
			{
				setOrderFinished = true;
			}
			LOG_INFO << "removeOrderByPrice3 OrderRef: " <<rtnOrder.OrderRef
				<< ", orderByPriceIter->second->OrderList: " << orderByPriceIter->second->OrderList.size();

			orderByPriceIter->second->OrderList.erase(orderIter);
			break;
		}
	}
	if (orderByPriceIter->second->OrderList.size() == 0)
	{
		//清空map中该Price记录
		map->erase(orderByPriceIter);
		tracePendingOrders(rtnOrder.Direction);
	}
	else
	{
		updateVolumes(rtnOrder.Direction, rtnOrder.LimitPrice);
	}
}
void DealOrders::getCloseVolumes(double& closeToday)
{
	closeToday = 0;
	auto map = orderMap(targetDirection());
	for (auto iter : *map)
	{
		closeToday += iter.second->VolumeCloseToday;
	}
}

void DealOrders::tracePendingOrders(char direction)
{
	if (direction == INNER_DIRECTION_Buy)
	{
		LOG_DEBUG << "Buy: StrategyID:" << position_.StrategyID << " InstrumentID:" << position_.InstrumentID << " size:" << buyOrders_->size();
		auto iter = buyOrders_->rbegin();
		for (; iter != buyOrders_->rend(); ++iter)
		{
			 LOG_DEBUG << "--Price:" << iter->first << " Volume:" << iter->second->Volume
				<< " VolumeCloseT:" << iter->second->VolumeCloseToday << " VolumePendingCancel:" << iter->second->VolumePendingCancel;
			 for (auto order : iter->second->OrderList)
			 {
				 LOG_DEBUG << "----VolumeRemain:" << order.VolumeRemained << " OrderStatus:" << order.OrderStatus << " OrderRef:" << order.OrderRef;
			 }
		}
	}
	else
	{
		LOG_DEBUG << "Sell: StrategyID:" << position_.StrategyID << " InstrumentID:" << position_.InstrumentID << " size:" << sellOrders_->size();
		auto iter = sellOrders_->rbegin();
		for (; iter != sellOrders_->rend(); ++iter)
		{
			LOG_DEBUG << "--Price:" << iter->first << " Volume:" << iter->second->Volume
				<< " VolumeCloseT:" << iter->second->VolumeCloseToday << " VolumePendingCancel:" << iter->second->VolumePendingCancel;
			for (auto order : iter->second->OrderList)
			{
				LOG_DEBUG << "----VolumeRemain:" << order.VolumeRemained << " OrderStatus:" << order.OrderStatus << " OrderRef:" << order.OrderRef;
			}
		}
	}
}
bool DealOrders::exceedMaxPosition(double appendVolume)
{
	if (targetDirection() == INNER_DIRECTION_Buy)
	{
		double buyVolume = 0.0;
		for (auto buyIter : *buyOrders_)
		{
			buyVolume += buyIter.second->Volume;
		}
		LOG_DEBUG << "exceedMaxPosition netPosition:" << position_.NetPosition << " buyVolume:" << buyVolume
			<< " appendVolume:" << appendVolume << " maxPosition:" << maxPosition();
		double maxPos = maxPosition();
		if (currSetOrder_->setOrderOptions.isReduceOnly == 1)
		{
			maxPos = 0.0;
		}
		if (std::isgreater(position_.NetPosition + buyVolume + appendVolume, maxPos))
			return true;
		else
			return false;
	}
	else
	{
		double sellVolume = 0.0;
		for (auto sellIter : *sellOrders_)
		{
			sellVolume += sellIter.second->Volume;
		}
		LOG_DEBUG << "exceedMaxPosition netPosition:" << position_.NetPosition << " buyVolume:" << sellVolume
			<< " appendVolume:" << appendVolume << " maxPosition:" << maxPosition();
		double maxPos = -1.0*maxPosition();
		if (currSetOrder_->setOrderOptions.isReduceOnly == 1)
		{
			maxPos = 0.0;
		}
		if (std::isless(position_.NetPosition - sellVolume - appendVolume, maxPos))
			return true;
		else
			return false;
	}
}
