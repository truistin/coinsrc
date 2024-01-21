#pragma once 
#ifndef SPOT_GATEWAY_MktDATASTORECALLBACK_H
#define SPOT_GATEWAY_MktDATASTORECALLBACK_H
#include <spot/base/DataStruct.h>
#include <spot/utility/Logging.h>
#include <spot/utility/SPSCQueue.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/utility/TradingDay.h>
#include <spot/comm/MqDispatch.h>
#include <spot/base/SpotInitConfigTable.h>

using namespace spot::utility;
using namespace spot::base;
using namespace moodycamel;
namespace spot
{
	namespace gtway
	{
		class MktDataStoreCallBack
		{
		public:
			virtual void storeHandle(InnerMarketData *innerMktData) = 0;
			virtual void storeHandle(InnerMarketTrade *innerMktTrade) = 0;
			virtual void storeHandle(InnerMarketOrder *innerMktOrder) = 0;


			// virtual void storeHandle(InnerMarketData&& innerMktData) = 0;
			// virtual void storeHandle(InnerMarketTrade&& innerMktTrade) = 0;
			// virtual void storeHandle(InnerMarketOrder&& innerMktOrder) = 0;

			virtual ~MktDataStoreCallBack() {};
		};
	}
}
#endif