#include <functional>
#include <spot/gateway/HuobTraderSpi.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Compatible.h>
#include <spot/huobapi/HuobApi.h>
#include <spot/huobapi/HuobStruct.h>
#include <spot/gateway/OrderStoreCallBack.h>
#include "spot/base/InitialData.h"
#include "spot/base/InstrumentManager.h"

using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spotrapidjson;
HuobTraderSpi::HuobTraderSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore)
{
	queryIndex_ = 0;
	apiInfo_ = apiInfo;
	orderStore_ = orderStore;
	traderApi_ = new HuobApi(apiInfo->userId_,apiInfo->passwd_,apiInfo->investorId_);
}
HuobTraderSpi::~HuobTraderSpi()
{
	delete traderApi_;
}

void HuobTraderSpi::SetIsCryptoFuture(bool isCryptoFuture)
{
	isCryptoFuture_ = isCryptoFuture;
}

void HuobTraderSpi::Init()
{
	if (isCryptoFuture_)
	{
		setTradeLoginStatusMap(InterfaceHuobFuture, true);
	}
	else
	{
		setTradeLoginStatusMap(InterfaceHuob, true);
	}
	std::thread apiThread(&HuobTraderSpi::Run, this);
	apiThread.detach();
}

void HuobTraderSpi::Run()
{
	while (true)
	{
		SLEEP(InitialData::getOrderQueryIntervalMili());
		queryIndex_++;
		//保证一次正常order，一次异常order的查询
		if (!orderMap2_.empty() && (queryIndex_ % 2 == 0))
		{
			queryOrderStatus(false); //根据OrderRef查询
		}
		else
		{
			queryOrderStatus(true); //根据OrderSysId查询
		}
	}
}

void HuobTraderSpi::queryOrderStatus(bool isByOrderId)
{
	MutexLockGuard lock(mutex_);
	map<uint64_t, Order>* orderMap = nullptr;
	set<uint64_t>* finishedOrders = nullptr;

	if (isByOrderId) //OrderSysId
	{
		orderMap = &orderMap_;
		finishedOrders = &finishedOrders_;
	}
	else //OrderRef
	{
		orderMap = &orderMap2_;
		finishedOrders = &finishedOrders2_;
	}

	if (isCryptoFuture_)
	{
		//期货使用批量查询
		UpdateUserOrderMap(*orderMap, isByOrderId);
	}
	else
	{
		//现货只能逐个查询
		auto iter = orderMap->begin();
		for (; iter != orderMap->end(); ++iter)
		{
			LOG_INFO << "orderMap_ Field: " << iter->second.VolumeFilled;
			UpdateUserOrder(iter->first, iter->second, isByOrderId);
		}
	}

	for (auto id : *finishedOrders)
	{
		orderMap->erase(id);
	}
	finishedOrders->clear();
}

void HuobTraderSpi::UpdateUserOrderMap(map<uint64_t, Order>& orderMap, bool isByOrderId)
{
	map<std::string, map<uint64_t, Order*>> orderMapBySymbol;
	convert2OrderMapBySymbol(orderMap, orderMapBySymbol);

	auto iter = orderMapBySymbol.begin();
	for (; iter != orderMapBySymbol.end(); iter++)
	{
		string symbol = (*iter).first;
		map<uint64_t, Order*> orderMap2 = (*iter).second;
		updateUserOrderMapBySymbol(symbol, orderMap2, isByOrderId);
	}
}

void HuobTraderSpi::UpdateUserOrder(uint64_t order_id, Order& preOrder, bool isByOrderId)
{
	string orderID = to_string(order_id);
	string instrumentID(preOrder.InstrumentID);						//btc_usd   ltc_usd    eth_usd    etc_usd    bch_usd
	auto instrument = InstrumentManager::getInstrument(instrumentID);

	HuobOrder queryOrder;
	if (instrument->isFuture())
	{
		traderApi_->GetFutureOrder(orderID, queryOrder, isByOrderId);
	}
	else  if (instrument->isCrypto())
	{
		traderApi_->GetOrder(orderID, queryOrder);
	}

	LOG_INFO << "orderMap_ Field: " << preOrder.VolumeFilled;

	PutOrderToQueue(queryOrder, preOrder, isByOrderId);
	return;
}

