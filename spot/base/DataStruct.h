#ifndef SPOT_BASE_DATASTRUCT_H
#define SPOT_BASE_DATASTRUCT_H
#include <spot/utility/Types.h>
#include <spot/utility/Utility.h>
#include "spot/utility/Compatible.h"
#include "spot/utility/Utility.h"
#include <spot/base/MqStruct.h>
#include "spot/utility/Logging.h"

#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string.h>
#include <atomic>
#include <sstream>
#include <set>

#define QUEUE_SPOT_MAX pow(10,10)
///
#define INNER_OFFSET_Open '0'
///
#define INNER_OFFSET_CloseToday '1'
///
#define INNER_OFFSET_Undefined '2'
///
///#define INNER_OFFSET_Close '2'

///
//#define INNER_OFFSET_CloseYesterday '4'

///
#define INNER_DIRECTION_Buy '0'
///
#define INNER_DIRECTION_Sell '1'
///
#define INNER_DIRECTION_Undefined '2'

//
#define OKEXFUTURE_OFFSET_Long '1'
//
#define OKEXFUTURE_OFFSET_Short '2'
//
#define OKEXFUTURE_OFFSET_CloseLong '3'
//
#define OKEXFUTURE_OFFSET_CloseShort '4'

//
#define CRYPTO_CASH_Minimum_Match_Size 0.00000001

#define ORDERTYPE_DEFAULT 0
#define ORDERTYPE_FAK 1
#define ORDERTYPE_LIMIT_CROSS 2
#define ORDERTYPE_FOK 3
#define ORDERTYPE_POST_ONLY 4
#define ORDERTYPE_MARKET 5

#define SECONDARY_TRADEABLE_FALSE '0'
#define SECONDARY_TRADEABLE_TRUE '1'

#define HEDGEFLAG_Speculation 1 //

#define BORROWFLAG_DEFAULT 0 //

#define MDLEVELTYPE_DEFAULT '0'  //
#define MDLEVELTYPE_TICKER '1'  //

#define OKEX_ORDER_STATUS_SUBMITTED 0 //0 
#define OKEX_ORDER_STATUS_PARTIALFILLED 1  // 1 
#define OKEX_ORDER_STATUS_FILLED 2  // 2 
#define OKEX_ORDER_STATUS_CANCELLED -1  // -1 
#define OKEX_ORDER_STATUS_CANCELPROCESSING 4  // 4 
#define OKEX_ORDER_STATUS_CANCELINPROGRESS 5  // 5 

static const std::string LINEAR = "linear";
static const std::string INVERSE = "inverse";
static const std::string LEVERAGE = "leverage";

static const std::string Crypto_Future_This_Week = "this_week";
static const std::string Crypto_Future_Next_Week = "next_week";
static const std::string Crypto_Future_Quarter = "quarter";

static const std::string Crypto_Future_Perpetual = "perpetual";
static const std::string Crypto_Future_This_Quarter = "this_quarter";
static const std::string Crypto_Future_Next_Quarter = "next_quarter";

static const std::string DOUBLETYPE = "double";
static const std::string INTTYPE = "int";
static const std::string STRINGTYPE = "string";
static const std::string DATETYPE = "date";

static const std::string FEETYPE_MAKER = "Maker";
static const std::string FEETYPE_TAKER = "Taker";
static const std::string FEEFORMAT_ABSOLUTE = "Absolute";
static const std::string FEEFORMAT_PERCENTAGE = "Percentage";

static const std::string AssetType_TD = "TD";
static const std::string AssetType_FuturePerpetual = "PERPETUAL";
static const std::string AssetType_Perp = "PERP";
static const std::string AssetType_FutureSwap = "SWAP";
static const std::string AssetType_Future = "Future";
static const std::string AssetType_FutureOption = "Option";

static const std::string AssetType_Crypto = "Crypto";
static const std::string AssetType_OKExIndex = "OKExIndex";

static const std::string AssetType_CryptoDb = "Crypto_db";

static const std::string Exchange_OKEX = "OKEX";
static const std::string Exchange_BYBIT = "BYBIT";
static const std::string Exchange_BITSTAMP = "BITSTAMP";
static const std::string Exchange_HUOB = "HUOB";
static const std::string Exchange_BITFINEX = "BITFINEX";
static const std::string Exchange_BINANCE = "BINANCE";
static const std::string Exchange_DERIBIT = "DERIBIT";
static const std::string Exchange_QSDEX = "QSDEX";

