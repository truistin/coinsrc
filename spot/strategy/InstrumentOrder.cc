#include "spot/strategy/InstrumentOrder.h"
#include "spot/strategy/OrderController.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/TradingPeriod.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/strategy/Initializer.h"
#include "spot/base/InitialDataExt.h"
#include "spot/utility/TradingDay.h"

using namespace spot;
using namespace spot::base;
using namespace spot::strategy;

InstrumentOrder::InstrumentOrder(const StrategyInstrumentPNLDaily &position, Adapter *adapter)
{
	string instrumentID = position.InstrumentID;
	auto instrument = InstrumentManager::getInstrument(instrumentID);
	if (instrument == nullptr)
	{
		LOG_FATAL << "cann't find instrument:" << instrumentID;
	}
	auto strategyParams = ParametersManager::getStrategyParameters(position.StrategyID);
	if (strategyParams == nullptr)
	{
		LOG_FATAL << "cann't find strategy parameters. strategyID:" << position.StrategyID;
	}

	
	Order order;
	memcpy(order.TradingDay, position.TradingDay, min(sizeof(order.TradingDay) -1 , strlen(position.TradingDay)));
	memcpy(order.InstrumentID, instrumentID.c_str(), min(sizeof(order.InstrumentID) - 1, instrumentID.size()));

	order.StrategyID = position.StrategyID;	

	string exchangeID = instrument->getExchange();
	memcpy(order.ExchangeCode, exchangeID.c_str(), min(sizeof(order.ExchangeCode) - 1, exchangeID.size()));
	auto interfaceType = InitialDataExt::getTdInterfaceType(exchangeID);
	string investorID = InitialData::getInvestorID(interfaceType.c_str(), TradeType);
	memcpy(order.CounterType, interfaceType.c_str(), min(sizeof(order.CounterType) - 1, interfaceType.size()));
	char *buff = CacheAllocator::instance()->allocate(sizeof(DealOrders));
	pendingOrders_ = new (buff) DealOrders(position, adapter);

	buff = CacheAllocator::instance()->allocate(sizeof(map<char, SetOrderInfo*>));
	setOrderInfoMap_ = new (buff) map<char, SetOrderInfo*>;

	initSetOrderInfoMap(order);

}
InstrumentOrder::~InstrumentOrder()
{
	delete pendingOrders_;	
}

void InstrumentOrder::setOrder(char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t cancelSize)
{
	//override
	auto setOrderInfoIter = setOrderInfoMap_->find(direction);
	setOrderInfoIter->second->Direction = direction;
	setOrderInfoIter->second->Price = price;
	setOrderInfoIter->second->Quantity = quantity;
	setOrderInfoIter->second->setOrderOptions = setOrderOptions;
	setOrderInfoIter->second->Finished = false;
	setOrderInfoIter->second->cancelSize = cancelSize;
	setOrderInfoIter->second->pOrder->Direction = direction;
	setOrderInfoIter->second->pOrder->OrderType = setOrderOptions.orderType;

	memcpy(setOrderInfoIter->second->pOrder->MTaker, setOrderOptions.MTaker, MTakerLen);
	memcpy(setOrderInfoIter->second->pOrder->TimeInForce, setOrderOptions.TimeInForce, TimeInForceLen);
	memcpy(setOrderInfoIter->second->pOrder->Category, setOrderOptions.Category, min(uint16_t(CategoryLen), uint16_t(strlen(setOrderOptions.Category))));
	memcpy(setOrderInfoIter->second->pOrder->CoinType, setOrderOptions.CoinType, min(uint16_t(CoinTypeLen), uint16_t(strlen(setOrderOptions.CoinType))));
	memcpy(setOrderInfoIter->second->pOrder->StrategyType, setOrderOptions.StType, min(uint16_t(StrategyTypeLen), uint16_t(strlen(setOrderOptions.StType))));

	std::string today = TradingDay::getToday();
	memcpy(setOrderInfoIter->second->pOrder->TradingDay, today.c_str(), min(TradingDayLen, (uint16_t)today.size()));
//	strcpy(setOrderInfoIter->second->pOrder->Tag, setOrderOptions.orderTag);
	//call pendingOrders
	pendingOrders_->setOrder(setOrderInfoIter->second);
}


