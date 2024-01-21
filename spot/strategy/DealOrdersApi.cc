#include "spot/strategy/DealOrdersApi.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/base/TradingPeriod.h"

using namespace spot::base;
using namespace spot::strategy;
DealOrdersApi::DealOrdersApi(const StrategyInstrumentPNLDaily &position, Adapter *adapter)
	:position_(position),adapterPendingOrder_(adapter)
{
	instrument_ = InstrumentManager::getInstrument(position_.InstrumentID);
	if (instrument_ == nullptr)
	{
		LOG_FATAL << "initOffsetPriority instrument is NULL ";
	}
//	initOffsetPriority();
	auto strategyParams = ParametersManager::getStrategyParameters(position.StrategyID);
	cbMinSetOrderIntervalMs_ = strategyParams->getCbMinSetOrderIntervalMs();
	reqOrderEpochMsMap_[INNER_DIRECTION_Buy] = 0;
	reqOrderEpochMsMap_[INNER_DIRECTION_Sell] = 0;
	orderTimeStamp_ = 0;
}
DealOrdersApi::~DealOrdersApi()
{

}
void DealOrdersApi::newOrder(double closeTodayVolume, SetOrderInfo *setOrderInfo)
{
	//check upperlimit/lowerlimit price
/*	if ((IS_DOUBLE_NORMAL(instrument_->orderBook()->upperLimit())
		&& std::isgreater(setOrderInfo->pOrder->LimitPrice, instrument_->orderBook()->upperLimit()))
		|| (IS_DOUBLE_NORMAL(instrument_->orderBook()->lowerLimit())
			&& std::isless(setOrderInfo->pOrder->LimitPrice, instrument_->orderBook()->lowerLimit())))
	{
		LOG_WARN << "reach limit price.upperlimit:" << instrument_->orderBook()->upperLimit()
			<< " lowerlimit:" << instrument_->orderBook()->lowerLimit()
			<< " limit pirce:" << setOrderInfo->pOrder->LimitPrice; 
		return;
	}*/
	//make a local copy for use later in the code
	setOrderInfo_ = setOrderInfo;

	LOG_DEBUG << "setOrderOptions - OrderType: " << setOrderInfo_->setOrderOptions.orderType;  //<< " ForcedOpen: " << setOrderInfo_->setOrderOptions.isForcedOpen;

	// gTickToOrderMeasure->addMeasurePoint(4);
	if ((instrument_->getExchange() == Exchange_BINANCE) ||
		(instrument_->getExchange() == Exchange_OKEX) ||
		(instrument_->getExchange() == Exchange_BYBIT)) {
			openOrder(*setOrderInfo->pOrder);
	} else {
		if (setOrderInfo->Direction == INNER_DIRECTION_Buy)
			splitBuyOrders(*setOrderInfo->pOrder, closeTodayVolume);
		else
			splitSellOrders(*setOrderInfo->pOrder, closeTodayVolume);
	}
}

void DealOrdersApi::splitBuyOrders(Order &order, double closeToday)
{	
	if (IS_DOUBLE_LESS((position_.TodayShort - closeToday), 0)) {
		LOG_FATAL << "position_.TodayShort less than PendingcloseToday: " << position_.TodayShort
			<< ", closeToday: " << closeToday;
	}

	// if total short position is zero,proceed to place order
	else if (IS_DOUBLE_ZERO(position_.TodayShort - closeToday))
	{
		openOrder(order);
	}

	//if today's short position is bigger or the same as the order we are trying to place
	//simply close Today's short 
	else if (std::isgreaterequal(position_.TodayShort - closeToday, order.VolumeTotalOriginal))
	{
		closeTodayOrder(order);
	}
	else // (std::islessequal(position_.TodayShort - closeToday, order.VolumeTotalOriginal))
	{
		// if today's short position is small than the order, close what's left first then open a new
		// long with remaining quantity
		double openVolume = order.VolumeTotalOriginal - (position_.TodayShort - closeToday);
		order.VolumeTotalOriginal = position_.TodayShort - closeToday;
		closeTodayOrder(order);
		order.VolumeTotalOriginal = openVolume;
		openOrder(order);
	}
}

