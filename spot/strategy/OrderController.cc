#include "spot/strategy/OrderController.h"
#include "spot/strategy/StrategyFactory.h"
#include "spot/base/InitialData.h"
#include "spot/utility/Logging.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/TradingPeriod.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/MeasureOrderToAck.h"

#ifdef __I_ShmMt_MD__
#include "spot/shmmd/ShmMtServer.h"
#endif //__I_ShmMt_MD__

using namespace spot;
using namespace spot::base;
using namespace spot::strategy;
using namespace boost::posix_time;
#ifdef __I_ShmMt_MD__
using namespace spot::shmmd;
#endif

using namespace spot;
int OrderController::orderRef_ = 0;
Adapter * OrderController::adapterGateway_;
RtnOrderMap OrderController::orderMap_;

OrderController::OrderController()
{
	int days = 0;
	orderRef_ = initOrderRef(InitialData::getSpotID(), TradingDay::getToday(),days);
	initOrderStatusMappingMap();
	orderMap_.clear();
}
OrderController::~OrderController()
{
}

int OrderController::getOrderRef()
{
	orderRef_++;
	return orderRef_;
}

void OrderController::setAdapter(Adapter *adapter)
{
	adapterGateway_ = adapter;
}

int OrderController::query(const Order &order)
{
	int ret =	adapterGateway_->query(order);
	return ret;
}

int OrderController::reqOrder(Order &order)
{
	// gTickToOrderMeasure->addMeasurePoint(5);
	insertOrder(order);
	order.EpochTimeReqBefore = CURR_MSTIME_POINT;
	int ret =	adapterGateway_->reqOrder(order);
	order.EpochTimeReqAfter = CURR_MSTIME_POINT;
	
	if (ret == 0)
	{
		// gTickToOrderMeasure->sendTick2OrderMetricData(order.OrderRef,order.CounterType, order.ExchangeCode, order.StrategyID);

		LOG_DEBUG << "reqOrder success detail strategyID:" << order.StrategyID << " instrumentID:" << order.InstrumentID
			<< " orderRef:" << order.OrderRef << " volume:" << order.VolumeTotalOriginal << " limitPrice:" << order.LimitPrice
			<< " direction:" << order.Direction << " offset:" << order.Offset;
		memcpy(order.InsertTime, TradingPeriod::getCurrTime().c_str(), sizeof(order.InsertTime));
		order.EpochTimeReturn = CURR_MSTIME_POINT;
		//insertOrder(order);

		RmqWriter::writeOrder(order);
	}
	else
	{
		//orderMap_.erase(RtnOrderMap::value_type(order.OrderRef, order));
		LOG_ERROR << "reqOrder failed detail: " << order.toString();
	}
	return ret;
}
void OrderController::insertOrder(const Order &order)
{
	orderMap_.insert(RtnOrderMap::value_type(order.OrderRef, order));
}

int OrderController::cancelOrder(Order &order)
{
	if (order.CancelAttempts >= maxCancelOrderAttempts)
	{
		LOG_WARN << "cancel order reached maxCancelOrderAttempts:" << maxCancelOrderAttempts
			<< " CancelAttempts:" << order.CancelAttempts << " OrderRef:" << order.OrderRef;
		return 0;
	}

	int ret = adapterGateway_->cancelOrder(order);
	if (ret != 0)
	{
		string buffer = "Error:cancelOrder failed. OrderRef:";
		buffer = buffer + std::to_string(order.OrderRef);
		buffer = buffer + ",OrderSysId:";
		buffer = buffer +  order.OrderSysID;
		LOG_ERROR << buffer.c_str();
	}
	//LOG_INFO << "cancelOrder orderRef:" << order.OrderRef << " orderSysID:" << order.OrderSysID;
	// logInfoInnerOrder("cancelOrder info", order);
	//LOG_INFO << "cancelOrder info: " << order.toString();
	auto orderIter = orderMap_.find(order.OrderRef);
	if (orderIter != orderMap_.end())
	{
		orderIter->second.OrderStatus = PendingCancel;
		orderIter->second.CancelAttempts++;
		memcpy(order.InsertTime, TradingPeriod::getCurrTime().c_str(), sizeof(order.InsertTime));
		order.EpochTimeReturn = CURR_MSTIME_POINT;
	}
	else
	{
		string buffer = "Error:cancelOrder can't find order. OrderRef:";
		buffer = buffer + std::to_string(order.OrderRef);
		LOG_ERROR << buffer.c_str();
	}
	RmqWriter::writeOrder(order);
	return ret;
}

