#include "spot/strategy/Strategy.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/TradingPeriod.h"
#include "spot/base/InitialData.h"
#include "spot/strategy/StrategyInstrumentManager.h"
#include "spot/strategy/Initializer.h"
#include "spot/strategy/DealOrders.h"

using namespace spot;
using namespace spot::base;
using namespace spot::strategy;
using namespace std;

Strategy::Strategy(int strategyID, StrategyParameter *params)
	:strategyID_(strategyID),parameters_(params)
{	
	isTradeable_ = false;

	time_point<system_clock> tpNow = system_clock::now();
	auto ttNow = system_clock::to_time_t(tpNow);
	lastPtm_ = localtime(&ttNow);

	okexOrdersPer2kMsec_ = *parameters()->getOkxOrdersPer2Sec();
	okexOrder2kMsecStep_ = *parameters()->getOkxOrderMsecStep();

	bianOrdersPerMin_ = *parameters()->getBianOrdersPerMin();
	bianOrdersPerTenSec_ = *parameters()->getBianOrdersPerTenSec();
	bianOrderSecStep_ = *parameters()->getBianOrderSecStep();

	okCancelLimitVec_.push_back(UINT64_MAX);
	okCancelLimitVec_.push_back(UINT64_MAX);
	okCancelLimitVec_.push_back(0);

	okReqLimitVec_.push_back(UINT64_MAX);
	okReqLimitVec_.push_back(UINT64_MAX);
	okReqLimitVec_.push_back(0);

	bianOrderOneMinSum_ = 0;
	lastSecIndex_ = 0;
	lastBaMin_ = 0;

	memset(&bian10SecOrders_, 0, sizeof(uint32_t) * 6);
	LOG_INFO << "Strategy params okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: "
		<< okexOrder2kMsecStep_ << ", bianOrdersPerMin_: " << bianOrdersPerMin_ << ", bianOrdersPerTenSec_: "
		<< bianOrdersPerTenSec_ << ", bianOrderSecStep_: " << bianOrderSecStep_;

}
Strategy::~Strategy()
{
	for (auto iter : strategyInstrumentList_)
	{
		delete iter;
	}
}

void Strategy::getCurrentPtm(tm& ptm) {
    time_point<system_clock> tpNow = system_clock::now();
    auto ttNow = system_clock::to_time_t(tpNow);
    ptm = *localtime(&ttNow);
}

