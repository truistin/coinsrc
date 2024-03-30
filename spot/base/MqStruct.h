#ifndef __SPOT_BASE_MQSTRUCT__
#define __SPOT_BASE_MQSTRUCT__

#include <spot/utility/Utility.h>
#include <spot/utility/Types.h>
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/stringbuffer.h>
#include <spot/rapidjson/writer.h>
#include <spot/utility/Compatible.h>
#include <functional>
#include <map>
using namespace std::placeholders;
namespace spot
{
  namespace base
  {
    const static uint16_t AlarmtextLen = 4000;
    const static uint16_t CancelTimeLen = 30;
    const static uint16_t CategoryLen = 30;
    const static uint16_t CoinTypeLen = 12;
    const static uint16_t CounterTypeLen = 62;
    const static uint16_t CurrPointLen = 64;
    const static uint16_t EndTradingDayLen = 30;
    const static uint16_t ExchangeCodeLen = 20;
    const static uint16_t FeeFormatLen = 20;
    const static uint16_t FeeTypeLen = 20;
    const static uint16_t FieldNameLen = 100;
    const static uint16_t FieldTypeLen = 20;
    const static uint16_t FieldValueLen = 255;
    const static uint16_t FrontAddrLen = 150;
    const static uint16_t FrontQueryAddrLen = 150;
    const static uint16_t InsertTimeLen = 8;
    const static uint16_t InstrumentIDLen = 40;
    const static uint16_t InterfaceTypeLen = 62;
    const static uint16_t InvestorIDLen = 40;
    const static uint16_t LastPointLen = 64;
    const static uint16_t LocalAddrLen = 150;
    const static uint16_t LogTextLen = 4000;
    const static uint16_t LogTimeLen = 40;
    const static uint16_t LogTypeLen = 10;
    const static uint16_t MTakerLen = 30;
    const static uint16_t NameLen = 50;
    const static uint16_t OrderSysIDLen = 120;
    const static uint16_t ProductNameLen = 150;
    const static uint16_t RefSymbolLen = 40;
    const static uint16_t StartTradingDayLen = 30;
    const static uint16_t StatusMsgLen = 150;
    const static uint16_t StrategyNameLen = 50;
    const static uint16_t StrategyTypeLen = 30;
    const static uint16_t SymbolLen = 40;
    const static uint16_t TimeInForceLen = 30;
    const static uint16_t TradeableTypeLen = 20;
    const static uint16_t TradingDayLen = 30;
    const static uint16_t TypeLen = 20;
    const static uint16_t UpdateTimeLen = 30;
    const static uint16_t UserIDLen = 150;
    const static uint16_t ValueStringLen = 150;
    const static uint16_t ValueTypeLen = 20;