void HuobTraderSpi::PutOrderToQueue(HuobOrder &queryOrder, Order &preOrder, bool isByOrderId)
{
	LOG_INFO << "recv order record. Huob Order:" << queryOrder.to_string();
	auto rspOrder = new Order();
	memcpy(rspOrder, &preOrder, sizeof(Order));
	rspOrder->EpochTimeReturn = CURR_USTIME_POINT;
	rspOrder->Price = queryOrder.price;
	rspOrder->Fee = queryOrder.fees;

	double nowTradeVolume = queryOrder.tradeAmount;
	double preTradeVolume = preOrder.VolumeFilled;

	if (queryOrder.orderStatus == huob_reject)
	{
		LOG_INFO << "Unknown status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
		//string instrumentID(queryOrder.instrument_id);	//btc_usd ltc_usd eth_usd etc_usd bch_usd
		//auto instrument = InstrumentManager::getInstrument(instrumentID);
		//
		//rspOrder->OrderStatus = NewRejected;
		//rspOrder->OrderMsgType = RtnOrderType;

		//preOrder.OrderStatus = NewRejected;
		//finishedOrders_.insert(queryOrder.orderID);
		//LOG_INFO << "NewRejected status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;



		//orderStore_->storeHandle(rspOrder);
	}
	else if (queryOrder.orderStatus == huob_filled)
	{
		if (nowTradeVolume > preTradeVolume) //计算成交变化量
		{
			double gapVolume = nowTradeVolume - preTradeVolume;
			rspOrder->Volume =gapVolume;
			rspOrder->OrderStatus = Filled;
			rspOrder->OrderMsgType = RtnTradeType;

			preOrder.VolumeFilled = nowTradeVolume;
			preOrder.OrderStatus = Filled;

			orderStore_->storeHandle(rspOrder);
			LOG_INFO << "Fill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
		}
		LOG_INFO << "Filled, PutOrderToQueue storeHandle order, OrderID:" << queryOrder.orderID;
		if (isByOrderId)
		{
			finishedOrders_.insert(queryOrder.orderID);
		}
		else
		{
			finishedOrders2_.insert(queryOrder.clientOrderID);
		}
	}
	else if (queryOrder.orderStatus == huob_partial_filled || queryOrder.orderStatus == huob_canceled || queryOrder.orderStatus == huob_partial_canceled)
	{
		if (nowTradeVolume > preTradeVolume)
		{
			int gapVolume = nowTradeVolume - preTradeVolume;

			rspOrder->OrderStatus = Partiallyfilled;
			rspOrder->Volume = gapVolume;
			rspOrder->OrderMsgType = RtnTradeType;

			preOrder.VolumeFilled = nowTradeVolume;
			preOrder.OrderStatus = Partiallyfilled;

			LOG_INFO << "Partiallyfilled, PutOrderToQueue storeHandle order, OrderID:" << queryOrder.orderID;
			orderStore_->storeHandle(rspOrder);
			LOG_INFO << "PartifiallyFill status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
		}
		if (queryOrder.orderStatus == huob_canceled || queryOrder.orderStatus == huob_partial_canceled)
		{
			rspOrder->OrderStatus = Cancelled;
			rspOrder->OrderMsgType = RtnOrderType;

			preOrder.VolumeFilled = nowTradeVolume;
			preOrder.OrderStatus = Cancelled;

			LOG_INFO << "Cancelled status. " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " VolumeTraded " << queryOrder.tradeAmount << " preTradeVolume " << preTradeVolume;
			LOG_INFO << "Cancelled, PutOrderToQueue storeHandle order, OrderID:" << queryOrder.orderID;
			orderStore_->storeHandle(rspOrder);
			if (isByOrderId)
			{
				finishedOrders_.insert(queryOrder.orderID);
			}
			else
			{
				finishedOrders2_.insert(queryOrder.clientOrderID);
			}
		}
	}
	else
	{
		LOG_DEBUG << "status " << "OrderSysID " << queryOrder.orderID << " OrderRef " << preOrder.OrderRef << " OrderStatus " << rspOrder->OrderStatus << " old Status " << preOrder.OrderStatus;
	}
}