void OrderController::OnRtnOrder(Order &order)
{
	auto orderMapIter = orderMap_.find(order.OrderRef);
	if (orderMapIter == orderMap_.end())
	{
		LOG_WARN << "can't find order " << order.OrderRef << ", OrderStatus:" << order.OrderStatus;
		return;
	}

	LOG_DEBUG << "OrderInMap " << orderMapIter->second.toString();

	updateTrade(order, orderMapIter->second);

	if (!isInvalidOrderStateTransition(orderMapIter->second.OrderStatus, order.OrderStatus))
	{
		if (isCanceledOrderBeforePartiallyfilled(order, orderMapIter->second)) {
			return;
		}
		if (order.OrderStatus == New)
		{
			orderMapIter->second.EpochTimeReturn = order.EpochTimeReturn;
		}


		orderMapIter->second.OrderStatus = order.OrderStatus;

		updateOrder(order, orderMapIter->second);

		memcpy(order.TradingDay,orderMapIter->second.TradingDay, min(sizeof(order.TradingDay) - 1, sizeof(orderMapIter->second.TradingDay) -1));
		orderMapIter->second.EpochTimeReturn = order.EpochTimeReturn;

		Order tOrder;
		memcpy(&tOrder, &orderMapIter->second, sizeof(Order));
		// OnStrategyRtnOrder(order, orderMapIter->second);

		OnStrategyRtnOrder(order, tOrder);
	}
	else 
	{
		LOG_WARN << "Invalid OrderStatus Transaction, OrderRef:" << order.OrderRef << " OrderSysID:" << order.OrderSysID 
			<< " Old OrderStatus:" << orderMapIter->second.OrderStatus << " New OrderStatus:" << order.OrderStatus << " Volume:" << order.Volume
			<< ", VolumeTotalOriginal: " << order.VolumeTotalOriginal;;
	}

	if (orderMapIter->second.OrderStatus == OrderStatus::Filled || orderMapIter->second.OrderStatus == OrderStatus::Cancelled ||
		orderMapIter->second.OrderStatus  == OrderStatus::NewRejected) {
		orderMap_.erase(orderMapIter);
	}
}
void OrderController::OnStrategyRtnOrder(Order &rtnOrder, Order &cacheOrder)
{
	LOG_DEBUG << "writeOrder:" << cacheOrder.toString();
	RmqWriter::writeOrder(cacheOrder);	
	auto strategyMap = StrategyFactory::strategyMap();
	auto strategyIter = strategyMap.find(cacheOrder.StrategyID);
	if (strategyIter != strategyMap.end())
	{
		strategyIter->second->OnRtnOrder(cacheOrder);
		if (checkCancelBeforePartiallyfilledOrder(cacheOrder))
		{
			cacheOrder.OrderStatus = OrderStatus::Cancelled;
			LOG_WARN << "canceled Order from canceledBeforePartiallyFilledOrderMap orderRef=" << cacheOrder.OrderRef;
			strategyIter->second->OnRtnOrder(cacheOrder);

			RmqWriter::writeOrder(cacheOrder);
		}
	}
}

bool OrderController::isInvalidOrderStateTransition(char currentOrderStatus, char newOrderStatus)
{
	auto key = make_pair<char, char>(move(currentOrderStatus), move(newOrderStatus));
	auto iter = orderStatusMappingMap_.find(key);
	if(iter == orderStatusMappingMap_.end())
	{
		LOG_WARN << "invalid OrderStatus transition. currentOrderStatus:" << currentOrderStatus
			<< " newOrderStatus:" << newOrderStatus;
		return true;
	}
	return false;
}
const RtnOrderMap& OrderController::orderMap()
{
	return orderMap_;
}
void OrderController::initOrderStatusMappingMap()
{
	auto length = sizeof(OrderStatusMappingArray) / sizeof(OrderStatusMapping);
	for (size_t index = 0; index < length; ++index)
	{
		auto orderStatusMapping = OrderStatusMappingArray[index];
		auto key = make_pair(orderStatusMapping.currentStatus, orderStatusMapping.nextStatus);
		orderStatusMappingMap_[key] = orderStatusMapping.currentStatus;
	}
}