static const std::string InterfaceMode_Single = "Single";
static const std::string InterfaceMode_Double = "Double";

static const std::string InterfaceMode_MqWriteMQ = "MqWriteMQ";
static const std::string TimerInterval = "TimerInterval";
static const std::string ForceCloseTimerInterval = "ForceCloseTimerInterval";
static const std::string BianListenKeyInterval = "BianListenKeyInterval";
static const std::string BybitQryInterval = "BybitQryInterval";
static const std::string OkPingPongInterval = "OkPingPongInterval";
static const std::string ByPingPongInterval = "ByPingPongInterval";
static const std::string ColoTimeDeviation = "ColoTimeDeviation";
static const std::string LastPriceDeviation = "LastPriceDeviation";
static const std::string L5Bid1Ask1Deviation = "L5Bid1Ask1Deviation";
static const std::string SignalMaxValue = "SignalMaxValue";
static const std::string OrderCancelMaxAttempts = "OrderCancelMaxAttempts";
static const std::string OrderQueryIntervalMili = "OrderQueryIntervalMili";
static const std::string SpotmdSleepIntervalus = "SpotmdSleepIntervalus";
static const std::string MarketDataQueueModeStr = "MarketDataQueueMode";
static const std::string MeasureEnable = "MeasureEnable";
static const std::string IsOkSimParam = "IsOkSimParam";
static const std::string IsBaSimParam = "IsBaSimParam";
static const std::string IsBySimParam = "IsBySimParam";

//mktDataBinnerLagToleranceWarn in seconds
const static long long mktDataBinnerLagToleranceWarn = 60;

//mktDataBinnerLagToleranceSkip in seconds
const static long long mktDataBinnerLagToleranceSkip = 120;

const static int ERR_MaxCancelOrderReached = 9999;

namespace spot
{
	int char2int(const char **ps);
	int getMessageID();

	namespace base
	{
		const static uint16_t EesSymbolLen = 8;
		const static uint16_t InvestorIdRefLen = 12;
		const static uint16_t AccountIDLen = 12;
		const static uint16_t InstrumentNameLen = 30;

		const static uint16_t CommentLen = 9;
		const static uint16_t DateLen = 10;
		const static uint16_t IndentificationLen = 14;
		const static uint16_t SymLen = 9;
		const static uint16_t DTimeLen = 19;
		const static uint16_t DatesLen = 10;
		const static uint16_t ExchangeIDLen = 20;

		const static uint16_t OpenLen = 8;

		const static uint16_t ParameterLen = 1023;
		const static uint16_t PasswordLen = 19;
		const static uint16_t Pri_contractLen = 14;
		const static uint16_t Root_symLen = 9;
		const static uint16_t Sec_contractLen = 14;
		const static uint16_t StrategyClassLen = 19;
		const static uint16_t ParamNameLen = 62;
		const static uint16_t ParamValueStringLen = 62;
		const static uint16_t TradingPeriodTypeLen = 9;
		const static uint16_t FUN_INSTRUTMENT = 1;
		const static uint16_t FUN_ORDER_INSERT = 2;
		const static uint16_t FUN_ORDER_ACTION = 3;
		const static uint16_t FUN_QRY_INVESTORPOS = 4;
		const static uint16_t FUN_TRADING_ACCOUT = 5;
		const static uint16_t FUN_QRY_ORDER = 6;
		const static uint16_t FUN_QRY_TRADE = 7;

