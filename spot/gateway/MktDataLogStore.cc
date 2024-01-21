#include <spot/gateway/MktDataLogStore.h>

MktDataLogStore::MktDataLogStore(moodycamel::ConcurrentQueue<QueueNode> *queue, int writeQ) :queue_(queue), writeQueue_(writeQ) {}
void MktDataLogStore::storeHandle(InnerMarketData *innerMktData)
{
	if (writeQueue_ == 0)
	{
		LOG_INFO << "WRMDFILE," << innerMktData->InstrumentID << "," << innerMktData->ExchangeID << "," << innerMktData->LastPrice << "," << innerMktData->BidPrice1 << "," << innerMktData->BidPrice2 << "," << innerMktData->BidPrice3 << "," << innerMktData->BidPrice4 << "," << innerMktData->BidPrice5 << "," << innerMktData->AskPrice1 << "," << innerMktData->AskPrice2 << "," << innerMktData->AskPrice3 << "," << innerMktData->AskPrice4 << "," << innerMktData->AskPrice5 << \
			"," << innerMktData->BidVolume1 << "," << innerMktData->BidVolume2 << "," << innerMktData->BidVolume3 << "," << innerMktData->BidVolume4 << "," << innerMktData->BidVolume5 << \
			"," << innerMktData->AskVolume1 << "," << innerMktData->AskVolume2 << "," << innerMktData->AskVolume3 << "," << innerMktData->AskVolume4 << "," << innerMktData->AskVolume5 << \
			"," << innerMktData->Volume << "," << innerMktData->Turnover << ","  <<
			", " << innerMktData->TradingDay << "," << innerMktData->EpochTime << "," << innerMktData->UpdateMillisec << "," << innerMktData->UpdateTime;
	}
	else
	{
		writeRmq(innerMktData);
		auto usTime = SpotInitConfigTable::instance().getSpotmdSleepInterval();
		if (usTime != 0)
		{
			SLEEPUS(usTime);
		}
	}

}
void MktDataLogStore::storeHandle(InnerMarketTrade *innerMktTrade) 
{
	LOG_INFO << "WRMTFILE," << innerMktTrade->InstrumentID
		<< "," << innerMktTrade->ExchangeID << "," << innerMktTrade->Tid
		<< "," << innerMktTrade->Direction << "," << innerMktTrade->Price
		<< "," << innerMktTrade->Volume << "," << innerMktTrade->Turnover
		<< "," << innerMktTrade->MessageID
		<< "," << innerMktTrade->TradingDay << "," << innerMktTrade->EpochTime << "," << innerMktTrade->UpdateMillisec
		<< "," << innerMktTrade->UpdateTime;
}
void MktDataLogStore::storeHandle(InnerMarketOrder *innerMktOrder) 
{
	LOG_INFO << "WRMOFILE," << innerMktOrder->InstrumentID
		<< "," << innerMktOrder->ExchangeID << "," << innerMktOrder->OrderSysID
		<< "," << innerMktOrder->Direction << "," << innerMktOrder->Price
		<< "," << innerMktOrder->Volume
		<< "," << innerMktOrder->UpdateTime << "," << innerMktOrder->UpdateMillisec
		<< "," << innerMktOrder->OrdType << "," << innerMktOrder->EpochTime
		<< "," << innerMktOrder->TradingDay;
}
void MktDataLogStore::writeRmq(InnerMarketData *innerMktData)
{
#ifndef INIT_WITHOUT_RMQ
	MarketData *mqMarketData = new  MarketData();	
	strcpy(mqMarketData->InstrumentID, innerMktData->InstrumentID);
	strcpy(mqMarketData->ExchangeCode, innerMktData->ExchangeID);
	strcpy(mqMarketData->TradingDay, TradingDay::getToday().c_str());
	mqMarketData->LastPrice = innerMktData->LastPrice;
	mqMarketData->AskPrice1 = innerMktData->AskPrice1;
	mqMarketData->AskPrice2 = innerMktData->AskPrice2;
	mqMarketData->AskPrice3 = innerMktData->AskPrice3;
	mqMarketData->AskPrice4 = innerMktData->AskPrice4;
	mqMarketData->AskPrice5 = innerMktData->AskPrice5;
	mqMarketData->BidPrice1 = innerMktData->BidPrice1;
	mqMarketData->BidPrice2 = innerMktData->BidPrice2;
	mqMarketData->BidPrice3 = innerMktData->BidPrice3;
	mqMarketData->BidPrice4 = innerMktData->BidPrice4;
	mqMarketData->BidPrice5 = innerMktData->BidPrice5;
	mqMarketData->AskVolume1 = innerMktData->AskVolume1;
	mqMarketData->AskVolume2 = innerMktData->AskVolume2;
	mqMarketData->AskVolume3 = innerMktData->AskVolume3;
	mqMarketData->AskVolume4 = innerMktData->AskVolume4;
	mqMarketData->AskVolume5 = innerMktData->AskVolume5;
	mqMarketData->BidVolume1 = innerMktData->BidVolume1;
	mqMarketData->BidVolume2 = innerMktData->BidVolume2;
	mqMarketData->BidVolume3 = innerMktData->BidVolume3;
	mqMarketData->BidVolume4 = innerMktData->BidVolume4;
	mqMarketData->BidVolume5 = innerMktData->BidVolume5;
	strcpy(mqMarketData->UpdateTime, innerMktData->UpdateTime);
	mqMarketData->UpdateMillisec = innerMktData->UpdateMillisec;
	mqMarketData->Volume = innerMktData->Volume;
	mqMarketData->Turnover = innerMktData->Turnover;
	mqMarketData->HighestPrice = innerMktData->HighestPrice;
	mqMarketData->LowestPrice = innerMktData->LowestPrice;
	mqMarketData->LastSeqNum = innerMktData->LastSeqNum;
	//write into rmq directly
	RabbitMqProcess *rmqProcess = RabbitMqProcess::instance();
	if (rmqProcess)
	{
		rmqProcess->publishMeseage(mqMarketData->toJson(), RMQ_Priority_High);
	}
#endif
}