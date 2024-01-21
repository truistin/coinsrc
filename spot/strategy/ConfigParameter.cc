#include<iostream>
#include "spot/strategy/ConfigParameter.h"
#include "spot/strategy/Initializer.h"
#include "spot/base/InstrumentManager.h"
#include "spot/utility/Logging.h"
#include "spot/base/InitialDataExt.h"
#include "spot/base/DataStruct.h"
#include "spot/websocket/websocket.h"
using namespace std;
using namespace spot::base;
using namespace spot::strategy;
ConfigParameter::ConfigParameter(const char* iniFileName)
{
	readIni_ = new ReadIni(iniFileName);
	if (readIni_->itemSize() == 0)
	{
		string errStr = "error: file ../config/spot.ini item size == 0";
		cout << errStr << endl;
		LOG_ERROR << (errStr.c_str());
	}

	string logLevelStr = readIni_->getIni("LogLevel");

	string logMqLevelStr = readIni_->getIni("LogMqLevel");
	logMqLevel_ = atoi(logMqLevelStr.c_str());
	string spotIDStr = readIni_->getIni("SpotID");
	spotID_ = atoi(spotIDStr.c_str());
	mqParam_.spotid = spotID_;

	// rabbitAddr_ = readIni_->getIni("RabbitAddr");
	// string rabbitPortStr = readIni_->getIni("RabbitPort");
	// rabbitUser_ = readIni_->getIni("RabbitUser");
	// rabbitPassword_ = readIni_->getIni("RabbitPassword");
	string startTimeStr = readIni_->getIni("StartTime");

	// rabbitPort_ = atoi(rabbitPortStr.c_str());
	logLevel_ = atoi(logLevelStr.c_str());	
	startTime_ = atoi(startTimeStr.c_str());

	// mqParam_.hostname = rabbitAddr_;
	// mqParam_.port = rabbitPort_;
	// mqParam_.user = rabbitUser_;
	// mqParam_.password = rabbitPassword_;
	mqParam_.spotid = spotID_;

	if (readIni_->itemExists("DistKey")) {
		distKey_ = readIni_->getIni("DistKey");
	}
	else {
		distKey_ = "";
	}
	if (readIni_->itemExists("ShmKey")) {
		shmKey_ = readIni_->getIni("ShmKey");
	}
	else {
		shmKey_ = "";
	}
	if (readIni_->itemExists("TradeShmKey")) {
		tradeShmKey_ = readIni_->getIni("TradeShmKey");
	}
	else {
		tradeShmKey_ = "";
	}

#ifdef __USE_CRYPTO__
	if (readIni_->itemExists("CurlProxyAddr"))
	{
		gCurlProxyAddr = readIni_->getIni("CurlProxyAddr");
	}
	if (readIni_->itemExists("WebsocketProxyAddr"))
	{
		gWebsocketProxyAddr = readIni_->getIni("WebsocketProxyAddr");
	}
#endif //__USE_CRYPTO__
}
ConfigParameter::~ConfigParameter()
{
	delete readIni_;
}
void ConfigParameter::init(GateWay *gateway)
{
	//load spot config from database and local ini
	//mulitple gateway configuration, one config row per interface
	initTdConfig(gateway);
	initMdConfig(gateway);
	//gateway params 
	auto initConfigMap = InitialData::spotInitConfigMap();
	for (auto const &iter : initConfigMap) 
	{
		(*gateway->spotInitConfigMap())[iter.first] = iter.second;
	}
}