int OrderController::initOrderRef(int spotID, string tradingDay, int tradingSessionLength) //tradingSessionLength is in hours
{
	//Order reference has to be an unique incremental int throughout a particular trading session
	//trading session length could vary from 1 day (e.g. China domestic) to 7 days( e.g. CME)
	//this get init at at program start, after that Spot will simply + 1
	//To guranteed this number is unique and incremental through out a trading session, we take the difference
	//between the time now in seconds minus start of the trading session in seconds

	//this id is a type of int, and max int value = 2,147,483,647
	//this id number looks like this:
	//    [3 digits Spot ID] + [unique incremental int]
	//e.g.[214]				+ [7,483,647]

	//Get the ptime of when this trading session starts e.g. 7 days ago
	time_duration tradingSessionDurationHours(tradingSessionLength, 0, 0);	
	string tradingDayMidnightStr = tradingDay + "T000000";
	ptime tradingDayMidnight(from_iso_string(tradingDayMidnightStr));
	ptime tradingSessionResetTime = tradingDayMidnight - tradingSessionDurationHours;

	//get the ptime now
	ptime currTime = second_clock::local_time();

	//get the duration diff
	time_duration diff = currTime - tradingSessionResetTime;

	// [3 digits Spot ID] + [unique incremental int]
	//Spot ID is at the 8th Digit, get the duration diff in seconds which is a 6 digits number then multiply by 10 to get a 7 digits number
	//Assuming the trading session is 7 days, therefore 7 digits number will start with 6, this means: 
	
	//1. for a single spot the max number of order you can place for a given trading session is 9,999,999 minus 6,xxx,xxx, less than three million
	//2. this alogrithm will  break if spot is restarted after shutdown AND prior to shutdown a spot is placing on avg more than 10 order per second  
	//between the first start time and the restart time

	return spotID * 10000000 + diff.total_seconds() * 10;
}
void OrderController::updateTrade(Order &order, Order &cacheOrder)
{
	// �ж� fill vs.partial fill
	if (order.CounterType == InterfaceOKExV5
		|| order.CounterType == InterfaceBinance
		|| order.CounterType == InterfaceBybit
		)
	{
		if (order.OrderMsgType == RtnTradeType && IS_DOUBLE_GREATER(order.VolumeFilled, cacheOrder.VolumeFilled))
		{
			cacheOrder.Price = order.Price;
			cacheOrder.TradeID = order.TradeID;
			if (order.QryFlag == QryType::QRYORDER) {
				LOG_WARN << "updateTrade QryFlag: " << cacheOrder.OrderRef
					<< "cacheOrder.Volume: " << cacheOrder.Volume
					<< ", cacheOrder.VolumeTotalOriginal: " << cacheOrder.VolumeTotalOriginal
					<< ", cacheOrder.VolumeFilled: " << cacheOrder.VolumeFilled
					<< ", cacheOrder.VolumeRemained: " << cacheOrder.VolumeRemained

					<< ", order.VolumeTotalOriginal: " << order.VolumeTotalOriginal
					<< ", order.VolumeFilled: " << order.VolumeFilled
					<< ", order.VolumeRemained: " << order.VolumeRemained;

				cacheOrder.Volume = order.VolumeFilled - cacheOrder.Volume;
				cacheOrder.VolumeFilled = order.VolumeFilled;
				cacheOrder.VolumeRemained = cacheOrder.VolumeTotalOriginal - cacheOrder.VolumeFilled;
			} else {
				cacheOrder.Volume = order.Volume;
				cacheOrder.VolumeFilled += order.Volume;
				cacheOrder.VolumeRemained = cacheOrder.VolumeTotalOriginal - cacheOrder.VolumeFilled;
			}
			cacheOrder.Fee = order.Fee;
			memcpy(cacheOrder.UpdateTime, order.UpdateTime, sizeof(order.UpdateTime));
			
			//VolumeRemained = -0.00000100 okex spot market strange rounding issue result in our order size rounded e.g.  1.669449 => 1.66945
			//consider the order filled if volume remain is zero or falls between 0 and -0.000001(included)
			if (IS_DOUBLE_ZERO(cacheOrder.VolumeRemained)
			|| (IS_DOUBLE_GREATER_EQUAL(cacheOrder.VolumeRemained, -0.000001) && IS_DOUBLE_LESS(cacheOrder.VolumeRemained, 0.0)))
				order.OrderStatus = OrderStatus::Filled;
			else
				order.OrderStatus = OrderStatus::Partiallyfilled;
			
			LOG_INFO << "OrderRef:" << order.OrderRef << " OrderSysID:" << order.OrderSysID << " Volume:" << order.Volume << " OrderStatus:" << order.OrderStatus;
#ifdef __I_ShmMt_MD__
			if (ShmMtServer::instance().isInit()) {
				ShmMtServer::instance().update(&cacheOrder);
			}
#endif //__I_ShmMt_MD__
		}
	}
	else
	{
		LOG_FATAL << "Unknown CounterType: " << order.CounterType;
	}
}
void OrderController::updateCurrentTrade(Order &order, Order &cacheOrder)
{
	if (order.OrderMsgType == RtnTradeType)
	{
		order.StrategyID = cacheOrder.StrategyID;
		order.VolumeTotalOriginal = cacheOrder.VolumeTotalOriginal;		
		
//		SpreadInstrumentManager::updateTrade(order);
		memcpy(cacheOrder.UpdateTime, order.UpdateTime, sizeof(order.UpdateTime));
		LOG_INFO << "InstId:" << order.InstrumentID << " OrderRef:" << order.OrderRef << " OrderSysID:" << order.OrderSysID << " Volume:" << order.Volume << 
			" OrderStatus:" << order.OrderStatus << " spreadInst Status:" << cacheOrder.OrderStatus;
#ifdef __I_ShmMt_MD__
		if (ShmMtServer::instance().isInit())
		{
			ShmMtServer::instance().update(&order);
		}
#endif //__I_ShmMt_MD__
	}
}
void OrderController::updateOrder(const Order &order, Order &cacheOrder)
{
	//price,status, sysid
	//cacheOrder.OrderStatus = order.OrderStatus;

	// if (strlen(order.OrderSysID) >0)
	// 	memcpy(cacheOrder.OrderSysID, order.OrderSysID, sizeof(order.OrderSysID));

	cacheOrder.OrderMsgType = order.OrderMsgType;
	memcpy(cacheOrder.UpdateTime, order.UpdateTime, sizeof(order.UpdateTime));
	// memcpy(cacheOrder.CancelTime, order.CancelTime, sizeof(order.CancelTime));
	// memcpy(cacheOrder.InsertTime, order.InsertTime, sizeof(order.InsertTime));

	cacheOrder.OrdRejReason = order.OrdRejReason;
	memcpy(cacheOrder.StatusMsg, order.StatusMsg, StatusMsgLen);

}
bool OrderController::isCanceledOrderBeforePartiallyfilled(Order &rtnOrder, Order &cacheOrder)
{
	//����Ƿ���Ҫ����ı����ر�������
	// if (cacheOrder.OrderType == ORDERTYPE_FAK &&
	// 	rtnOrder.OrderStatus == OrderStatus::Cancelled &&
	// 	rtnOrder.VolumeFilled != cacheOrder.VolumeFilled)
	if (rtnOrder.OrderStatus == OrderStatus::Cancelled &&
		rtnOrder.VolumeFilled != cacheOrder.VolumeFilled)
	{
		LOG_WARN << "isCanceledOrderBeforePartiallyfilled OrderRef=" << rtnOrder.OrderRef
			<< "rtnOrder.VolumeFilled:" << rtnOrder.VolumeFilled << "cacheOrder.VolumeFilled:" << cacheOrder.VolumeFilled;
		canceledBeforePartiallyfilledOrderMap_[rtnOrder.OrderRef] = rtnOrder;
		return true;
	}
	return false;
}
bool OrderController::checkCancelBeforePartiallyfilledOrder(Order &cacheOrder)
{
	//����Ƿ���Ҫ�ѻ����Cancelled��������
	// if ((cacheOrder.OrderType == ORDERTYPE_FAK) &&
	// 	cacheOrder.OrderStatus == OrderStatus::Partiallyfilled)
	if (cacheOrder.OrderStatus == OrderStatus::Partiallyfilled)
	{
		auto iter = canceledBeforePartiallyfilledOrderMap_.find(cacheOrder.OrderRef);
		if (iter != canceledBeforePartiallyfilledOrderMap_.end())
		{
			if (iter->second.VolumeFilled == cacheOrder.VolumeFilled) {
				canceledBeforePartiallyfilledOrderMap_.erase(iter);
				LOG_INFO << "canceledBeforePartiallyfilledOrderMap VolumeFilled:" << iter->second.VolumeFilled
			         << " orderMap VolumeFilled:" << cacheOrder.VolumeFilled;
				return true;
			}
			else
				LOG_WARN << "canceledBeforePartiallyfilledOrderMap VolumeFilled:" << iter->second.VolumeFilled
				         << " orderMap VolumeFilled:" << cacheOrder.VolumeFilled;
		}
	}
	return false;
}