		const static uint16_t TID_WriteStrategyInstrumentPNLDaily = 101;
		const static uint16_t TID_WriteTimeBinner = 102;
		const static uint16_t TID_WriteOrder = 104;
		const static uint16_t TID_WriteLog = 105;
		const static uint16_t TID_WriteHeartBeat = 106;
		const static uint16_t TID_WriteStrategySwitch = 107;
		const static uint16_t TID_WriteMdLastPrice = 108;
		const static uint16_t TID_WriteStrategyParam = 109;
		const static uint16_t TID_WriteUpdateStrategySignals = 110;
		const static uint16_t TID_WriteSignalValue = 111;
		const static uint16_t TID_WriteMetricData = 112;
		const static uint16_t TID_WriteSimTrade = 113;
		const static uint16_t TID_WriteSimStrategyParam = 114;
		const static uint16_t TID_WriteMqMkData = 115;
		const static uint16_t TID_WriteMqMkTrade = 116;
		const static uint16_t TID_TimeOut = 117;
		const static uint16_t TID_WriteUpdateOptionPricingParams = 118;
		const static uint16_t TID_WriteUpdateInstrumentInfoExt = 119;
		const static uint16_t TID_WriteAlarm = 120;
		const static uint16_t TID_WriteUpdateStrategyParamsExt = 121;
		const static uint16_t TID_WriteUpdateStrategyInstrumentSignals = 122;

		const static uint16_t TID_Init = 998;
		const static uint16_t TID_InitFinished = 999;

		struct commQryPosiNode
		{
			char symbol[InstrumentIDLen+1];
			double size;
			char side[DateLen+1];
			double liqPrice;
			double entryPrice;
			uint64_t updatedTime;
			commQryPosiNode() {
				memset(this, 0, sizeof(commQryPosiNode));
			}
			void setSide(const char* s) {
				memset(this->side, 0, sizeof(this->side));
				//memset(TradingDay, 0, sizeof(TradingDay));
				memcpy(this->side, s, min(DateLen, uint16_t(strlen(s))));
			}
		};

		struct commQryMarkPxNode
		{
			char symbol[InstrumentIDLen+1];
			double markPx;
			uint64_t updatedTime;
			double fundingRate;
			commQryMarkPxNode() {
				memset(this, 0, sizeof(commQryMarkPxNode));
			}
		};
		
		struct QueueNode
		{
			QueueNode()
			{
				Tid = 0;
				Data = nullptr;
			}
			int Tid;
			void *Data;
		};

		struct SetOrderOptions
		{
			int checkStrategyCircuit; // Always 1, except QC!!!
			int checkSelfCrossing;
			int orderType;
			int isForcedOpen = 0;
			int isReduceOnly = 0;
			char exchangeCode[ExchangeCodeLen+1];
			char TimeInForce[TimeInForceLen+1];
			char Category[CategoryLen+1];
			char MTaker[MTakerLen+1];
			char CoinType[CoinTypeLen+1];
			SetOrderOptions() {
			    memset(this, 0, sizeof(SetOrderOptions));
				checkStrategyCircuit = 1; 
				orderType = ORDERTYPE_DEFAULT;
			}
//			char orderTag[TagLen + 1] = "";
		};

		struct SetOrderInfo
		{
			char Direction;
			uint32_t cancelSize;
			double  Quantity;
			double Price;
			bool Finished;
			SetOrderOptions setOrderOptions;
			Order *pOrder;
		};

		class ApiInfoDetail
		{
		public:
			ApiInfoDetail()
			{
				tradingId = 0;
				memset(userId_, 0, sizeof(userId_));
				memset(investorId_, 0, sizeof(investorId_));
				memset(passwd_, 0, sizeof(passwd_));
				memset(frontTdAddr_, 0, sizeof(frontTdAddr_));
				memset(frontQueryAddr_, 0, sizeof(frontQueryAddr_));
				memset(frontMdAddr_, 0, sizeof(frontMdAddr_));
				memset(InterfaceAddr_, 0, sizeof(InterfaceAddr_));
				memset(type_, 0, sizeof(type_));

				frontId_ = 0;
				threadNum_ = 0;
				subAll_ = 0; 
				interfaceID_ = 0;
				orderRef_ = 0;
				pMdApi_ = nullptr;
				pTdApi_ = nullptr;
				pTdSpi_ = nullptr;
				mdInit = false;
				tdInit = false;
				mdLevelType_ = MDLEVELTYPE_DEFAULT;
			}
			int  tradingId;
			char userId_[128 + 1];
			char investorId_[63 + 1];
			char passwd_[128 + 1];
			bool mdInit;
			bool tdInit;
			char frontTdAddr_[130 + 1];
			char frontQueryAddr_[130 + 1];
			char frontMdAddr_[130 + 1];
			char mdLevelType_;
			char InterfaceAddr_[30 + 1];
			int frontId_;
			int threadNum_;
			char type_[63+1];
			int subAll_;
			int interfaceID_;
			volatile int orderRef_; //���޸�
			void* pMdApi_;
			void* pTdApi_;
			void *pTdSpi_;
			void *pMdSpi_;
			char exchangeCode_[20 + 1];
		};
		class UserDetail
		{
		public:
			UserDetail()
			{
				apiType_ = 0;
				memset(userId_, 0, sizeof(userId_));
				memset(userPasswd_, 0, sizeof(userPasswd_));
				memset(investorId_, 0, sizeof(investorId_));
			}
			~UserDetail()
			{
				investMap_.clear();
			}
			int apiType_;
			char userId_[UserIDLen + 1];
			char userPasswd_[40 + 1];
			char investorId_[InvestorIDLen + 1];
			std::unordered_map<string, string> investMap_; //investor
		};
		typedef std::shared_ptr<UserDetail> UserDetailPtr;
		class UserInfo
		{
		public:
			UserInfo() { ; }
			~UserInfo()
			{
				userInfoMap_.clear();
			}

