#include "MdInterface.h"
#include "spot/gateway/MktDataLogStore.h"
#include "spot/gateway/GateWay.h"
#include "spot/base/InitialDataExt.h"
#include "spot/base/InstrumentManager.h"
#include "spot/distributor/DistServer.h"
#include "spot/shmmd/ShmMdServer.h"
#include "spot/shmmd/ShmMtServer.h"
#include "spot/gateway/ShmMdSpi.h"
//#include "spot/gateway/ShmMoSpi.h"
#include "spot/gateway/DistMdSpi.h"
#include "spot/gateway/ShmMtSpi.h"

#ifdef __I_Bybit_MD__
#include "spot/gateway/BybitMdSpi.h"
#endif //__I_Bybit_MD__

#ifdef __I_OKExV5_MD__
#include "spot/gateway/OKxV5MdSpi.h"
#endif //__I_OKExV5_MD__

#ifdef __I_Bian_MD__
#include "spot/gateway/BianMdSpi.h"
#endif //__I_Bian_MD__

using namespace spot::utility;
using namespace spot::base;
using namespace spot::gtway;
using namespace spot::distributor;
using namespace moodycamel;

MdInterface::MdInterface()
{	
	mdQueue_ = new moodycamel::ConcurrentQueue<QueueNode>;
	// dataQue_ = new moodycamel::ConcurrentQueue<InnerMarketData>;
	// tradeQue_ = new moodycamel::ConcurrentQueue<InnerMarketTrade>;
}
MdInterface::~MdInterface()
{
}
MdInterface& MdInterface::instance()
{
	static MdInterface instance_;
	return instance_;
}
MktDataStoreCallBack* MdInterface::getDataStore(ConcurrentQueue<QueueNode> *mdQueue, int writeRMQ)
{
	MktDataStoreCallBack *mdStore = nullptr;

	mdStore = new MktDataLogStore(mdQueue, writeRMQ);
	return mdStore;
}
void MdInterface::init(Initializer &initializer, ConfigParameter &config)
{
	init(initializer);
	initShm(config);
	initMd();
}
void MdInterface::init(Initializer &initializer)
{
	auto interfaceInfoListMap = InitialData::getInterfaceSpotMdInfoListMap();
	for (auto const &entry : interfaceInfoListMap)
	{
		auto interfaceType = entry.first;
		auto interfaceIds = entry.second;
		for (auto const &idsEntry : interfaceIds)
		{
			int interfaceId = idsEntry.first;
			vector<string> subInstVec;
			subInstVec = initializer.getInstrumentList(InitialDataExt::getMdExchangeList(interfaceType, interfaceId));
			instSubMap[interfaceType][interfaceId] = subInstVec;
		}
		
		for (auto IDInfoListMap : entry.second)
		{
			auto interfaceID = IDInfoListMap.first;
			auto infoList = IDInfoListMap.second;
			std::shared_ptr<ApiInfoDetail> ptrDetail = make_shared<ApiInfoDetail>();
			//get user id / investor id / broker id based on interface type
			strcpy(ptrDetail->investorId_, InitialData::getInvestorID(interfaceType.c_str(), MdType, interfaceID).c_str());
			strcpy(ptrDetail->userId_, InitialData::getUserID(interfaceType.c_str(), MdType, interfaceID).c_str());
			//strcpy(ptrDetail->brokerId_, InitialData::getBrokerID(interfaceType.c_str(), MdType, interfaceID).c_str());
			string mdLevelType("MDLevelType_");
			mdLevelType += interfaceType;
			string mdType = SpotInitConfigTable::instance().getValueByKey(mdLevelType);
			if (!mdType.empty())
			{
				ptrDetail->mdLevelType_ = stoi(mdType);
			}
			string passwordKey("MdPassword_");
			passwordKey += interfaceType;
			passwordKey += "_" + to_string(interfaceID);
			string password = SpotInitConfigTable::instance().getValueByKey(passwordKey);
			if (!password.empty()) {
				strcpy(ptrDetail->passwd_, password.c_str());
			}			
			ptrDetail->subAll_ = SpotInitConfigTable::instance().getSubAll();
			LOG_INFO << "SubAll:" << ptrDetail->subAll_;
			auto mdInfo = infoList.begin();
			if (mdInfo != infoList.end())
			{
				strcpy(ptrDetail->frontMdAddr_, mdInfo->FrontAddr);
				strcpy(ptrDetail->InterfaceAddr_, mdInfo->LocalAddr);
				strcpy(ptrDetail->frontQueryAddr_, mdInfo->FrontQueryAddr);
				strcpy(ptrDetail->exchangeCode_, mdInfo->ExchangeCode);
			}
			strcpy(ptrDetail->type_, interfaceType.c_str());
			ptrDetail->interfaceID_ = interfaceID;
			string flowPath("./flow_");
			flowPath += interfaceType + "_" + to_string(interfaceID) + "/md";
			string interfaceTypeID = interfaceType + to_string(interfaceID) + "_md";
			if (getFilePath(flowPath))
			{
				flowdirMap[interfaceTypeID] = flowPath + "/";
			}
			else
			{
				LOG_ERROR << "Create filepath failed: " << flowPath;
			}
			apiMdInfoDetailMap[interfaceType][interfaceID] = ptrDetail;
		}
	}
}
void MdInterface::initShm(ConfigParameter &config)
{
	std::string distKey = config.distKey();
	if (!distKey.empty()) {
		for (auto it = instSubMap.begin(); it != instSubMap.end(); ++it)
		{
			auto idInstruments = it->second;
			for (auto it2 = idInstruments.begin(); it2 != idInstruments.end(); ++it2)
			{
				vector<string>& instruments = it2->second;
				DistServer::instance().addInstruments(instruments);
			}
		}
		LOG_INFO << "create dist md server, dist key:[" << distKey << "] instruments size:" << DistServer::instance().size();
		if (!DistServer::instance().init(distKey)) {
			LOG_FATAL << "create dist md server failed";
		}
		DistServer::instance().start();
	}

	std::string shmKey = config.shmKey();
	if (!shmKey.empty()) {
		for (auto it = instSubMap.begin(); it != instSubMap.end(); ++it)
		{
			auto idInstruments = it->second;
			for (auto it2 = idInstruments.begin(); it2 != idInstruments.end(); ++it2)
			{
				vector<string>& instruments = it2->second;
				ShmMdServer::instance().addInstruments(instruments);
			}
		}
		LOG_INFO << "create shm md server, shm key:" << shmKey << " instruments size:" << ShmMdServer::instance().size();
		if (!ShmMdServer::instance().init(shmKey)) {
			LOG_FATAL << "create shm md server failed";
		}
	}

	std::string tradeShmKey = config.tradeShmKey();
	if (!tradeShmKey.empty()) {
		// std::list<std::string> subInst = InstrumentManager::getAllInstruments();
		// ShmMtServer::instance().addInstruments(subInst);
		for (auto it = instSubMap.begin(); it != instSubMap.end(); ++it)
		{
			auto idInstruments = it->second;
			for (auto it2 = idInstruments.begin(); it2 != idInstruments.end(); ++it2)
			{
				vector<string>& instruments = it2->second;
				ShmMtServer::instance().addInstruments(instruments);
			}
		}
		LOG_INFO << "create shm mt server, shm key:" << tradeShmKey << " instruments size:" << ShmMtServer::instance().size();
		if (!ShmMtServer::instance().init(tradeShmKey)) {
			LOG_FATAL << "create shm mt server failed";
		}
	}

	// std::string orderShmKey = config.orderShmKey();
	// if (!orderShmKey.empty())
	// {
	// 	std::list<std::string> subInst = InstrumentManager::getAllInstruments();
	// 	ShmMoServer::instance().addInstruments(subInst);
	// 	for (auto it = instSubMap.begin(); it != instSubMap.end(); ++it)
	// 	{
	// 		auto idInstruments = it->second;
	// 		for (auto it2 = idInstruments.begin(); it2 != idInstruments.end(); ++it2)
	// 		{
	// 			vector<string>& instruments = it2->second;
	// 			ShmMoServer::instance().addInstruments(instruments);
	// 		}
	// 	}
	// 	LOG_INFO << "create shm mo server, shm key:" << orderShmKey << " instruments size:" << ShmMoServer::instance().size();
	// 	if (!ShmMoServer::instance().init(orderShmKey)) {
	// 		LOG_FATAL << "create shm mo server failed";
	// 	}
	// }
}
void MdInterface::initMd()
{
	int writeRMQ = SpotInitConfigTable::instance().getMqWriteMQ();
	LOG_INFO << "initMd MqWriteMQ:" << writeRMQ;
	for (auto iter : apiMdInfoDetailMap)
	{
		auto interfaceType = iter.first;
		for (auto apiIter : iter.second)
		{
			auto interfaceID = apiIter.first;
			auto interfaceTypeID = interfaceType + to_string(interfaceID) + "_md";

			if (interfaceType == InterfaceShmMd)
			{
				MktDataStoreCallBack *mdStore = getDataStore(mdQueue_, writeRMQ);
				auto mdPtr = new ShmMdSpi(apiIter.second, mdStore);
				if (!apiIter.second->subAll_)
				{
					for (auto inst : instSubMap[InterfaceShmMd][interfaceID])
					{
						mdPtr->AddSubInst(inst.c_str());
					}
				}
				else
				{
					mdPtr->setSubAll(apiIter.second->subAll_ == 1);
				}
				mdPtr->Init();
				LOG_INFO << "MdInterface ShmMdClient init";
			}
			else if (interfaceType == InterfaceShmMt)
			{
				MktDataStoreCallBack *mdStore = getDataStore(mdQueue_, writeRMQ);
				auto mtPtr = new ShmMtSpi(apiIter.second, mdStore);
				if (!apiIter.second->subAll_)
				{
					for (auto inst : instSubMap[InterfaceShmMt][interfaceID])
					{
						mtPtr->AddSubInst(inst.c_str());
					}
				}
				else
				{
					mtPtr->setSubAll(apiIter.second->subAll_ == 1);
				}
				mtPtr->Init();
				LOG_INFO << "MdInterface ShmMtClient init";
			}
			// else if (interfaceType == InterfaceShmMo)
			// {
			// 	MktDataStoreCallBack *mdStore = getDataStore(mdQueue_, writeRMQ);
			// 	auto mtPtr = new ShmMoSpi(apiIter.second, mdStore);
			// 	if (!apiIter.second->subAll_)
			// 	{
			// 		for (auto inst : instSubMap[InterfaceShmMo][interfaceID])
			// 		{
			// 			mtPtr->AddSubInst(inst.c_str());
			// 		}
			// 	}
			// 	else
			// 	{
			// 		mtPtr->setSubAll(true);
			// 	}
			// 	mtPtr->Init();
			// }
			// else if (interfaceType == InterfaceDistMd)
			// {
			// 	MktDataLogStore *mdStrore = new MktDataLogStore(mdQueue_, writeRMQ);
			// 	auto mdPtr = new DistMdSpi(apiIter.second, mdStrore);
			// 	if (!apiIter.second->subAll_)
			// 	{
			// 		for (auto inst : instSubMap[InterfaceDistMd][interfaceID])
			// 		{
			// 			mdPtr->AddSubInst(inst.c_str());
			// 		}
			// 	}
			// 	else
			// 	{
			// 		mdPtr->setSubAll(apiIter.second->subAll_);
			// 	}
			// 	mdPtr->Init();
			// } 
			// if (0 != 0) {
			// 	LOG_FATAL << "0000000000";
			// }
#ifdef __I_Bian_MD__		        
			else if (interfaceType == InterfaceBinance)
			{
				MktDataLogStore *mdStore = new MktDataLogStore(mdQueue_, writeRMQ);
				auto mdPtr = new BianMdSpi(apiIter.second, mdStore);
				for (auto inst : instSubMap[InterfaceBinance][interfaceID])
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->Init();
			}
#endif //__I_Bian_MD__
#ifdef __I_OKExV5_MD__
			else if (interfaceType == InterfaceOKExV5)
			{
				MktDataLogStore *mdStore = new MktDataLogStore(mdQueue_, writeRMQ);
				auto mdPtr = new OKxV5MdSpi(apiIter.second, mdStore);
				// auto& here is important, cause diff compiler use copy or quote
				for (auto& inst : instSubMap[InterfaceOKExV5][interfaceID]) 
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->init();
			}
#endif //__I_OKExV5_MD__
#ifdef __I_Bybit_MD__
			else if (interfaceType == InterfaceBybit)
			{
				MktDataLogStore *mdStore = new MktDataLogStore(mdQueue_, writeRMQ);
				auto mdPtr = new BybitMdSpi(apiIter.second, mdStore);
				// auto& here is important, cause diff compiler use copy or quote
				for (auto& inst : instSubMap[InterfaceBybit][interfaceID]) 
				{
					mdPtr->AddSubInst(inst.c_str());
				}
				mdPtr->init();
			}
#endif //__I_Bybit_MD__
			else if (interfaceType == InterfaceSim)
			{
				for (auto inst : instSubMap[InterfaceSim][interfaceID])
				{
					LOG_INFO << "add instrument:" << inst;
				}
			}
			else
			{
				cout << "Invalid md interfaceType: " << interfaceType << endl;
				LOG_FATAL << "Invalid md interfaceType:" << interfaceType;
				break;
			}
		}
	}


}