int HuobTraderSpi::testOrderInsert(Order &innerOrder)
{
	memset(&innerOrder, 0, sizeof(innerOrder));
	memcpy(innerOrder.InstrumentID, "ETH0914_l_hb",sizeof(innerOrder.InstrumentID));
	memcpy(innerOrder.InvestorID, "10758476", sizeof(innerOrder.InvestorID));
	//memcpy(innerOrder.InstrumentID, "btc_usdt_hb", sizeof(innerOrder.InstrumentID));
	innerOrder.Direction = INNER_DIRECTION_Buy;
	//innerOrder.Direction = INNER_DIRECTION_Sell;
	innerOrder.LimitPrice = 228;
	innerOrder.VolumeTotalOriginal = 1;
	innerOrder.Price = 228;
	//memcpy(innerOrder.OrderSysID ,"154161",sizeof(innerOrder.OrderSysID));
	return 0;
}

int HuobTraderSpi::ReqOrderInsert(const Order &innerOrder)
{
	MutexLockGuard lock(mutex_);
	string instrumentID(innerOrder.InstrumentID);	//btc_usd ltc_usd eth_usd etc_usd bch_usd
	auto instrument = InstrumentManager::getInstrument(instrumentID);
	string result;

	HuobOrder reqOrder;
	if (innerOrder.Direction == INNER_DIRECTION_Buy)
	{
		reqOrder.type = huob_buy_limit;
	}
	else if (innerOrder.Direction == INNER_DIRECTION_Sell)
	{
		reqOrder.type = huob_sell_limit;
	}


	strcpy(reqOrder.instrument_id, innerOrder.InstrumentID);
	reqOrder.amount = innerOrder.VolumeTotalOriginal;
	reqOrder.price = innerOrder.LimitPrice;
	strcpy(reqOrder.account_id, innerOrder.InvestorID);

	uint64_t id = 0;
	//=====================================
	//reqOrder.offset = 1;
	//id = traderApi_->ReqFutureOrderInsert(reqOrder);
	//if (id != 0)
	//{
	//	string order_id = to_string(id);
	//	int ret = traderApi_->CancelFutureOrder(order_id);
	//	HuobOrder hbOrder;
	//	traderApi_->GetFutureOrder(order_id, hbOrder);
	//}
	//return 0;
	//=====================================
	if (instrument->isFuture())
	{
		reqOrder.offset = getFutureOrderOffsetType(innerOrder.Direction, innerOrder.Offset, instrument);
	}
	
	if (instrument->isFuture())
	{
		id = traderApi_->ReqFutureOrderInsert(reqOrder);
	}
	else if (instrument->isCrypto())
	{
		id = traderApi_->ReqOrderInsert(reqOrder);
	}

	if (id != 0)
	{
		Order *rspOrder = new Order();
		memcpy(rspOrder, &innerOrder, sizeof(Order));
		rspOrder->EpochTimeReturn = CURR_USTIME_POINT;
		ostringstream sysid;
		sysid << id;
		strcpy(rspOrder->OrderSysID, sysid.str().c_str());
		rspOrder->OrderMsgType = RtnOrderType;
		rspOrder->OrderStatus = OrderStatus::New;
		//rspOrder->CounterSysID = traderApi_->GetCurrentNonce();
		LOG_INFO << "ReqOrderInsert storeHandle order, OrderID:"<< rspOrder->OrderSysID;
		orderStore_->storeHandle(rspOrder);
		orderMap_[id] = *rspOrder;
	}
	else
	{
		Order *rspOrder = new Order();
		memcpy(rspOrder, &innerOrder, sizeof(Order));
		rspOrder->EpochTimeReturn = CURR_USTIME_POINT;
		rspOrder->OrderMsgType = RtnOrderType;
		rspOrder->OrderRef = innerOrder.OrderRef;
		rspOrder->OrderStatus = OrderStatus::PendingNew;
		LOG_WARN << "unknown status, OrderRef:" << rspOrder->OrderRef;
		orderMap2_[innerOrder.OrderRef] = *rspOrder;
	}
	return 0;
}
int HuobTraderSpi::ReqOrderCancel(const Order &innerOrder)
{
	MutexLockGuard lock(mutex_);
	string instrumentID(innerOrder.InstrumentID);	//btc_usd   ltc_usd    eth_usd    etc_usd    bch_usd
	auto instrument = InstrumentManager::getInstrument(instrumentID);
	bool ret;

	string order_id(innerOrder.OrderSysID);
	string symbol = instrumentID.substr(0, 3);

	//========================
	/*ret = traderApi_->CancelFutureOrder(order_id);
	return 0;*/
	//==========================
	if (instrument->isFuture())
	{
		ret = traderApi_->CancelFutureOrder(order_id, symbol);
	}
	else if (instrument->isCrypto())
	{
		ret = traderApi_->CancelOrder(order_id);
	}
	Order *rspOrder = new Order();
	memcpy(rspOrder, &innerOrder, sizeof(Order));
	rspOrder->OrderMsgType = RtnOrderType;
	if (!ret)
	{
		rspOrder->OrderStatus = OrderStatus::CancelRejected;
		rspOrder->EpochTimeReturn = CURR_USTIME_POINT;
		orderStore_->storeHandle(rspOrder);
	}

	return 0;
}

