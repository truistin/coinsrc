#include "spot/strategy/Initializer.h"
#include "spot/utility/Logging.h"
#include "spot/base/DataStruct.h"
#include "spot/base/ParametersManager.h"
#include "spot/strategy/StrategyFactory.h"
#include "spot/base/InstrumentManager.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/TradingPeriodManager.h"
#include "spot/utility/TradingTime.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/InitialData.h"
#include "spot/base/MqDecode.h"
#include <string.h>
using namespace spot;
using namespace spot::utility;
using namespace spot::base;
using namespace spot::strategy;

map<int, list<StrategySymbol> > Initializer::strategySymbolMap_;
map<int, list<string> > Initializer::strategyInstrumentMap_;
map<string, map<string, SymbolInstrument> > Initializer::symbolInstrumentMap_;
Initializer::Initializer()
{
}
Initializer::~Initializer()
{
}
Initializer::Initializer(int spotID)
{
	InitialData::setSpotID(spotID);
}
bool Initializer::init(spsc_queue<QueueNode> *initQueue, bool isMarketData)
{
	bool ret = true;
	try
	{
#ifndef INIT_WITHOUT_RMQ
		cout << "test" << endl;
		sendInitInfo();
#endif
		ret = getInitData(initQueue, isMarketData);
	}
	catch (exception ex)
	{
		string errStr = "init failed. " + string(ex.what());
		std::cout << errStr << InitialData::getSpotID() << std::endl;
		LOG_ERROR << errStr.c_str();
		ret = false;
	}

	LOG_INFO << "Initializer::init return";

	return ret;
}
void Initializer::sendInitInfo()
{
	QueueNode node;
	node.Tid = TID_Init;
	StrategyInit *strategyInit = new StrategyInit();
	strategyInit->SpotID = InitialData::getSpotID();
	memcpy(strategyInit->TradingDay, TradingDay::getToday().c_str(), TradingDayLen);
	node.Data = strategyInit;
	RmqWriter::enqueue(node);
	LOG_INFO << "send init request. spotID = " << InitialData::getSpotID();
}
bool Initializer::getInitData(spsc_queue<QueueNode> *initQueue, bool isMarketData)
{
	auto ret = false;
	auto unfinished = true;
	isSPOTMD_ = isMarketData;
	QueueNode node;
	while (unfinished)
	{
		ret = initQueue->dequeue(node);
		if (ret)
		{
			LOG_INFO << "getInitData recv Tid:" << node.Tid;
			switch (node.Tid)
			{
			case TID_SpotProductInfo:
				saveSpotProductInfo(node);
				break;
			case TID_SpotMdInfo:
				saveSpotMdInfo(node);
				break;
			case TID_SpotTdInfo:
				saveSpotTdInfo(node);
				break;
			case TID_StrategyInfo:
				saveStrategyInfo(reinterpret_cast<list<StrategyInfo> *>(node.Data));
				break;
			case TID_SpotStrategy:
				saveSpotStrategy(reinterpret_cast<list<SpotStrategy> *>(node.Data));
				break;
			case TID_StrategySymbol:
				saveStrategySymbol(reinterpret_cast<list<StrategySymbol> *>(node.Data));
				break;
			case TID_StrategyParam:
				saveStrategyParam(reinterpret_cast<list<StrategyParam> *>(node.Data));
				break;
			case TID_SymbolInfo:
				saveSymbolInfo(reinterpret_cast<list<SymbolInfo> *>(node.Data));
				break;
			case TID_SymbolTradingFee:
				saveSymbolTradingFee(reinterpret_cast<list<SymbolTradingFee> *>(node.Data));
				break;
			case TID_SymbolInstrument:
				saveSymbolInstrument(reinterpret_cast<list<SymbolInstrument> *>(node.Data));
				break;
			case TID_StrategyInstrumentPNLDaily:
				saveStrategyInstrumentPNLDaily(reinterpret_cast<list<StrategyInstrumentPNLDaily> *>(node.Data));
				break;
			case TID_SpotInitConfig:
				saveSpotInitConfig(reinterpret_cast<list<SpotInitConfig> *>(node.Data));
				break;
			case TID_InstrumentTradingFee:
				saveInstrumentTradingFee(reinterpret_cast<list<InstrumentTradingFee> *>(node.Data));
				break;
			case TID_InitFinished:
				unfinished = false;
				LOG_INFO << "getInitData TID_InitFinished";
				break;
			case TID_TimeOut:
			{
				static int time_out_count = 0;
				if (++time_out_count > 4)
					LOG_FATAL << "error recv TID_TimeOut";
				break;
			}
			default:
				LOG_FATAL << "error tid:" << node.Tid;
			}
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	if (!isMarketData)
		ret = initAll();

	return ret;
}

void Initializer::saveInstrumentTradingFee(list<InstrumentTradingFee> *tradingFeeList)
{
	for (auto tradingFee : *tradingFeeList)
	{
		if (FEETYPE_MAKER.compare(tradingFee.FeeType) != 0 && FEETYPE_TAKER.compare(tradingFee.FeeType) != 0)
		{
			LOG_FATAL << "error FeeType invalid, FeeType:" << tradingFee.FeeType << " inst:" << tradingFee.InstrumentID;
		}
		if (FEEFORMAT_ABSOLUTE.compare(tradingFee.FeeFormat) != 0 && FEEFORMAT_PERCENTAGE.compare(tradingFee.FeeFormat) != 0)
		{
			LOG_FATAL << "error FeeFormat invalid";
		}
		auto instTradingFeeIter = instTradingFeeMap_.find(tradingFee.InstrumentID);
		if (instTradingFeeIter == instTradingFeeMap_.end())
		{
			map<string, InstrumentTradingFee> tradingFeeMap;
			tradingFeeMap[tradingFee.FeeType] = tradingFee;
			instTradingFeeMap_[tradingFee.InstrumentID] = tradingFeeMap;
		}
		else
		{
			instTradingFeeIter->second[tradingFee.FeeType] = tradingFee;
		}
	}
	// delete tradingFeeList; defalut shallow copy
}

void Initializer::saveSpotProductInfo(const QueueNode &node)
{
	auto productInfoList = reinterpret_cast<list<SpotProductInfo> *>(node.Data);
	if (productInfoList->size() == 0)
	{
		LOG_FATAL << "saveSpotProductInfo invalid SpotProductInfo: size = 0";
	}

	for (auto productInfo : *productInfoList)
	{
		LOG_INFO << productInfo.toString();
		InitialData::addSpotProductInfo(productInfo);
	}
	// delete productInfoList; defalut shallow copy
}

void Initializer::saveSpotInitConfig(list<SpotInitConfig> *spotInitConfigList)
{
	InitialData::addSpotInitConfig(*spotInitConfigList);
	// delete spotInitConfigList; defalut shallow copy
}

void Initializer::saveSpotTdInfo(const QueueNode &node)
{
	auto tdInfoList = reinterpret_cast<list<SpotTdInfo> *>(node.Data);

	// If spotmd, not need to validate Td info
	if (!isSPOTMD_)
	{
		validateTdInfoList(*tdInfoList);
	}

	for (auto tdInfo : *tdInfoList)
	{
		LOG_INFO << tdInfo.toString();
		InitialData::addSpotTdInfo(tdInfo);
	}
	// delete tdInfoList; defalut shallow copy
}

void Initializer::saveSpotMdInfo(const QueueNode &node)
{
	auto mdInfoList = reinterpret_cast<list<SpotMdInfo> *>(node.Data);

	validateMdInfoList(*mdInfoList);

	for (auto mdInfo : *mdInfoList)
	{
		LOG_INFO << mdInfo.toString();
		InitialData::addSpotMdInfo(mdInfo);
	}
	// delete mdInfoList;
}

void Initializer::saveStrategyInfo(list<StrategyInfo> *strategyList)
{
	// auto strategyList = reinterpret_cast<list<StrategyInfo>*>(node.Data);
	for (auto strategy : *strategyList)
	{
		LOG_INFO << strategy.toString();
		InitialData::addStrategyInfo(strategy);
	}
	// delete strategyList;
}

void Initializer::saveSpotStrategy(list<SpotStrategy> *spotStrategyList)
{
	// auto spotStrategyList = reinterpret_cast<list<SpotStrategy>*>(node.Data);
	for (auto spotStrategy : *spotStrategyList)
	{
		if (spotStrategy.SpotID != InitialData::getSpotID())
		{
			LOG_FATAL << "saveSpotStrategy invalid spotid:" << spotStrategy.SpotID << " this spotid:"
					  << "spotID_";
			return;
		}
		InitialData::addStrategyId(spotStrategy.StrategyID);
		LOG_INFO << spotStrategy.toString();
	}
	// delete spotStrategyList;
}
void Initializer::saveStrategySymbol(list<StrategySymbol> *strategySymbolList)
{
	// auto strategySymbolList = reinterpret_cast<list<StrategySymbol>*>(node.Data);
	for (auto strategySymbol : *strategySymbolList)
	{
		LOG_INFO << strategySymbol.toString();
		InitialData::addStrategySymbol(strategySymbol);
	}
	// delete strategySymbolList;
}

void Initializer::saveStrategyInstrumentPNLDaily(list<StrategyInstrumentPNLDaily> *strategyInstPNLList)
{
	for (auto strategyInstPNL : *strategyInstPNLList)
	{
		LOG_INFO << strategyInstPNL.toString();
		InitialData::addPnlDaily(strategyInstPNL);
	}
	// delete strategyInstPNLList;
}
void Initializer::saveStrategyParam(list<StrategyParam> *strategyParamList)
{
	for (auto strategyParam : *strategyParamList)
	{
		LOG_INFO << strategyParam.toString();
		ParametersManager::addStrategyParameter(strategyParam);
	}
	// delete strategyParamList;
}
void Initializer::saveSymbolInfo(list<SymbolInfo> *symbolList)
{
	for (auto symbol : *symbolList)
	{
		LOG_INFO << symbol.toString();
		InitialData::addSymbolInfo(symbol);
	}
	// delete symbolList;
}
void Initializer::saveSymbolInstrument(list<SymbolInstrument> *symbolInstList)
{
	for (auto symbolInst : *symbolInstList)
	{
		LOG_INFO << "Initializer::saveSymbolInstrument" << symbolInst.toString();
		auto symbolInstIter = symbolInstrumentMap_.find(symbolInst.Symbol);
		if (symbolInstIter == symbolInstrumentMap_.end())
		{
			map<string, SymbolInstrument> symbolInstMap;
			symbolInstMap[symbolInst.InstrumentID] = symbolInst;
			symbolInstrumentMap_[symbolInst.Symbol] = symbolInstMap;
		}
		else
		{
			symbolInstIter->second[symbolInst.InstrumentID] = symbolInst;
		}
	}
	// delete symbolInstList;
}
void Initializer::validateTdInfoList(list<SpotTdInfo> &tdInfoList)
{
	// it cannot be zero
	if (tdInfoList.size() == 0)
	{
		LOG_FATAL << "saveSpotTdProductInfo invalid SpotTdInfo: size = 0";
	}

	// check dupe in exchange codes
	for (auto config_outer : tdInfoList)
	{
		int counter = 0;
		for (auto config_inner : tdInfoList)
		{
			if (config_outer.ExchangeCode == config_inner.ExchangeCode)
			{
				counter++;
			}

			if (counter > 1)
			{
				LOG_FATAL << "saveSpotTdProductInfo duplicated exchange code found in ExchangeCode:  " << config_inner.ExchangeCode;
			}
		}
	}

	// check for the same interface
	for (auto config_outer : tdInfoList)
	{
		for (auto config_inner : tdInfoList)
		{
			if (strncmp(config_outer.InterfaceType, config_inner.InterfaceType, 63) == 0)
			{
				if (config_outer.InterfaceID == config_inner.InterfaceID)
				{
					// skip this validation check for DFZQ stock interface type, different investor id applies to the same interface, but userids should be different
					// for e.g. Shanghai A vs Shanghai H vs Shezhen A vs Shenzhen H
					if (strcmp(config_inner.UserID, config_outer.UserID) != 0 || strcmp(config_inner.FrontAddr, config_outer.FrontAddr) != 0 || strcmp(config_inner.LocalAddr, config_outer.LocalAddr) != 0)
					{
						LOG_FATAL << "saveSpotTdInfo for the same interface difference in userid/brokerid/MdAddr/LocalAddr detected";
					}
				}
			}
		}
	}
}
void Initializer::validateMdInfoList(list<SpotMdInfo> &mdInfoList)
{
	// it cannot be zero
	if (mdInfoList.size() == 0)
	{
		LOG_FATAL << "saveSpotMdProductInfo invalid SpotMdInfo: size = 0";
	}

	// check for the same interface

	for (auto config_outer : mdInfoList)
	{
		int count = 0;
		for (auto config_inner : mdInfoList)
		{
			if (strncmp(config_outer.InterfaceType, config_inner.InterfaceType, 63) == 0 && strcmp(config_inner.ExchangeCode, config_outer.ExchangeCode) == 0)
			{
				if (config_outer.InterfaceID == config_inner.InterfaceID)
				{
					count++;
				}

				if (count > 1)
				{
					LOG_FATAL << "saveSpotMdInfo for the same interfaceID/interfaceType/exchangeCode detected";
				}
			}
		}
	}

	for (auto config_outer : mdInfoList)
	{
		for (auto config_inner : mdInfoList)
		{
			if (strcmp(config_outer.InterfaceType, config_inner.InterfaceType) == 0 && config_outer.InterfaceID == config_inner.InterfaceID)
			{
				if (strcmp(config_inner.UserID, config_outer.UserID) != 0 || strcmp(config_inner.FrontAddr, config_outer.FrontAddr) != 0 || strcmp(config_inner.LocalAddr, config_outer.LocalAddr) != 0)
				{
					LOG_FATAL << "saveSpotMdInfo for the same interface difference in investorid/userid/brokerid/MdAddr/LocalAddr detected InterfaceType1: " 
						<< config_outer.InterfaceType 
						<< ", InterfaceType2: " << config_inner.InterfaceType

						<< ", InterfaceID1: " << config_outer.InterfaceID
						<< ", InterfaceID2: " << config_inner.InterfaceID

						<< ", UserID1: " << config_outer.UserID 
						<< ", UserID2: " << config_inner.UserID 

						<< ", FrontAddr1: " << config_outer.FrontAddr
						<< ", FrontAddr2: " << config_inner.FrontAddr

						<< ", LocalAddr1: " << config_outer.LocalAddr
						<< ", LocalAddr2: " << config_inner.LocalAddr;
				}
			}
		}
	}
}

void Initializer::saveSymbolTradingFee(list<SymbolTradingFee> *symbolTradingFeeList)
{
	LOG_INFO << "Initializer::saveSymbolTradingFee: " << symbolTradingFeeList->size();
	for (auto symbolTradingFee : *symbolTradingFeeList)
	{
		LOG_INFO << symbolTradingFee.toString();
		if (FEETYPE_MAKER.compare(symbolTradingFee.FeeType) != 0 && FEETYPE_TAKER.compare(symbolTradingFee.FeeType) != 0)
		{
			LOG_FATAL << "error FeeType invalid, FeeType:" << symbolTradingFee.FeeType << " symbol:" << symbolTradingFee.Symbol;
		}
		if (FEEFORMAT_ABSOLUTE.compare(symbolTradingFee.FeeFormat) != 0 && FEEFORMAT_PERCENTAGE.compare(symbolTradingFee.FeeFormat) != 0)
		{
			LOG_FATAL << "error FeeFormat invalid";
		}
		auto symbolTradingFeeIter = symbolTradingFeeMap_.find(symbolTradingFee.Symbol);
		if (symbolTradingFeeIter == symbolTradingFeeMap_.end())
		{
			map<string, SymbolTradingFee> tradingFeeMap;
			tradingFeeMap[symbolTradingFee.FeeType] = symbolTradingFee;
			symbolTradingFeeMap_[symbolTradingFee.Symbol] = tradingFeeMap;
		}
		else
		{
			symbolTradingFeeIter->second[symbolTradingFee.FeeType] = symbolTradingFee;
		}
	}

	// delete symbolTradingFeeList;
}

bool Initializer::initAll()
{
	if (InitialData::spotInitConfigMap().size() == 0)
		LOG_INFO << "there is no spotInitConfig info";
	ParametersManager::initParameters();
	StrategyFactory::registerStrategys();
	for (auto strategyIter : InitialData::strategyList())
	{
		auto strategyInfoIter = InitialData::strategyInfoMap().find(strategyIter);
		if (strategyInfoIter == InitialData::strategyInfoMap().end())
		{
			LOG_FATAL << "cann't find strategyInfo strategyID:" << strategyIter;
			return false;
		}
		StrategyFactory::addStrategy(strategyInfoIter->first, strategyInfoIter->second.StrategyName);
		auto ret = initSymbol(strategyIter);
		if (!ret)
		{
			return false;
		}
	}
	return true;
}

bool Initializer::initSymbol(int strategyID)
{
	auto strategySymbolIter = InitialData::strategySymbolMap().find(strategyID);
	if (strategySymbolIter == InitialData::strategySymbolMap().end())
	{
		LOG_FATAL << "initAll cann't find StrategySymbol strategyID:" << strategyID;
		return false;
	}
	for (auto symbolIter : strategySymbolIter->second)
	{
		bool ret = false;
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(symbolIter.Symbol);
		if (symbolInfo == nullptr)
		{
			LOG_FATAL << "symbolinfo cann't find symbol:" << symbolIter.Symbol;
		}
		if (AssetType_FutureSwap.compare(symbolInfo->Type) == 0 || AssetType_FuturePerpetual.compare(symbolInfo->Type) == 0
			|| AssetType_Perp.compare(symbolInfo->Type) == 0 || AssetType_Spot.compare(symbolInfo->Type) == 0)
		{
			ret = initInstrument(symbolIter.StrategyID, symbolIter.Symbol);
		}
		/*	else if (AssetType_FutureOption.compare(symbolInfo->Type) == 0)
			{
				ret = initOptionInstrument(symbolIter.StrategyID, symbolIter.Symbol);
			}*/
	/*	else if (AssetType_TD.compare(symbolInfo->Type) == 0 || AssetType_Crypto.compare(symbolInfo->Type) == 0 || AssetType_OKExIndex.compare(symbolInfo->Type) == 0)
		{
			ret = addInstrument(symbolIter.StrategyID, symbolIter.Symbol, symbolIter.Symbol, PRIMARY_TYPE);
		}*/
		else
		{
			LOG_FATAL << "invalid symbol type.symbol:" << symbolIter.Symbol
					  << " type:" << symbolInfo->Type;
		}

		if (!ret)
		{
			return false;
		}
	}
	return true;
}

Instrument *Initializer::getSymbolInstrument(string symbol, string tradeableType)
{
	SymbolInfo *symbolInfo = InitialData::getSymbolInfo(symbol);

	string symbolType = symbolInfo->Type;

	if (AssetType_CryptoDb.compare(symbolType) == 0)
	{
		auto tradeableInstIter = symbolInstrumentMap_.find(symbol);
		if (tradeableInstIter != symbolInstrumentMap_.end())
		{
			for (auto instIter : tradeableInstIter->second)
			{
				auto type = instIter.second.TradeableType;

				if (tradeableType.compare(type) == 0)
					return InstrumentManager::getInstrument(instIter.second.InstrumentID);
			}
		}
		LOG_FATAL << "Unable to find instrment ID for symbol:" << symbol << " tradeableType:" << tradeableType;
	}
	else if (AssetType_Crypto.compare(symbolType) == 0)
	{
		return InstrumentManager::getInstrument(symbol);
	}
	else
	{
		LOG_FATAL << "invalid symbol type.symbol:" << symbol << " type:" << symbolType;
	}
	return nullptr; // only for compile warning
}

string Initializer::getSymbolInstrumentID(string symbol, string tradeableType, string symbolType)
{
	if (AssetType_CryptoDb.compare(symbolType) == 0)
	{
		auto symbolInstIter = symbolInstrumentMap_.find(symbol);
		if (symbolInstIter != symbolInstrumentMap_.end())
		{
			for (auto instIter : symbolInstIter->second)
			{
				auto type = instIter.second.TradeableType;

				if (tradeableType.compare(type) == 0)
					return instIter.second.InstrumentID;
			}
		}

		// LOG_FATAL << "Unable to find instrment ID for symbol:" << symbol << " tradeableType:" << tradeableType;
		return string();
	}
	else if (AssetType_Crypto.compare(symbolType) == 0)
	{
		return symbol;
	}
	else
	{
		// LOG_FATAL << "invalid symbol type.symbol:" << symbol << " type:" << symbolType;
		return string();
	}
}

list<string> Initializer::getOKExTradeableInstrumentID(string symbol, string tradeableType, string symbolType)
{
	list<string> result;
	if (AssetType_Future.compare(symbolType) == 0 || AssetType_FutureOption.compare(symbolType) == 0)
	{
		auto tradeableInstIter = symbolInstrumentMap_.find(symbol);
		if (tradeableInstIter != symbolInstrumentMap_.end())
		{
			for (auto instIter : tradeableInstIter->second)
			{
				auto type = instIter.second.TradeableType;

				if (tradeableType.compare(type) == 0)
					result.push_back(instIter.second.InstrumentID);
			}
		}
	}
	else if (AssetType_TD.compare(symbolType) == 0 || AssetType_Crypto.compare(symbolType) == 0)
	{
		result.push_back(symbol);
	}
	else
	{
	}
	return result;
}

bool Initializer::addTradingFees(const string& symbol, Instrument* instrument)
{
	LOG_INFO << "addTradingFees: " << symbolTradingFeeMap_.size();
	auto symbolIter = symbolTradingFeeMap_.find(symbol);
	if (symbolIter != symbolTradingFeeMap_.end())
	{
		for (auto feeIter : symbolIter->second)
		{
			instrument->addTradingFee(feeIter.second);
		}
		//add InstrumentTradingFee into instrument
		auto iter = instTradingFeeMap_.find(instrument->getInstrumentID());
		if (iter != instTradingFeeMap_.end())
		{
			for (auto feeIter : iter->second)
			{
				LOG_DEBUG << feeIter.second.toString();
				instrument->addInstrumentTradingFee(&feeIter.second);
			}
		}
	}
	else
	{
		LOG_FATAL << "cann't find tradingFee symbol:" << symbol.c_str();
		return false;
	}
	return true;
}

bool Initializer::initInstrument(int strategyID, string symbol)
{
	SymbolInfo *symbolInfo = InitialData::getSymbolInfo(symbol);

	if (symbolInfo == nullptr)
	{
		LOG_FATAL << "symbolinfo cann't find symbol:" << symbol;
	}
	auto symbolInstIter = symbolInstrumentMap_.find(symbol);
	if (symbolInstIter == symbolInstrumentMap_.end())
	{
		LOG_FATAL << "cann't find symbol:" << symbol.c_str() << " in symbolInstrument table";
		return false;
	}

	for (auto instIter : symbolInstIter->second)
	{
		string tradeableType(instIter.second.TradeableType);

		if (!addInstrument(strategyID, symbol, instIter.first, tradeableType))
			return false;
	}

	return true;
}

void Initializer::initStrategyInstrument(int strategyID, string instrumentID)
{
	auto strategyIter = strategyInstrumentMap_.find(strategyID);
	if (strategyIter != strategyInstrumentMap_.end())
	{
		strategyIter->second.push_back(instrumentID);
	}
	else
	{
		list<string> instrumentList;
		instrumentList.push_back(instrumentID);
		strategyInstrumentMap_[strategyID] = instrumentList;
	}
}

bool Initializer::addInstrument(int strategyID, string symbol, string instrumentID, string tradeableType)
{
	initStrategyInstrument(strategyID, instrumentID);

	SymbolInfo *symbolInfo = InitialData::getSymbolInfo(symbol);
	auto instrument = InstrumentManager::addInstrument(instrumentID, *symbolInfo);
	instrument->setTradeableType(tradeableType);

	auto ret = addTradingFees(symbol, instrument);
	if (!ret)
	{
		LOG_FATAL << "initInstrument. addTradingFees failed symbol:" << symbol.c_str();
		return false;
	}
	return true;
}

void Initializer::getInstrumentListByStrategyID(int strategyID, list<string> &instrumentList)
{
	auto strategyIter = strategyInstrumentMap_.find(strategyID);
	if (strategyIter != strategyInstrumentMap_.end())
	{
		instrumentList = strategyIter->second;
	}
}

vector<string> Initializer::getInstrumentList()
{
	vector<string> instVec;

	// first get from tradable instrument
	for (const auto &symbolInstrument : symbolInstrumentMap_)
	{
		for (const auto &inst : symbolInstrument.second)
		{
			instVec.push_back(inst.first);
		}
	}

	// enrich with additional data for market data recording
	// i.e. Stock and TD are not defined in tradeable instrument,
	// so we will add it here
	for (auto strategyIter : InitialData::strategyList())
	{
		auto strategySymbolIter = InitialData::strategySymbolMap().find(strategyIter);
		if (strategySymbolIter == InitialData::strategySymbolMap().end())
		{
			LOG_FATAL << "initAll cann't find StrategySymbol strategyID:" << strategyIter;
		}
		for (auto symbolIter : strategySymbolIter->second)
		{
			if (symbolInstrumentMap_.find(symbolIter.Symbol) == symbolInstrumentMap_.end())
			{
				instVec.push_back(symbolIter.Symbol);
			}
		}
	}

	return instVec;
}
vector<string> Initializer::getInstrumentList(list<string> exchangeList)
{
	vector<string> instVec;
	// enrich with additional data for market data recording
	// i.e. Stock and TD are not defined in tradeable instrument,
	// so we will add it here
	//遍历策略
	for (auto strategyIter : InitialData::strategyList())
	{
		auto strategySymbolIter = InitialData::strategySymbolMap().find(strategyIter);
		if (strategySymbolIter == InitialData::strategySymbolMap().end())
		{
			LOG_FATAL << "initAll cann't find StrategySymbol strategyID:" << strategyIter;
		}
		//遍历策略使用的产品代码
		for (auto symbolIter : strategySymbolIter->second)
		{
			//获取产品代码信息
			auto sympolInfo = InitialData::getSymbolInfo(symbolIter.Symbol);
			//判断产品交易所
			if (sympolInfo != nullptr && find(exchangeList.begin(), exchangeList.end(), sympolInfo->ExchangeCode) != exchangeList.end())
			{
				const auto &symbolInstrument = symbolInstrumentMap_.find(symbolIter.Symbol);
				if (symbolInstrument == symbolInstrumentMap_.end())
				{
					//股票和现货直接在strategySymbol里面配置
					instVec.push_back(symbolIter.Symbol);
				}
				else
				{
					//期货从symbolInstrument获取相应合约
					for (const auto &inst : symbolInstrument->second)
					{
						instVec.push_back(inst.first);
					}
				}
			}
		}
	}

	return instVec;
}