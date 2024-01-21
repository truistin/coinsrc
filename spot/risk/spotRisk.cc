#include <math.h>
#include <boost/math/distributions/normal.hpp>
#include "spot/utility/Logging.h"
#include "spot/base/StrategyParameter.h"

#include "spotRisk.h"

using namespace spot::risk;
using namespace spot::utility;
using namespace boost::math;
using namespace spot::strategy;

spotRisk::spotRisk(Strategy* s)
{
	st_ = s;
	StrategyParameter* param = st_->parameters();
	okexOrdersPer2kMsec_ = *param->getOkxOrdersPer2Sec();
	okexOrder2kMsecStep_ = *param->getOkxOrderMsecStep();

	bianOrdersPerMin_ = *param->getBianOrdersPerMin();
	bianOrdersPerTenSec_ = *param->getBianOrdersPerTenSec();
	bianOrderSecStep_ = *param->getBianOrderSecStep();

	byReqOrdPers_ = *param->getByReqOrdPers();
	byCanOrdPers_ = *param->getByCanOrdPers();

	okCancelLimitVec_.push_back(UINT64_MAX);
	okCancelLimitVec_.push_back(UINT64_MAX);
	okCancelLimitVec_.push_back(0);

	okReqLimitVec_.push_back(UINT64_MAX);
	okReqLimitVec_.push_back(UINT64_MAX);
	okReqLimitVec_.push_back(0);

	bianOrderOneMinSum_ = 0;
	lastSecIndex_ = 0;
	lastBaMin_ = 0;

	accReqOrdPers_ = 0;
	accCanOrdPers_ = 0;

	lastReqOrdTime_ = CURR_MSTIME_POINT;

	memset(&bian10SecOrders_, 0, sizeof(uint32_t) * 6);
	LOG_INFO << "Strategy params okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: "
		<< okexOrder2kMsecStep_ << ", bianOrdersPerMin_: " << bianOrdersPerMin_ << ", bianOrdersPerTenSec_: "
		<< bianOrdersPerTenSec_ << ", bianOrderSecStep_: " << bianOrderSecStep_;
}

spotRisk::~spotRisk()
{

}

void spotRisk::getCurrentPtm(tm& ptm) {
    time_point<system_clock> tpNow = system_clock::now();
    auto ttNow = system_clock::to_time_t(tpNow);
    ptm = *localtime(&ttNow);
}