int HuobTraderSpi::getFutureOrderOffsetType(char direction, char offset, Instrument *instrument)
{
	int type; // 1:开多   2:开空   3:平多   4:平空

	if (direction == INNER_DIRECTION_Buy && offset == INNER_OFFSET_Open)
	{
		type = 1;
	}
	else if (direction == INNER_DIRECTION_Sell && offset = INNER_OFFSET_Open)
	{
		type = 2;
	}

	else if (direction == INNER_DIRECTION_Buy && offset = INNER_OFFSET_CloseToday)
	{
		type = 3;
	}
	else if (direction == INNER_DIRECTION_Sell && offset = INNER_OFFSET_CloseToday)
	{
		type = 4;
	}

	return type;
}
string HuobTraderSpi::getFutureConstractCode(Instrument *inst)
{
	string inststr = inst->getInstrumentID();
	string symbol = inststr.substr(0, inststr.find_first_of('_'));

	return symbol;
}
string HuobTraderSpi::getSpotConstractCode(Instrument *inst)
{
	string inststr = inst->getInstrumentID();
	string symbol = inststr.substr(0, inststr.find_last_of('_'));
	symbol.erase(std::remove(symbol.begin(), symbol.end(), '_'), symbol.end());
	
	return symbol;
}

void HuobTraderSpi::convert2OrderMapBySymbol(map<uint64_t, Order>& orderMap, map<std::string, map<uint64_t, Order*>>& queryOrderMap)
{
	auto iter = orderMap.begin();
	for (; iter != orderMap.end(); iter++)
	{
		uint64_t order_id = (*iter).first;
		Order& order = (*iter).second;

		string instrumentID(order.InstrumentID);
		string symbol = instrumentID.substr(0, 3);

		queryOrderMap[symbol][order_id] = &order;
	}
}

void HuobTraderSpi::updateUserOrderMapBySymbol(const std::string& symbol, map<uint64_t, Order*>& orderMap, bool isByOrderId)
{
	int orderSize = orderMap.size();

	int orderIndex = 0;
	list<string> orderIDList;
	list<HuobOrder> resultOrderList;

	auto iter = orderMap.begin();
	for (int i = 0; i < orderSize; i++)
	{
		Order* order = iter->second;
		string orderID;
		if (isByOrderId)
		{
			orderID = order->OrderSysID;
		}
		else
		{
			ostringstream id;
			id << order->OrderRef;
			orderID = id.str();
		}
		orderIDList.push_back(orderID);
		iter++;  orderIndex++;
		if (iter == orderMap.end() || orderIndex == 20)
		{
			traderApi_->GetFutureOrderList(symbol, orderIDList, resultOrderList, isByOrderId);
			for (auto queryResultOrder : resultOrderList)
			{
				if (isByOrderId)
				{
					auto preOrderIter = orderMap.find(queryResultOrder.orderID);
					if (preOrderIter != orderMap.end())
					{
						Order* preOrder = preOrderIter->second;
						PutOrderToQueue(queryResultOrder, *preOrder, isByOrderId);
					}
				}
				else
				{
					auto preOrderIter = orderMap.find(queryResultOrder.clientOrderID);
					if (preOrderIter != orderMap.end())
					{
						Order* preOrder = preOrderIter->second;
						ostringstream sysid;
						sysid << queryResultOrder.orderID;
						strcpy(preOrder->OrderSysID, sysid.str().c_str());//把查询到的order sys id赋值上去
						PutOrderToQueue(queryResultOrder, *preOrder, isByOrderId);
					}
				}
			}

			orderIDList.clear();
			resultOrderList.clear();
			orderIndex = 0;
			if (iter == orderMap.end())
			{
				break;
			}
		}
	}
	return;
}
