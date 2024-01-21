#pragma once 
#include "spot/gateway/MktDataStoreCallBack.h"
using namespace spot::gtway;
class MktDataLogStore : public MktDataStoreCallBack
{
public:
	MktDataLogStore(moodycamel::ConcurrentQueue<QueueNode> *queue, int writeQ);
	void storeHandle(InnerMarketData *innerMktData) override;
	void storeHandle(InnerMarketTrade *innerMktTrade) override;
	void storeHandle(InnerMarketOrder *innerMktOrder) override;

	void writeRmq(InnerMarketData *innerMktData);
private:
	moodycamel::ConcurrentQueue<QueueNode> *queue_;
	int writeQueue_;
	map<string, int> sendTimesMap_;
};