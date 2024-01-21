#include "spot/strategy/StrategyCircuit.h"
#include "spot/utility/Measure.h"
#include "spot/utility/Utility.h"
#include "spot/strategy/OrderController.h"
#include "spot/strategy/Initializer.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/DataStruct.h"
#include "spot/utility/TradingTime.h"
#include "spot/base/InitialDataExt.h"
using namespace spot;
using namespace spot::base;
using namespace spot::strategy;
using namespace spot::utility;

StrategyCircuit::StrategyCircuit(int strategyID, StrategyParameter *params)
	:Strategy(strategyID, params),
	isCountInit_(false)
{
	//setTradeable(false);
	setTradeable(true);

    // void *buff = CacheAllocator::instance()->allocate(sizeof(vector<int>));
    // bianOrderTenSecVec_ = new(buff) vector<int>(86400, 0);

    // void *buff1 = CacheAllocator::instance()->allocate(sizeof(vector<int>));
    // okexReqOrder2SecVec_ = new(buff1) vector<int>(86400, 0);

    // void *buff2 = CacheAllocator::instance()->allocate(sizeof(vector<int>));
    // okexCancelOrder2SecVec_ = new(buff2) vector<int>(86400, 0);
}

void StrategyCircuit::initCount()
{
	for (auto strategyInstrument : strategyInstrumentList())
	{
		isOrderBookInitialised_[strategyInstrument] = false;
		setBuyOrderEpochMs_[strategyInstrument] = 0;
		setSellOrderEpochMs_[strategyInstrument] = 0;
		tradeCount_[strategyInstrument] = 0;
		cancelCount_[strategyInstrument] = 0;
		rejectCount_[strategyInstrument] = 0;
		lastOrderTimeStampMs_[strategyInstrument] = 0;
	}

	isCountInit_ = true;
}

void StrategyCircuit::OnRtnInnerMarketData(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
	if (!isCountInit_)
	{
		initCount();
	}

	if (!isOrderBookInitialised(strategyInstrument) && isTradeable())
	{
		checkOrderBookInitialised(strategyInstrument);
	}

	OnRtnInnerMarketDataTradingLogic(marketData, strategyInstrument);
}
void StrategyCircuit::OnRtnInnerMarketTrade(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument)
{
	OnRtnTradeTradingLogic(marketTrade, strategyInstrument);
}