			typedef std::unordered_map<string, UserDetailPtr> UserInfoMap; //userid + userDetailMap
			UserInfoMap userInfoMap_;
			UserDetailPtr getUserDetail(string userid);
		};

		typedef std::shared_ptr<ApiInfoDetail> ApiDetailPtr;
		class GateWayApiInfo
		{
		public:
			GateWayApiInfo() { ; }
			~GateWayApiInfo()
			{
				apiInfoMap_.clear();
			}

			typedef std::unordered_map<string, ApiDetailPtr> ApiInfoMap;
			ApiDetailPtr getApiDetail(string investorId);
			ApiDetailPtr getApiDetailByType(string type);
			ApiInfoMap apiInfoMap_;
		};
		class SignalValue
		{
		public:
			SignalValue(const string& currTime, const string& instID, int signalID, double value, const string& refInstID)
				:currSystemTime_(currTime), instrumentID_(instID), signalID_(signalID), signalValue_(value), refInstrumentID_(refInstID)
			{};
			string currSystemTime_;
			string instrumentID_;
			int signalID_;
			double signalValue_;
			string refInstrumentID_;
		};
		class SimTrade
		{
		public:
			SimTrade(int strategyID, const string& instrumentID, const string& tradingDay, const string& eventID,
				const string& tradeTime, char direction, double price, int tradeSize)
				:strategyID_(strategyID), instrumentID_(instrumentID), tradingDay_(tradingDay), eventID_(eventID),
				tradeTime_(tradeTime), tradeDirection_(direction), tradePrice_(price), tradeSize_(tradeSize)
			{};
			int strategyID_;
			string instrumentID_;
			string tradingDay_;
			string eventID_;
			string tradeTime_;
			char   tradeDirection_;
			double tradePrice_;
			int    tradeSize_;
		};

		struct InnerMarketData
		{
			// member has volatile cannot be memset or memcpy
			InnerMarketData() {
				memset(this, 0, sizeof(InnerMarketData));
			}
			// InnerMarketData() {
			// 	memset(TradingDay, 0, TradingDayLen+1);
			// 	memset(InstrumentID, 0, InstrumentIDLen+1);
			// 	memset(ExchangeID, 0, ExchangeIDLen+1);
			// 	memset(UpdateTime, 0, UpdateTimeLen+1);
			// 	LastPrice = 0;
			// }
			volatile uint64_t  StartSeqNum;
			char	TradingDay[TradingDayLen + 1];
			char	InstrumentID[InstrumentIDLen + 1];
			char  ExchangeID[ExchangeIDLen + 1];
			double	LastPrice;
			double	HighestPrice;
			double	LowestPrice;
			double	Volume;
			double	Turnover;
			double	BidPrice1;
			double	BidVolume1;
			double	AskPrice1;
			double	AskVolume1;
			double	BidPrice2;
			double	BidVolume2;
			double	AskPrice2;
			double	AskVolume2;
			double	BidPrice3;
			double	BidVolume3;
			double	AskPrice3;
			double	AskVolume3;
			double	BidPrice4;
			double	BidVolume4;
			double	AskPrice4;
			double	AskVolume4;
			double	BidPrice5;
			double	BidVolume5;
			double	AskPrice5;
			double	AskVolume5;
			char 	LevelType;

