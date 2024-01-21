#ifndef SPOT_RISK_DELTA_H
#define SPOT_RISK_DELTA_H

#include <iostream>
#include "spotRiskBase.h"
#include "spot/strategy/Strategy.h"
#include "spot/utility/Mutex.h"

using namespace std;
using namespace spot;
namespace spot
{
	namespace risk
	{
		class spotRisk : public spotRiskBase
		{
		public:
			spotRisk(Strategy* s);

			virtual ~spotRisk();
			void getCurrentPtm(tm& ptm);
			bool VaildBianOrdersCountbyDurationOneMin(uint32_t& orderNum);
			bool VaildBianOrdersCountbyDurationTenSec(uint32_t& orderNum);

			bool VaildOkCancelCountbyDuration2s(uint32_t& cancelSize);
			bool VaildOkReqCountbyDuration2s(uint32_t& reqSize);

			bool VaildByLimitOrdersSlidePers(uint32_t& reqNum);
		public:
			Strategy* st_;

			int bianOrdersPerMin_; // orders sum per min
			int bianOrdersPerTenSec_; // orders sum consective 10 secs

			uint32_t bianOrderOneMinSum_;
			uint32_t lastBaMin_;
			//vector<int> *bianOrderTenSecVec_; // sub: sec, value: orderCount


    		int bianOrderSecStep_; //10s
			int okexOrdersPer2kMsec_;
			int okexOrder2kMsecStep_;

			int lastSecIndex_;
			uint32_t bian10SecOrders_[6];

			uint16_t byReqOrdPers_;
			uint16_t byCanOrdPers_;

			uint16_t accReqOrdPers_;
			uint16_t accCanOrdPers_;

			uint64_t lastReqOrdTime_;
			uint64_t lastCanOrdTime_;

			MutexLock ba10SecMutex_;
			MutexLock ba1MinMutex_;

			// 60/2s, firstOrder : last firstOrder timestamp, second : 2s right threshold timeStamp, third : current Oders num
			vector<uint64_t> okReqLimitVec_; 
			vector<uint64_t> okCancelLimitVec_; 
		};
	}
}
#endif