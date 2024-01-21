#ifndef __MQDECODE_H__
#define __MQDECODE_H__
#include<list>
#include<spot/base/DataStruct.h>
#include<spot/base/JsonDecode.h>
using namespace spot::base;

const static uint16_t TID_StrategyInstrumentPNLDaily = 1;
static std::list<StrategyInstrumentPNLDaily>* decodeStrategyInstrumentPNLDaily(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategyInstrumentPNLDaily>;
    StrategyInstrumentPNLDaily obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SymbolInfo = 2;
static std::list<SymbolInfo>* decodeSymbolInfo(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SymbolInfo>;
    SymbolInfo obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SymbolTradingFee = 3;
static std::list<SymbolTradingFee>* decodeSymbolTradingFee(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SymbolTradingFee>;
    SymbolTradingFee obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SymbolInstrument = 4;
static std::list<SymbolInstrument>* decodeSymbolInstrument(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SymbolInstrument>;
    SymbolInstrument obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_StrategyInfo = 5;
static std::list<StrategyInfo>* decodeStrategyInfo(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategyInfo>;
    StrategyInfo obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_StrategyParam = 6;
static std::list<StrategyParam>* decodeStrategyParam(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategyParam>;
    StrategyParam obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_Order = 7;
static std::list<Order>* decodeOrder(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<Order>;
    Order obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_StrategySymbol = 8;
static std::list<StrategySymbol>* decodeStrategySymbol(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategySymbol>;
    StrategySymbol obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_LogInfo = 9;
const static uint16_t TID_StrategyFinish = 10;
const static uint16_t TID_HeartBeat = 11;
const static uint16_t TID_SpotStrategy = 12;
static std::list<SpotStrategy>* decodeSpotStrategy(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SpotStrategy>;
    SpotStrategy obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_MetricData = 13;
const static uint16_t TID_SpotInitConfig = 14;
static std::list<SpotInitConfig>* decodeSpotInitConfig(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SpotInitConfig>;
    SpotInitConfig obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_MarketData = 15;
static std::list<MarketData>* decodeMarketData(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<MarketData>;
    MarketData obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SpotProductInfo = 16;
static std::list<SpotProductInfo>* decodeSpotProductInfo(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SpotProductInfo>;
    SpotProductInfo obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SpotTdInfo = 17;
static std::list<SpotTdInfo>* decodeSpotTdInfo(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SpotTdInfo>;
    SpotTdInfo obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_SpotMdInfo = 18;
static std::list<SpotMdInfo>* decodeSpotMdInfo(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<SpotMdInfo>;
    SpotMdInfo obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_Alarm = 19;
static std::list<Alarm>* decodeAlarm(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<Alarm>;
    Alarm obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_InstrumentTradingFee = 20;
static std::list<InstrumentTradingFee>* decodeInstrumentTradingFee(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<InstrumentTradingFee>;
    InstrumentTradingFee obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_UpdateStrategyParams = 21;
static std::list<UpdateStrategyParams>* decodeUpdateStrategyParams(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<UpdateStrategyParams>;
    UpdateStrategyParams obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_StrategySwitch = 22;
static std::list<StrategySwitch>* decodeStrategySwitch(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategySwitch>;
    StrategySwitch obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
const static uint16_t TID_StrategyInit = 23;
static std::list<StrategyInit>* decodeStrategyInit(spotrapidjson::Value &arrayValue)
{
    auto dataList = new std::list<StrategyInit>;
    StrategyInit obj;
    map<string, setMethod> method;
    obj.initMethodMap(method);
    for (auto iter = arrayValue.Begin(); iter != arrayValue.End(); ++iter)
    {
        memset(&obj,0,sizeof(obj));
        decodeFields(*iter, method);
        dataList->push_back(obj);
    }
    return dataList;
};
static QueueNode decode(std::string strTitle, spotrapidjson::Value & arrayValue)
{
    QueueNode queueNode;
    if (strTitle.compare("Alarm") == 0)
    {
        queueNode.Tid = TID_Alarm;
        queueNode.Data = decodeAlarm(arrayValue);
    }
    else if (strTitle.compare("InstrumentTradingFee") == 0)
    {
        queueNode.Tid = TID_InstrumentTradingFee;
        queueNode.Data = decodeInstrumentTradingFee(arrayValue);
    }
    else if (strTitle.compare("MarketData") == 0)
    {
        queueNode.Tid = TID_MarketData;
        queueNode.Data = decodeMarketData(arrayValue);
    }
    else if (strTitle.compare("Order") == 0)
    {
        queueNode.Tid = TID_Order;
        queueNode.Data = decodeOrder(arrayValue);
    }
    else if (strTitle.compare("SpotInitConfig") == 0)
    {
        queueNode.Tid = TID_SpotInitConfig;
        queueNode.Data = decodeSpotInitConfig(arrayValue);
    }
    else if (strTitle.compare("SpotMdInfo") == 0)
    {
        queueNode.Tid = TID_SpotMdInfo;
        queueNode.Data = decodeSpotMdInfo(arrayValue);
    }
    else if (strTitle.compare("SpotProductInfo") == 0)
    {
        queueNode.Tid = TID_SpotProductInfo;
        queueNode.Data = decodeSpotProductInfo(arrayValue);
    }
    else if (strTitle.compare("SpotStrategy") == 0)
    {
        queueNode.Tid = TID_SpotStrategy;
        queueNode.Data = decodeSpotStrategy(arrayValue);
    }
    else if (strTitle.compare("SpotTdInfo") == 0)
    {
        queueNode.Tid = TID_SpotTdInfo;
        queueNode.Data = decodeSpotTdInfo(arrayValue);
    }
    else if (strTitle.compare("Strategy") == 0)
    {
        queueNode.Tid = TID_StrategyInfo;
        queueNode.Data = decodeStrategyInfo(arrayValue);
    }
    else if (strTitle.compare("StrategyInit") == 0)
    {
        queueNode.Tid = TID_StrategyInit;
        queueNode.Data = decodeStrategyInit(arrayValue);
    }
    else if (strTitle.compare("StrategyInstrumentPNLDaily") == 0)
    {
        queueNode.Tid = TID_StrategyInstrumentPNLDaily;
        queueNode.Data = decodeStrategyInstrumentPNLDaily(arrayValue);
    }
    else if (strTitle.compare("StrategyParam") == 0)
    {
        queueNode.Tid = TID_StrategyParam;
        queueNode.Data = decodeStrategyParam(arrayValue);
    }
    else if (strTitle.compare("StrategySwitch") == 0)
    {
        queueNode.Tid = TID_StrategySwitch;
        queueNode.Data = decodeStrategySwitch(arrayValue);
    }
    else if (strTitle.compare("StrategySymbol") == 0)
    {
        queueNode.Tid = TID_StrategySymbol;
        queueNode.Data = decodeStrategySymbol(arrayValue);
    }
    else if (strTitle.compare("SymbolInfo") == 0)
    {
        queueNode.Tid = TID_SymbolInfo;
        queueNode.Data = decodeSymbolInfo(arrayValue);
    }
    else if (strTitle.compare("SymbolInstrument") == 0)
    {
        queueNode.Tid = TID_SymbolInstrument;
        queueNode.Data = decodeSymbolInstrument(arrayValue);
    }
    else if (strTitle.compare("SymbolTradingFee") == 0)
    {
        queueNode.Tid = TID_SymbolTradingFee;
        queueNode.Data = decodeSymbolTradingFee(arrayValue);
    }
    else if (strTitle.compare("UpdateStrategyParams") == 0)
    {
        queueNode.Tid = TID_UpdateStrategyParams;
        queueNode.Data = decodeUpdateStrategyParams(arrayValue);
    }
    return queueNode;
}
#endif