bool spotRisk::VaildOkReqCountbyDuration2s(uint32_t& reqSize)
{
	uint64_t currentEpochMs = CURR_MSTIME_POINT;
	static int countSum = 0;
	if (currentEpochMs < okReqLimitVec_[0]) {
		countSum++;
		okReqLimitVec_[0] = currentEpochMs;
		okReqLimitVec_[1] = currentEpochMs + 2000;
		if (countSum > 1) {
			LOG_FATAL << "VaildOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
				okReqLimitVec_[2] << ", reqSize: " << reqSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;
		}
	}
	if (currentEpochMs >= okReqLimitVec_[0] && currentEpochMs < okReqLimitVec_[1]) {
		if (okReqLimitVec_[2] + reqSize <= okexOrdersPer2kMsec_) {
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "VaildOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;

		return false;
	} else if (currentEpochMs >= okReqLimitVec_[1] && (currentEpochMs - okReqLimitVec_[1] < okexOrder2kMsecStep_)) {
		okReqLimitVec_[2] = 0;
		if (okReqLimitVec_[2] + reqSize <= okexOrdersPer2kMsec_) {
			okReqLimitVec_[0] = okReqLimitVec_[1];
			okReqLimitVec_[1] = okReqLimitVec_[0] + okexOrder2kMsecStep_;
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "VaildOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;

		return false;
	} else if (currentEpochMs >= okReqLimitVec_[1] + okexOrder2kMsecStep_) {
		okReqLimitVec_[2] = 0;
		if (okReqLimitVec_[2] + reqSize <= okexOrdersPer2kMsec_) {
			okReqLimitVec_[0] = currentEpochMs;
			okReqLimitVec_[1] = okReqLimitVec_[0] + okexOrder2kMsecStep_;
			okReqLimitVec_[2] = okReqLimitVec_[2] + reqSize;
			return true;
		}
		LOG_INFO << "VaildOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
			okReqLimitVec_[2] << ", reqSize: " << reqSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;

		return false;
	}
	LOG_FATAL << "VaildOkReqCountbyDuration2s okReqLimitVec_[0]: " << okReqLimitVec_[0] << ", okReqLimitVec_[1]: " << okReqLimitVec_[1] << ", okReqLimitVec_[2]: " <<
		okReqLimitVec_[2] << ", reqSize: " << reqSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_ << ", countSum: " << countSum;
	return false;
}

bool spotRisk::VaildOkCancelCountbyDuration2s(uint32_t& cancelSize)
{
	if (cancelSize == 0) return true;
	uint64_t currentEpochMs = CURR_MSTIME_POINT;
	static int countSum = 0;

	if (currentEpochMs < okCancelLimitVec_[0]) {
		countSum++;
		okCancelLimitVec_[0] = currentEpochMs;
		okCancelLimitVec_[1] = currentEpochMs + 2000;
		if (countSum > 1) {
			LOG_FATAL << "VaildOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;
		}

	} else if (currentEpochMs >= okCancelLimitVec_[0] && currentEpochMs < okCancelLimitVec_[1]) {
		if (okCancelLimitVec_[2] + cancelSize <= okexOrdersPer2kMsec_) {
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			cancelSize = okexOrdersPer2kMsec_ - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = okexOrdersPer2kMsec_;
			LOG_WARN << "VaildOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;
		}
	} else if (currentEpochMs >= okCancelLimitVec_[1] && (currentEpochMs - okCancelLimitVec_[1] < okexOrder2kMsecStep_)) {
		okCancelLimitVec_[2] = 0;
		if (okCancelLimitVec_[2] + cancelSize <= okexOrdersPer2kMsec_) {
			okCancelLimitVec_[0] = okCancelLimitVec_[1];
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okexOrder2kMsecStep_;
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			okCancelLimitVec_[0] = okCancelLimitVec_[1];
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okexOrder2kMsecStep_;
			cancelSize = okexOrdersPer2kMsec_ - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = okexOrdersPer2kMsec_;
			LOG_WARN << "VaildOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
				okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;

		}
	} else if (currentEpochMs >= okCancelLimitVec_[1] + okexOrder2kMsecStep_) {
		okCancelLimitVec_[2] = 0;
		if (okCancelLimitVec_[2] + cancelSize <= okexOrdersPer2kMsec_) {
			okCancelLimitVec_[0] = currentEpochMs;
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okexOrder2kMsecStep_;
			okCancelLimitVec_[2] = okCancelLimitVec_[2] + cancelSize;
		} else {
			okCancelLimitVec_[0] = currentEpochMs;
			okCancelLimitVec_[1] = okCancelLimitVec_[0] + okexOrder2kMsecStep_;
			cancelSize = okexOrdersPer2kMsec_ - okCancelLimitVec_[2];
			okCancelLimitVec_[2] = okexOrdersPer2kMsec_;
				LOG_WARN << "VaildOkCancelCountbyDuration2s okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
			okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_;

		}
	}
	LOG_INFO << "VaildOkCancelCountbyDuration2s first okCancelLimitVec_[0]: " << okCancelLimitVec_[0] << ", okCancelLimitVec_[1]: " << okCancelLimitVec_[1] << ", okCancelLimitVec_[2]: " <<
		okCancelLimitVec_[2] << ", cancelSize: " << cancelSize << ", okexOrdersPer2kMsec_: " << okexOrdersPer2kMsec_ << ", okexOrder2kMsecStep_: " << okexOrder2kMsecStep_<< ", countSum: " << countSum;

	return true;
}

bool spotRisk::VaildBianOrdersCountbyDurationTenSec(uint32_t& orderNum)
{
	MutexLockGuard lock(ba10SecMutex_);
	tm ptm;
    getCurrentPtm(ptm);
	int secondCount = ptm.tm_hour * 60 * 60 + ptm.tm_min * 60 + ptm.tm_sec;
	uint32_t secCnt = secondCount % 60;
	uint32_t secIndex = secCnt / bianOrderSecStep_; // bianTimeStep : 10s
	// LOG_INFO << "VaildBianOrdersCountbyDurationTenSec secIndex: " << secIndex;
	// cout << "VaildBianOrdersCountbyDurationTenSec secIndex: " << secIndex;

	if (secIndex == lastSecIndex_) {
		if (bian10SecOrders_[secIndex] + orderNum <= bianOrdersPerTenSec_) {
			bian10SecOrders_[secIndex] = bian10SecOrders_[secIndex] + orderNum;
		} else {
			orderNum = bianOrdersPerTenSec_ - bian10SecOrders_[secIndex];
			bian10SecOrders_[secIndex] = bianOrdersPerTenSec_;
			LOG_INFO << "VaildBianOrdersCountbyDurationTenSec secIndex : " << secIndex << ", lastSecIndex_: " << lastSecIndex_ << ", bian10SecOrders_[secIndex]: " 
				<< bian10SecOrders_[secIndex] << ", bian10SecOrders_[lastSecIndex_]: " << bian10SecOrders_[lastSecIndex_]
				<< ", bianOrdersPerTenSec_: " << ", bianOrderSecStep_: " << bianOrderSecStep_
				<< ", secondCount: " << secondCount << ", orderNum: " << orderNum;
			return false;
		}
	} else {
		memset(&bian10SecOrders_, 0, sizeof(uint32_t) * 6);
		lastSecIndex_ = secIndex;
		if (orderNum <= bianOrdersPerTenSec_) {
			bian10SecOrders_[secIndex] = orderNum;
		} else {
			orderNum = bianOrdersPerTenSec_;
			bian10SecOrders_[secIndex] = bianOrdersPerTenSec_;
			LOG_INFO << "VaildBianOrdersCountbyDurationTenSec secIndex : " << secIndex << ", lastSecIndex_: " << lastSecIndex_ << ", bian10SecOrders_[secIndex]: " 
				<< bian10SecOrders_[secIndex] << ", bian10SecOrders_[lastSecIndex_]: " << bian10SecOrders_[lastSecIndex_]
				<< ", bianOrdersPerTenSec_: " << ", bianOrderSecStep_: " << bianOrderSecStep_
				<< ", secondCount: " << secondCount << ", orderNum: " << orderNum;
			return false;
		}
	}
	// LOG_INFO << "VaildBianOrdersCountbyDurationTenSec secIndex : " << secIndex << ", lastSecIndex_: " << lastSecIndex_ << ", bian10SecOrders_[secIndex]: " 
	// 	<< bian10SecOrders_[secIndex] << ", bian10SecOrders_[lastSecIndex_]: " << bian10SecOrders_[lastSecIndex_]
	// 	<< ", bianOrdersPerTenSec_: " << ", bianOrderSecStep_: " << bianOrderSecStep_
	// 	<< ", secondCount: " << secondCount << ", orderNum: " << orderNum;
	return true;
}

bool spotRisk::VaildBianOrdersCountbyDurationOneMin(uint32_t& orderNum)
{
	MutexLockGuard lock(ba1MinMutex_);
	tm ptm;
    getCurrentPtm(ptm);
	uint32_t currMin = ptm.tm_hour * 60 + ptm.tm_min;
	if (currMin != lastBaMin_) {
		lastBaMin_ = currMin;
		bianOrderOneMinSum_ = 0;
		if (bianOrderOneMinSum_ + orderNum <= bianOrdersPerMin_) {
			bianOrderOneMinSum_ = bianOrderOneMinSum_ + orderNum;
		} else {
			orderNum = bianOrdersPerMin_;
			bianOrderOneMinSum_ = bianOrdersPerMin_;
			LOG_INFO << "VaildBianOrdersCountbyDurationOneMin lastBaMin_: " << lastBaMin_ << ", bianOrderOneMinSum_: "<< bianOrderOneMinSum_ << ", orderNum: " <<
				orderNum << ", bianOrdersPerMin_: " << bianOrdersPerMin_;
			return false;
		}
	} else {
		if (bianOrderOneMinSum_ + orderNum <= bianOrdersPerMin_) {
			bianOrderOneMinSum_ = bianOrderOneMinSum_ + orderNum;
		} else {
			orderNum = bianOrdersPerMin_ - bianOrderOneMinSum_;
			bianOrderOneMinSum_ = bianOrdersPerMin_;
			LOG_INFO << "VaildBianOrdersCountbyDurationOneMin lastBaMin_: " << lastBaMin_ << ", bianOrderOneMinSum_: "<< bianOrderOneMinSum_ << ", orderNum: " <<
				orderNum << ", bianOrdersPerMin_: " << bianOrdersPerMin_;
			return false;
		}
	}

	// LOG_INFO << "VaildBianOrdersCountbyDurationOneMin lastBaMin_: " << lastBaMin_ << ", bianOrderOneMinSum_: "<< bianOrderOneMinSum_ << ", orderNum: " <<
	// 	orderNum << ", bianOrdersPerMin_: " << bianOrdersPerMin_;
	return true;
}

	// accReqOrdPers_ = 0;
	// accCanOrdPers_ = 0;

	// byReqOrdPers_ = 
	// byCanOrdPers_ = 

bool spotRisk::VaildByLimitOrdersSlidePers(uint32_t& reqNum)
{
	uint64_t now = CURR_MSTIME_POINT;
	if (now - lastReqOrdTime_ < 1000) {
		if (reqNum + accReqOrdPers_ <= byReqOrdPers_) {
			lastReqOrdTime_ = now;
			accReqOrdPers_ = reqNum + accReqOrdPers_;
			return true;
		} else {
			return false;
			LOG_WARN << "VaildByitOrdersSlidePers less accReqOrdPers_: " << accReqOrdPers_
				<< ", reqNum: " << reqNum << ", lastReqOrdTime_: " << lastReqOrdTime_;
		}
	} else {
		if (reqNum  <= byReqOrdPers_) {
			lastReqOrdTime_ = now;
			accReqOrdPers_ = reqNum;
			return true;
		} else {
			return false;
			LOG_WARN << "VaildByitOrdersSlidePers more accReqOrdPers_: " << accReqOrdPers_
				<< ", reqNum: " << reqNum << ", lastReqOrdTime_: " << lastReqOrdTime_;
		}
	}
}

// bool spotRisk::VaildByitOrdersCountbyDurationTenSec(int orderLimitCount, int byTimeStep, int secondCount)
// {
// 	int countSum = 0;
// 	if (secondCount >= byTimeStep - 1) {
// 		int i = 0;
// 		while (i < byTimeStep) {
// 			countSum = countSum + (*byOrderTenSecVec_)[secondCount - i];
// 			i++;
// 		}
// 	} else {
// 		int i = secondCount;
// 		while (i >= 0) {
// 			countSum = countSum + (*byOrderTenSecVec_)[i];
// 			i--;
// 		}
// 		int j = byTimeStep - (secondCount + 1);
// 		int k = 0;
// 		while (k < j) {
// 			countSum = countSum + (*byOrderTenSecVec_)[86399 - k];
// 			k++;
// 		}
// 	}
// 	if (secondCount < byTimeStep) {
// 		memset(byOrderTenSecVec_ + secondCount + 1, 0, (sizeof(int) * (86400 - byTimeStep)));
// 	} else {
// 		memset(byOrderTenSecVec_, 0, (sizeof(int) * (secondCount - byTimeStep + 1)));
// 		memset(byOrderTenSecVec_ + secondCount + 1, 0, (sizeof(int) * (86400 - secondCount - 1)));
// 	}
// 	if (countSum <= orderLimitCount) {
// 		LOG_INFO << "VaildByitOrdersCountbyDurationTenSec countSum: " << countSum << ", orderLimitCount: " << ", byTimeStep: " <<
// 			byTimeStep << ", secondCount: " << secondCount;
// 		return true;
// 	}
// 	LOG_INFO << "VaildByitOrdersCountbyDurationTenSec countSum: " << countSum << ", orderLimitCount: " << ", byTimeStep: " <<
// 		byTimeStep << ", secondCount: " << secondCount;
// 	return false;
// }