void DealOrdersApi::splitSellOrders(Order &order, double closeToday)
{
	if (IS_DOUBLE_LESS((position_.TodayLong - closeToday), 0)) {
		LOG_FATAL << "position_.TodayLong less than PendingcloseToday: " << position_.TodayLong 
			<< ", closeToday: " << closeToday;
	}
	//if force open flag is set
/*	if (setOrderInfo_->setOrderOptions.isForcedOpen)
	{
		LOG_FATAL << "CB Alarm - splitSellOrders isForcedOpen" << setOrderInfo_->setOrderOptions.isForcedOpen;
		openOrder(order);
	}*/

	else if (IS_DOUBLE_ZERO(position_.TodayLong - closeToday))
	{
		openOrder(order);
	}

	else if (std::isgreaterequal(position_.TodayLong - closeToday , order.VolumeTotalOriginal))
	{
		closeTodayOrder(order);
	}
	else
	{
		//if smaller close yesteday first then open a new with remaining quantity
		double openVolume = order.VolumeTotalOriginal - (position_.TodayLong - closeToday);
		order.VolumeTotalOriginal = position_.TodayLong - closeToday;
		closeTodayOrder(order);
		order.VolumeTotalOriginal = openVolume;
		openOrder(order);
	}
}

inline void DealOrdersApi::openOrder(Order &order)
{
	order.VolumeRemained = order.VolumeTotalOriginal;
	order.Offset = INNER_OFFSET_Open;
	reqOrder(order);
}

inline void DealOrdersApi::closeTodayOrder(Order &order)
{
	order.VolumeRemained = order.VolumeTotalOriginal;
	order.Offset = INNER_OFFSET_CloseToday;
	reqOrder(order);
}
int DealOrdersApi::reqOrder(Order &order)
{	
	int ret = adapterPendingOrder_->reqOrder(order);
	if (ret == 0)
	{
		reqOrderEpochMsMap_[order.Direction] = CURR_MSTIME_POINT;
		orderTimeStamp_ = CURR_MSTIME_POINT;
	}
	return ret;
}
bool DealOrdersApi::isReqOrderIntervalReady(char direction)
{
	if (*cbMinSetOrderIntervalMs_ == 0)
		return true;

	long long currentEpochMs = CURR_MSTIME_POINT;
	long long reqOrderEpochMs = reqOrderEpochMsMap_[direction];

	long long reqOrderInterval = currentEpochMs - reqOrderEpochMs;
	if (reqOrderInterval < *cbMinSetOrderIntervalMs_)
	{
		LOG_WARN << "SetOrderInterval too small " << instrument_->getInstrumentID()
			<< ": direction = " << direction
			<< " setOrderInterval = " << reqOrderInterval
			<< " minSetOrderIntervalMs = " << *cbMinSetOrderIntervalMs_; 
		return false;
	}
	else
	{
		return true;
	}
}
bool DealOrdersApi::isReqOrderIntervalReadyWithDir()
{
	if (*cbMinSetOrderIntervalMs_ == 0)
		return true;

	long long currentEpochMs = CURR_MSTIME_POINT;
	long long reqOrderEpochMs = orderTimeStamp_;

	long long reqOrderInterval = currentEpochMs - reqOrderEpochMs;

	if (reqOrderInterval < *cbMinSetOrderIntervalMs_)
	{
		LOG_WARN << "SetOrderInterval too small " << instrument_->getInstrumentID()
			<< " setOrderInterval = " << reqOrderInterval
			<< " minSetOrderIntervalMs = " << *cbMinSetOrderIntervalMs_; 
		return false;
	}
	else
	{
		return true;
	}
}