			char	UpdateTime[UpdateTimeLen + 1];
			uint64_t	UpdateMillisec;
			int MessageID;
			uint64_t EpochTime;
			volatile uint64_t  LastSeqNum; //identify old udp package
			volatile uint64_t  EndSeqNum;

			double getBidPrice(int level) const
			{
				switch (level)
				{
				case 1:
					return BidPrice1;
				case 2:
					return BidPrice2;
				case 3:
					return BidPrice3;
				case 4:
					return BidPrice4;
				case 5:
					return BidPrice5;
				default:
					return NAN;
				}
			}
			void setBidPrice(int level, double price)
			{
				switch (level)
				{
				case 1:
					BidPrice1 = price;
					break;
				case 2:
					BidPrice2 = price;
					break;
				case 3:
					BidPrice3 = price;
					break;
				case 4:
					BidPrice4 = price;
					break;
				case 5:
					BidPrice5 = price;
					break;
				}
			}
			double getAskPrice(int level) const
			{
				switch (level)
				{
				case 1:
					return AskPrice1;
				case 2:
					return AskPrice2;
				case 3:
					return AskPrice3;
				case 4:
					return AskPrice4;
				case 5:
					return AskPrice5;
				default:
					return NAN;
				}
			}
			void setAskPrice(int level, double price)
			{
				switch (level)
				{
				case 1:
					AskPrice1 = price;
					break;
				case 2:
					AskPrice2 = price;
					break;
				case 3:
					AskPrice3 = price;
					break;
				case 4:
					AskPrice4 = price;
					break;
				case 5:
					AskPrice5 = price;
					break;
				}
			}
			double getBidVolume(int level) const
			{
				switch (level)
				{
				case 1:
					return BidVolume1;
				case 2:
					return BidVolume2;
				case 3:
					return BidVolume3;
				case 4:
					return BidVolume4;
				case 5:
					return BidVolume5;
				default:
					return 0;
				}
			}
			void setBidVolume(int level, double volume)
			{
				switch (level)
				{
				case 1:
					BidVolume1 = volume;
					break;
				case 2:
					BidVolume2 = volume;
					break;
				case 3:
					BidVolume3 = volume;
					break;
				case 4:
					BidVolume4 = volume;
					break;
				case 5:
					BidVolume5 = volume;
					break;
					;
				}
			}
			double getAskVolume(int level) const
			{
				switch (level)
				{
				case 1:
					return AskVolume1;
				case 2:
					return AskVolume2;
				case 3:
					return AskVolume3;
				case 4:
					return AskVolume4;
				case 5:
					return AskVolume5;
				default:
					return 0;
				}
			}
			void setAskVolume(int level, double volume)
			{
				switch (level)
				{
				case 1:
					AskVolume1 = volume;
					break;
				case 2:
					AskVolume2 = volume;
					break;
				case 3:
					AskVolume3 = volume;
					break;
				case 4:
					AskVolume4 = volume;
					break;
				case 5:
					AskVolume5 = volume;
					break;
				}
			}
		};

		struct InnerMarketTrade
		{
			// member has volatile cannot be memset or memcpy
			InnerMarketTrade() {
				memset(this, 0, sizeof(InnerMarketTrade));
			}
			// InnerMarketTrade() {
			// 	memset(TradingDay, 0, TradingDayLen+1);
			// 	memset(InstrumentID, 0, InstrumentIDLen+1);
			// 	memset(ExchangeID, 0, ExchangeIDLen+1);
			// 	memset(UpdateTime, 0, UpdateTimeLen+1);
			// }
			char TradingDay[TradingDayLen + 1];
			char InstrumentID[InstrumentIDLen + 1];
			char ExchangeID[ExchangeIDLen + 1];
			char Tid[ParamNameLen+1];
			char Direction;
			double Price;
			double Volume;
			double Turnover;
			char UpdateTime[UpdateTimeLen + 1];
			uint64_t	UpdateMillisec;
			///��Ϣid
			int MessageID;
			uint64_t EpochTime;
			volatile uint64_t LastSeqNum; //identify old package
			volatile uint64_t  StartSeqNum;
			volatile uint64_t  EndSeqNum;
			// char ExecType;

		};