//Update internal order book upon receiving 1.return order update 2. send in an new order 3. cancel an existing order 
void InstrumentOrder::updateOrderByPrice(const Order &order)
{
	switch (order.OrderStatus)
	{
		//update - we pass the copy return order update into the internal orderbook - special handling for partial-fill after pendngcancel
	case New:
	case Partiallyfilled:
	{
		pendingOrders_->updateOrderByPrice(order);
		break;
	}
	case CancelRejected:
	{
		pendingOrders_->updateOrderByPrice(order);
		break;
	}
		//remove - we removed orders from the list that are in terminal state
	case Filled:
	{
		bool setOrderFinished = false;
		pendingOrders_->removeOrderByPrice(order, setOrderFinished);
		LOG_INFO << "InstrumentOrder::updateOrderByPrice Filled " << order.OrderRef;
		if (setOrderFinished)
		{
			//Finished set true if pendingcancel order filled.
			auto setOrderInfoIter = setOrderInfoMap_->find(order.Direction);
			if (setOrderInfoIter != setOrderInfoMap_->end())
			{
				setOrderInfoIter->second->Finished = true;
				LOG_INFO << "setOrder finished by order id:" << order.OrderRef << " direction:" << order.Direction;
			}
		}
		break;
	}
	case Cancelled:
	{
		bool setOrderFinished = false;
		LOG_INFO << "InstrumentOrder::updateOrderByPrice Cancelled " << order.OrderRef;
		pendingOrders_->removeOrderByPrice(order, setOrderFinished);
		break;
	}
	case NewRejected:
	{
		bool setOrderFinished = false;
		LOG_INFO << "InstrumentOrder::updateOrderByPrice NewRejected " << order.OrderRef;
		pendingOrders_->removeOrderByPrice(order, setOrderFinished);		
		break;
	}
	default:
	{
		string buffer = "updateOrderByPrice invalid OrderStatus: ";
		buffer = buffer + order.OrderStatus;
		LOG_ERROR << (buffer.c_str());
		break;
	}
	}
}

void InstrumentOrder::checkSetOrder(char direction)
{
	auto setOrderInfoIter = setOrderInfoMap_->find(direction);
	if (setOrderInfoIter == setOrderInfoMap_->end())
	{

		LOG_INFO << "checkSetOrder. there is no data in setOrderInfoMap";
		return;
	}
	if (!setOrderInfoIter->second->Finished)
	{
		SetOrderInfo *setOrderInfo = setOrderInfoIter->second;
		LOG_INFO << "checkSetOrder direction:" << direction << " price:"
			<< setOrderInfo->Price << " quantity:" << setOrderInfo->Quantity;

		// gTickToOrderMeasure->reset();	
		pendingOrders_->setOrder(setOrderInfo);
	}
}

inline int InstrumentOrder::getOrderType(char direction)
{
	return (*setOrderInfoMap_)[direction]->setOrderOptions.orderType;
}
void InstrumentOrder::initSetOrderInfoMap(Order order)
{
	auto buySetOrderInfo = new SetOrderInfo;
	initSetOrderInfo(buySetOrderInfo);
	buySetOrderInfo->Direction = INNER_DIRECTION_Buy;
	auto buyOrder = new Order();
	memcpy(buyOrder, &order, sizeof(Order));
	buySetOrderInfo->pOrder = buyOrder;
	(*setOrderInfoMap_)[INNER_DIRECTION_Buy] = buySetOrderInfo;

	auto sellSetOrderInfo = new SetOrderInfo;
	initSetOrderInfo(sellSetOrderInfo);
	sellSetOrderInfo->Direction = INNER_DIRECTION_Sell;
	auto sellOrder = new Order();
	memcpy(sellOrder, &order, sizeof(Order));
	sellSetOrderInfo->pOrder = sellOrder;
	(*setOrderInfoMap_)[INNER_DIRECTION_Sell] = sellSetOrderInfo;
}
void InstrumentOrder::initSetOrderInfo(SetOrderInfo *setOrderInfo)
{	
	setOrderInfo->Quantity = 0;
	setOrderInfo->Price = NAN;
	setOrderInfo->Finished = true;
} 
double InstrumentOrder::getTopPrice(char direction)
{
	return pendingOrders_->getTopPrice(direction);
}
double InstrumentOrder::getBottomPrice(char direction)
{
	return pendingOrders_->getBottomPrice(direction);
}
int InstrumentOrder::getCancelOrderSize(char direction)
{
	return pendingOrders_->getCancelOrderSize(direction);
}

int InstrumentOrder::query(const Order &order)
{
	return pendingOrders_->query(order);
}

void InstrumentOrder::cancelOrder(const Order &order)
{
	auto map = pendingOrders_->orderMap(order.Direction);
	auto iter = map->find(order.LimitPrice);
	if (iter != map->end())
	{
		auto orderIter = iter->second->OrderList.begin();
		for (; orderIter != iter->second->OrderList.end(); ++orderIter)
		{
			if (orderIter->OrderRef == order.OrderRef)
				pendingOrders_->cancelOrder(*orderIter);
		}
	}
}