    class StrategyInstrumentPNLDaily
    {
    public:
      StrategyInstrumentPNLDaily()
      {
        memset(this,0,sizeof(StrategyInstrumentPNLDaily));
      }
      int StrategyID;
      char InstrumentID[InstrumentIDLen+1];
      char TradingDay[TradingDayLen+1];
      double AvgBuyPrice;
      double AvgSellPrice;
      double BuyQuantity;
      double SellQuantity;
      double TodayLong;
      double TodayShort;
      double NetPosition;
      double Turnover;
      double AggregatedFeeByRate;
      double AggregatedFee;
      double Profit;
      double EntryPrice;

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setTradingDay( void *value , uint16_t length = 0) {
        memcpy(TradingDay, static_cast<char*>(value), length <TradingDayLen  ? length :TradingDayLen);
      }
      void setAvgBuyPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          AvgBuyPrice = *static_cast<double*>(value);
        else if (length == 0)
          AvgBuyPrice = (double)*static_cast<int*>(value);
        else
          AvgBuyPrice = (double)*static_cast<int64_t*>(value);
      }
      void setAvgSellPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          AvgSellPrice = *static_cast<double*>(value);
        else if (length == 0)
          AvgSellPrice = (double)*static_cast<int*>(value);
        else
          AvgSellPrice = (double)*static_cast<int64_t*>(value);
      }
      void setBuyQuantity( void *value , uint16_t length = 0) {
        if (length == 8)
          BuyQuantity = *static_cast<double*>(value);
        else if (length == 0)
          BuyQuantity = (double)*static_cast<int*>(value);
        else
          BuyQuantity = (double)*static_cast<int64_t*>(value);
      }
      void setSellQuantity( void *value , uint16_t length = 0) {
        if (length == 8)
          SellQuantity = *static_cast<double*>(value);
        else if (length == 0)
          SellQuantity = (double)*static_cast<int*>(value);
        else
          SellQuantity = (double)*static_cast<int64_t*>(value);
      }
      void setTodayLong( void *value , uint16_t length = 0) {
        if (length == 8)
          TodayLong = *static_cast<double*>(value);
        else if (length == 0)
          TodayLong = (double)*static_cast<int*>(value);
        else
          TodayLong = (double)*static_cast<int64_t*>(value);
      }
      void setTodayShort( void *value , uint16_t length = 0) {
        if (length == 8)
          TodayShort = *static_cast<double*>(value);
        else if (length == 0)
          TodayShort = (double)*static_cast<int*>(value);
        else
          TodayShort = (double)*static_cast<int64_t*>(value);
      }
      void setNetPosition( void *value , uint16_t length = 0) {
        if (length == 8)
          NetPosition = *static_cast<double*>(value);
        else if (length == 0)
          NetPosition = (double)*static_cast<int*>(value);
        else
          NetPosition = (double)*static_cast<int64_t*>(value);
      }
      void setTurnover( void *value , uint16_t length = 0) {
        if (length == 8)
          Turnover = *static_cast<double*>(value);
        else if (length == 0)
          Turnover = (double)*static_cast<int*>(value);
        else
          Turnover = (double)*static_cast<int64_t*>(value);
      }
      void setAggregatedFeeByRate( void *value , uint16_t length = 0) {
        if (length == 8)
          AggregatedFeeByRate = *static_cast<double*>(value);
        else if (length == 0)
          AggregatedFeeByRate = (double)*static_cast<int*>(value);
        else
          AggregatedFeeByRate = (double)*static_cast<int64_t*>(value);
      }
      void setAggregatedFee( void *value , uint16_t length = 0) {
        if (length == 8)
          AggregatedFee = *static_cast<double*>(value);
        else if (length == 0)
          AggregatedFee = (double)*static_cast<int*>(value);
        else
          AggregatedFee = (double)*static_cast<int64_t*>(value);
      }
      void setProfit( void *value , uint16_t length = 0) {
        if (length == 8)
          Profit = *static_cast<double*>(value);
        else if (length == 0)
          Profit = (double)*static_cast<int*>(value);
        else
          Profit = (double)*static_cast<int64_t*>(value);
      }
      void setEntryPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          EntryPrice = *static_cast<double*>(value);
        else if (length == 0)
          EntryPrice = (double)*static_cast<int*>(value);
        else
          EntryPrice = (double)*static_cast<int64_t*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]InstrumentID=[%s]TradingDay=[%s]AvgBuyPrice=[%f]AvgSellPrice=[%f]BuyQuantity=[%f]SellQuantity=[%f]TodayLong=[%f]TodayShort=[%f]NetPosition=[%f]Turnover=[%f]AggregatedFeeByRate=[%f]AggregatedFee=[%f]Profit=[%f]EntryPrice=[%f]",
                StrategyID,InstrumentID,TradingDay,AvgBuyPrice,AvgSellPrice,BuyQuantity,SellQuantity,TodayLong,TodayShort,NetPosition,Turnover,AggregatedFeeByRate,AggregatedFee,Profit,EntryPrice);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&StrategyInstrumentPNLDaily::setStrategyID, this, _1,_2);
        methodMap["InstrumentID"] = std::bind(&StrategyInstrumentPNLDaily::setInstrumentID, this, _1,_2);
        methodMap["TradingDay"] = std::bind(&StrategyInstrumentPNLDaily::setTradingDay, this, _1,_2);
        methodMap["AvgBuyPrice"] = std::bind(&StrategyInstrumentPNLDaily::setAvgBuyPrice, this, _1,_2);
        methodMap["AvgSellPrice"] = std::bind(&StrategyInstrumentPNLDaily::setAvgSellPrice, this, _1,_2);
        methodMap["BuyQuantity"] = std::bind(&StrategyInstrumentPNLDaily::setBuyQuantity, this, _1,_2);
        methodMap["SellQuantity"] = std::bind(&StrategyInstrumentPNLDaily::setSellQuantity, this, _1,_2);
        methodMap["TodayLong"] = std::bind(&StrategyInstrumentPNLDaily::setTodayLong, this, _1,_2);
        methodMap["TodayShort"] = std::bind(&StrategyInstrumentPNLDaily::setTodayShort, this, _1,_2);
        methodMap["NetPosition"] = std::bind(&StrategyInstrumentPNLDaily::setNetPosition, this, _1,_2);
        methodMap["Turnover"] = std::bind(&StrategyInstrumentPNLDaily::setTurnover, this, _1,_2);
        methodMap["AggregatedFeeByRate"] = std::bind(&StrategyInstrumentPNLDaily::setAggregatedFeeByRate, this, _1,_2);
        methodMap["AggregatedFee"] = std::bind(&StrategyInstrumentPNLDaily::setAggregatedFee, this, _1,_2);
        methodMap["Profit"] = std::bind(&StrategyInstrumentPNLDaily::setProfit, this, _1,_2);
        methodMap["EntryPrice"] = std::bind(&StrategyInstrumentPNLDaily::setEntryPrice, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        if (strlen(TradingDay) != 0){
          int length = strlen(TradingDay) < TradingDayLen ? strlen(TradingDay):TradingDayLen;
          doc.AddMember("TradingDay", spotrapidjson::Value().SetString(TradingDay,length), allocator);
        }
        if (!std::isnan(AvgBuyPrice))
          doc.AddMember("AvgBuyPrice",AvgBuyPrice, allocator);
        if (!std::isnan(AvgSellPrice))
          doc.AddMember("AvgSellPrice",AvgSellPrice, allocator);
        if (!std::isnan(BuyQuantity))
          doc.AddMember("BuyQuantity",BuyQuantity, allocator);
        if (!std::isnan(SellQuantity))
          doc.AddMember("SellQuantity",SellQuantity, allocator);
        if (!std::isnan(TodayLong))
          doc.AddMember("TodayLong",TodayLong, allocator);
        if (!std::isnan(TodayShort))
          doc.AddMember("TodayShort",TodayShort, allocator);
        if (!std::isnan(NetPosition))
          doc.AddMember("NetPosition",NetPosition, allocator);
        if (!std::isnan(Turnover))
          doc.AddMember("Turnover",Turnover, allocator);
        if (!std::isnan(AggregatedFeeByRate))
          doc.AddMember("AggregatedFeeByRate",AggregatedFeeByRate, allocator);
        if (!std::isnan(AggregatedFee))
          doc.AddMember("AggregatedFee",AggregatedFee, allocator);
        if (!std::isnan(Profit))
          doc.AddMember("Profit",Profit, allocator);
        if (!std::isnan(EntryPrice))
          doc.AddMember("EntryPrice",EntryPrice, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("StrategyInstrumentPNLDaily"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SymbolInfo
    {
    public:
      SymbolInfo()
      {
        memset(this,0,sizeof(SymbolInfo));
      }
      char Symbol[SymbolLen+1];
      char ExchangeCode[ExchangeCodeLen+1];
      int Multiplier;
      double Margin;
      char Type[TypeLen+1];
      double MaxOrderSize;
      double MinOrderSize;
      double CoinOrderSize;
      int MTaker;
      int LongShort;
      double Fragment;
      double MvRatio;
      char RefSymbol[RefSymbolLen+1];
      double Thresh;
      double PreTickSize;
      double QtyTickSize;
      double MaxDeltaLimit;
      double OpenThresh;
      double CloseThresh;
      int CloseFlag;
      double MinAmount;
      double PosAdj;

      void setSymbol( void *value , uint16_t length = 0) {
        memcpy(Symbol, static_cast<char*>(value), length <SymbolLen  ? length :SymbolLen);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setMultiplier( void *value , uint16_t length = 0) {
        Multiplier = *static_cast<int*>(value);
      }
      void setMargin( void *value , uint16_t length = 0) {
        if (length == 8)
          Margin = *static_cast<double*>(value);
        else if (length == 0)
          Margin = (double)*static_cast<int*>(value);
        else
          Margin = (double)*static_cast<int64_t*>(value);
      }
      void setType( void *value , uint16_t length = 0) {
        memcpy(Type, static_cast<char*>(value), length <TypeLen  ? length :TypeLen);
      }
      void setMaxOrderSize( void *value , uint16_t length = 0) {
        if (length == 8)
          MaxOrderSize = *static_cast<double*>(value);
        else if (length == 0)
          MaxOrderSize = (double)*static_cast<int*>(value);
        else
          MaxOrderSize = (double)*static_cast<int64_t*>(value);
      }
      void setMinOrderSize( void *value , uint16_t length = 0) {
        if (length == 8)
          MinOrderSize = *static_cast<double*>(value);
        else if (length == 0)
          MinOrderSize = (double)*static_cast<int*>(value);
        else
          MinOrderSize = (double)*static_cast<int64_t*>(value);
      }
      void setCoinOrderSize( void *value , uint16_t length = 0) {
        if (length == 8)
          CoinOrderSize = *static_cast<double*>(value);
        else if (length == 0)
          CoinOrderSize = (double)*static_cast<int*>(value);
        else
          CoinOrderSize = (double)*static_cast<int64_t*>(value);
      }
      void setMTaker( void *value , uint16_t length = 0) {
        MTaker = *static_cast<int*>(value);
      }
      void setLongShort( void *value , uint16_t length = 0) {
        LongShort = *static_cast<int*>(value);
      }
      void setFragment( void *value , uint16_t length = 0) {
        if (length == 8)
          Fragment = *static_cast<double*>(value);
        else if (length == 0)
          Fragment = (double)*static_cast<int*>(value);
        else
          Fragment = (double)*static_cast<int64_t*>(value);
      }
      void setMvRatio( void *value , uint16_t length = 0) {
        if (length == 8)
          MvRatio = *static_cast<double*>(value);
        else if (length == 0)
          MvRatio = (double)*static_cast<int*>(value);
        else
          MvRatio = (double)*static_cast<int64_t*>(value);
      }
      void setRefSymbol( void *value , uint16_t length = 0) {
        memcpy(RefSymbol, static_cast<char*>(value), length <RefSymbolLen  ? length :RefSymbolLen);
      }
      void setThresh( void *value , uint16_t length = 0) {
        if (length == 8)
          Thresh = *static_cast<double*>(value);
        else if (length == 0)
          Thresh = (double)*static_cast<int*>(value);
        else
          Thresh = (double)*static_cast<int64_t*>(value);
      }
      void setPreTickSize( void *value , uint16_t length = 0) {
        if (length == 8)
          PreTickSize = *static_cast<double*>(value);
        else if (length == 0)
          PreTickSize = (double)*static_cast<int*>(value);
        else
          PreTickSize = (double)*static_cast<int64_t*>(value);
      }
      void setQtyTickSize( void *value , uint16_t length = 0) {
        if (length == 8)
          QtyTickSize = *static_cast<double*>(value);
        else if (length == 0)
          QtyTickSize = (double)*static_cast<int*>(value);
        else
          QtyTickSize = (double)*static_cast<int64_t*>(value);
      }
      void setMaxDeltaLimit( void *value , uint16_t length = 0) {
        if (length == 8)
          MaxDeltaLimit = *static_cast<double*>(value);
        else if (length == 0)
          MaxDeltaLimit = (double)*static_cast<int*>(value);
        else
          MaxDeltaLimit = (double)*static_cast<int64_t*>(value);
      }
      void setOpenThresh( void *value , uint16_t length = 0) {
        if (length == 8)
          OpenThresh = *static_cast<double*>(value);
        else if (length == 0)
          OpenThresh = (double)*static_cast<int*>(value);
        else
          OpenThresh = (double)*static_cast<int64_t*>(value);
      }
      void setCloseThresh( void *value , uint16_t length = 0) {
        if (length == 8)
          CloseThresh = *static_cast<double*>(value);
        else if (length == 0)
          CloseThresh = (double)*static_cast<int*>(value);
        else
          CloseThresh = (double)*static_cast<int64_t*>(value);
      }
      void setCloseFlag( void *value , uint16_t length = 0) {
        CloseFlag = *static_cast<int*>(value);
      }
      void setMinAmount( void *value , uint16_t length = 0) {
        if (length == 8)
          MinAmount = *static_cast<double*>(value);
        else if (length == 0)
          MinAmount = (double)*static_cast<int*>(value);
        else
          MinAmount = (double)*static_cast<int64_t*>(value);
      }
      void setPosAdj( void *value , uint16_t length = 0) {
        if (length == 8)
          PosAdj = *static_cast<double*>(value);
        else if (length == 0)
          PosAdj = (double)*static_cast<int*>(value);
        else
          PosAdj = (double)*static_cast<int64_t*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "Symbol=[%s]ExchangeCode=[%s]Multiplier=[%d]Margin=[%f]Type=[%s]MaxOrderSize=[%f]MinOrderSize=[%f]CoinOrderSize=[%f]MTaker=[%d]LongShort=[%d]Fragment=[%f]MvRatio=[%f]RefSymbol=[%s]Thresh=[%f]PreTickSize=[%f]QtyTickSize=[%f]MaxDeltaLimit=[%f]OpenThresh=[%f]CloseThresh=[%f]CloseFlag=[%d]MinAmount=[%f]PosAdj=[%f]",
                Symbol,ExchangeCode,Multiplier,Margin,Type,MaxOrderSize,MinOrderSize,CoinOrderSize,MTaker,LongShort,Fragment,MvRatio,RefSymbol,Thresh,PreTickSize,QtyTickSize,MaxDeltaLimit,OpenThresh,CloseThresh,CloseFlag,MinAmount,PosAdj);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["Symbol"] = std::bind(&SymbolInfo::setSymbol, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&SymbolInfo::setExchangeCode, this, _1,_2);
        methodMap["Multiplier"] = std::bind(&SymbolInfo::setMultiplier, this, _1,_2);
        methodMap["Margin"] = std::bind(&SymbolInfo::setMargin, this, _1,_2);
        methodMap["Type"] = std::bind(&SymbolInfo::setType, this, _1,_2);
        methodMap["MaxOrderSize"] = std::bind(&SymbolInfo::setMaxOrderSize, this, _1,_2);
        methodMap["MinOrderSize"] = std::bind(&SymbolInfo::setMinOrderSize, this, _1,_2);
        methodMap["CoinOrderSize"] = std::bind(&SymbolInfo::setCoinOrderSize, this, _1,_2);
        methodMap["MTaker"] = std::bind(&SymbolInfo::setMTaker, this, _1,_2);
        methodMap["LongShort"] = std::bind(&SymbolInfo::setLongShort, this, _1,_2);
        methodMap["Fragment"] = std::bind(&SymbolInfo::setFragment, this, _1,_2);
        methodMap["MvRatio"] = std::bind(&SymbolInfo::setMvRatio, this, _1,_2);
        methodMap["RefSymbol"] = std::bind(&SymbolInfo::setRefSymbol, this, _1,_2);
        methodMap["Thresh"] = std::bind(&SymbolInfo::setThresh, this, _1,_2);
        methodMap["PreTickSize"] = std::bind(&SymbolInfo::setPreTickSize, this, _1,_2);
        methodMap["QtyTickSize"] = std::bind(&SymbolInfo::setQtyTickSize, this, _1,_2);
        methodMap["MaxDeltaLimit"] = std::bind(&SymbolInfo::setMaxDeltaLimit, this, _1,_2);
        methodMap["OpenThresh"] = std::bind(&SymbolInfo::setOpenThresh, this, _1,_2);
        methodMap["CloseThresh"] = std::bind(&SymbolInfo::setCloseThresh, this, _1,_2);
        methodMap["CloseFlag"] = std::bind(&SymbolInfo::setCloseFlag, this, _1,_2);
        methodMap["MinAmount"] = std::bind(&SymbolInfo::setMinAmount, this, _1,_2);
        methodMap["PosAdj"] = std::bind(&SymbolInfo::setPosAdj, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(Symbol) != 0){
          int length = strlen(Symbol) < SymbolLen ? strlen(Symbol):SymbolLen;
          doc.AddMember("Symbol", spotrapidjson::Value().SetString(Symbol,length), allocator);
        }
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        doc.AddMember("Multiplier",Multiplier, allocator);
        if (!std::isnan(Margin))
          doc.AddMember("Margin",Margin, allocator);
        if (strlen(Type) != 0){
          int length = strlen(Type) < TypeLen ? strlen(Type):TypeLen;
          doc.AddMember("Type", spotrapidjson::Value().SetString(Type,length), allocator);
        }
        if (!std::isnan(MaxOrderSize))
          doc.AddMember("MaxOrderSize",MaxOrderSize, allocator);
        if (!std::isnan(MinOrderSize))
          doc.AddMember("MinOrderSize",MinOrderSize, allocator);
        if (!std::isnan(CoinOrderSize))
          doc.AddMember("CoinOrderSize",CoinOrderSize, allocator);
        doc.AddMember("MTaker",MTaker, allocator);
        doc.AddMember("LongShort",LongShort, allocator);
        if (!std::isnan(Fragment))
          doc.AddMember("Fragment",Fragment, allocator);
        if (!std::isnan(MvRatio))
          doc.AddMember("MvRatio",MvRatio, allocator);
        if (strlen(RefSymbol) != 0){
          int length = strlen(RefSymbol) < RefSymbolLen ? strlen(RefSymbol):RefSymbolLen;
          doc.AddMember("RefSymbol", spotrapidjson::Value().SetString(RefSymbol,length), allocator);
        }
        if (!std::isnan(Thresh))
          doc.AddMember("Thresh",Thresh, allocator);
        if (!std::isnan(PreTickSize))
          doc.AddMember("PreTickSize",PreTickSize, allocator);
        if (!std::isnan(QtyTickSize))
          doc.AddMember("QtyTickSize",QtyTickSize, allocator);
        if (!std::isnan(MaxDeltaLimit))
          doc.AddMember("MaxDeltaLimit",MaxDeltaLimit, allocator);
        if (!std::isnan(OpenThresh))
          doc.AddMember("OpenThresh",OpenThresh, allocator);
        if (!std::isnan(CloseThresh))
          doc.AddMember("CloseThresh",CloseThresh, allocator);
        doc.AddMember("CloseFlag",CloseFlag, allocator);
        if (!std::isnan(MinAmount))
          doc.AddMember("MinAmount",MinAmount, allocator);
        if (!std::isnan(PosAdj))
          doc.AddMember("PosAdj",PosAdj, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SymbolInfo"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SymbolTradingFee
    {
    public:
      SymbolTradingFee()
      {
        memset(this,0,sizeof(SymbolTradingFee));
      }
      char Symbol[SymbolLen+1];
      double FeeRate;
      char FeeType[FeeTypeLen+1];
      char FeeFormat[FeeFormatLen+1];

      void setSymbol( void *value , uint16_t length = 0) {
        memcpy(Symbol, static_cast<char*>(value), length <SymbolLen  ? length :SymbolLen);
      }
      void setFeeRate( void *value , uint16_t length = 0) {
        if (length == 8)
          FeeRate = *static_cast<double*>(value);
        else if (length == 0)
          FeeRate = (double)*static_cast<int*>(value);
        else
          FeeRate = (double)*static_cast<int64_t*>(value);
      }
      void setFeeType( void *value , uint16_t length = 0) {
        memcpy(FeeType, static_cast<char*>(value), length <FeeTypeLen  ? length :FeeTypeLen);
      }
      void setFeeFormat( void *value , uint16_t length = 0) {
        memcpy(FeeFormat, static_cast<char*>(value), length <FeeFormatLen  ? length :FeeFormatLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "Symbol=[%s]FeeRate=[%f]FeeType=[%s]FeeFormat=[%s]",
                Symbol,FeeRate,FeeType,FeeFormat);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["Symbol"] = std::bind(&SymbolTradingFee::setSymbol, this, _1,_2);
        methodMap["FeeRate"] = std::bind(&SymbolTradingFee::setFeeRate, this, _1,_2);
        methodMap["FeeType"] = std::bind(&SymbolTradingFee::setFeeType, this, _1,_2);
        methodMap["FeeFormat"] = std::bind(&SymbolTradingFee::setFeeFormat, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(Symbol) != 0){
          int length = strlen(Symbol) < SymbolLen ? strlen(Symbol):SymbolLen;
          doc.AddMember("Symbol", spotrapidjson::Value().SetString(Symbol,length), allocator);
        }
        if (!std::isnan(FeeRate))
          doc.AddMember("FeeRate",FeeRate, allocator);
        if (strlen(FeeType) != 0){
          int length = strlen(FeeType) < FeeTypeLen ? strlen(FeeType):FeeTypeLen;
          doc.AddMember("FeeType", spotrapidjson::Value().SetString(FeeType,length), allocator);
        }
        if (strlen(FeeFormat) != 0){
          int length = strlen(FeeFormat) < FeeFormatLen ? strlen(FeeFormat):FeeFormatLen;
          doc.AddMember("FeeFormat", spotrapidjson::Value().SetString(FeeFormat,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SymbolTradingFee"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SymbolInstrument
    {
    public:
      SymbolInstrument()
      {
        memset(this,0,sizeof(SymbolInstrument));
      }
      char TradingDay[TradingDayLen+1];
      char Symbol[SymbolLen+1];
      char InstrumentID[InstrumentIDLen+1];
      char TradeableType[TradeableTypeLen+1];
      char StartTradingDay[StartTradingDayLen+1];
      char EndTradingDay[EndTradingDayLen+1];

      void setTradingDay( void *value , uint16_t length = 0) {
        memcpy(TradingDay, static_cast<char*>(value), length <TradingDayLen  ? length :TradingDayLen);
      }
      void setSymbol( void *value , uint16_t length = 0) {
        memcpy(Symbol, static_cast<char*>(value), length <SymbolLen  ? length :SymbolLen);
      }
      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setTradeableType( void *value , uint16_t length = 0) {
        memcpy(TradeableType, static_cast<char*>(value), length <TradeableTypeLen  ? length :TradeableTypeLen);
      }
      void setStartTradingDay( void *value , uint16_t length = 0) {
        memcpy(StartTradingDay, static_cast<char*>(value), length <StartTradingDayLen  ? length :StartTradingDayLen);
      }
      void setEndTradingDay( void *value , uint16_t length = 0) {
        memcpy(EndTradingDay, static_cast<char*>(value), length <EndTradingDayLen  ? length :EndTradingDayLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "TradingDay=[%s]Symbol=[%s]InstrumentID=[%s]TradeableType=[%s]StartTradingDay=[%s]EndTradingDay=[%s]",
                TradingDay,Symbol,InstrumentID,TradeableType,StartTradingDay,EndTradingDay);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["TradingDay"] = std::bind(&SymbolInstrument::setTradingDay, this, _1,_2);
        methodMap["Symbol"] = std::bind(&SymbolInstrument::setSymbol, this, _1,_2);
        methodMap["InstrumentID"] = std::bind(&SymbolInstrument::setInstrumentID, this, _1,_2);
        methodMap["TradeableType"] = std::bind(&SymbolInstrument::setTradeableType, this, _1,_2);
        methodMap["StartTradingDay"] = std::bind(&SymbolInstrument::setStartTradingDay, this, _1,_2);
        methodMap["EndTradingDay"] = std::bind(&SymbolInstrument::setEndTradingDay, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(TradingDay) != 0){
          int length = strlen(TradingDay) < TradingDayLen ? strlen(TradingDay):TradingDayLen;
          doc.AddMember("TradingDay", spotrapidjson::Value().SetString(TradingDay,length), allocator);
        }
        if (strlen(Symbol) != 0){
          int length = strlen(Symbol) < SymbolLen ? strlen(Symbol):SymbolLen;
          doc.AddMember("Symbol", spotrapidjson::Value().SetString(Symbol,length), allocator);
        }
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        if (strlen(TradeableType) != 0){
          int length = strlen(TradeableType) < TradeableTypeLen ? strlen(TradeableType):TradeableTypeLen;
          doc.AddMember("TradeableType", spotrapidjson::Value().SetString(TradeableType,length), allocator);
        }
        if (strlen(StartTradingDay) != 0){
          int length = strlen(StartTradingDay) < StartTradingDayLen ? strlen(StartTradingDay):StartTradingDayLen;
          doc.AddMember("StartTradingDay", spotrapidjson::Value().SetString(StartTradingDay,length), allocator);
        }
        if (strlen(EndTradingDay) != 0){
          int length = strlen(EndTradingDay) < EndTradingDayLen ? strlen(EndTradingDay):EndTradingDayLen;
          doc.AddMember("EndTradingDay", spotrapidjson::Value().SetString(EndTradingDay,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SymbolInstrument"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategyInfo
    {
    public:
      StrategyInfo()
      {
        memset(this,0,sizeof(StrategyInfo));
      }
      int StrategyID;
      char StrategyName[StrategyNameLen+1];

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setStrategyName( void *value , uint16_t length = 0) {
        memcpy(StrategyName, static_cast<char*>(value), length <StrategyNameLen  ? length :StrategyNameLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]StrategyName=[%s]",
                StrategyID,StrategyName);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&StrategyInfo::setStrategyID, this, _1,_2);
        methodMap["StrategyName"] = std::bind(&StrategyInfo::setStrategyName, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(StrategyName) != 0){
          int length = strlen(StrategyName) < StrategyNameLen ? strlen(StrategyName):StrategyNameLen;
          doc.AddMember("StrategyName", spotrapidjson::Value().SetString(StrategyName,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("Strategy"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategyParam
    {
    public:
      StrategyParam()
      {
        memset(this,0,sizeof(StrategyParam));
      }
      int StrategyID;
      char Name[NameLen+1];
      char ValueType[ValueTypeLen+1];
      double ValueDouble;
      int ValueInt;
      char ValueString[ValueStringLen+1];

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setName( void *value , uint16_t length = 0) {
        memcpy(Name, static_cast<char*>(value), length <NameLen  ? length :NameLen);
      }
      void setValueType( void *value , uint16_t length = 0) {
        memcpy(ValueType, static_cast<char*>(value), length <ValueTypeLen  ? length :ValueTypeLen);
      }
      void setValueDouble( void *value , uint16_t length = 0) {
        if (length == 8)
          ValueDouble = *static_cast<double*>(value);
        else if (length == 0)
          ValueDouble = (double)*static_cast<int*>(value);
        else
          ValueDouble = (double)*static_cast<int64_t*>(value);
      }
      void setValueInt( void *value , uint16_t length = 0) {
        ValueInt = *static_cast<int*>(value);
      }
      void setValueString( void *value , uint16_t length = 0) {
        memcpy(ValueString, static_cast<char*>(value), length <ValueStringLen  ? length :ValueStringLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]Name=[%s]ValueType=[%s]ValueDouble=[%f]ValueInt=[%d]ValueString=[%s]",
                StrategyID,Name,ValueType,ValueDouble,ValueInt,ValueString);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&StrategyParam::setStrategyID, this, _1,_2);
        methodMap["Name"] = std::bind(&StrategyParam::setName, this, _1,_2);
        methodMap["ValueType"] = std::bind(&StrategyParam::setValueType, this, _1,_2);
        methodMap["ValueDouble"] = std::bind(&StrategyParam::setValueDouble, this, _1,_2);
        methodMap["ValueInt"] = std::bind(&StrategyParam::setValueInt, this, _1,_2);
        methodMap["ValueString"] = std::bind(&StrategyParam::setValueString, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(Name) != 0){
          int length = strlen(Name) < NameLen ? strlen(Name):NameLen;
          doc.AddMember("Name", spotrapidjson::Value().SetString(Name,length), allocator);
        }
        if (strlen(ValueType) != 0){
          int length = strlen(ValueType) < ValueTypeLen ? strlen(ValueType):ValueTypeLen;
          doc.AddMember("ValueType", spotrapidjson::Value().SetString(ValueType,length), allocator);
        }
        if (!std::isnan(ValueDouble))
          doc.AddMember("ValueDouble",ValueDouble, allocator);
        doc.AddMember("ValueInt",ValueInt, allocator);
        if (strlen(ValueString) != 0){
          int length = strlen(ValueString) < ValueStringLen ? strlen(ValueString):ValueStringLen;
          doc.AddMember("ValueString", spotrapidjson::Value().SetString(ValueString,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("StrategyParam"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class Order
    {
    public:
      Order()
      {
        memset(this,0,sizeof(Order));
      }
      int StrategyID;
      char Direction;
      char Offset;
      char OrderStatus;
      int OrderType;
      int OrderRef;
      double LimitPrice;
      double VolumeTotalOriginal;
      double VolumeRemained;
      char InstrumentID[InstrumentIDLen+1];
      char OrderMsgType;
      char ExchangeCode[ExchangeCodeLen+1];
      char CounterType[CounterTypeLen+1];
      char OrderSysID[OrderSysIDLen+1];
      uint64_t TradeID;
      char InsertTime[InsertTimeLen+1];
      char UpdateTime[UpdateTimeLen+1];
      char CancelTime[CancelTimeLen+1];
      char TradingDay[TradingDayLen+1];
      int CancelAttempts;
      double Volume;
      double VolumeFilled;
      double Price;
      int OrdRejReason;
      double FeeRate;
      uint64_t TimeStamp;
      char UserID[UserIDLen+1];
      char StatusMsg[StatusMsgLen+1];
      uint64_t EpochTimeReturn;
      uint64_t EpochTimeReqBefore;
      uint64_t EpochTimeReqAfter;
      char TimeInForce[TimeInForceLen+1];
      double AvgPrice;
      char Category[CategoryLen+1];
      char MTaker[MTakerLen+1];
      double Fee;
      int QryFlag;
      char CoinType[CoinTypeLen+1];
      char StrategyType[StrategyTypeLen+1];

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setDirection( void *value , uint16_t length = 0) {
        Direction = *static_cast<char*>(value);
      }
      void setOffset( void *value , uint16_t length = 0) {
        Offset = *static_cast<char*>(value);
      }
      void setOrderStatus( void *value , uint16_t length = 0) {
        OrderStatus = *static_cast<char*>(value);
      }
      void setOrderType( void *value , uint16_t length = 0) {
        OrderType = *static_cast<int*>(value);
      }
      void setOrderRef( void *value , uint16_t length = 0) {
        OrderRef = *static_cast<int*>(value);
      }
      void setLimitPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          LimitPrice = *static_cast<double*>(value);
        else if (length == 0)
          LimitPrice = (double)*static_cast<int*>(value);
        else
          LimitPrice = (double)*static_cast<int64_t*>(value);
      }
      void setVolumeTotalOriginal( void *value , uint16_t length = 0) {
        if (length == 8)
          VolumeTotalOriginal = *static_cast<double*>(value);
        else if (length == 0)
          VolumeTotalOriginal = (double)*static_cast<int*>(value);
        else
          VolumeTotalOriginal = (double)*static_cast<int64_t*>(value);
      }
      void setVolumeRemained( void *value , uint16_t length = 0) {
        if (length == 8)
          VolumeRemained = *static_cast<double*>(value);
        else if (length == 0)
          VolumeRemained = (double)*static_cast<int*>(value);
        else
          VolumeRemained = (double)*static_cast<int64_t*>(value);
      }
      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setOrderMsgType( void *value , uint16_t length = 0) {
        OrderMsgType = *static_cast<char*>(value);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setCounterType( void *value , uint16_t length = 0) {
        memcpy(CounterType, static_cast<char*>(value), length <CounterTypeLen  ? length :CounterTypeLen);
      }
      void setOrderSysID( void *value , uint16_t length = 0) {
        memcpy(OrderSysID, static_cast<char*>(value), length <OrderSysIDLen  ? length :OrderSysIDLen);
      }
      void setTradeID( void *value , uint16_t length = 0) {
        if (length == 0)
          TradeID = *static_cast<int*>(value);
        else
          TradeID = *static_cast<uint64_t*>(value);
      }
      void setInsertTime( void *value , uint16_t length = 0) {
        memcpy(InsertTime, static_cast<char*>(value), length <InsertTimeLen  ? length :InsertTimeLen);
      }
      void setUpdateTime( void *value , uint16_t length = 0) {
        memcpy(UpdateTime, static_cast<char*>(value), length <UpdateTimeLen  ? length :UpdateTimeLen);
      }
      void setCancelTime( void *value , uint16_t length = 0) {
        memcpy(CancelTime, static_cast<char*>(value), length <CancelTimeLen  ? length :CancelTimeLen);
      }
      void setTradingDay( void *value , uint16_t length = 0) {
        memcpy(TradingDay, static_cast<char*>(value), length <TradingDayLen  ? length :TradingDayLen);
      }
      void setCancelAttempts( void *value , uint16_t length = 0) {
        CancelAttempts = *static_cast<int*>(value);
      }
      void setVolume( void *value , uint16_t length = 0) {
        if (length == 8)
          Volume = *static_cast<double*>(value);
        else if (length == 0)
          Volume = (double)*static_cast<int*>(value);
        else
          Volume = (double)*static_cast<int64_t*>(value);
      }
      void setVolumeFilled( void *value , uint16_t length = 0) {
        if (length == 8)
          VolumeFilled = *static_cast<double*>(value);
        else if (length == 0)
          VolumeFilled = (double)*static_cast<int*>(value);
        else
          VolumeFilled = (double)*static_cast<int64_t*>(value);
      }
      void setPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          Price = *static_cast<double*>(value);
        else if (length == 0)
          Price = (double)*static_cast<int*>(value);
        else
          Price = (double)*static_cast<int64_t*>(value);
      }
      void setOrdRejReason( void *value , uint16_t length = 0) {
        OrdRejReason = *static_cast<int*>(value);
      }
      void setFeeRate( void *value , uint16_t length = 0) {
        if (length == 8)
          FeeRate = *static_cast<double*>(value);
        else if (length == 0)
          FeeRate = (double)*static_cast<int*>(value);
        else
          FeeRate = (double)*static_cast<int64_t*>(value);
      }
      void setTimeStamp( void *value , uint16_t length = 0) {
        if (length == 0)
          TimeStamp = *static_cast<int*>(value);
        else
          TimeStamp = *static_cast<uint64_t*>(value);
      }
      void setUserID( void *value , uint16_t length = 0) {
        memcpy(UserID, static_cast<char*>(value), length <UserIDLen  ? length :UserIDLen);
      }
      void setStatusMsg( void *value , uint16_t length = 0) {
        memcpy(StatusMsg, static_cast<char*>(value), length <StatusMsgLen  ? length :StatusMsgLen);
      }
      void setEpochTimeReturn( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTimeReturn = *static_cast<int*>(value);
        else
          EpochTimeReturn = *static_cast<uint64_t*>(value);
      }
      void setEpochTimeReqBefore( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTimeReqBefore = *static_cast<int*>(value);
        else
          EpochTimeReqBefore = *static_cast<uint64_t*>(value);
      }
      void setEpochTimeReqAfter( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTimeReqAfter = *static_cast<int*>(value);
        else
          EpochTimeReqAfter = *static_cast<uint64_t*>(value);
      }
      void setTimeInForce( void *value , uint16_t length = 0) {
        memcpy(TimeInForce, static_cast<char*>(value), length <TimeInForceLen  ? length :TimeInForceLen);
      }
      void setAvgPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          AvgPrice = *static_cast<double*>(value);
        else if (length == 0)
          AvgPrice = (double)*static_cast<int*>(value);
        else
          AvgPrice = (double)*static_cast<int64_t*>(value);
      }
      void setCategory( void *value , uint16_t length = 0) {
        memcpy(Category, static_cast<char*>(value), length <CategoryLen  ? length :CategoryLen);
      }
      void setMTaker( void *value , uint16_t length = 0) {
        memcpy(MTaker, static_cast<char*>(value), length <MTakerLen  ? length :MTakerLen);
      }
      void setFee( void *value , uint16_t length = 0) {
        if (length == 8)
          Fee = *static_cast<double*>(value);
        else if (length == 0)
          Fee = (double)*static_cast<int*>(value);
        else
          Fee = (double)*static_cast<int64_t*>(value);
      }
      void setQryFlag( void *value , uint16_t length = 0) {
        QryFlag = *static_cast<int*>(value);
      }
      void setCoinType( void *value , uint16_t length = 0) {
        memcpy(CoinType, static_cast<char*>(value), length <CoinTypeLen  ? length :CoinTypeLen);
      }
      void setStrategyType( void *value , uint16_t length = 0) {
        memcpy(StrategyType, static_cast<char*>(value), length <StrategyTypeLen  ? length :StrategyTypeLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]Direction=[%c]Offset=[%c]OrderStatus=[%c]OrderType=[%d]OrderRef=[%d]LimitPrice=[%f]VolumeTotalOriginal=[%f]VolumeRemained=[%f]InstrumentID=[%s]OrderMsgType=[%c]ExchangeCode=[%s]CounterType=[%s]OrderSysID=[%s]TradeID=[%I64d]InsertTime=[%s]UpdateTime=[%s]CancelTime=[%s]TradingDay=[%s]CancelAttempts=[%d]Volume=[%f]VolumeFilled=[%f]Price=[%f]OrdRejReason=[%d]FeeRate=[%f]TimeStamp=[%I64d]UserID=[%s]StatusMsg=[%s]EpochTimeReturn=[%I64d]EpochTimeReqBefore=[%I64d]EpochTimeReqAfter=[%I64d]TimeInForce=[%s]AvgPrice=[%f]Category=[%s]MTaker=[%s]Fee=[%f]QryFlag=[%d]CoinType=[%s]StrategyType=[%s]",
                StrategyID,Direction,Offset,OrderStatus,OrderType,OrderRef,LimitPrice,VolumeTotalOriginal,VolumeRemained,InstrumentID,OrderMsgType,ExchangeCode,CounterType,OrderSysID,TradeID,InsertTime,UpdateTime,CancelTime,TradingDay,CancelAttempts,Volume,VolumeFilled,Price,OrdRejReason,FeeRate,TimeStamp,UserID,StatusMsg,EpochTimeReturn,EpochTimeReqBefore,EpochTimeReqAfter,TimeInForce,AvgPrice,Category,MTaker,Fee,QryFlag,CoinType,StrategyType);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&Order::setStrategyID, this, _1,_2);
        methodMap["Direction"] = std::bind(&Order::setDirection, this, _1,_2);
        methodMap["Offset"] = std::bind(&Order::setOffset, this, _1,_2);
        methodMap["OrderStatus"] = std::bind(&Order::setOrderStatus, this, _1,_2);
        methodMap["OrderType"] = std::bind(&Order::setOrderType, this, _1,_2);
        methodMap["OrderRef"] = std::bind(&Order::setOrderRef, this, _1,_2);
        methodMap["LimitPrice"] = std::bind(&Order::setLimitPrice, this, _1,_2);
        methodMap["VolumeTotalOriginal"] = std::bind(&Order::setVolumeTotalOriginal, this, _1,_2);
        methodMap["VolumeRemained"] = std::bind(&Order::setVolumeRemained, this, _1,_2);
        methodMap["InstrumentID"] = std::bind(&Order::setInstrumentID, this, _1,_2);
        methodMap["OrderMsgType"] = std::bind(&Order::setOrderMsgType, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&Order::setExchangeCode, this, _1,_2);
        methodMap["CounterType"] = std::bind(&Order::setCounterType, this, _1,_2);
        methodMap["OrderSysID"] = std::bind(&Order::setOrderSysID, this, _1,_2);
        methodMap["TradeID"] = std::bind(&Order::setTradeID, this, _1,_2);
        methodMap["InsertTime"] = std::bind(&Order::setInsertTime, this, _1,_2);
        methodMap["UpdateTime"] = std::bind(&Order::setUpdateTime, this, _1,_2);
        methodMap["CancelTime"] = std::bind(&Order::setCancelTime, this, _1,_2);
        methodMap["TradingDay"] = std::bind(&Order::setTradingDay, this, _1,_2);
        methodMap["CancelAttempts"] = std::bind(&Order::setCancelAttempts, this, _1,_2);
        methodMap["Volume"] = std::bind(&Order::setVolume, this, _1,_2);
        methodMap["VolumeFilled"] = std::bind(&Order::setVolumeFilled, this, _1,_2);
        methodMap["Price"] = std::bind(&Order::setPrice, this, _1,_2);
        methodMap["OrdRejReason"] = std::bind(&Order::setOrdRejReason, this, _1,_2);
        methodMap["FeeRate"] = std::bind(&Order::setFeeRate, this, _1,_2);
        methodMap["TimeStamp"] = std::bind(&Order::setTimeStamp, this, _1,_2);
        methodMap["UserID"] = std::bind(&Order::setUserID, this, _1,_2);
        methodMap["StatusMsg"] = std::bind(&Order::setStatusMsg, this, _1,_2);
        methodMap["EpochTimeReturn"] = std::bind(&Order::setEpochTimeReturn, this, _1,_2);
        methodMap["EpochTimeReqBefore"] = std::bind(&Order::setEpochTimeReqBefore, this, _1,_2);
        methodMap["EpochTimeReqAfter"] = std::bind(&Order::setEpochTimeReqAfter, this, _1,_2);
        methodMap["TimeInForce"] = std::bind(&Order::setTimeInForce, this, _1,_2);
        methodMap["AvgPrice"] = std::bind(&Order::setAvgPrice, this, _1,_2);
        methodMap["Category"] = std::bind(&Order::setCategory, this, _1,_2);
        methodMap["MTaker"] = std::bind(&Order::setMTaker, this, _1,_2);
        methodMap["Fee"] = std::bind(&Order::setFee, this, _1,_2);
        methodMap["QryFlag"] = std::bind(&Order::setQryFlag, this, _1,_2);
        methodMap["CoinType"] = std::bind(&Order::setCoinType, this, _1,_2);
        methodMap["StrategyType"] = std::bind(&Order::setStrategyType, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        doc.AddMember("Direction", spotrapidjson::Value().SetString(&Direction, 1), allocator);
        doc.AddMember("Offset", spotrapidjson::Value().SetString(&Offset, 1), allocator);
        doc.AddMember("OrderStatus", spotrapidjson::Value().SetString(&OrderStatus, 1), allocator);
        doc.AddMember("OrderType",OrderType, allocator);
        doc.AddMember("OrderRef",OrderRef, allocator);
        if (!std::isnan(LimitPrice))
          doc.AddMember("LimitPrice",LimitPrice, allocator);
        if (!std::isnan(VolumeTotalOriginal))
          doc.AddMember("VolumeTotalOriginal",VolumeTotalOriginal, allocator);
        if (!std::isnan(VolumeRemained))
          doc.AddMember("VolumeRemained",VolumeRemained, allocator);
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        doc.AddMember("OrderMsgType", spotrapidjson::Value().SetString(&OrderMsgType, 1), allocator);
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        if (strlen(CounterType) != 0){
          int length = strlen(CounterType) < CounterTypeLen ? strlen(CounterType):CounterTypeLen;
          doc.AddMember("CounterType", spotrapidjson::Value().SetString(CounterType,length), allocator);
        }
        if (strlen(OrderSysID) != 0){
          int length = strlen(OrderSysID) < OrderSysIDLen ? strlen(OrderSysID):OrderSysIDLen;
          doc.AddMember("OrderSysID", spotrapidjson::Value().SetString(OrderSysID,length), allocator);
        }
        doc.AddMember("TradeID",TradeID, allocator);
        if (strlen(InsertTime) != 0){
          int length = strlen(InsertTime) < InsertTimeLen ? strlen(InsertTime):InsertTimeLen;
          doc.AddMember("InsertTime", spotrapidjson::Value().SetString(InsertTime,length), allocator);
        }
        if (strlen(UpdateTime) != 0){
          int length = strlen(UpdateTime) < UpdateTimeLen ? strlen(UpdateTime):UpdateTimeLen;
          doc.AddMember("UpdateTime", spotrapidjson::Value().SetString(UpdateTime,length), allocator);
        }
        if (strlen(CancelTime) != 0){
          int length = strlen(CancelTime) < CancelTimeLen ? strlen(CancelTime):CancelTimeLen;
          doc.AddMember("CancelTime", spotrapidjson::Value().SetString(CancelTime,length), allocator);
        }
        if (strlen(TradingDay) != 0){
          int length = strlen(TradingDay) < TradingDayLen ? strlen(TradingDay):TradingDayLen;
          doc.AddMember("TradingDay", spotrapidjson::Value().SetString(TradingDay,length), allocator);
        }
        doc.AddMember("CancelAttempts",CancelAttempts, allocator);
        if (!std::isnan(Volume))
          doc.AddMember("Volume",Volume, allocator);
        if (!std::isnan(VolumeFilled))
          doc.AddMember("VolumeFilled",VolumeFilled, allocator);
        if (!std::isnan(Price))
          doc.AddMember("Price",Price, allocator);
        doc.AddMember("OrdRejReason",OrdRejReason, allocator);
        if (!std::isnan(FeeRate))
          doc.AddMember("FeeRate",FeeRate, allocator);
        doc.AddMember("TimeStamp",TimeStamp, allocator);
        if (strlen(UserID) != 0){
          int length = strlen(UserID) < UserIDLen ? strlen(UserID):UserIDLen;
          doc.AddMember("UserID", spotrapidjson::Value().SetString(UserID,length), allocator);
        }
        if (strlen(StatusMsg) != 0){
          int length = strlen(StatusMsg) < StatusMsgLen ? strlen(StatusMsg):StatusMsgLen;
          doc.AddMember("StatusMsg", spotrapidjson::Value().SetString(StatusMsg,length), allocator);
        }
        doc.AddMember("EpochTimeReturn",EpochTimeReturn, allocator);
        doc.AddMember("EpochTimeReqBefore",EpochTimeReqBefore, allocator);
        doc.AddMember("EpochTimeReqAfter",EpochTimeReqAfter, allocator);
        if (strlen(TimeInForce) != 0){
          int length = strlen(TimeInForce) < TimeInForceLen ? strlen(TimeInForce):TimeInForceLen;
          doc.AddMember("TimeInForce", spotrapidjson::Value().SetString(TimeInForce,length), allocator);
        }
        if (!std::isnan(AvgPrice))
          doc.AddMember("AvgPrice",AvgPrice, allocator);
        if (strlen(Category) != 0){
          int length = strlen(Category) < CategoryLen ? strlen(Category):CategoryLen;
          doc.AddMember("Category", spotrapidjson::Value().SetString(Category,length), allocator);
        }
        if (strlen(MTaker) != 0){
          int length = strlen(MTaker) < MTakerLen ? strlen(MTaker):MTakerLen;
          doc.AddMember("MTaker", spotrapidjson::Value().SetString(MTaker,length), allocator);
        }
        if (!std::isnan(Fee))
          doc.AddMember("Fee",Fee, allocator);
        doc.AddMember("QryFlag",QryFlag, allocator);
        if (strlen(CoinType) != 0){
          int length = strlen(CoinType) < CoinTypeLen ? strlen(CoinType):CoinTypeLen;
          doc.AddMember("CoinType", spotrapidjson::Value().SetString(CoinType,length), allocator);
        }
        if (strlen(StrategyType) != 0){
          int length = strlen(StrategyType) < StrategyTypeLen ? strlen(StrategyType):StrategyTypeLen;
          doc.AddMember("StrategyType", spotrapidjson::Value().SetString(StrategyType,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("Order"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategySymbol
    {
    public:
      StrategySymbol()
      {
        memset(this,0,sizeof(StrategySymbol));
      }
      int StrategyID;
      char Symbol[SymbolLen+1];

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setSymbol( void *value , uint16_t length = 0) {
        memcpy(Symbol, static_cast<char*>(value), length <SymbolLen  ? length :SymbolLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]Symbol=[%s]",
                StrategyID,Symbol);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&StrategySymbol::setStrategyID, this, _1,_2);
        methodMap["Symbol"] = std::bind(&StrategySymbol::setSymbol, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(Symbol) != 0){
          int length = strlen(Symbol) < SymbolLen ? strlen(Symbol):SymbolLen;
          doc.AddMember("Symbol", spotrapidjson::Value().SetString(Symbol,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("StrategySymbol"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class LogInfo
    {
    public:
      LogInfo()
      {
        memset(this,0,sizeof(LogInfo));
      }
      char LogTime[LogTimeLen+1];
      char LogType[LogTypeLen+1];
      uint64_t EpochTime;
      char LogText[LogTextLen+1];

      void setLogTime( void *value , uint16_t length = 0) {
        memcpy(LogTime, static_cast<char*>(value), length <LogTimeLen  ? length :LogTimeLen);
      }
      void setLogType( void *value , uint16_t length = 0) {
        memcpy(LogType, static_cast<char*>(value), length <LogTypeLen  ? length :LogTypeLen);
      }
      void setEpochTime( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTime = *static_cast<int*>(value);
        else
          EpochTime = *static_cast<uint64_t*>(value);
      }
      void setLogText( void *value , uint16_t length = 0) {
        memcpy(LogText, static_cast<char*>(value), length <LogTextLen  ? length :LogTextLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "LogTime=[%s]LogType=[%s]EpochTime=[%I64d]LogText=[%s]",
                LogTime,LogType,EpochTime,LogText);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["LogTime"] = std::bind(&LogInfo::setLogTime, this, _1,_2);
        methodMap["LogType"] = std::bind(&LogInfo::setLogType, this, _1,_2);
        methodMap["EpochTime"] = std::bind(&LogInfo::setEpochTime, this, _1,_2);
        methodMap["LogText"] = std::bind(&LogInfo::setLogText, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(LogTime) != 0){
          int length = strlen(LogTime) < LogTimeLen ? strlen(LogTime):LogTimeLen;
          doc.AddMember("LogTime", spotrapidjson::Value().SetString(LogTime,length), allocator);
        }
        if (strlen(LogType) != 0){
          int length = strlen(LogType) < LogTypeLen ? strlen(LogType):LogTypeLen;
          doc.AddMember("LogType", spotrapidjson::Value().SetString(LogType,length), allocator);
        }
        doc.AddMember("EpochTime",EpochTime, allocator);
        if (strlen(LogText) != 0){
          int length = strlen(LogText) < LogTextLen ? strlen(LogText):LogTextLen;
          doc.AddMember("LogText", spotrapidjson::Value().SetString(LogText,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("LogInfo"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategyFinish
    {
    public:
      StrategyFinish()
      {
        memset(this,0,sizeof(StrategyFinish));
      }

      string toJson() const {
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("InitFinish"), outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class HeartBeat
    {
    public:
      HeartBeat()
      {
        memset(this,0,sizeof(HeartBeat));
      }

      string toJson() const {
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("Heartbeat"), outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SpotStrategy
    {
    public:
      SpotStrategy()
      {
        memset(this,0,sizeof(SpotStrategy));
      }
      int SpotID;
      int StrategyID;

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]StrategyID=[%d]",
                SpotID,StrategyID);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&SpotStrategy::setSpotID, this, _1,_2);
        methodMap["StrategyID"] = std::bind(&SpotStrategy::setStrategyID, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        doc.AddMember("StrategyID",StrategyID, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SpotStrategy"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class MetricData
    {
    public:
      MetricData()
      {
        memset(this,0,sizeof(MetricData));
      }
      int KeyID;
      int StrategyID;
      char ExchangeCode[ExchangeCodeLen+1];
      char LastPoint[LastPointLen+1];
      char CurrPoint[CurrPointLen+1];
      uint64_t EpochTime;
      int EpochTimeDiff;
      char InterfaceType[InterfaceTypeLen+1];

      void setKeyID( void *value , uint16_t length = 0) {
        KeyID = *static_cast<int*>(value);
      }
      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setLastPoint( void *value , uint16_t length = 0) {
        memcpy(LastPoint, static_cast<char*>(value), length <LastPointLen  ? length :LastPointLen);
      }
      void setCurrPoint( void *value , uint16_t length = 0) {
        memcpy(CurrPoint, static_cast<char*>(value), length <CurrPointLen  ? length :CurrPointLen);
      }
      void setEpochTime( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTime = *static_cast<int*>(value);
        else
          EpochTime = *static_cast<uint64_t*>(value);
      }
      void setEpochTimeDiff( void *value , uint16_t length = 0) {
        EpochTimeDiff = *static_cast<int*>(value);
      }
      void setInterfaceType( void *value , uint16_t length = 0) {
        memcpy(InterfaceType, static_cast<char*>(value), length <InterfaceTypeLen  ? length :InterfaceTypeLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "KeyID=[%d]StrategyID=[%d]ExchangeCode=[%s]LastPoint=[%s]CurrPoint=[%s]EpochTime=[%I64d]EpochTimeDiff=[%d]InterfaceType=[%s]",
                KeyID,StrategyID,ExchangeCode,LastPoint,CurrPoint,EpochTime,EpochTimeDiff,InterfaceType);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["KeyID"] = std::bind(&MetricData::setKeyID, this, _1,_2);
        methodMap["StrategyID"] = std::bind(&MetricData::setStrategyID, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&MetricData::setExchangeCode, this, _1,_2);
        methodMap["LastPoint"] = std::bind(&MetricData::setLastPoint, this, _1,_2);
        methodMap["CurrPoint"] = std::bind(&MetricData::setCurrPoint, this, _1,_2);
        methodMap["EpochTime"] = std::bind(&MetricData::setEpochTime, this, _1,_2);
        methodMap["EpochTimeDiff"] = std::bind(&MetricData::setEpochTimeDiff, this, _1,_2);
        methodMap["InterfaceType"] = std::bind(&MetricData::setInterfaceType, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("KeyID",KeyID, allocator);
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        if (strlen(LastPoint) != 0){
          int length = strlen(LastPoint) < LastPointLen ? strlen(LastPoint):LastPointLen;
          doc.AddMember("LastPoint", spotrapidjson::Value().SetString(LastPoint,length), allocator);
        }
        if (strlen(CurrPoint) != 0){
          int length = strlen(CurrPoint) < CurrPointLen ? strlen(CurrPoint):CurrPointLen;
          doc.AddMember("CurrPoint", spotrapidjson::Value().SetString(CurrPoint,length), allocator);
        }
        doc.AddMember("EpochTime",EpochTime, allocator);
        doc.AddMember("EpochTimeDiff",EpochTimeDiff, allocator);
        if (strlen(InterfaceType) != 0){
          int length = strlen(InterfaceType) < InterfaceTypeLen ? strlen(InterfaceType):InterfaceTypeLen;
          doc.AddMember("InterfaceType", spotrapidjson::Value().SetString(InterfaceType,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("MetricData"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SpotInitConfig
    {
    public:
      SpotInitConfig()
      {
        memset(this,0,sizeof(SpotInitConfig));
      }
      int SpotID;
      char FieldName[FieldNameLen+1];
      char FieldValue[FieldValueLen+1];
      char FieldType[FieldTypeLen+1];

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setFieldName( void *value , uint16_t length = 0) {
        memcpy(FieldName, static_cast<char*>(value), length <FieldNameLen  ? length :FieldNameLen);
      }
      void setFieldValue( void *value , uint16_t length = 0) {
        memcpy(FieldValue, static_cast<char*>(value), length <FieldValueLen  ? length :FieldValueLen);
      }
      void setFieldType( void *value , uint16_t length = 0) {
        memcpy(FieldType, static_cast<char*>(value), length <FieldTypeLen  ? length :FieldTypeLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]FieldName=[%s]FieldValue=[%s]FieldType=[%s]",
                SpotID,FieldName,FieldValue,FieldType);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&SpotInitConfig::setSpotID, this, _1,_2);
        methodMap["FieldName"] = std::bind(&SpotInitConfig::setFieldName, this, _1,_2);
        methodMap["FieldValue"] = std::bind(&SpotInitConfig::setFieldValue, this, _1,_2);
        methodMap["FieldType"] = std::bind(&SpotInitConfig::setFieldType, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        if (strlen(FieldName) != 0){
          int length = strlen(FieldName) < FieldNameLen ? strlen(FieldName):FieldNameLen;
          doc.AddMember("FieldName", spotrapidjson::Value().SetString(FieldName,length), allocator);
        }
        if (strlen(FieldValue) != 0){
          int length = strlen(FieldValue) < FieldValueLen ? strlen(FieldValue):FieldValueLen;
          doc.AddMember("FieldValue", spotrapidjson::Value().SetString(FieldValue,length), allocator);
        }
        if (strlen(FieldType) != 0){
          int length = strlen(FieldType) < FieldTypeLen ? strlen(FieldType):FieldTypeLen;
          doc.AddMember("FieldType", spotrapidjson::Value().SetString(FieldType,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SpotInitConfig"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class MarketData
    {
    public:
      MarketData()
      {
        memset(this,0,sizeof(MarketData));
      }
      char TradingDay[TradingDayLen+1];
      char InstrumentID[InstrumentIDLen+1];
      char ExchangeCode[ExchangeCodeLen+1];
      double LastPrice;
      double HighestPrice;
      double LowestPrice;
      double Volume;
      double Turnover;
      double BidPrice1;
      double BidVolume1;
      double AskPrice1;
      double AskVolume1;
      double BidPrice2;
      double BidVolume2;
      double AskPrice2;
      double AskVolume2;
      double BidPrice3;
      double BidVolume3;
      double AskPrice3;
      double AskVolume3;
      double BidPrice4;
      double BidVolume4;
      double AskPrice4;
      double AskVolume4;
      double BidPrice5;
      double BidVolume5;
      double AskPrice5;
      double AskVolume5;
      char UpdateTime[UpdateTimeLen+1];
      uint64_t UpdateMillisec;
      int MessageID;
      uint64_t EpochTime;
      uint64_t LastSeqNum;

      void setTradingDay( void *value , uint16_t length = 0) {
        memcpy(TradingDay, static_cast<char*>(value), length <TradingDayLen  ? length :TradingDayLen);
      }
      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setLastPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          LastPrice = *static_cast<double*>(value);
        else if (length == 0)
          LastPrice = (double)*static_cast<int*>(value);
        else
          LastPrice = (double)*static_cast<int64_t*>(value);
      }
      void setHighestPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          HighestPrice = *static_cast<double*>(value);
        else if (length == 0)
          HighestPrice = (double)*static_cast<int*>(value);
        else
          HighestPrice = (double)*static_cast<int64_t*>(value);
      }
      void setLowestPrice( void *value , uint16_t length = 0) {
        if (length == 8)
          LowestPrice = *static_cast<double*>(value);
        else if (length == 0)
          LowestPrice = (double)*static_cast<int*>(value);
        else
          LowestPrice = (double)*static_cast<int64_t*>(value);
      }
      void setVolume( void *value , uint16_t length = 0) {
        if (length == 8)
          Volume = *static_cast<double*>(value);
        else if (length == 0)
          Volume = (double)*static_cast<int*>(value);
        else
          Volume = (double)*static_cast<int64_t*>(value);
      }
      void setTurnover( void *value , uint16_t length = 0) {
        if (length == 8)
          Turnover = *static_cast<double*>(value);
        else if (length == 0)
          Turnover = (double)*static_cast<int*>(value);
        else
          Turnover = (double)*static_cast<int64_t*>(value);
      }
      void setBidPrice1( void *value , uint16_t length = 0) {
        if (length == 8)
          BidPrice1 = *static_cast<double*>(value);
        else if (length == 0)
          BidPrice1 = (double)*static_cast<int*>(value);
        else
          BidPrice1 = (double)*static_cast<int64_t*>(value);
      }
      void setBidVolume1( void *value , uint16_t length = 0) {
        if (length == 8)
          BidVolume1 = *static_cast<double*>(value);
        else if (length == 0)
          BidVolume1 = (double)*static_cast<int*>(value);
        else
          BidVolume1 = (double)*static_cast<int64_t*>(value);
      }
      void setAskPrice1( void *value , uint16_t length = 0) {
        if (length == 8)
          AskPrice1 = *static_cast<double*>(value);
        else if (length == 0)
          AskPrice1 = (double)*static_cast<int*>(value);
        else
          AskPrice1 = (double)*static_cast<int64_t*>(value);
      }
      void setAskVolume1( void *value , uint16_t length = 0) {
        if (length == 8)
          AskVolume1 = *static_cast<double*>(value);
        else if (length == 0)
          AskVolume1 = (double)*static_cast<int*>(value);
        else
          AskVolume1 = (double)*static_cast<int64_t*>(value);
      }
      void setBidPrice2( void *value , uint16_t length = 0) {
        if (length == 8)
          BidPrice2 = *static_cast<double*>(value);
        else if (length == 0)
          BidPrice2 = (double)*static_cast<int*>(value);
        else
          BidPrice2 = (double)*static_cast<int64_t*>(value);
      }
      void setBidVolume2( void *value , uint16_t length = 0) {
        if (length == 8)
          BidVolume2 = *static_cast<double*>(value);
        else if (length == 0)
          BidVolume2 = (double)*static_cast<int*>(value);
        else
          BidVolume2 = (double)*static_cast<int64_t*>(value);
      }
      void setAskPrice2( void *value , uint16_t length = 0) {
        if (length == 8)
          AskPrice2 = *static_cast<double*>(value);
        else if (length == 0)
          AskPrice2 = (double)*static_cast<int*>(value);
        else
          AskPrice2 = (double)*static_cast<int64_t*>(value);
      }
      void setAskVolume2( void *value , uint16_t length = 0) {
        if (length == 8)
          AskVolume2 = *static_cast<double*>(value);
        else if (length == 0)
          AskVolume2 = (double)*static_cast<int*>(value);
        else
          AskVolume2 = (double)*static_cast<int64_t*>(value);
      }
      void setBidPrice3( void *value , uint16_t length = 0) {
        if (length == 8)
          BidPrice3 = *static_cast<double*>(value);
        else if (length == 0)
          BidPrice3 = (double)*static_cast<int*>(value);
        else
          BidPrice3 = (double)*static_cast<int64_t*>(value);
      }
      void setBidVolume3( void *value , uint16_t length = 0) {
        if (length == 8)
          BidVolume3 = *static_cast<double*>(value);
        else if (length == 0)
          BidVolume3 = (double)*static_cast<int*>(value);
        else
          BidVolume3 = (double)*static_cast<int64_t*>(value);
      }
      void setAskPrice3( void *value , uint16_t length = 0) {
        if (length == 8)
          AskPrice3 = *static_cast<double*>(value);
        else if (length == 0)
          AskPrice3 = (double)*static_cast<int*>(value);
        else
          AskPrice3 = (double)*static_cast<int64_t*>(value);
      }
      void setAskVolume3( void *value , uint16_t length = 0) {
        if (length == 8)
          AskVolume3 = *static_cast<double*>(value);
        else if (length == 0)
          AskVolume3 = (double)*static_cast<int*>(value);
        else
          AskVolume3 = (double)*static_cast<int64_t*>(value);
      }
      void setBidPrice4( void *value , uint16_t length = 0) {
        if (length == 8)
          BidPrice4 = *static_cast<double*>(value);
        else if (length == 0)
          BidPrice4 = (double)*static_cast<int*>(value);
        else
          BidPrice4 = (double)*static_cast<int64_t*>(value);
      }
      void setBidVolume4( void *value , uint16_t length = 0) {
        if (length == 8)
          BidVolume4 = *static_cast<double*>(value);
        else if (length == 0)
          BidVolume4 = (double)*static_cast<int*>(value);
        else
          BidVolume4 = (double)*static_cast<int64_t*>(value);
      }
      void setAskPrice4( void *value , uint16_t length = 0) {
        if (length == 8)
          AskPrice4 = *static_cast<double*>(value);
        else if (length == 0)
          AskPrice4 = (double)*static_cast<int*>(value);
        else
          AskPrice4 = (double)*static_cast<int64_t*>(value);
      }
      void setAskVolume4( void *value , uint16_t length = 0) {
        if (length == 8)
          AskVolume4 = *static_cast<double*>(value);
        else if (length == 0)
          AskVolume4 = (double)*static_cast<int*>(value);
        else
          AskVolume4 = (double)*static_cast<int64_t*>(value);
      }
      void setBidPrice5( void *value , uint16_t length = 0) {
        if (length == 8)
          BidPrice5 = *static_cast<double*>(value);
        else if (length == 0)
          BidPrice5 = (double)*static_cast<int*>(value);
        else
          BidPrice5 = (double)*static_cast<int64_t*>(value);
      }
      void setBidVolume5( void *value , uint16_t length = 0) {
        if (length == 8)
          BidVolume5 = *static_cast<double*>(value);
        else if (length == 0)
          BidVolume5 = (double)*static_cast<int*>(value);
        else
          BidVolume5 = (double)*static_cast<int64_t*>(value);
      }
      void setAskPrice5( void *value , uint16_t length = 0) {
        if (length == 8)
          AskPrice5 = *static_cast<double*>(value);
        else if (length == 0)
          AskPrice5 = (double)*static_cast<int*>(value);
        else
          AskPrice5 = (double)*static_cast<int64_t*>(value);
      }
      void setAskVolume5( void *value , uint16_t length = 0) {
        if (length == 8)
          AskVolume5 = *static_cast<double*>(value);
        else if (length == 0)
          AskVolume5 = (double)*static_cast<int*>(value);
        else
          AskVolume5 = (double)*static_cast<int64_t*>(value);
      }
      void setUpdateTime( void *value , uint16_t length = 0) {
        memcpy(UpdateTime, static_cast<char*>(value), length <UpdateTimeLen  ? length :UpdateTimeLen);
      }
      void setUpdateMillisec( void *value , uint16_t length = 0) {
        if (length == 0)
          UpdateMillisec = *static_cast<int*>(value);
        else
          UpdateMillisec = *static_cast<uint64_t*>(value);
      }
      void setMessageID( void *value , uint16_t length = 0) {
        MessageID = *static_cast<int*>(value);
      }
      void setEpochTime( void *value , uint16_t length = 0) {
        if (length == 0)
          EpochTime = *static_cast<int*>(value);
        else
          EpochTime = *static_cast<uint64_t*>(value);
      }
      void setLastSeqNum( void *value , uint16_t length = 0) {
        if (length == 0)
          LastSeqNum = *static_cast<int*>(value);
        else
          LastSeqNum = *static_cast<uint64_t*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "TradingDay=[%s]InstrumentID=[%s]ExchangeCode=[%s]LastPrice=[%f]HighestPrice=[%f]LowestPrice=[%f]Volume=[%f]Turnover=[%f]BidPrice1=[%f]BidVolume1=[%f]AskPrice1=[%f]AskVolume1=[%f]BidPrice2=[%f]BidVolume2=[%f]AskPrice2=[%f]AskVolume2=[%f]BidPrice3=[%f]BidVolume3=[%f]AskPrice3=[%f]AskVolume3=[%f]BidPrice4=[%f]BidVolume4=[%f]AskPrice4=[%f]AskVolume4=[%f]BidPrice5=[%f]BidVolume5=[%f]AskPrice5=[%f]AskVolume5=[%f]UpdateTime=[%s]UpdateMillisec=[%I64d]MessageID=[%d]EpochTime=[%I64d]LastSeqNum=[%I64d]",
                TradingDay,InstrumentID,ExchangeCode,LastPrice,HighestPrice,LowestPrice,Volume,Turnover,BidPrice1,BidVolume1,AskPrice1,AskVolume1,BidPrice2,BidVolume2,AskPrice2,AskVolume2,BidPrice3,BidVolume3,AskPrice3,AskVolume3,BidPrice4,BidVolume4,AskPrice4,AskVolume4,BidPrice5,BidVolume5,AskPrice5,AskVolume5,UpdateTime,UpdateMillisec,MessageID,EpochTime,LastSeqNum);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["TradingDay"] = std::bind(&MarketData::setTradingDay, this, _1,_2);
        methodMap["InstrumentID"] = std::bind(&MarketData::setInstrumentID, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&MarketData::setExchangeCode, this, _1,_2);
        methodMap["LastPrice"] = std::bind(&MarketData::setLastPrice, this, _1,_2);
        methodMap["HighestPrice"] = std::bind(&MarketData::setHighestPrice, this, _1,_2);
        methodMap["LowestPrice"] = std::bind(&MarketData::setLowestPrice, this, _1,_2);
        methodMap["Volume"] = std::bind(&MarketData::setVolume, this, _1,_2);
        methodMap["Turnover"] = std::bind(&MarketData::setTurnover, this, _1,_2);
        methodMap["BidPrice1"] = std::bind(&MarketData::setBidPrice1, this, _1,_2);
        methodMap["BidVolume1"] = std::bind(&MarketData::setBidVolume1, this, _1,_2);
        methodMap["AskPrice1"] = std::bind(&MarketData::setAskPrice1, this, _1,_2);
        methodMap["AskVolume1"] = std::bind(&MarketData::setAskVolume1, this, _1,_2);
        methodMap["BidPrice2"] = std::bind(&MarketData::setBidPrice2, this, _1,_2);
        methodMap["BidVolume2"] = std::bind(&MarketData::setBidVolume2, this, _1,_2);
        methodMap["AskPrice2"] = std::bind(&MarketData::setAskPrice2, this, _1,_2);
        methodMap["AskVolume2"] = std::bind(&MarketData::setAskVolume2, this, _1,_2);
        methodMap["BidPrice3"] = std::bind(&MarketData::setBidPrice3, this, _1,_2);
        methodMap["BidVolume3"] = std::bind(&MarketData::setBidVolume3, this, _1,_2);
        methodMap["AskPrice3"] = std::bind(&MarketData::setAskPrice3, this, _1,_2);
        methodMap["AskVolume3"] = std::bind(&MarketData::setAskVolume3, this, _1,_2);
        methodMap["BidPrice4"] = std::bind(&MarketData::setBidPrice4, this, _1,_2);
        methodMap["BidVolume4"] = std::bind(&MarketData::setBidVolume4, this, _1,_2);
        methodMap["AskPrice4"] = std::bind(&MarketData::setAskPrice4, this, _1,_2);
        methodMap["AskVolume4"] = std::bind(&MarketData::setAskVolume4, this, _1,_2);
        methodMap["BidPrice5"] = std::bind(&MarketData::setBidPrice5, this, _1,_2);
        methodMap["BidVolume5"] = std::bind(&MarketData::setBidVolume5, this, _1,_2);
        methodMap["AskPrice5"] = std::bind(&MarketData::setAskPrice5, this, _1,_2);
        methodMap["AskVolume5"] = std::bind(&MarketData::setAskVolume5, this, _1,_2);
        methodMap["UpdateTime"] = std::bind(&MarketData::setUpdateTime, this, _1,_2);
        methodMap["UpdateMillisec"] = std::bind(&MarketData::setUpdateMillisec, this, _1,_2);
        methodMap["MessageID"] = std::bind(&MarketData::setMessageID, this, _1,_2);
        methodMap["EpochTime"] = std::bind(&MarketData::setEpochTime, this, _1,_2);
        methodMap["LastSeqNum"] = std::bind(&MarketData::setLastSeqNum, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(TradingDay) != 0){
          int length = strlen(TradingDay) < TradingDayLen ? strlen(TradingDay):TradingDayLen;
          doc.AddMember("TradingDay", spotrapidjson::Value().SetString(TradingDay,length), allocator);
        }
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        if (!std::isnan(LastPrice))
          doc.AddMember("LastPrice",LastPrice, allocator);
        if (!std::isnan(HighestPrice))
          doc.AddMember("HighestPrice",HighestPrice, allocator);
        if (!std::isnan(LowestPrice))
          doc.AddMember("LowestPrice",LowestPrice, allocator);
        if (!std::isnan(Volume))
          doc.AddMember("Volume",Volume, allocator);
        if (!std::isnan(Turnover))
          doc.AddMember("Turnover",Turnover, allocator);
        if (!std::isnan(BidPrice1))
          doc.AddMember("BidPrice1",BidPrice1, allocator);
        if (!std::isnan(BidVolume1))
          doc.AddMember("BidVolume1",BidVolume1, allocator);
        if (!std::isnan(AskPrice1))
          doc.AddMember("AskPrice1",AskPrice1, allocator);
        if (!std::isnan(AskVolume1))
          doc.AddMember("AskVolume1",AskVolume1, allocator);
        if (!std::isnan(BidPrice2))
          doc.AddMember("BidPrice2",BidPrice2, allocator);
        if (!std::isnan(BidVolume2))
          doc.AddMember("BidVolume2",BidVolume2, allocator);
        if (!std::isnan(AskPrice2))
          doc.AddMember("AskPrice2",AskPrice2, allocator);
        if (!std::isnan(AskVolume2))
          doc.AddMember("AskVolume2",AskVolume2, allocator);
        if (!std::isnan(BidPrice3))
          doc.AddMember("BidPrice3",BidPrice3, allocator);
        if (!std::isnan(BidVolume3))
          doc.AddMember("BidVolume3",BidVolume3, allocator);
        if (!std::isnan(AskPrice3))
          doc.AddMember("AskPrice3",AskPrice3, allocator);
        if (!std::isnan(AskVolume3))
          doc.AddMember("AskVolume3",AskVolume3, allocator);
        if (!std::isnan(BidPrice4))
          doc.AddMember("BidPrice4",BidPrice4, allocator);
        if (!std::isnan(BidVolume4))
          doc.AddMember("BidVolume4",BidVolume4, allocator);
        if (!std::isnan(AskPrice4))
          doc.AddMember("AskPrice4",AskPrice4, allocator);
        if (!std::isnan(AskVolume4))
          doc.AddMember("AskVolume4",AskVolume4, allocator);
        if (!std::isnan(BidPrice5))
          doc.AddMember("BidPrice5",BidPrice5, allocator);
        if (!std::isnan(BidVolume5))
          doc.AddMember("BidVolume5",BidVolume5, allocator);
        if (!std::isnan(AskPrice5))
          doc.AddMember("AskPrice5",AskPrice5, allocator);
        if (!std::isnan(AskVolume5))
          doc.AddMember("AskVolume5",AskVolume5, allocator);
        if (strlen(UpdateTime) != 0){
          int length = strlen(UpdateTime) < UpdateTimeLen ? strlen(UpdateTime):UpdateTimeLen;
          doc.AddMember("UpdateTime", spotrapidjson::Value().SetString(UpdateTime,length), allocator);
        }
        doc.AddMember("UpdateMillisec",UpdateMillisec, allocator);
        doc.AddMember("MessageID",MessageID, allocator);
        doc.AddMember("EpochTime",EpochTime, allocator);
        doc.AddMember("LastSeqNum",LastSeqNum, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("MarketData"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SpotProductInfo
    {
    public:
      SpotProductInfo()
      {
        memset(this,0,sizeof(SpotProductInfo));
      }
      int SpotID;
      char ProductName[ProductNameLen+1];

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setProductName( void *value , uint16_t length = 0) {
        memcpy(ProductName, static_cast<char*>(value), length <ProductNameLen  ? length :ProductNameLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]ProductName=[%s]",
                SpotID,ProductName);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&SpotProductInfo::setSpotID, this, _1,_2);
        methodMap["ProductName"] = std::bind(&SpotProductInfo::setProductName, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        if (strlen(ProductName) != 0){
          int length = strlen(ProductName) < ProductNameLen ? strlen(ProductName):ProductNameLen;
          doc.AddMember("ProductName", spotrapidjson::Value().SetString(ProductName,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SpotProductInfo"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SpotTdInfo
    {
    public:
      SpotTdInfo()
      {
        memset(this,0,sizeof(SpotTdInfo));
      }
      int SpotID;
      char UserID[UserIDLen+1];
      char InvestorID[InvestorIDLen+1];
      char ExchangeCode[ExchangeCodeLen+1];
      char InterfaceType[InterfaceTypeLen+1];
      char FrontAddr[FrontAddrLen+1];
      char FrontQueryAddr[FrontQueryAddrLen+1];
      char LocalAddr[LocalAddrLen+1];
      int InterfaceID;

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setUserID( void *value , uint16_t length = 0) {
        memcpy(UserID, static_cast<char*>(value), length <UserIDLen  ? length :UserIDLen);
      }
      void setInvestorID( void *value , uint16_t length = 0) {
        memcpy(InvestorID, static_cast<char*>(value), length <InvestorIDLen  ? length :InvestorIDLen);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setInterfaceType( void *value , uint16_t length = 0) {
        memcpy(InterfaceType, static_cast<char*>(value), length <InterfaceTypeLen  ? length :InterfaceTypeLen);
      }
      void setFrontAddr( void *value , uint16_t length = 0) {
        memcpy(FrontAddr, static_cast<char*>(value), length <FrontAddrLen  ? length :FrontAddrLen);
      }
      void setFrontQueryAddr( void *value , uint16_t length = 0) {
        memcpy(FrontQueryAddr, static_cast<char*>(value), length <FrontQueryAddrLen  ? length :FrontQueryAddrLen);
      }
      void setLocalAddr( void *value , uint16_t length = 0) {
        memcpy(LocalAddr, static_cast<char*>(value), length <LocalAddrLen  ? length :LocalAddrLen);
      }
      void setInterfaceID( void *value , uint16_t length = 0) {
        InterfaceID = *static_cast<int*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]UserID=[%s]InvestorID=[%s]ExchangeCode=[%s]InterfaceType=[%s]FrontAddr=[%s]FrontQueryAddr=[%s]LocalAddr=[%s]InterfaceID=[%d]",
                SpotID,UserID,InvestorID,ExchangeCode,InterfaceType,FrontAddr,FrontQueryAddr,LocalAddr,InterfaceID);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&SpotTdInfo::setSpotID, this, _1,_2);
        methodMap["UserID"] = std::bind(&SpotTdInfo::setUserID, this, _1,_2);
        methodMap["InvestorID"] = std::bind(&SpotTdInfo::setInvestorID, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&SpotTdInfo::setExchangeCode, this, _1,_2);
        methodMap["InterfaceType"] = std::bind(&SpotTdInfo::setInterfaceType, this, _1,_2);
        methodMap["FrontAddr"] = std::bind(&SpotTdInfo::setFrontAddr, this, _1,_2);
        methodMap["FrontQueryAddr"] = std::bind(&SpotTdInfo::setFrontQueryAddr, this, _1,_2);
        methodMap["LocalAddr"] = std::bind(&SpotTdInfo::setLocalAddr, this, _1,_2);
        methodMap["InterfaceID"] = std::bind(&SpotTdInfo::setInterfaceID, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        if (strlen(UserID) != 0){
          int length = strlen(UserID) < UserIDLen ? strlen(UserID):UserIDLen;
          doc.AddMember("UserID", spotrapidjson::Value().SetString(UserID,length), allocator);
        }
        if (strlen(InvestorID) != 0){
          int length = strlen(InvestorID) < InvestorIDLen ? strlen(InvestorID):InvestorIDLen;
          doc.AddMember("InvestorID", spotrapidjson::Value().SetString(InvestorID,length), allocator);
        }
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        if (strlen(InterfaceType) != 0){
          int length = strlen(InterfaceType) < InterfaceTypeLen ? strlen(InterfaceType):InterfaceTypeLen;
          doc.AddMember("InterfaceType", spotrapidjson::Value().SetString(InterfaceType,length), allocator);
        }
        if (strlen(FrontAddr) != 0){
          int length = strlen(FrontAddr) < FrontAddrLen ? strlen(FrontAddr):FrontAddrLen;
          doc.AddMember("FrontAddr", spotrapidjson::Value().SetString(FrontAddr,length), allocator);
        }
        if (strlen(FrontQueryAddr) != 0){
          int length = strlen(FrontQueryAddr) < FrontQueryAddrLen ? strlen(FrontQueryAddr):FrontQueryAddrLen;
          doc.AddMember("FrontQueryAddr", spotrapidjson::Value().SetString(FrontQueryAddr,length), allocator);
        }
        if (strlen(LocalAddr) != 0){
          int length = strlen(LocalAddr) < LocalAddrLen ? strlen(LocalAddr):LocalAddrLen;
          doc.AddMember("LocalAddr", spotrapidjson::Value().SetString(LocalAddr,length), allocator);
        }
        doc.AddMember("InterfaceID",InterfaceID, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SpotTdInfo"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class SpotMdInfo
    {
    public:
      SpotMdInfo()
      {
        memset(this,0,sizeof(SpotMdInfo));
      }
      int SpotID;
      char UserID[UserIDLen+1];
      char InvestorID[InvestorIDLen+1];
      char ExchangeCode[ExchangeCodeLen+1];
      char InterfaceType[InterfaceTypeLen+1];
      char FrontAddr[FrontAddrLen+1];
      char LocalAddr[LocalAddrLen+1];
      int InterfaceID;
      char FrontQueryAddr[FrontQueryAddrLen+1];

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setUserID( void *value , uint16_t length = 0) {
        memcpy(UserID, static_cast<char*>(value), length <UserIDLen  ? length :UserIDLen);
      }
      void setInvestorID( void *value , uint16_t length = 0) {
        memcpy(InvestorID, static_cast<char*>(value), length <InvestorIDLen  ? length :InvestorIDLen);
      }
      void setExchangeCode( void *value , uint16_t length = 0) {
        memcpy(ExchangeCode, static_cast<char*>(value), length <ExchangeCodeLen  ? length :ExchangeCodeLen);
      }
      void setInterfaceType( void *value , uint16_t length = 0) {
        memcpy(InterfaceType, static_cast<char*>(value), length <InterfaceTypeLen  ? length :InterfaceTypeLen);
      }
      void setFrontAddr( void *value , uint16_t length = 0) {
        memcpy(FrontAddr, static_cast<char*>(value), length <FrontAddrLen  ? length :FrontAddrLen);
      }
      void setLocalAddr( void *value , uint16_t length = 0) {
        memcpy(LocalAddr, static_cast<char*>(value), length <LocalAddrLen  ? length :LocalAddrLen);
      }
      void setInterfaceID( void *value , uint16_t length = 0) {
        InterfaceID = *static_cast<int*>(value);
      }
      void setFrontQueryAddr( void *value , uint16_t length = 0) {
        memcpy(FrontQueryAddr, static_cast<char*>(value), length <FrontQueryAddrLen  ? length :FrontQueryAddrLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]UserID=[%s]InvestorID=[%s]ExchangeCode=[%s]InterfaceType=[%s]FrontAddr=[%s]LocalAddr=[%s]InterfaceID=[%d]FrontQueryAddr=[%s]",
                SpotID,UserID,InvestorID,ExchangeCode,InterfaceType,FrontAddr,LocalAddr,InterfaceID,FrontQueryAddr);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&SpotMdInfo::setSpotID, this, _1,_2);
        methodMap["UserID"] = std::bind(&SpotMdInfo::setUserID, this, _1,_2);
        methodMap["InvestorID"] = std::bind(&SpotMdInfo::setInvestorID, this, _1,_2);
        methodMap["ExchangeCode"] = std::bind(&SpotMdInfo::setExchangeCode, this, _1,_2);
        methodMap["InterfaceType"] = std::bind(&SpotMdInfo::setInterfaceType, this, _1,_2);
        methodMap["FrontAddr"] = std::bind(&SpotMdInfo::setFrontAddr, this, _1,_2);
        methodMap["LocalAddr"] = std::bind(&SpotMdInfo::setLocalAddr, this, _1,_2);
        methodMap["InterfaceID"] = std::bind(&SpotMdInfo::setInterfaceID, this, _1,_2);
        methodMap["FrontQueryAddr"] = std::bind(&SpotMdInfo::setFrontQueryAddr, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        if (strlen(UserID) != 0){
          int length = strlen(UserID) < UserIDLen ? strlen(UserID):UserIDLen;
          doc.AddMember("UserID", spotrapidjson::Value().SetString(UserID,length), allocator);
        }
        if (strlen(InvestorID) != 0){
          int length = strlen(InvestorID) < InvestorIDLen ? strlen(InvestorID):InvestorIDLen;
          doc.AddMember("InvestorID", spotrapidjson::Value().SetString(InvestorID,length), allocator);
        }
        if (strlen(ExchangeCode) != 0){
          int length = strlen(ExchangeCode) < ExchangeCodeLen ? strlen(ExchangeCode):ExchangeCodeLen;
          doc.AddMember("ExchangeCode", spotrapidjson::Value().SetString(ExchangeCode,length), allocator);
        }
        if (strlen(InterfaceType) != 0){
          int length = strlen(InterfaceType) < InterfaceTypeLen ? strlen(InterfaceType):InterfaceTypeLen;
          doc.AddMember("InterfaceType", spotrapidjson::Value().SetString(InterfaceType,length), allocator);
        }
        if (strlen(FrontAddr) != 0){
          int length = strlen(FrontAddr) < FrontAddrLen ? strlen(FrontAddr):FrontAddrLen;
          doc.AddMember("FrontAddr", spotrapidjson::Value().SetString(FrontAddr,length), allocator);
        }
        if (strlen(LocalAddr) != 0){
          int length = strlen(LocalAddr) < LocalAddrLen ? strlen(LocalAddr):LocalAddrLen;
          doc.AddMember("LocalAddr", spotrapidjson::Value().SetString(LocalAddr,length), allocator);
        }
        doc.AddMember("InterfaceID",InterfaceID, allocator);
        if (strlen(FrontQueryAddr) != 0){
          int length = strlen(FrontQueryAddr) < FrontQueryAddrLen ? strlen(FrontQueryAddr):FrontQueryAddrLen;
          doc.AddMember("FrontQueryAddr", spotrapidjson::Value().SetString(FrontQueryAddr,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("SpotMdInfo"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class Alarm
    {
    public:
      Alarm()
      {
        memset(this,0,sizeof(Alarm));
      }
      int SpotID;
      uint64_t AlarmEpochTime;
      int SeverityLevel;
      char Alarmtext[AlarmtextLen+1];

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setAlarmEpochTime( void *value , uint16_t length = 0) {
        if (length == 0)
          AlarmEpochTime = *static_cast<int*>(value);
        else
          AlarmEpochTime = *static_cast<uint64_t*>(value);
      }
      void setSeverityLevel( void *value , uint16_t length = 0) {
        SeverityLevel = *static_cast<int*>(value);
      }
      void setAlarmtext( void *value , uint16_t length = 0) {
        memcpy(Alarmtext, static_cast<char*>(value), length <AlarmtextLen  ? length :AlarmtextLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]AlarmEpochTime=[%I64d]SeverityLevel=[%d]Alarmtext=[%s]",
                SpotID,AlarmEpochTime,SeverityLevel,Alarmtext);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&Alarm::setSpotID, this, _1,_2);
        methodMap["AlarmEpochTime"] = std::bind(&Alarm::setAlarmEpochTime, this, _1,_2);
        methodMap["SeverityLevel"] = std::bind(&Alarm::setSeverityLevel, this, _1,_2);
        methodMap["Alarmtext"] = std::bind(&Alarm::setAlarmtext, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        doc.AddMember("AlarmEpochTime",AlarmEpochTime, allocator);
        doc.AddMember("SeverityLevel",SeverityLevel, allocator);
        if (strlen(Alarmtext) != 0){
          int length = strlen(Alarmtext) < AlarmtextLen ? strlen(Alarmtext):AlarmtextLen;
          doc.AddMember("Alarmtext", spotrapidjson::Value().SetString(Alarmtext,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("Alarm"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class InstrumentTradingFee
    {
    public:
      InstrumentTradingFee()
      {
        memset(this,0,sizeof(InstrumentTradingFee));
      }
      char InstrumentID[InstrumentIDLen+1];
      double FeeRate;
      char FeeType[FeeTypeLen+1];
      char FeeFormat[FeeFormatLen+1];

      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setFeeRate( void *value , uint16_t length = 0) {
        if (length == 8)
          FeeRate = *static_cast<double*>(value);
        else if (length == 0)
          FeeRate = (double)*static_cast<int*>(value);
        else
          FeeRate = (double)*static_cast<int64_t*>(value);
      }
      void setFeeType( void *value , uint16_t length = 0) {
        memcpy(FeeType, static_cast<char*>(value), length <FeeTypeLen  ? length :FeeTypeLen);
      }
      void setFeeFormat( void *value , uint16_t length = 0) {
        memcpy(FeeFormat, static_cast<char*>(value), length <FeeFormatLen  ? length :FeeFormatLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "InstrumentID=[%s]FeeRate=[%f]FeeType=[%s]FeeFormat=[%s]",
                InstrumentID,FeeRate,FeeType,FeeFormat);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["InstrumentID"] = std::bind(&InstrumentTradingFee::setInstrumentID, this, _1,_2);
        methodMap["FeeRate"] = std::bind(&InstrumentTradingFee::setFeeRate, this, _1,_2);
        methodMap["FeeType"] = std::bind(&InstrumentTradingFee::setFeeType, this, _1,_2);
        methodMap["FeeFormat"] = std::bind(&InstrumentTradingFee::setFeeFormat, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        if (!std::isnan(FeeRate))
          doc.AddMember("FeeRate",FeeRate, allocator);
        if (strlen(FeeType) != 0){
          int length = strlen(FeeType) < FeeTypeLen ? strlen(FeeType):FeeTypeLen;
          doc.AddMember("FeeType", spotrapidjson::Value().SetString(FeeType,length), allocator);
        }
        if (strlen(FeeFormat) != 0){
          int length = strlen(FeeFormat) < FeeFormatLen ? strlen(FeeFormat):FeeFormatLen;
          doc.AddMember("FeeFormat", spotrapidjson::Value().SetString(FeeFormat,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("InstrumentTradingFee"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class UpdateStrategyParams
    {
    public:
      UpdateStrategyParams()
      {
        memset(this,0,sizeof(UpdateStrategyParams));
      }
      int StrategyID;
      char Name[NameLen+1];
      int ValueInt;
      double ValueDouble;
      char ValueString[ValueStringLen+1];
      char ValueType[ValueTypeLen+1];

      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setName( void *value , uint16_t length = 0) {
        memcpy(Name, static_cast<char*>(value), length <NameLen  ? length :NameLen);
      }
      void setValueInt( void *value , uint16_t length = 0) {
        ValueInt = *static_cast<int*>(value);
      }
      void setValueDouble( void *value , uint16_t length = 0) {
        if (length == 8)
          ValueDouble = *static_cast<double*>(value);
        else if (length == 0)
          ValueDouble = (double)*static_cast<int*>(value);
        else
          ValueDouble = (double)*static_cast<int64_t*>(value);
      }
      void setValueString( void *value , uint16_t length = 0) {
        memcpy(ValueString, static_cast<char*>(value), length <ValueStringLen  ? length :ValueStringLen);
      }
      void setValueType( void *value , uint16_t length = 0) {
        memcpy(ValueType, static_cast<char*>(value), length <ValueTypeLen  ? length :ValueTypeLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "StrategyID=[%d]Name=[%s]ValueInt=[%d]ValueDouble=[%f]ValueString=[%s]ValueType=[%s]",
                StrategyID,Name,ValueInt,ValueDouble,ValueString,ValueType);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["StrategyID"] = std::bind(&UpdateStrategyParams::setStrategyID, this, _1,_2);
        methodMap["Name"] = std::bind(&UpdateStrategyParams::setName, this, _1,_2);
        methodMap["ValueInt"] = std::bind(&UpdateStrategyParams::setValueInt, this, _1,_2);
        methodMap["ValueDouble"] = std::bind(&UpdateStrategyParams::setValueDouble, this, _1,_2);
        methodMap["ValueString"] = std::bind(&UpdateStrategyParams::setValueString, this, _1,_2);
        methodMap["ValueType"] = std::bind(&UpdateStrategyParams::setValueType, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("StrategyID",StrategyID, allocator);
        if (strlen(Name) != 0){
          int length = strlen(Name) < NameLen ? strlen(Name):NameLen;
          doc.AddMember("Name", spotrapidjson::Value().SetString(Name,length), allocator);
        }
        doc.AddMember("ValueInt",ValueInt, allocator);
        if (!std::isnan(ValueDouble))
          doc.AddMember("ValueDouble",ValueDouble, allocator);
        if (strlen(ValueString) != 0){
          int length = strlen(ValueString) < ValueStringLen ? strlen(ValueString):ValueStringLen;
          doc.AddMember("ValueString", spotrapidjson::Value().SetString(ValueString,length), allocator);
        }
        if (strlen(ValueType) != 0){
          int length = strlen(ValueType) < ValueTypeLen ? strlen(ValueType):ValueTypeLen;
          doc.AddMember("ValueType", spotrapidjson::Value().SetString(ValueType,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("UpdateStrategyParams"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategySwitch
    {
    public:
      StrategySwitch()
      {
        memset(this,0,sizeof(StrategySwitch));
      }
      char InstrumentID[InstrumentIDLen+1];
      int StrategyID;
      int Tradeable;

      void setInstrumentID( void *value , uint16_t length = 0) {
        memcpy(InstrumentID, static_cast<char*>(value), length <InstrumentIDLen  ? length :InstrumentIDLen);
      }
      void setStrategyID( void *value , uint16_t length = 0) {
        StrategyID = *static_cast<int*>(value);
      }
      void setTradeable( void *value , uint16_t length = 0) {
        Tradeable = *static_cast<int*>(value);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "InstrumentID=[%s]StrategyID=[%d]Tradeable=[%d]",
                InstrumentID,StrategyID,Tradeable);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["InstrumentID"] = std::bind(&StrategySwitch::setInstrumentID, this, _1,_2);
        methodMap["StrategyID"] = std::bind(&StrategySwitch::setStrategyID, this, _1,_2);
        methodMap["Tradeable"] = std::bind(&StrategySwitch::setTradeable, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        if (strlen(InstrumentID) != 0){
          int length = strlen(InstrumentID) < InstrumentIDLen ? strlen(InstrumentID):InstrumentIDLen;
          doc.AddMember("InstrumentID", spotrapidjson::Value().SetString(InstrumentID,length), allocator);
        }
        doc.AddMember("StrategyID",StrategyID, allocator);
        doc.AddMember("Tradeable",Tradeable, allocator);
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("StrategySwitch"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

    class StrategyInit
    {
    public:
      StrategyInit()
      {
        memset(this,0,sizeof(StrategyInit));
      }
      int SpotID;
      char TradingDay[TradingDayLen+1];

      void setSpotID( void *value , uint16_t length = 0) {
        SpotID = *static_cast<int*>(value);
      }
      void setTradingDay( void *value , uint16_t length = 0) {
        memcpy(TradingDay, static_cast<char*>(value), length <TradingDayLen  ? length :TradingDayLen);
      }
      string toString()
      {
        char buffer[2048];
        SNPRINTF(buffer, sizeof buffer, "SpotID=[%d]TradingDay=[%s]",
                SpotID,TradingDay);
        return buffer;
      }

      void initMethodMap(std::map<std::string, setMethod> &methodMap){
        methodMap["SpotID"] = std::bind(&StrategyInit::setSpotID, this, _1,_2);
        methodMap["TradingDay"] = std::bind(&StrategyInit::setTradingDay, this, _1,_2);
      }

      string toJson() const {
        spotrapidjson::Document doc;
        doc.SetObject();
        spotrapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        spotrapidjson::Document outDoc;
        outDoc.SetObject();
        spotrapidjson::Document::AllocatorType& outAllocator = outDoc.GetAllocator();
        doc.AddMember("SpotID",SpotID, allocator);
        if (strlen(TradingDay) != 0){
          int length = strlen(TradingDay) < TradingDayLen ? strlen(TradingDay):TradingDayLen;
          doc.AddMember("TradingDay", spotrapidjson::Value().SetString(TradingDay,length), allocator);
        }
        outDoc.AddMember("Title", spotrapidjson::Value().SetString("StrategyInit"), outAllocator);
        outDoc.AddMember("Content", doc, outAllocator);
        spotrapidjson::StringBuffer strbuf;
        spotrapidjson::Writer<spotrapidjson::StringBuffer> writer(strbuf);
        outDoc.Accept(writer);
        return strbuf.GetString();
      }
    };

  }
}
#endif