void StrategyCircuit::OnNewOrder(const Order &order, StrategyInstrument *strategyInstrument)
{
	OnNewOrderTradingLogic(order, strategyInstrument);
}
void StrategyCircuit::OnNewReject(const Order &order, StrategyInstrument *strategyInstrument)
{
	rejectCount_[strategyInstrument]  += 1;
//	checkMaxRejects(strategyInstrument);

	OnNewRejectTradingLogic(order, strategyInstrument);
}
void StrategyCircuit::OnCancelReject(const Order &order, StrategyInstrument *strategyInstrument)
{
	if (IS_DOUBLE_ZERO(order.VolumeRemained))
	{
		LOG_WARN << "Order filled before cancel " << order.InstrumentID
			<< ", OrderRef = " << order.OrderRef
			<< ": OrderPrice = " << order.Price
			<< " RunoverVolume = " << order.Volume;

		// run over
		return;
	}
	else if (order.CancelAttempts < *parameters()->getCbMaxCancelAttempts())
	{
		cancelRejectsMap_[&order] = strategyInstrument;
	} else {
		LOG_WARN << "Max cancel attemps reached " << order.InstrumentID
			<< ", OrderRef =  " << order.OrderRef
			<< ": OrderPrice = " << order.Price
			<< " RemainedVolume = " << order.VolumeRemained
			<< ",CancelAttempts = " << *parameters()->getCbMaxCancelAttempts();
	}

	rejectCount_[strategyInstrument] += 1;
//	checkMaxRejects(strategyInstrument);
}
void StrategyCircuit::OnPartiallyFilled(const Order &order, StrategyInstrument *strategyInstrument)
{
	tradeCount_[strategyInstrument] += 1;
//	checkMaxTrades(strategyInstrument);

	OnPartiallyFilledTradingLogic(order, strategyInstrument);
}
void StrategyCircuit::OnFilled(const Order &order, StrategyInstrument *strategyInstrument)
{
	tradeCount_[strategyInstrument] += 1;
//	checkMaxTrades(strategyInstrument);

	OnFilledTradingLogic(order, strategyInstrument);

	// remove order from cancelRejcts if needed
	cancelRejectsMap_.erase(&order);
}
void StrategyCircuit::OnCanceled(const Order &order, StrategyInstrument *strategyInstrument)
{
	// remove order from cancelRejcts if needed
	cancelRejectsMap_.erase(&order);
	// return;
	if (order.OrderType != ORDERTYPE_FAK)
	{
		cancelCount_[strategyInstrument] += 1;
	//	  checkMaxCancels(strategyInstrument);
	}

	OnCanceledTradingLogic(order, strategyInstrument);
}
void StrategyCircuit::OnTimer()
{
	if (!isCountInit_)
	{
		initCount();
	}

	LOG_DEBUG << "Timer: StrategyCircuit check triggered";

	for (auto strategyInstrument : strategyInstrumentList())
	{

		checkMarketDataConsistency(strategyInstrument);
		checkOrderBookStale(strategyInstrument);

		checkPendingCancelTimeout(strategyInstrument);
		checkPendingOrderTimeout(strategyInstrument);

	//	checkMaxPosition(strategyInstrument);
	}
	checkTradeability();
	OnTimerTradingLogic();
}
void StrategyCircuit::OnCheckCancelOrder()
{
	checkCancelRejects();
}
void StrategyCircuit::OnError()
{
}
bool StrategyCircuit::turnOffStrategy()
{
	LOG_INFO << "StrategyCircuit::turnOffStrategy StrategyID:" << getStrategyID();
	cancelAllOrders();

	if (isTradeable())
	{
		setTradeable(false);
		RmqWriter::writeStrategyInstrumentTradeable(getStrategyID(), "", false);
	}

	return true;
}
bool StrategyCircuit::turnOffInstrument(string instrumentID)
{
	LOG_INFO << "StrategyCircuit::turnOffInstrument based on instrumentID: " << instrumentID << "; StrategyID: " << getStrategyID();
	for (auto strategyInstrument : strategyInstrumentList())
	{
		if (instrumentID.compare(strategyInstrument->getInstrumentID()) == 0)
		{
			turnOffInstrument(strategyInstrument);
			break;
		}
	}
	return true;
}
bool StrategyCircuit::turnOnStrategy()
{
	LOG_INFO << "StrategyCircuit::turnOnStrategy StrategyID:" << getStrategyID();
	setTradeable(true);
	RmqWriter::writeStrategyInstrumentTradeable(getStrategyID(), "", true);
	return true;
}
bool StrategyCircuit::turnOnInstrument(string instrumentID)
{
	LOG_INFO << "StrategyCircuit::turnOnInstrument StrategyID:" << getStrategyID()
		<< " InstrumentID:" << instrumentID.c_str();
	for (auto strategyInstrument : strategyInstrumentList())
	{
		if (instrumentID.compare(strategyInstrument->getInstrumentID()) == 0)
		{
			strategyInstrument->tradeable(true);
			RmqWriter::writeStrategyInstrumentTradeable(getStrategyID(), instrumentID, true);
			break;
		}
	}
	return true;
}

bool StrategyCircuit::isSetOrderIntervalReady(StrategyInstrument *strategyInstrument, char direction)
{
	// check setOrderInterval
	long long currentEpochMs = CURR_MSTIME_POINT;
	long long setOrderEpochMs = (direction == INNER_DIRECTION_Buy) ? setBuyOrderEpochMs_[strategyInstrument] : setSellOrderEpochMs_[strategyInstrument];

	long long setOrderInterval = currentEpochMs - setOrderEpochMs;
	if (setOrderInterval < *parameters()->getCbMinSetOrderIntervalMs())
	{
		LOG_WARN << "SetOrderInterval too small " << strategyInstrument->getInstrumentID()
			<< ": direction = " << direction
			<< " setOrderInterval = " << setOrderInterval
			<< " minSetOrderIntervalMs = " << *parameters()->getCbMinSetOrderIntervalMs();
		return false;
	}
	else
	{
		if (direction == INNER_DIRECTION_Buy)
			setBuyOrderEpochMs_[strategyInstrument] = currentEpochMs;
		else
			setSellOrderEpochMs_[strategyInstrument] = currentEpochMs;

		return true;
	}
}

