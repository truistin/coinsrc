#include <spot/gateway/MktDataQueStore.h>
#include <spot/base/DataStruct.h>
using namespace spot;
using namespace spot::gtway;

void MktDataQueStore::storeHandle(InnerMarketData *innerMktData)
{
	if (writeQ_ == 0) {
		mdQueue_->enqueue(*innerMktData);
	} 
	
	if (writeQ_ == 1 ){
		LOG_INFO << "WRMDFILE," << innerMktData->InstrumentID << "," << innerMktData->ExchangeID << "," << innerMktData->LastPrice << "," << innerMktData->BidPrice1 << "," << innerMktData->BidPrice2 << "," << innerMktData->BidPrice3 << "," << innerMktData->BidPrice4 << "," << innerMktData->BidPrice5 << "," << innerMktData->AskPrice1 << "," << innerMktData->AskPrice2 << "," << innerMktData->AskPrice3 << "," << innerMktData->AskPrice4 << "," << innerMktData->AskPrice5 << \
			"," << innerMktData->BidVolume1 << "," << innerMktData->BidVolume2 << "," << innerMktData->BidVolume3 << "," << innerMktData->BidVolume4 << "," << innerMktData->BidVolume5 << \
			"," << innerMktData->AskVolume1 << "," << innerMktData->AskVolume2 << "," << innerMktData->AskVolume3 << "," << innerMktData->AskVolume4 << "," << innerMktData->AskVolume5 << \
			"," << innerMktData->Volume << "," << innerMktData->Turnover << ","  <<
			", " << innerMktData->TradingDay << "," << innerMktData->EpochTime << "," << innerMktData->UpdateMillisec << "," << innerMktData->UpdateTime;
	}

	if (writeQ_ == 2 ){
		mdQueue_->enqueue(*innerMktData);
		LOG_INFO << "WRMDFILE," << innerMktData->InstrumentID << "," << innerMktData->ExchangeID << "," << innerMktData->LastPrice << "," << innerMktData->BidPrice1 << "," << innerMktData->BidPrice2 << "," << innerMktData->BidPrice3 << "," << innerMktData->BidPrice4 << "," << innerMktData->BidPrice5 << "," << innerMktData->AskPrice1 << "," << innerMktData->AskPrice2 << "," << innerMktData->AskPrice3 << "," << innerMktData->AskPrice4 << "," << innerMktData->AskPrice5 << \
			"," << innerMktData->BidVolume1 << "," << innerMktData->BidVolume2 << "," << innerMktData->BidVolume3 << "," << innerMktData->BidVolume4 << "," << innerMktData->BidVolume5 << \
			"," << innerMktData->AskVolume1 << "," << innerMktData->AskVolume2 << "," << innerMktData->AskVolume3 << "," << innerMktData->AskVolume4 << "," << innerMktData->AskVolume5 << \
			"," << innerMktData->Volume << "," << innerMktData->Turnover << ","  <<
			", " << innerMktData->TradingDay << "," << innerMktData->EpochTime << "," << innerMktData->UpdateMillisec << "," << innerMktData->UpdateTime;
	}
  
}

void MktDataQueStore::storeHandle(InnerMarketTrade *innerMktTrade)
{
	if (writeQ_ == 0) {
		mtQueue_->enqueue(*innerMktTrade);
	} 

	if (writeQ_ == 1) {
		LOG_INFO << "WRMTFILE," << innerMktTrade->InstrumentID
			<< "," << innerMktTrade->ExchangeID << "," << innerMktTrade->Tid
			<< "," << innerMktTrade->Direction << "," << innerMktTrade->Price
			<< "," << innerMktTrade->Volume << "," << innerMktTrade->Turnover
			<< "," << innerMktTrade->MessageID
			<< "," << innerMktTrade->TradingDay << "," << innerMktTrade->EpochTime << "," << innerMktTrade->UpdateMillisec
			<< "," << innerMktTrade->UpdateTime;
	}

	if (writeQ_ == 2) {
		mtQueue_->enqueue(*innerMktTrade);
		LOG_INFO << "WRMTFILE," << innerMktTrade->InstrumentID
			<< "," << innerMktTrade->ExchangeID << "," << innerMktTrade->Tid
			<< "," << innerMktTrade->Direction << "," << innerMktTrade->Price
			<< "," << innerMktTrade->Volume << "," << innerMktTrade->Turnover
			<< "," << innerMktTrade->MessageID
			<< "," << innerMktTrade->TradingDay << "," << innerMktTrade->EpochTime << "," << innerMktTrade->UpdateMillisec
			<< "," << innerMktTrade->UpdateTime;
	}
	
}

void MktDataQueStore::storeHandle(InnerMarketOrder *innerMktOrder)
{
	moQueue_->enqueue(*innerMktOrder);
}

