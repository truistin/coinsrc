#include "spot/strategy/StrategyInstrument.h"
#include "spot/base/InitialData.h"
#include "spot/base/InstrumentManager.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/MeasureLatencyCache.h"
using namespace spot;
using namespace spot::base;
using namespace spot::strategy;

StrategyInstrument::StrategyInstrument(Strategy *strategy, string instrumentID, Adapter *adapter)
{
	instrumentID_ = instrumentID;
	strategy_ = strategy;
	strategyID_ = strategy->getStrategyID();
	tradeable_ = false;
	initPosition();
	char *buff = CacheAllocator::instance()->allocate(sizeof(InstrumentOrder));
	instrumentOrder_ = new (buff) InstrumentOrder(position_.pnlDaily(),adapter);
	Instrument *instrument = InstrumentManager::getInstrument(instrumentID_);
	if (instrument)
	{
		instrument_ = instrument;
		position_.setInstrument(instrument);
	}
	else
	{
		LOG_FATAL << "cann't find instrument:" << instrumentID_.c_str();
	}
}
StrategyInstrument::~StrategyInstrument()
{
	delete instrumentOrder_;
}
void StrategyInstrument::initPosition()
{
	StrategyInstrumentPNLDaily *pnlDaily = InitialData::getPNLDaily(strategyID_, instrumentID_);
	if (pnlDaily)
	{
		position_.initPosition(*pnlDaily);
	}
	else
	{
		StrategyInstrumentPNLDaily pnl;
		pnl.StrategyID = strategyID_;
		memcpy(pnl.InstrumentID, instrumentID_.c_str(), sizeof(pnl.InstrumentID));
		//memcpy(pnl.TradingDay, TradingDay::getString().c_str(), sizeof(pnl.TradingDay));TradingDay::getToday()
		memcpy(pnl.TradingDay, TradingDay::getToday().c_str(), sizeof(pnl.TradingDay));
		pnl.AvgBuyPrice = 0.0;
		pnl.AvgSellPrice = 0.0;
		pnl.AggregatedFeeByRate = 0.0;
		position_.initPosition(pnl);
	}
}

void StrategyInstrument::setOrder(char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t cancelSize)
{
	// gTickToOrderMeasure->addMeasurePoint(3);
	instrumentOrder_->setOrder(direction, price, quantity, setOrderOptions, cancelSize);
}
void StrategyInstrument::checkSetOrder(char direction)
{
	instrumentOrder_->checkSetOrder(direction);
}
Instrument* StrategyInstrument::instrument()
{
	return instrument_;
}
void StrategyInstrument::OnRtnOrder(const Order &rtnOrder)
{
	LOG_DEBUG << "Inst:" << rtnOrder.InstrumentID << " OrderRef:" << rtnOrder.OrderRef << " Volume:" << rtnOrder.Volume
		<< " VolumeFilled:" << rtnOrder.VolumeFilled
		<< " VolumeRemained:" << rtnOrder.VolumeRemained
		<< " OrderStatus:" << rtnOrder.OrderStatus;
	instrumentOrder_->updateOrderByPrice(rtnOrder);
	updatePosition(rtnOrder);
}
void StrategyInstrument::updatePosition(const Order &rtnOrder)
{
	if (rtnOrder.OrderStatus == OrderStatus::Partiallyfilled
		|| rtnOrder.OrderStatus == OrderStatus::Filled)
	{
		if (IS_DOUBLE_ZERO(rtnOrder.Volume))
		{
			LOG_DEBUG << "do not need update position.inst:" << rtnOrder.InstrumentID << " volume:" << rtnOrder.Volume;
			return;
		}
		const StrategyInstrumentPNLDaily &pnlDaily = position_.OnRtnOrder(rtnOrder);
		RmqWriter::writePNLDaily(pnlDaily);
	}
}
double StrategyInstrument::getTopPrice(char direction)
{
	return instrumentOrder_->getTopPrice(direction);
}
double StrategyInstrument::getBottomPrice(char direction)
{
	return instrumentOrder_->getBottomPrice(direction);
}
void StrategyInstrument::cancelOrder(const Order &order)
{
	instrumentOrder_->cancelOrder(order);
}

int StrategyInstrument::query(const Order &order)
{
	return instrumentOrder_->query(order);
}

bool StrategyInstrument::hasUnfinishedOrder()
{
	if (buyOrders()->size() + sellOrders()->size() > 0)
		return true;
	else
		return false;
}
int StrategyInstrument::getOrdersCount(char direction, double price)
{
	OrderByPriceMap *orderByPriceMap = instrumentOrder_->getPendingOrders(direction);
	if (orderByPriceMap)
	{
		auto orderByPriceIter = orderByPriceMap->find(price);
		if(orderByPriceIter != orderByPriceMap->end())
		{
			return orderByPriceIter->second->OrderList.size();
		}
	}
	return 0;
}

double StrategyInstrument::getOrdersSize(char direction, double price)
{
	OrderByPriceMap *orderByPriceMap = instrumentOrder_->getPendingOrders(direction);
	if (orderByPriceMap)
	{
		if (std::isnan(price))
		{
			double ordersSize = 0.0;
			for (auto orderByPriceIter = orderByPriceMap->begin(); orderByPriceIter != orderByPriceMap->end(); orderByPriceIter++)
				ordersSize += orderByPriceIter->second->Volume;

			return ordersSize;
		}
		else
		{
			auto orderByPriceIter = orderByPriceMap->find(price);
			if (orderByPriceIter != orderByPriceMap->end())
			{
				return orderByPriceIter->second->Volume;
			}
		}
	}
	return 0.0;
}