bool StrategyCircuit::judgeOkexReqCancelExceedCheck(StrategyInstrument *strategyInstrument, tm *ptm, bool OrderFlag,
                                                  char direction, uint32_t& orderNum) {
	//std::lock_guard<std::mutex> lock(okexMtx_);
    const char *exchangeID = strategyInstrument->instrument()->getExchange();
    if (strcmp(exchangeID, "OKEX") == 0) {
		//int secCount = ptm->tm_hour * 60 * 60 + ptm->tm_min * 60 + ptm->tm_sec;
		if (OrderFlag) {
			if (JudgeOkReqCountbyDuration2s(ptm, okexOrdersPer2kMsec_, okexOrder2kMsecStep_, 1)) {
				return true;
			} else {
				return false;
			}
				
		} else {
			if (JudgeOkCancelCountbyDuration2s(ptm, okexOrdersPer2kMsec_, okexOrder2kMsecStep_, orderNum)) {
				return true;
			} else {
				LOG_FATAL << "IMPOSSIBLE SITUATION orderNum: " << orderNum << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_
					<< ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;
				return false;
			}
		}
    }
    return true;									
}

bool StrategyCircuit::judgeBianReqCancelExceedCheck(StrategyInstrument *strategyInstrument, tm *ptm,
		bool OrderFlag, char direction, uint32_t& orderNum)
{
	// in all situation,cancelorder returns true. but if orderNum == 0 return false in reqorder
	//std::lock_guard<std::mutex> lock(bianMtx_);
    const char *exchangeID = strategyInstrument->instrument()->getExchange();
    if (strcmp(exchangeID, "BINANCE") == 0) {
		int secCount = ptm->tm_hour * 60 * 60 + ptm->tm_min * 60 + ptm->tm_sec;
		if (OrderFlag) {
			uint32_t orderNumSec = orderNum;
			JudgeBianOrdersCountbyDurationTenSec(bianOrdersPerTenSec_, bianOrderSecStep_, secCount, orderNumSec);
			if (orderNumSec == 0) return false;

			uint32_t orderNumMin = orderNum;
			JudgeBianOrdersCountbyDurationOneMin(ptm, bianOrdersPerMin_, orderNumMin);
			if (orderNumMin == 0) return false;

			orderNum = 1;
			return true;
		} else {
			uint32_t orderNumSec = orderNum;
			JudgeBianOrdersCountbyDurationTenSec(bianOrdersPerTenSec_, bianOrderSecStep_, secCount, orderNumSec);

			uint32_t orderNumMin = orderNum;
			JudgeBianOrdersCountbyDurationOneMin(ptm, bianOrdersPerMin_, orderNumMin);

			orderNum = orderNumSec < orderNumMin? orderNumSec : orderNumMin;
			return true;
		}
    }
    return true;
}


// bool StrategyCircuit::isSetOrderIntervalReady(StrategyInstrument *strategyInstrument, double price, double quantity)
// {
// 	// check setOrderInterval
// 	long long currentEpochMs = CURR_MSTIME_POINT;
// 	long long setOrderEpochMs = lastOrderTimeStampMs_[strategyInstrument];

// 	long long orderInterval = currentEpochMs - setOrderEpochMs;

// 	if (orderInterval < *parameters()->getCbMinSetOrderIntervalMs())
// 	{
		
// 		// LOG_WARN << "SetOrderInterval too small " << strategyInstrument->getInstrumentID()
// 		// 	<< " setOrderInterval = " << orderInterval
// 		// 	<< " minSetOrderIntervalMs = " << *parameters()->getCbMinSetOrderIntervalMs()
// 		// 	<< " currentEpochMs = " << currentEpochMs
// 		// 	<< "lastOrderTimeStampMs_[strategyInstrument] = " << lastOrderTimeStampMs_[strategyInstrument]
// 		// 	<< "price = " << price
// 		// 	<< "quantity = " << quantity;
		
// 		return false;
// 	}
// 	else
// 	{
// 		// LOG_WARN << "SetOrderInterval is enough " << strategyInstrument->getInstrumentID()
// 		// 	<< " setOrderInterval = " << orderInterval
// 		// 	<< " minSetOrderIntervalMs = " << *parameters()->getCbMinSetOrderIntervalMs()
// 		// 	<< " currentEpochMs = " << currentEpochMs
// 		// 	<< "lastOrderTimeStampMs_[strategyInstrument] = " << lastOrderTimeStampMs_[strategyInstrument]
// 		// 	<< "price = " << price
// 		// 	<< "quantity = " << quantity;

