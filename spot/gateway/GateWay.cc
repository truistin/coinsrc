#include <spot/gateway/GateWay.h>

#include <spot/gateway/MktDataQueStore.h>
#include <spot/gateway/OrderQueStore.h>
#include <spot/utility/ReadIni.h>
#include <spot/utility/Logging.h>

#include "spot/base/InstrumentManager.h"
#include "spot/base/InitialDataExt.h"
#include <spot/cache/CacheAllocator.h>
#include "spot/shmmd/ShmManagement.h"

#ifdef __I_Bitstamp_TD__
#include "spot/gateway/BitstampTraderSpi.h"
#endif //__I_Bitstamp_TD__
#ifdef __I_Bitstamp_MD__
#include "spot/gateway/BitstampMdSpi.h"
#endif //__I_Bitstamp_MD__

#ifdef __I_Bybit_MD__
#include "spot/gateway/BybitMdSpi.h"
#endif //__I_Bybit_MD__

#ifdef __I_Bybit_TD__
#include "spot/gateway/BybitTdSpi.h"
#endif //__I_Bybit_TD__

#ifdef __I_OKExV5_TD__
#include "spot/gateway/OKxV5TdSpi.h"
#endif //__I_OKExV5_TD__
#ifdef __I_OKExV5_MD__
#include "spot/gateway/OKxV5MdSpi.h"
#endif //__I_OKExV5_MD__

#ifdef __I_Huob_TD__
#include "spot/gateway/HuobTraderSpi.h"
#endif //__I_Huob_TD__
#ifdef __I_Huob_MD__
#include "spot/gateway/HuobMdSpi.h"
#endif //__I_Huob_MD__

#ifdef __I_HuobFuture_MD__
#include "spot/gateway/HuobFutureMdSpi.h"
#endif //__I_HuobFuture_MD__

#ifdef __I_Bian_TD__
#include "spot/gateway/BianTdSpi.h"
#endif //__I_Bian_TD__
#ifdef __I_Bian_MD__
#include "spot/gateway/BianMdSpi.h"
#endif //__I_Bian_MD__

#ifdef __I_Bitfinex_TD__
#include "spot/gateway/BitfinexTraderSpi.h"
#endif //__I_Bitfinex_TD__
#ifdef __I_Bitfinex_MD__
#include "spot/gateway/BitfinexMdSpi.h"
#endif //__I_Bitfinex_MD__

#ifdef __I_Deribit_TD__
#include "spot/gateway/DeribitTraderSpi.h"
#endif //__I_Deribit_TD__
#ifdef __I_Deribit_MD__
#include "spot/gateway/DeribitMdSpi.h"
#endif //__I_Deribit_MD__


using namespace spot;
using namespace spot::gtway;


GateWay::GateWay()
{
	char *buff = CacheAllocator::instance()->allocate(sizeof(ConcurrentQueue<InnerMarketData>));
	mdQueue_ = new (buff) ConcurrentQueue<InnerMarketData>();

	buff = CacheAllocator::instance()->allocate(sizeof(ConcurrentQueue<InnerMarketTrade>));
	mtQueue_ = new (buff) ConcurrentQueue<InnerMarketTrade>();

	buff = CacheAllocator::instance()->allocate(sizeof(ConcurrentQueue<Order>));
	orderQueue_ = new (buff) ConcurrentQueue<Order>();

	buff = CacheAllocator::instance()->allocate(sizeof(map<std::pair<string, int>, map<int, RequestFun>*>));
	requestFunMap_ = new (buff) map<std::pair<string, int>, map<int, RequestFun>*>;	

	apiTdInfoDetailMap_ = new  map<string, map<int, shared_ptr<ApiInfoDetail>>>();
	apiMdInfoDetailMap_ = new  map<string, map<int, shared_ptr<ApiInfoDetail>>>();

	subInstlMap_ = new map<string, map<int, vector<string>>>();
	mdLastPriceQueue_ = new moodycamel::ConcurrentQueue<QueueNode>();
	spotInitConfigMap_ = new map<string, SpotInitConfig>();
	// requestFunMap_ = new  map<std::pair<string, int>, map<int, RequestFun>*>;
	marketDataCallback_ = nullptr;
	marketTradeCallback_ = nullptr;
}