		struct InnerBookTicker
		{
			InnerBookTicker() {
				memset(InstrumentID, 0, sizeof(InstrumentID));
				TickerId = 0;
				ReceiveTime = 0;
				MatchTime = 0;
				bidPrice = 0.0;
				bidAmount = 0.0;
				askPrice = 0.0;
				askAmount = 0.0;
			}
			char InstrumentID[InstrumentIDLen + 1];

			uint64_t TickerId;

			uint64_t ReceiveTime;

			uint64_t MatchTime;

			double bidPrice;

			double bidAmount;

			double askPrice;

			double askAmount;
		};
		struct InnerMarketOrder
		{
			// member has volatile cannot be memset or memcpy
			// InnerMarketOrder() {
			// 	memset(this, 0, sizeof(InnerMarketOrder));
			// }
			InnerMarketOrder() {
				memset(TradingDay, 0, TradingDayLen+1);
				memset(InstrumentID, 0, InstrumentIDLen+1);
				memset(ExchangeID, 0, ExchangeIDLen+1);
				memset(UpdateTime, 0, UpdateTimeLen+1);
				memset(OrderSysID, 0, OrderSysIDLen+1);
			}
			char TradingDay[TradingDayLen + 1];
			char InstrumentID[InstrumentIDLen + 1];
			char ExchangeID[ExchangeIDLen + 1];
			char OrderSysID[OrderSysIDLen + 1];
			char Direction;
			double Price;
			double Volume;
			char UpdateTime[UpdateTimeLen + 1];
			int	UpdateMillisec;
			int MessageID;
			uint64_t EpochTime;
			volatile uint64_t LastSeqNum; //identify old package
			char OrdType;//
			uint64_t OrderNO;
			uint64_t BizIndex;
		};

		const static uint16_t TradingAccountType = 15;
		const static uint16_t InstrumentType = 16;
		const static uint16_t InstrumentEndType = 17;
		const static uint16_t MdRspUserLoginType = 18;
		const static uint16_t InnerMarketDataType = 19;
		const static uint16_t RspOrderRejectType = 20;
		const static uint16_t CancelOrderRejectType = 21;

		const static uint16_t RspErrorType = 22;
		const static uint16_t RtnOrderType = 23;
		const static uint16_t RtnTradeType = 24;
		const static uint16_t RtnCancelTradeType = 34;
		const static uint16_t RtnTradeVolumeType = 35;
		const static uint16_t SettlementInfoConfirmType = 25;
		const static uint16_t ErrRtnOrderCancelType = 26;
		const static uint16_t RspInvestorPositionType = 27;
		const static uint16_t RspQryOrderType = 28;
		const static uint16_t RspOrderCancelType = 29;
		const static uint16_t RspQryTradeType = 30;
		const static uint16_t DeferDeliveryQuotType = 31;
		const static uint8_t  HeartBeatType = 32;
		const static uint16_t RtnRejectType = 33;

		const static string InterfaceSimulator = "Simulator";
		const static string InterfaceSim = "Sim";
		const static string InterfaceDistMd = "DistMd";//
		const static string InterfaceShmMd = "ShmMd";//
		const static string InterfaceShmMt = "ShmMt";//
		const static string InterfaceTapNeiPan = "TapNeiPan";//()		

		//
		const static string InterfaceOKEx = "OKEx";
		const static string InterfaceOKExV5 = "OKEX";
		const static string InterfaceBybit = "BYBIT";
		const static string InterfaceBitstamp = "Bitstamp";
		const static string InterfaceHuob = "Huob";
		const static string InterfaceHuobFuture = "HuobFuture";
		const static string InterfaceBitfinex = "Bitfinex";
		const static string InterfaceBinance = "BINANCE";
		const static string InterfaceCoinSuper = "CoinSuper";
		const static string InterfaceDeribit = "Deribit";
		const static string InterfaceQsdex = "Qsdex";
		const static string InterfaceUnKnow = "UnKnow";

		const static uint16_t BUFFER_SIZE = 1024;

		enum OPTION_RISK_TYPE
		{
			OPTION_RISK_TYPE_DELTA = 0,
			OPTION_RISK_TYPE_VEGA,
			OPTION_RISK_TYPE_GAMMA
		};

#pragma pack(push, 1)
#pragma pack(pop)