// 		lastOrderTimeStampMs_[strategyInstrument] = currentEpochMs;
// 		return true;
// 	}
// }

bool StrategyCircuit::calcCancelTimesByOkBian(StrategyInstrument *strategyInstrument, tm *ptm, bool OrderFlag,
                                                  char direction, uint64_t currentEpochMs, uint32_t& cancelSize)
{
	if (!judgeBianReqCancelExceedCheck(strategyInstrument, ptm, 0, direction, cancelSize)) {
		LOG_WARN << "Set Bian cancel interval not ready for " << strategyInstrument->getInstrumentID();
		return false;
	} else if (!judgeOkexReqCancelExceedCheck(strategyInstrument, ptm, 0, direction, cancelSize)) {
		LOG_WARN << "Set Okex cancel interval not ready for " << strategyInstrument->getInstrumentID();
		return false;
	}
	return true;
}

bool StrategyCircuit::setOrder(StrategyInstrument *strategyInstrument, char direction, double price, double quantity, SetOrderOptions setOrderOptions, uint32_t operationSize)
{
	// check tradeability
	// strategyInstrument->tradeable(true);
	if (!strategyInstrument->isTradeable() || !isTradeable())
	{
		if (!IS_DOUBLE_ZERO(price*quantity))
		{
			LOG_WARN << "StrategyCircuit::setOrder() Tradeability is off for " << strategyInstrument->getInstrumentID()
				<< ": OrderPrice = " << price
				<< " Quantity = " << quantity;	
		}
		else if (!strategyInstrument->hasUnfinishedOrder()) {
			LOG_ERROR << "StrategyCircuit Tradeability is off & spot hasUnfinishedOrder ";
		}
		return false;	
	}

	// check orderIntervalReady, order time gap, asyhttp, needn't
	// if (!isSetOrderIntervalReady(strategyInstrument, direction))
	// {
	// 	//LOG_WARN << "Set order interval not ready for " << strategyInstrument->getInstrumentID();
	// 	return false;
	// }

	tm ptm;
    getCurrentPtm(ptm);
	uint64_t currentEpochMs = CURR_MSTIME_POINT;
	// uint32_t cancelSize = strategyInstrument->getInstrumentOrder()->getCancelOrderSize(direction);

	if ((IS_DOUBLE_ZERO(price) || IS_DOUBLE_ZERO(quantity))) {
		// LOG_INFO << "before CANCEL Strategy::cancel setOrder: " << cancelSize << ", direction: " << direction ;
		// if (!calcCancelTimesByOkBian(strategyInstrument, &ptm, 0, direction, currentEpochMs, cancelSize)) {
		// 	LOG_FATAL << "impossible CancelOrder interval is not enough " << strategyInstrument->getInstrumentID();
		// 	return false;
		// }
		// // cancellation
		// LOG_INFO << "after CANCEL Strategy::cancel setOrder: " << cancelSize << ", direction: " << direction ;
		// if (cancelSize == 0) return true;
		Strategy::setOrder(strategyInstrument, direction, 0, 0, setOrderOptions, operationSize);
		return true;
	} 
	// else {
	// 	uint32_t reqSize = 1;
	// 	if (!judgeBianReqCancelExceedCheck(strategyInstrument, &ptm, 1, direction, reqSize)) {
	// 		//LOG_WARN << "Set Bian order interval not ready for " << strategyInstrument->getInstrumentID();
	// 		return false;
	// 	} 
	// 	// okex can't take order when the pending orders beyond 60.
	// 	if (!judgeOkexReqCancelExceedCheck(strategyInstrument, &ptm, 1, direction, reqSize)) {
	// 		return false;
	// 	}
	// 	// okex 50ms interval protect
	// }

	// check setOrderOptions
	// if (setOrderOptions.checkStrategyCircuit)
	// {
		// check orderBookInit
		if (!strategyInstrument->instrument()->orderBook()->isInit())
		{
			LOG_WARN << "OrderBook not init for " << strategyInstrument->getInstrumentID();
			return false;
		}
	// }

    SymbolInfo *symbolInfo = InitialData::getSymbolInfo(strategyInstrument->getInstrumentID());
    string exchange = strategyInstrument->instrument()->getExchange();
    if(exchange == "OKEX") {
        // quantity = round(quantity / symbolInfo->CoinOrderSize); // ΪʲôҪround
		quantity = quantity / symbolInfo->CoinOrderSize;
    }

	// check price quantity
	if (std::islessequal(quantity, 0.0) || std::islessequal(price, 0.0) ||
		!IS_DOUBLE_NORMAL(price) || !IS_DOUBLE_NORMAL(quantity))
	{
		LOG_FATAL << "OrderQuantity is negative for " << strategyInstrument->getInstrumentID()
			<< ": OrderPrice = " << price
			<< " Quantity = " << quantity;
		return false;
	}
	if (direction == INNER_DIRECTION_Sell && strategyInstrument->position().getNetPosition() < 0) {
		if (!checkMaxPosition(strategyInstrument, symbolInfo->CoinOrderSize)) {
			LOG_WARN << "StrategyCircuit sell orders beyond TcMaxPosition: " << strategyInstrument->position().getNetPosition();
			return false;
		}
	}
	if (direction == INNER_DIRECTION_Buy && strategyInstrument->position().getNetPosition() > 0) {
		if (!checkMaxPosition(strategyInstrument, symbolInfo->CoinOrderSize)) {
			LOG_WARN << "StrategyCircuit buy orders beyond TcMaxPosition: " << strategyInstrument->position().getNetPosition();
			return false;
		}
	}

	Strategy::setOrder(strategyInstrument, direction, price, quantity, setOrderOptions);
	return true;
}