GateWay::~GateWay()
{
	delete apiTdInfoDetailMap_;
	delete apiMdInfoDetailMap_;
	delete mdQueue_;
	delete mtQueue_;
	delete orderQueue_;
	delete subInstlMap_;
	delete mdLastPriceQueue_;
	delete spotInitConfigMap_;
	delete requestFunMap_;
}

int GateWay::reqOrder(Order &order)
{
	int res = 0;
	auto funMapIter = requestFunMap_->find(std::make_pair(order.CounterType, InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0)));

	if (funMapIter != requestFunMap_->end())
	{
		auto funMap = funMapIter->second;
		auto funcPtr = funMap->find(0);
		assert(funcPtr != funMap->end());
		res = funcPtr->second(order);
	}
	else
	{
		res = -1;
		LOG_ERROR << "reqOrder gateway no found. interfaceType " << order.CounterType << ", interfaceID:" << InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0);
	}	
	return res;
}

int GateWay::cancelOrder(Order &order)
{
	int res = 0;
	auto funMapIter = requestFunMap_->find(std::make_pair(order.CounterType, InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0)));

	if (funMapIter != requestFunMap_->end())
	{
		auto funMap = funMapIter->second;
		auto funcPtr = funMap->find(1);
		assert(funcPtr != funMap->end());
		res = funcPtr->second(order);
	}
	else
	{
		res = -1;
		LOG_FATAL << "reqOrder gateway no found. interfaceType " << order.CounterType << ", interfaceID:" << InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0);
	}
	return res;
}

int GateWay::query(const Order &order)
{
	int res = 0;
	cout << "GateWay query: " << order.ExchangeCode << endl;
	auto funMapIter = requestFunMap_->find(std::make_pair(order.CounterType, InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0)));

	if (funMapIter != requestFunMap_->end())
	{
		auto funMap = funMapIter->second;
		auto funcPtr = funMap->find(2);
		assert(funcPtr != funMap->end());
		res = funcPtr->second(order);
	}
	else
	{
		res = -1;
		LOG_ERROR << "reqOrder gateway no found. interfaceType " << order.CounterType << ", interfaceID:" << InitialDataExt::getTdInterfaceID(order.ExchangeCode, 0);
	}	
	return res;
}

void GateWay::init()
{
	initMdGtway();
	initTdGtway();
}