void ConfigParameter::initTdConfig(GateWay *gateway)
{
	auto interfaceInfoListMap = InitialData::getInterfaceSpotTdInfoListMap();
	for (auto const &entry : interfaceInfoListMap)
	{
		LOG_INFO << "ConfigParameter::initTdConfig size: " <<interfaceInfoListMap.size() <<", first: " <<entry.first;
		auto interfaceType = entry.first;

		for (auto IDInfoListMap : entry.second)
		{
			LOG_INFO << "ConfigParameter::IDInfoListMap size: " << entry.second.size() << ", first: " << IDInfoListMap.first;
			auto interfaceID = IDInfoListMap.first;
			auto infoList = IDInfoListMap.second;

			vector<string> subInstVec = InstrumentManager::getInstrumentList(InitialDataExt::getTdExchangeList(interfaceType));
			(*gateway->subInstMap())[interfaceType][interfaceID] = subInstVec;

			std::shared_ptr<ApiInfoDetail> ptrDetail = make_shared<ApiInfoDetail>();
			//get user id / investor id / broker id based on interface type
			memcpy(ptrDetail->investorId_, InitialData::getInvestorID(interfaceType.c_str(), TradeType, interfaceID).c_str(), min(sizeof(ptrDetail->investorId_), InitialData::getInvestorID(interfaceType.c_str(), TradeType, interfaceID).size()+1));
			memcpy(ptrDetail->userId_, InitialData::getUserID(interfaceType.c_str(), TradeType, interfaceID).c_str(), min(sizeof(ptrDetail->userId_), InitialData::getUserID(interfaceType.c_str(), TradeType, interfaceID).size()+1));

			string passwordKey("TdPassword_");
			passwordKey += interfaceType;
			passwordKey += "_" + to_string(interfaceID);
			map<string, SpotInitConfig> initConfigMap = InitialData::spotInitConfigMap();
			auto iter = initConfigMap.find(passwordKey);
			string password;
			if (iter != initConfigMap.end()) {
				password = iter->second.FieldValue;
			}
			if (!password.empty()) {
				strcpy(ptrDetail->passwd_, password.c_str());
			}
			else if (readIni_->itemExists(passwordKey.c_str())) {
					strcpy(ptrDetail->passwd_, readIni_->getIni(passwordKey.c_str()));
			}

			auto tdInfo = infoList.begin();
			if (tdInfo != infoList.end())
			{
				memcpy(ptrDetail->frontTdAddr_, tdInfo->FrontAddr, min(sizeof(ptrDetail->frontTdAddr_), sizeof(tdInfo->FrontAddr)));
				//InterfaceAddr is only used to guava udp md - InterfaceAddr = local ip address
				memcpy(ptrDetail->InterfaceAddr_, tdInfo->LocalAddr, min(sizeof(ptrDetail->InterfaceAddr_), sizeof(tdInfo->LocalAddr)));
				memcpy(ptrDetail->frontQueryAddr_, tdInfo->FrontQueryAddr, min(sizeof(ptrDetail->frontQueryAddr_), sizeof(tdInfo->FrontQueryAddr)));
				memcpy(ptrDetail->exchangeCode_, tdInfo->ExchangeCode, min(sizeof(ptrDetail->exchangeCode_), sizeof(tdInfo->ExchangeCode)));
			}

			memcpy(ptrDetail->type_ , interfaceType.c_str(), min(sizeof(ptrDetail->type_), interfaceType.size()+1));
			ptrDetail->interfaceID_ = interfaceID;

			(*gateway->apiTdInfoDetailMap())[interfaceType][interfaceID] = ptrDetail;
			LOG_INFO << "ConfigParameter::interfaceType: " << interfaceType << ", interfaceID: " <<interfaceID  <<
			    "frontTdAddr_: " << ptrDetail->frontTdAddr_;
			setTradeLoginStatusMap(interfaceType, false);
		}
	}
}

void ConfigParameter::initMdConfig(GateWay *gateway)
{
	auto interfaceInfoListMap = InitialData::getInterfaceSpotMdInfoListMap();
	for (auto const &entry : interfaceInfoListMap)
	{
		auto interfaceType = entry.first;
		for (auto IDInfoListMap : entry.second)
		{
			auto interfaceID = IDInfoListMap.first;
			auto infoList = IDInfoListMap.second;
			vector<string> subInstVec = InstrumentManager::getInstrumentList(InitialDataExt::getMdExchangeList(interfaceType, interfaceID));
			
			(*gateway->subInstMap())[interfaceType][interfaceID] = subInstVec;

			std::shared_ptr<ApiInfoDetail> ptrDetail = make_shared<ApiInfoDetail>();
			//get user id / investor id / broker id based on interface type
			memcpy(ptrDetail->investorId_, InitialData::getInvestorID(interfaceType.c_str(), MdType, interfaceID).c_str(), min(sizeof(ptrDetail->investorId_), InitialData::getInvestorID(interfaceType.c_str(), MdType, interfaceID).size()+1));
			memcpy(ptrDetail->userId_, InitialData::getUserID(interfaceType.c_str(), MdType, interfaceID).c_str(), min(sizeof(ptrDetail->userId_), InitialData::getUserID(interfaceType.c_str(), MdType, interfaceID).size()+1));

			string passwordKey("MdPassword_");
			passwordKey += interfaceType;
			passwordKey += "_" + to_string(interfaceID);
		//	string password = SpotInitConfigTable::instance().getValueByKey(passwordKey);
			map<string, SpotInitConfig> initConfigMap = InitialData::spotInitConfigMap();
			auto iter = initConfigMap.find(passwordKey);
			string password;
			if (iter != initConfigMap.end()) {
				password = iter->second.FieldValue;
			}
			if (!password.empty()) {
				strcpy(ptrDetail->passwd_, password.c_str());
			}
			else if (readIni_->itemExists(passwordKey.c_str())) {
				strcpy(ptrDetail->passwd_, readIni_->getIni(passwordKey.c_str()));
			}


			auto mdInfo = infoList.begin();
			if (mdInfo != infoList.end())
			{
				memcpy(ptrDetail->frontMdAddr_, mdInfo->FrontAddr, min(sizeof(ptrDetail->frontMdAddr_), sizeof(mdInfo->FrontAddr)));
				//InterfaceAddr is only used to guava udp md - InterfaceAddr = local ip address
				memcpy(ptrDetail->InterfaceAddr_, mdInfo->LocalAddr, min(sizeof(ptrDetail->InterfaceAddr_), sizeof(mdInfo->LocalAddr)));
			}

			memcpy(ptrDetail->type_ , interfaceType.c_str(), min(sizeof(ptrDetail->type_), interfaceType.size()+1));
			ptrDetail->interfaceID_ = interfaceID;

			(*gateway->apiMdInfoDetailMap())[interfaceType][interfaceID] = ptrDetail;
		}
	}
}