void StrategyCircuit::cancelOrdersByInstrument(StrategyInstrument* strategyInstrument)
{
	SetOrderOptions setOrderOptions;
	LOG_WARN << "StrategyCircuit::cancelOrdersByInstrument";

	// tm ptm;
	// getCurrentPtm(ptm);
	// uint64_t currentEpochMs = CURR_MSTIME_POINT;
	// if (!calcCancelTimesByOkBian(strategyInstrument, &ptm, 0, INNER_DIRECTION_Sell, currentEpochMs))
	// {
	// 	LOG_ERROR << "StrategyCircuit cancelAllOrders Set order interval not ready for " << strategyInstrument->getInstrumentID();
	// 	return;
	// }

	Strategy::setOrder(strategyInstrument, INNER_DIRECTION_Sell, 0, 0, setOrderOptions); 	// cancel Sell first
	
	// if (!calcCancelTimesByOkBian(strategyInstrument, &ptm, 0, INNER_DIRECTION_Buy, currentEpochMs))
	// {
	// 	LOG_ERROR << "StrategyCircuit cancelAllOrders Set order interval not ready for " << strategyInstrument->getInstrumentID();
	// 	return;
	// }
	Strategy::setOrder(strategyInstrument, INNER_DIRECTION_Buy, 0, 0, setOrderOptions);		// cancel Buy
}

void StrategyCircuit::cancelAllOrders()
{
	
	for (auto iter : strategyInstrumentList())
	{
		cancelOrdersByInstrument(iter);
	}
}

void StrategyCircuit::turnOffInstrument(StrategyInstrument* strategyInstrument)
{
	LOG_INFO << "StrategyCircuit::turnOffInstrument InstrumentID:" << strategyInstrument->getInstrumentID() << " StrategyID: " << getStrategyID();
	cancelOrdersByInstrument(strategyInstrument);
	strategyInstrument->tradeable(false);
	RmqWriter::writeStrategyInstrumentTradeable(getStrategyID(), strategyInstrument->getInstrumentID(), false);
}

void StrategyCircuit::checkOrderBookInitialised(StrategyInstrument* strategyInstrument)
{
	if (!strategyInstrument->instrument()->orderBook()->isInit())
	{
		if (strategyInstrument->isTradeable())
			LOG_WARN << "OrderBook not initialised for " << strategyInstrument->getInstrumentID();
		return;
	}

	LOG_INFO << "OrderBook Initialised! StrategyID = " << getStrategyID() << "; InstrumentID = " << strategyInstrument->getInstrumentID();
	isOrderBookInitialised_[strategyInstrument] = true;
}