bool Strategy::JudgeOkReqCountbyDuration2s(tm* ptm, int orderLimitCount, int okTimeStep, int reqSize)
{
	uint64_t currentEpochMs = CURR_MSTIME_POINT;
	static int countSum = 0;
	if (currentEpochMs < okReqLimitVec_[0]) {
		countSum++;
		okReqLimitVec_[0] = currentEpochMs;
		okReqLimitVec_[1] = currentEpochMs + 2000;
		if (countSum > 1) {
			LOG_FATAL << "JudgeOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
				okReqLimitVec_[2] << ", reqSize: " << reqSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;
		}
	}
	if (currentEpochMs >= okReqLimitVec_[0] && currentEpochMs < okReqLimitVec_[1]) {
		if (okReqLimitVec_[2] + reqSize <= orderLimitCount) {
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "JudgeOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;

		return false;
	} else if (currentEpochMs >= okReqLimitVec_[1] && (currentEpochMs - okReqLimitVec_[1] < okTimeStep)) {
		okReqLimitVec_[2] = 0;
		if (okReqLimitVec_[2] + reqSize <= orderLimitCount) {
			okReqLimitVec_[0] = okReqLimitVec_[1];
			okReqLimitVec_[1] = okReqLimitVec_[0] + okTimeStep;
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "JudgeOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;

		return false;
	} else if (currentEpochMs >= okReqLimitVec_[1] + okTimeStep) {
		okReqLimitVec_[2] = 0;
		if (okReqLimitVec_[2] + reqSize <= orderLimitCount) {
			okReqLimitVec_[0] = currentEpochMs;
			okReqLimitVec_[1] = okReqLimitVec_[0] + okTimeStep;
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "JudgeOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;

		return false;
	}
	LOG_FATAL << "JudgeOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
		okReqLimitVec_[2] << ", reqSize: " << reqSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep << ", countSum: " << countSum;
	return false;
}

bool Strategy::JudgeOkCancelCountbyDuration2s(tm* ptm, int orderLimitCount, int okTimeStep, uint32_t& cancelSize)
{
	if (cancelSize == 0) return true;
	uint64_t currentEpochMs = CURR_MSTIME_POINT;
	static int countSum = 0;

	if (currentEpochMs < okCancelLimitVec_[0]) {
		countSum++;
		okCancelLimitVec_[0] = currentEpochMs;
		okCancelLimitVec_[1] = currentEpochMs + 2000;
		if (countSum > 1) {
			LOG_FATAL << "JudgeOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;
		}

	} else if (currentEpochMs >= okCancelLimitVec_[0] && currentEpochMs < okCancelLimitVec_[1]) {
		if (okCancelLimitVec_[2] + cancelSize <= orderLimitCount) {
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			cancelSize = orderLimitCount - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = orderLimitCount;
			LOG_WARN << "JudgeOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;
		}
	} else if (currentEpochMs >= okCancelLimitVec_[1] && (currentEpochMs - okCancelLimitVec_[1] < okTimeStep)) {
		okCancelLimitVec_[2] = 0;
		if (okCancelLimitVec_[2] + cancelSize <= orderLimitCount) {
			okCancelLimitVec_[0] = okCancelLimitVec_[1];
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okTimeStep;
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			okCancelLimitVec_[0] = okCancelLimitVec_[1];
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okTimeStep;
			cancelSize = orderLimitCount - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = orderLimitCount;
			LOG_WARN << "JudgeOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;

		}
	} else if (currentEpochMs >= okCancelLimitVec_[1] + okTimeStep) {
		okCancelLimitVec_[2] = 0;
		if (okCancelLimitVec_[2] + cancelSize <= orderLimitCount) {
			okCancelLimitVec_[0] = currentEpochMs;
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okTimeStep;
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			okCancelLimitVec_[0] = currentEpochMs;
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okTimeStep;
			cancelSize = orderLimitCount - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = orderLimitCount;
				LOG_WARN << "JudgeOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
			okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep;

		}
	}
	LOG_INFO << "JudgeOkCancelCountbyDuration2s first okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
		okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", orderLimitCount: " << orderLimitCount << ", okTimeStep: " << okTimeStep<< ", countSum: " << countSum;

	return true;
}
/*
bool Strategy::JudgeBianOrdersCountbyDurationTenSec(tm* ptm, int orderLimitCount, int bianTimeStep, int secondCount)
{
	int countSum = 0;
	if (secondCount >= bianTimeStep - 1) {
		int i = 0;
		while (i < bianTimeStep) {
			countSum = countSum + (*bianOrderTenSecVec_)[secondCount - i];
			i++;
		}
	} else {
		int i = secondCount;
		while (i >= 0) {
			countSum = countSum + (*bianOrderTenSecVec_)[i];
			i--;
		}
		int j = bianTimeStep - (secondCount + 1);
		int k = 0;
		while (k < j) {
			countSum = countSum + (*bianOrderTenSecVec_)[86399 - k];
			k++;
		}
	}
	if (secondCount < bianTimeStep) {
		memset(bianOrderTenSecVec_ + secondCount + 1, 0, (sizeof(int) * (86400 - bianTimeStep)));
	} else {
		memset(bianOrderTenSecVec_, 0, (sizeof(int) * (secondCount - bianTimeStep + 1)));
		memset(bianOrderTenSecVec_ + secondCount + 1, 0, (sizeof(int) * (86400 - secondCount - 1)));
	}
	if (countSum <= orderLimitCount) {
		LOG_INFO << "JudgeBianOrdersCountbyDurationTenSec countSum: " << countSum << ", orderLimitCount: " << ", bianTimeStep: " <<
			bianTimeStep << ", secondCount: " << secondCount;
		return true;
	}
	LOG_INFO << "JudgeBianOrdersCountbyDurationTenSec countSum: " << countSum << ", orderLimitCount: " << ", bianTimeStep: " <<
		bianTimeStep << ", secondCount: " << secondCount;
	return false;
}*/

bool Strategy::JudgeBianOrdersCountbyDurationTenSec(int orderLimitCount, int bianTimeStep, int secondCount, uint32_t& orderNum)
{
	uint32_t secCnt = secondCount % 60;
	uint32_t secIndex = secCnt / bianTimeStep; // bianTimeStep : 10s
	// LOG_INFO << "JudgeBianOrdersCountbyDurationTenSec secIndex: " << secIndex;
	// cout << "JudgeBianOrdersCountbyDurationTenSec secIndex: " << secIndex;

	if (secIndex == lastSecIndex_) {
		if (bian10SecOrders_[secIndex] + orderNum <= orderLimitCount) {
			bian10SecOrders_[secIndex] = bian10SecOrders_[secIndex] + orderNum;
		} else {
			orderNum = orderLimitCount - bian10SecOrders_[secIndex];
			bian10SecOrders_[secIndex] = orderLimitCount;
		}
	} else {
		memset(&bian10SecOrders_, 0, sizeof(uint32_t) * 6);
		lastSecIndex_ = secIndex;
		if (orderNum <= orderLimitCount) {
			bian10SecOrders_[secIndex] = orderNum;
		} else {
			orderNum = orderLimitCount;
			bian10SecOrders_[secIndex] = orderLimitCount;
		}
	}
	LOG_INFO << "JudgeBianOrdersCountbyDurationTenSec secIndex : " << secIndex << ", lastSecIndex_: " << lastSecIndex_ << ", bian10SecOrders_[secIndex]: " 
		<< bian10SecOrders_[secIndex] << ", bian10SecOrders_[lastSecIndex_]: " << bian10SecOrders_[lastSecIndex_]
		<< ", orderLimitCount: " << ", bianTimeStep: " << bianTimeStep
		<< ", secondCount: " << secondCount << ", orderNum: " << orderNum;
	return true;
}

bool Strategy::JudgeBianOrdersCountbyDurationOneMin(tm* ptm, int orderLimitCount, uint32_t& orderNum)
{
	uint32_t currMin = ptm->tm_hour * 60 + ptm->tm_min;
	if (currMin != lastBaMin_) {
		lastBaMin_ = currMin;
		bianOrderOneMinSum_ = 0;
		if (bianOrderOneMinSum_ + orderNum <= orderLimitCount) {
			bianOrderOneMinSum_ = bianOrderOneMinSum_ + orderNum;
		} else {
			orderNum = orderLimitCount - bianOrderOneMinSum_;
			bianOrderOneMinSum_ = orderLimitCount;
		}
	} else {
		if (bianOrderOneMinSum_ + orderNum <= orderLimitCount) {
			bianOrderOneMinSum_ = bianOrderOneMinSum_ + orderNum;
		} else {
			orderNum = orderLimitCount - bianOrderOneMinSum_;
			bianOrderOneMinSum_ = orderLimitCount;
		}
	}

	LOG_INFO << "JudgeBianOrdersCountbyDurationOneMin lastBaMin_: " << lastBaMin_ << ", bianOrderOneMinSum_: "<< bianOrderOneMinSum_ << ", orderNum: " <<
		orderNum << ", orderLimitCount: " << orderLimitCount;
	return true;
}

bool Strategy::setOrder(StrategyInstrument *strategyInstrument, char direction, double price, double volume, SetOrderOptions setOrderOptions, uint32_t cancelSize)
{
	strategyInstrument->setOrder(direction, price, volume, setOrderOptions, cancelSize);
	return true;
}

int Strategy::getStrategyID()
{
	return strategyID_;
}

StrategyParameter* Strategy::parameters()
{
	return parameters_;
}

const list<StrategyInstrument*>& Strategy::strategyInstrumentList()
{
	return strategyInstrumentList_;
}
void Strategy::addStrategyInstrument(const char* instrumentID,Adapter *adapter)
{
	char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyInstrument));
	StrategyInstrument *strategyInstrument = new (buff) StrategyInstrument(this, string(instrumentID), adapter);
	strategyInstrumentList_.push_back(strategyInstrument);
	//there can be only one "strategyInstrument" for a given key as "Instrument"

	auto instrument = InstrumentManager::getInstrument(instrumentID);
	instrumentStrategyInstrumentMap_[instrument] = strategyInstrument;

	LOG_INFO << "Add to instrumentStrategyInstrumentMap_: instrumentID = " << instrumentID << " strategyID = " << strategyID_;
	
	StrategyInstrumentManager::addStrategyInstrument(instrument, strategyInstrument);
}

void Strategy::OnRtnOrder(const Order &order)
{
	auto instrument = InstrumentManager::getInstrument(order.InstrumentID);
	
	auto iter = instrumentStrategyInstrumentMap_.find(instrument);

	//iter->second is a strategyinstrument
	if (iter != instrumentStrategyInstrumentMap_.end())
	{
		iter->second->OnRtnOrder(order);
		actionOrder(order, iter->second);
	}
	else
	{
		//this should never happen
		LOG_FATAL << "Nullptr found on strategyInstrument - instrumentID = " << instrument->getInstrumentID();
	}

}

void Strategy::actionOrder(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
	switch (rtnOrder.OrderStatus)
	{
	case OrderStatus::NewRejected:
		OnNewReject(rtnOrder, strategyInstrument);
		break;
	case OrderStatus::CancelRejected:
		OnCancelReject(rtnOrder, strategyInstrument);
		break;
	case OrderStatus::New:
		OnNewOrder(rtnOrder, strategyInstrument);
		break;
	case OrderStatus::Partiallyfilled:
		OnPartiallyFilled(rtnOrder, strategyInstrument);
		break;
	case OrderStatus::Filled:
		OnFilled(rtnOrder, strategyInstrument);
		break;
	case OrderStatus::Cancelled:
		OnCanceled(rtnOrder, strategyInstrument);
		break;
	default:
	  {
		string buffer = "actionOrder.invalid status:";
		buffer = buffer + rtnOrder.OrderStatus;
			LOG_ERROR << buffer.c_str();
			break;
	  }
	}
}

bool Strategy::hasUnfinishedOrder()
{
	for (auto strategyInstrument : strategyInstrumentList_)
	{
		if (strategyInstrument->hasUnfinishedOrder())
			return true;
	}
	return false;
}
StrategyInstrument* Strategy::getStrategyInstrument(Instrument* instrument)
{
	auto it = instrumentStrategyInstrumentMap_.find(instrument);
	if (it != instrumentStrategyInstrumentMap_.end()) {
		return it->second;
	}
	return nullptr;
}