void GateWay::initTdGtway()
{
	for (auto iter : *apiTdInfoDetailMap_)
	{
		LOG_INFO << "GateWay::initTdGtway: " << apiTdInfoDetailMap_->size() << ", first: " << iter.first;
		auto interfaceType = iter.first;
		for (auto apiInfoDetail : iter.second)
		{
			auto interfaceID = apiInfoDetail.first;
			string interfaceTypeID = interfaceType + to_string(interfaceID) + "_td";
			OrderQueStore* orderStore = new OrderQueStore(orderQueue_);

			map<int, RequestFun>* funMap;
			auto requestFunMapIter = requestFunMap_->find(std::make_pair(interfaceType, interfaceID));
			int count = 0;
			LOG_INFO << "GateWay::initTdGtway interfaceType: " << interfaceType << ", interfaceID: " <<interfaceID << ", requestFunMap_: " << requestFunMap_->size(); 
			if (requestFunMapIter == requestFunMap_->end())
			{	
				funMap = new map<int, RequestFun> ;
				(*requestFunMap_)[std::make_pair(interfaceType, interfaceID)] = funMap;
			}
			else
			{
				LOG_FATAL << "initTdGtway duplicated gateway init. interfaceType "<< interfaceType<<", interfaceID:"<<interfaceID << ", count: " <<count;
			}

			if (0 != 0) {
				//dummy
			}

#ifdef __I_Bybit_TD__
			else if (interfaceType == InterfaceBybit)
			{
				for (auto inst : (*subInstlMap_)[InterfaceBybit][interfaceID])
				{
					BybitApi::AddInstrument(inst.c_str());
				}
				BybitTdSpi* traderPtrBybit = new BybitTdSpi(apiInfoDetail.second, orderStore);
				traderPtrBybit->Init();
				(*funMap)[0] = std::bind(&BybitTdSpi::ReqOrderInsert, traderPtrBybit, std::placeholders::_1);
				(*funMap)[1] = std::bind(&BybitTdSpi::ReqOrderCancel, traderPtrBybit, std::placeholders::_1);
				(*funMap)[2] = std::bind(&BybitTdSpi::Query, traderPtrBybit, std::placeholders::_1);
			}
#endif //__I_Bybit_TD__

#ifdef __I_OKExV5_TD__
			else if (interfaceType == InterfaceOKExV5)
			{
				for (auto inst : (*subInstlMap_)[InterfaceOKExV5][interfaceID])
				{
					OkSwapApi::AddInstrument(inst.c_str());
				}
				OKxV5TdSpi* traderPtrOKExV5 = new OKxV5TdSpi(apiInfoDetail.second, orderStore);
				traderPtrOKExV5->Init();
				(*funMap)[0] = std::bind(&OKxV5TdSpi::ReqOrderInsert, traderPtrOKExV5, std::placeholders::_1);
				(*funMap)[1] = std::bind(&OKxV5TdSpi::ReqOrderCancel, traderPtrOKExV5, std::placeholders::_1);
				(*funMap)[2] = std::bind(&OKxV5TdSpi::Query, traderPtrOKExV5, std::placeholders::_1);
			}
#endif //__I_OKExV5_TD__

#ifdef __I_Huob_TD__
			else if (interfaceType == InterfaceHuob)
			{
				HuobTraderSpi* traderPtrHuob = new HuobTraderSpi(apiInfoDetail.second, orderStore);
				traderPtrHuob->SetIsCryptoFuture(false);
				traderPtrHuob->Init();
				(*funMap)[0] = std::bind(&HuobTraderSpi::ReqOrderInsert, traderPtrHuob, std::placeholders::_1);
				(*funMap)[1] = std::bind(&HuobTraderSpi::ReqOrderCancel, traderPtrHuob, std::placeholders::_1);
			}
#endif //__I_Huob_TD__

#ifdef __I_HuobFuture_TD__
			else if (interfaceType == InterfaceHuobFuture)
			{
				HuobTraderSpi* traderPtrHuobFuture = new HuobTraderSpi(apiInfoDetail.second, orderStore);
				traderPtrHuobFuture->SetIsCryptoFuture(true);
				traderPtrHuobFuture->Init();
				(*funMap)[0] = std::bind(&HuobTraderSpi::ReqOrderInsert, traderPtrHuobFuture, std::placeholders::_1);
				(*funMap)[1] = std::bind(&HuobTraderSpi::ReqOrderCancel, traderPtrHuobFuture, std::placeholders::_1);
			}
#endif //__I_HuobFuture_TD__

#ifdef __I_Bian_TD__
			else if (interfaceType == InterfaceBinance)
			{
				for (auto inst : (*subInstlMap_)[InterfaceBinance][interfaceID])
				{
					BnApi::AddInstrument(inst.c_str());
				}
				LOG_INFO << "__I_Bian_TD__: " << interfaceType;
				BianTdSpi* traderPtrBinance = new BianTdSpi(apiInfoDetail.second, orderStore);
				traderPtrBinance->Init();
				(*funMap)[0] = std::bind(&BianTdSpi::ReqOrderInsert, traderPtrBinance, std::placeholders::_1);
				(*funMap)[1] = std::bind(&BianTdSpi::ReqOrderCancel, traderPtrBinance, std::placeholders::_1);
				(*funMap)[2] = std::bind(&BianTdSpi::Query, traderPtrBinance, std::placeholders::_1);
			}
#endif //__I_Bian_TD__

#ifdef __I_Deribit_TD__
			else if (interfaceType == InterfaceDeribit)
			{
				DeribitTraderSpi* traderPtrDeribit = new DeribitTraderSpi(apiInfoDetail.second, orderStore);
				traderPtrDeribit->Init();
				(*funMap)[0] = std::bind(&DeribitTraderSpi::ReqOrderInsert, traderPtrDeribit, std::placeholders::_1);
				(*funMap)[1] = std::bind(&DeribitTraderSpi::ReqOrderCancel, traderPtrDeribit, std::placeholders::_1);
			}
#endif //__I_Deribit_TD__

		}
	}
}