bool StrategyCircuit::checkMarketDataConsistency(StrategyInstrument* strategyInstrument)
{
	if (strategyInstrument->isTradeable()
		&& (!strategyInstrument->instrument()->orderBook()->isConsistent()))
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMarketDataConsistency failed for " << strategyInstrument->getInstrumentID()
			<< " askPrice1:" << strategyInstrument->instrument()->orderBook()->askPrice()
			<< " bidPrice1:" << strategyInstrument->instrument()->orderBook()->bidPrice()
			<< " isTradeable: " << strategyInstrument->isTradeable();
		turnOffInstrument(strategyInstrument);
		return false;
	}
	return true;
}

void StrategyCircuit::checkPendingOrderTimeout(StrategyInstrument* strategyInstrument)
{
	// buy orders
	for (auto orderByPrice : *strategyInstrument->buyOrders())
	{
		for (auto innerOrder : orderByPrice.second->OrderList)
		{
			if (innerOrder.OrderStatus == OrderStatus::PendingNew || innerOrder.OrderStatus == OrderStatus::PendingCancel)
			{
				long long timeFromPending = CURR_MSTIME_POINT - innerOrder.TimeStamp;
				if (timeFromPending > *parameters()->getCbPendingOrderTimeoutLimitSec())
				{
					LOG_WARN << "CB Alarm " << getStrategyID() << " - checkPendingOrderTimeout failed for " << strategyInstrument->getInstrumentID()
					<< " OrderRef=" << innerOrder.OrderRef
					<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
					<< ", TimeStamp: " << innerOrder.TimeStamp;
					innerOrder.QryFlag = QryType::QRYORDER;
					strategyInstrument->query(innerOrder);
				}

				if (innerOrder.CancelAttempts == maxCancelOrderAttempts) {
					LOG_ERROR << "CB Alarm " << getStrategyID() << " , instrument: " << strategyInstrument->getInstrumentID()
					<< " OrderRef=" << innerOrder.OrderRef
					<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
					<< ", TimeStamp: " << innerOrder.TimeStamp;
					innerOrder.QryFlag = QryType::QRYORDER;
					strategyInstrument->query(innerOrder);
				}

				if (innerOrder.CancelAttempts > (maxCancelOrderAttempts + 2)) {
					turnOffInstrument(strategyInstrument);
					LOG_ERROR << "CB Alarm " << getStrategyID() << " - turnOffInstrument for " << strategyInstrument->getInstrumentID()
						<< " OrderRef=" << innerOrder.OrderRef
						<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
						<< ", TimeStamp: " << innerOrder.TimeStamp;
				}
			}
		}
	}

	// sell orders
	for (auto orderByPrice : *strategyInstrument->sellOrders())
	{
		for (auto innerOrder : orderByPrice.second->OrderList)
		{
			if (innerOrder.OrderStatus == OrderStatus::PendingNew || innerOrder.OrderStatus == OrderStatus::PendingCancel)
			{
				long long timeFromPending = CURR_MSTIME_POINT - innerOrder.TimeStamp;
				if (timeFromPending > *parameters()->getCbPendingOrderTimeoutLimitSec())
				{
					LOG_WARN << "CB Alarm " << getStrategyID() << " - checkPendingOrderTimeout failed for " << strategyInstrument->getInstrumentID()
						<< " OrderRef=" << innerOrder.OrderRef
						<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
						<< ", TimeStamp: " << innerOrder.TimeStamp;
					innerOrder.QryFlag = QryType::QRYORDER;
					strategyInstrument->query(innerOrder);
				}

				if (innerOrder.CancelAttempts == maxCancelOrderAttempts) {
					LOG_ERROR << "CB Alarm " << getStrategyID() << " , instrument: " << strategyInstrument->getInstrumentID()
					<< " OrderRef=" << innerOrder.OrderRef
					<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
					<< ", TimeStamp: " << innerOrder.TimeStamp;
					innerOrder.QryFlag = QryType::QRYORDER;
					strategyInstrument->query(innerOrder);
				}

				if (innerOrder.CancelAttempts > (maxCancelOrderAttempts + 2)) {
					LOG_ERROR << "CB Alarm " << getStrategyID() << " - turnOffInstrument for " << strategyInstrument->getInstrumentID()
					<< " OrderRef=" << innerOrder.OrderRef
					<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
					<< ", TimeStamp: " << innerOrder.TimeStamp;
					turnOffInstrument(strategyInstrument);
				}
			}
		}
	}
}