		template<typename T>
		void MarketDataToInField(const T *source, InnerMarketData &field)
		{
			memset(&field, 0, sizeof(InnerMarketData));
			field.EpochTime = CURR_MSTIME_POINT;
			field.MessageID = getMessageID();
			unsigned int len = strlen(source->TradingDay);
			memcpy(field.TradingDay, source->TradingDay, (len < TradingDayLen ? len : TradingDayLen));

			len = strlen(source->InstrumentID);
			memcpy(field.InstrumentID, source->InstrumentID, (len < InstrumentIDLen ? len : InstrumentIDLen));

			len = strlen(source->ExchangeID);
			memcpy(field.ExchangeID, source->ExchangeID, (len < ExchangeIDLen ? len : ExchangeIDLen));
			field.LastPrice = source->LastPrice;

			field.LowestPrice = source->LowestPrice;
			field.Volume = source->Volume;
			field.Turnover = source->Turnover;

			field.BidPrice1 = source->BidPrice1;
			field.BidVolume1 = source->BidVolume1;
			field.AskPrice1 = source->AskPrice1;
			field.AskVolume1 = source->AskVolume1;
			field.BidPrice2 = source->BidPrice2;
			field.BidVolume2 = source->BidVolume2;
			field.AskPrice2 = source->AskPrice2;
			field.AskVolume2 = source->AskVolume2;
			field.BidPrice3 = source->BidPrice3;
			field.BidVolume3 = source->BidVolume3;
			field.AskPrice3 = source->AskPrice3;
			field.AskVolume3 = source->AskVolume3;
			field.BidPrice4 = source->BidPrice4;
			field.BidVolume4 = source->BidVolume4;
			field.AskPrice4 = source->AskPrice4;
			field.AskVolume4 = source->AskVolume4;
			field.BidPrice5 = source->BidPrice5;
			field.BidVolume5 = source->BidVolume5;
			field.AskPrice5 = source->AskPrice5;
			field.AskVolume5 = source->AskVolume5;
			len = strlen(source->UpdateTime);
			memcpy(field.UpdateTime, source->UpdateTime, (len < UpdateTimeLen ? len : UpdateTimeLen));
			field.UpdateMillisec = source->UpdateMillisec;
		}

		enum QryType
		{
			QRYPOSI = 'P',
			QRYMPRICE = 'M',
			QRYORDER = 'R'
		};

		enum RcType
		{
			CancelAll = 'A'
		};

		enum OrderStatus
		{
			Partiallyfilled = 'P',
			Filled = 'F',
			New = 'A',//ACKED
			Cancelled = 'X',
			PendingCancel = 'C',
			PendingNew = 'N',
			NewRejected = 'R',
			CancelRejected = 'L',
			Suspended = 'S',
			Error = 'E'
		};

		enum ExecTypeState
		{
			Trade = 'T',
			AdlTrade = 'A', // 自動�???
			Funding = 'F', //資金費率
			BustTrade = 'B', // 強平
			Delivery = 'D', // USDC交割
			BlockTrade = 'E'
		};

		enum MarketDataQueueMode
		{
			Log = 0,
		    IndexServer = 1,
			CffexOffset = 2
		};
		struct MarketDataOffset
		{
			//char	InstrumentID[InstrumentIDLen + 1];
			long long LastPrice;
			long long AskPrice1;
			int AskVolume1;
		};
	}//gtway
	using namespace spot::base;
	unsigned getMin(unsigned int length1, unsigned int length2);

	void setTradeLoginStatusMap(string interfaceType,  bool status, int interfaceID =0 );
	bool isTradeLogin(string interfaceType, int interfaceID = 0);
	bool isCryptoExchange(const string& exchangeCode);
	void logInfoInnerOrder(string stage, const Order& innerOrder);
	void logExampleInnerOrder(string stage, const Order& innerOrder);

	extern double btc_mmr[10][5];

	extern double btc_cm_mmr[10][5];

	extern double eth_mmr[11][5];

	extern double eth_cm_mmr[11][5];

	struct TableInfo {
		TableInfo(string str, int r, int c) {
			memset(table_name, 0, 15);
			memcpy(table_name, str.c_str(), min(str.size(), sizeof(table_name)));
			rows = r;
			cols = c;
		}
		double** data;
		char table_name[15];
		int rows;  
		int cols;  
	};  