void GateWay::initMdGtway()
{	
	int writeRMQ = SpotInitConfigTable::instance().getMqWriteMQ();
	LOG_INFO << "initMdGtway getMqWriteMQ: " << writeRMQ;
	for (auto iter : *apiMdInfoDetailMap_)
	{
		auto interfaceType = iter.first;
		for (auto apiInfoDetail : iter.second)
		{
			auto interfaceID = apiInfoDetail.first;
			string interfaceTypeID = interfaceType + to_string(interfaceID) + "_md";

			if (0 != 0)
			{
				//dummy
			}
			else if (interfaceType == InterfaceDistMd)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_, writeRMQ);
				auto mdPtr = new DistMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceDistMd][interfaceID])
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->Init();
			}

			else if (interfaceType == InterfaceShmMd)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_, writeRMQ);
				auto mdPtr = new ShmMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceShmMd][interfaceID])
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				if(marketDataCallback_)
					mdPtr->setReckMarketDataCallback(marketDataCallback_);
				mdPtr->Init();
			}

			else if (interfaceType == InterfaceShmMt)
			{
				MktDataQueStore* mtStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_, writeRMQ);
				auto mtPtr = new ShmMtSpi(apiInfoDetail.second, mtStore);
				for (auto inst : (*subInstlMap_)[InterfaceShmMt][interfaceID])
				{
					mtPtr->AddSubInst(inst.c_str());
				}
				if(marketTradeCallback_)
					mtPtr->setRecvMarketTradeCallback(marketTradeCallback_);
				mtPtr->Init();
			}
#ifdef __I_Bian_MD__		        
			else if (interfaceType == InterfaceBinance)
			{
				LOG_INFO << "__I_Bian_MD__: " << interfaceType;
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new BianMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceBinance][interfaceID])
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->Init();
			}
#endif //__I_Bian_MD__

#ifdef __I_OKExV5_MD__
			else if (interfaceType == InterfaceOKExV5)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new OKxV5MdSpi(apiInfoDetail.second, mdStore);
				// auto& here is important, cause diff compiler use copy or quote
				for (auto& inst : (*subInstlMap_)[InterfaceOKExV5][interfaceID]) 
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->init();
			}
#endif //__I_OKExV5_MD__

#ifdef __I_Bybit_MD__
			else if (interfaceType == InterfaceBybit)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new BybitMdSpi(apiInfoDetail.second, mdStore);
				// auto& here is important, cause diff compiler use copy or quote
				for (auto& inst : (*subInstlMap_)[InterfaceBybit][interfaceID]) 
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->init();
			}
#endif //__I_Bybit_MD__

#ifdef __I_Huob_MD__
			else if (interfaceType == InterfaceHuob)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new HuobiMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceHuob][interfaceID])
				{
					auto instrument = InstrumentManager::getInstrument(inst);
					mdPtr->AddSubInst(instrument);
				}
				mdPtr->Init();
			}
#endif //__I_Huob_MD__

#ifdef __I_HuobFuture_MD__
			else if (interfaceType == InterfaceHuobFuture)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new HuobiFutureMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceHuobFuture][interfaceID])
				{
					auto instrument = InstrumentManager::getInstrument(inst);
					mdPtr->AddSubInst(instrument);
				}
				mdPtr->Init();
			}
#endif //__I_HuobFuture_MD__

#ifdef __I_Deribit_MD__
			else if (interfaceType == InterfaceDeribit)
			{
				MktDataQueStore* mdStore = new MktDataQueStore(mdQueue_, mtQueue_, mdLastPriceQueue_);
				auto mdPtr = new DeribitMdSpi(apiInfoDetail.second, mdStore);
				for (auto inst : (*subInstlMap_)[InterfaceDeribit][interfaceID])
				{
					auto instrument = InstrumentManager::getInstrument(inst);
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->Init();
			}
#endif //__I_Deribit_MD__
			LOG_INFO << "gateway init finish";
		}
	}
	ShmManagement::instance().start(false);
}

void GateWay::setReckMarketDataCallback(RecvMarketDataCallback marketDataCallback)
{
	marketDataCallback_ = marketDataCallback;
}

void GateWay::setRecvMarketTradeCallback(RecvMarketTradeCallback marketTradeCallback)
{
	marketTradeCallback_ = marketTradeCallback;
}