bool StrategyCircuit::checkOrderBookStale(StrategyInstrument* strategyInstrument)
{
	uint64_t now = CURR_MSTIME_POINT;
	uint64_t timeStamp = strategyInstrument->instrument()->orderBook()->getLastUpdateTimestamp();
	uint32_t timeFromLastUpdate = now - timeStamp;
	//LOG_INFO << "StrategyCircuit::checkOrderBookStale CURR_MSTIME_POINT: " << CURR_MSTIME_POINT << "LastUpdateTimestamp: " << strategyInstrument->instrument()->orderBook()->getLastUpdateTimestamp();
	if (strategyInstrument->isTradeable() && timeFromLastUpdate > *parameters()->getCbOrderBookStaleTimeLimitSec())
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - checkOrderBookStale failed for " << strategyInstrument->getInstrumentID()
			<< " currTime:" << TradingPeriod::getCurrTime().c_str() << ", strategyInstrument->isTradeable(): " << strategyInstrument->isTradeable()
			<< ", timeFromLastUpdate: " << timeFromLastUpdate << ", now: " << now << ", timeStamp: " << timeStamp;
		// std::cout << "CB Alarm " << getStrategyID() << " - checkOrderBookStale failed for " << strategyInstrument->getInstrumentID()
		// 	<< " currTime:" << TradingPeriod::getCurrTime().c_str() << ", strategyInstrument->isTradeable(): " << strategyInstrument->isTradeable()
		// 	<< ", timeFromLastUpdate: " << timeFromLastUpdate << ", now: " << now << ", timeStamp: " << timeStamp;
		strategyInstrument->instrument()->orderBook()->setIsInit(0);
		// turnOffInstrument(strategyInstrument);
		return false;
	}
	return true;
}

bool StrategyCircuit::checkMaxPosition(StrategyInstrument* strategyInstrument, double coinOrderSize)
{
	if ((strategyInstrument->instrument()->getInstrumentID().find("perp") != strategyInstrument->instrument()->getInstrumentID().npos)) {
		if (std::isgreater(abs(strategyInstrument->position().getNetPosition() * strategyInstrument->instrument()->getMultiplier() / 
			strategyInstrument->instrument()-> orderBook()->midPrice()), 
			*parameters()->getTcMaxPosition())) {
			LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMaxPosition failed for " << strategyInstrument->getInstrumentID()
				<< ", midPrice = " << strategyInstrument->instrument()-> orderBook()->midPrice()
				<< ", multiple = " << strategyInstrument->instrument()->getMultiplier()
				<< ", position = " << strategyInstrument->position().getNetPosition()
				<< ", maxPosition = " << *parameters()->getTcMaxPosition();
		//turnOffInstrument(strategyInstrument);
		return false;
		}

	}
	else {
		if (std::isgreater(abs(strategyInstrument->position().getNetPosition() * coinOrderSize) , *parameters()->getTcMaxPosition()))
		{
			LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMaxPosition failed for " << strategyInstrument->getInstrumentID()
				<< ": position = " << strategyInstrument->position().getNetPosition()
				<< " maxPosition = " << *parameters()->getTcMaxPosition();
			//turnOffInstrument(strategyInstrument);
			return false;
		}
	}
	return true;
}

void StrategyCircuit::checkTradeability()
{
	if (!isTradeable())
	{
		LOG_DEBUG << "tradeable is false";
		cancelAllOrders();
	}
}

void StrategyCircuit::checkMaxTrades(StrategyInstrument* strategyInstrument)
{
	string instrumentID = strategyInstrument->getInstrumentID();
	LOG_DEBUG << "checkMaxTrades: " << instrumentID << " tradeCount = " << tradeCount_[strategyInstrument];

	if (tradeCount_[strategyInstrument] >= *parameters()->getTcMaxTrades())
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMaxTrades exceeds tcMaxTrades: " << instrumentID
			<< " tradeCount = " << tradeCount_[strategyInstrument]
			<< " tcMaxTrades = " << *parameters()->getTcMaxTrades();
		turnOffInstrument(instrumentID);
	}
}

