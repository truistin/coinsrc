#pragma once 
#ifndef SPOT_MDINTERFACE_H
#define SPOT_MDINTERFACE_H
#include <spot/base/DataStruct.h>
#include <spot/utility/Logging.h>
#include <spot/utility/SPSCQueue.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/base/InitialData.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include "spot/strategy/Initializer.h"
#include "spot/strategy/ConfigParameter.h"

using namespace spot::utility;
using namespace spot::base;
using namespace spot::strategy;
using namespace moodycamel;

class MdInterface
{
public:
	MdInterface();
	~MdInterface();
	static MdInterface& instance();
	void init(Initializer &initializer, ConfigParameter &config);
	void init(Initializer &initializer);
	void initShm(ConfigParameter &config);
	void initMd();
	MktDataStoreCallBack* getDataStore(ConcurrentQueue<QueueNode> *mdQueue, int writeRMQ);
private:
	moodycamel::ConcurrentQueue<QueueNode> *mdQueue_;
	// ConcurrentQueue<InnerMarketData>* dataQue_;
	// ConcurrentQueue<InnerMarketTrade>* tradeQue_;
	map<string, map<int, vector<string>>> instSubMap;
	map<string, string> flowdirMap;
	map<string, map<int, shared_ptr<ApiInfoDetail>>> apiMdInfoDetailMap;
};
#endif