	extern vector<TableInfo> mmr_table;

	extern map<string, double> collateralRateMap;
	//mmr_table.push_back({reinterpret_cast<double*>(btc_mmr), 10, 5});  
	//cancel order attempts before send command
	extern int maxCancelOrderAttempts;

	extern bool gIsMqDispatcherRun;
	extern string gShmKey;
	extern string gBroadcastAddr;
	extern string gLocalAddr;
	extern int gStrategyThreadMsgID;
	extern atomic<bool> gCircuitBreakTimerActive;
	extern atomic<bool> gForceCloseTimeIntervalActive;
	extern atomic<bool> gCancelOrderActive;
	extern atomic<bool> gResetTimeOrderLimit;
	extern atomic<bool> gBianListenKeyFlag;
	extern atomic<bool> gBybitQryFlag;
	
	extern set<string> exchList;
	extern atomic<bool> gOkPingPongFlag;
	extern atomic<bool> gByPingPongFlag;

	extern atomic<int> bianAskPendingNum; 
	extern atomic<int> bianBidPendingNum; 
	extern bool gIsGuava;

	extern map<string, commQryPosiNode> gQryPosiInfo;
	extern map<string, bool> gIsPosiQryInfoSuccess;

	extern map<string, commQryMarkPxNode> gQryMarkPxInfo;
	extern map<string, bool> gIsMarkPxQryInfoSuccess;

	extern atomic<int> okAskPendingNum;
	extern atomic<int> okBidPendingNum; 

/*
	double btc_mmr[10][5] = {
		{0,	 50000, 	125,	0.004,	0},
		{50000, 500000,	100,	0.0050,	50},
		{500000, 10000000,	50,	0.01,	2550},
		{10000000, 80000000,	20,	0.025, 152550},
		{80000000, 150000000,	10,	0.05,	2152550},
		{150000000, 300000000,	5,	0.1,	9652550},
		{300000000, 450000000,	4,	0.125,  17152550},
		{450000000, 600000000,	3,	0.15,	28402550},
		{600000000, 800000000,	2,	0.25,	88402550},
		{800000000, 1000000000,	1,	0.5,	288402550}
	};

	double btc_cm_mmr[10][5] = {
		{0,  5, 	125,	0.004,	0},
		{5,  10, 	100,	0.005,	0.005},
		{10, 20,	50, 	0.001,	0.055},
		{20, 50,	20,	    0.025,	0.355},
		{50, 100,	10, 	0.05,	1.605},
		{100, 200,	5,		0.1,	6.605},
		{200, 400,	4,		0.125,	11.605},
		{400, 1000,	3,		0.15,	21.605},
		{1000, 1500,	2,	0.25,	121.605},
		{1500, 2500,	1,	0.5,	496.605}
	};

	double eth_mmr[11][5] = {
		{0,		 50000,		125,	0.004,	0},
		{50000,  	500000,		100,	0.005,		50},
		{500000, 	1000000,		75,		0.0065,		800},
		{1000000, 	5000000,	50,		0.01,		4300},
		{5000000,	 50000000,	20,		0.02,		54300},
		{50000000, 	 100000000,	10,		0.05,		1554300},
		{100000000,  150000000,	5,		0.1,		6554300},
		{150000000,  300000000,	4,		0.125,		10304300},
		{300000000,  400000000,	3,		0.15,		17804300},
		{400000000,  500000000,	2,		0.25,		57804300},
		{500000000,  800000000,	1,		0.5,		182804300}
	};

	double eth_cm_mmr[11][5] = {
		{0, 15,	100,	0.0050,	0},
		{15, 100,	75,	0.0065,	0.0225},
		{100, 250,	50,	0.01,	0.3725},
		{250, 500,	20,	0.025,	4.1225},
		{500, 1000,	10,	0.05,	16.6225},
		{1000, 2000, 5,	0.10,	66.6225},
		{2000, 2500, 4,	0.125,	116.6225},
		{2500, 5000, 3,	0.15,	179.1225},
		{5000, 10000,	2,	0.25,	679.1225},
		{10000, 50000,	1,	0.50,	3179.1225}
	};
*/
}
#endif