/*void StrategyCircuit::checkMaxCancels(StrategyInstrument* strategyInstrument)
{
	string instrumentID = strategyInstrument->getInstrumentID();
	LOG_DEBUG << "checkMaxCancels: " << instrumentID << " cancelCount = " << cancelCount_[strategyInstrument];

	if (cancelCount_[strategyInstrument] >= *parameters()->getTcMaxCancelAlarm())
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - please turn on ReduceOnly based on tcMaxCancelAlarm: " << instrumentID
			<< " cancelCount = " << cancelCount_[strategyInstrument]
			<< " tcMaxCancelAlarm = " << *parameters()->getTcMaxCancelAlarm();
	}

	if (cancelCount_[strategyInstrument] >= *parameters()->getTcMaxCancels())
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMaxCancels exceeds tcMaxCancels: " << instrumentID
			<< " cancelCount = " << cancelCount_[strategyInstrument]
			<< " tcMaxCancels = " << *parameters()->getTcMaxCancels();
		turnOffInstrument(instrumentID);
	}
}*/

/*void StrategyCircuit::checkMaxRejects(StrategyInstrument* strategyInstrument)
{
	string instrumentID = strategyInstrument->getInstrumentID();
	LOG_DEBUG << "checkMaxRejects: " << instrumentID << " rejectCount = " << rejectCount_[strategyInstrument];

	if (rejectCount_[strategyInstrument] >= *parameters()->getTcMaxRejects())
	{
		LOG_WARN << "CB Alarm " << getStrategyID() << " - checkMaxRejects exceeds tcMaxRejects: " << instrumentID
			<< " rejectCount = " << rejectCount_[strategyInstrument]
			<< " tcMaxRejects = " << *parameters()->getTcMaxRejects();
		turnOffInstrument(instrumentID);
	}
}*/

void StrategyCircuit::checkCancelRejects()
{
	for (auto cancelRejectIter : cancelRejectsMap_)
	{
		const Order* order = cancelRejectIter.first;
		if ((order->OrderStatus != Cancelled && order->OrderStatus != Filled)
			&& order->CancelAttempts < *parameters()->getCbMaxCancelAttempts())
		{
			// try to cancel the order
			StrategyInstrument* strategyInstrument = cancelRejectIter.second;

			// if (!calcCancelTimesByOkBian(strategyInstrument, &ptm, 0, direction)) {
			// 	LOG_WARN << "strategyTimer checkCancelRejects Set order interval not ready for " << strategyInstrument->getInstrumentID();
			// 	return;
			// }
			strategyInstrument->cancelOrder(*order);

			LOG_WARN << "Try to cancel again for instrumentID = " << strategyInstrument->getInstrumentID()
				<< "; orderSysID = " << order->OrderSysID << "; orderStatus = " << order->OrderStatus;
		}
	}
}

void StrategyCircuit::checkPendingCancelTimeout(StrategyInstrument* strategyInstrument)
{
	// buy orders
	for (auto orderByPrice : *strategyInstrument->buyOrders())
	{
		for (auto innerOrder : orderByPrice.second->OrderList)
		{
			if (innerOrder.OrderStatus == OrderStatus::PendingCancel)
			{
				long long timeFromPending = CURR_MSTIME_POINT - innerOrder.TimeStamp;
				if (timeFromPending > *parameters()->getCbPendingOrderTimeoutLimitSec())
				{
					LOG_WARN << "CB Alarm " << getStrategyID() << " - checkPendingCancel buy for " << strategyInstrument->getInstrumentID()
					<< " OrderRef=" << innerOrder.OrderRef
					<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
					<< ", TimeStamp: " << innerOrder.TimeStamp;
					strategyInstrument->cancelOrder(innerOrder);
				}
			}
		}
	}

	// sell orders
	for (auto orderByPrice : *strategyInstrument->sellOrders())
	{
		for (auto innerOrder : orderByPrice.second->OrderList)
		{
			if (innerOrder.OrderStatus == OrderStatus::PendingCancel)
			{
				long long timeFromPending = CURR_MSTIME_POINT - innerOrder.TimeStamp;
				if (timeFromPending > *parameters()->getCbPendingOrderTimeoutLimitSec())
				{
					LOG_WARN << "CB Alarm " << getStrategyID() << " - checkPendingCancel sell for " << strategyInstrument->getInstrumentID()
						<< " OrderRef=" << innerOrder.OrderRef
						<< ", CURR_MSTIME_POINT: " << CURR_MSTIME_POINT
						<< ", TimeStamp: " << innerOrder.TimeStamp;
					strategyInstrument->cancelOrder(innerOrder);
				}
			}
		}
	}
}
