#include "spot/strategy/Position.h"
#include "spot/utility/Logging.h"
#include "spot/utility/Utility.h"
#include "spot/utility/TradingDay.h"
#include "spot/base/InitialData.h"
using namespace spot::base;
using namespace spot::utility;
Position::Position()
{
	memset(&pnlDaily_, 0, sizeof(StrategyInstrumentPNLDaily));
	pnlDaily_.AvgBuyPrice = 0.0;
	pnlDaily_.AvgSellPrice = 0.0;
	pnlDaily_.AggregatedFeeByRate = 0.0;
}
Position::~Position()
{
}

void Position::setInstrument(Instrument* instrument)
{
	instrument_ = instrument;
}

void Position::setPnlDaily(const StrategyInstrumentPNLDaily PnlDaily)
{
	pnlDaily_ = PnlDaily;	
}

void Position::initPosition(const StrategyInstrumentPNLDaily &position)
{
	double netPosition = position.TodayLong - position.TodayShort;
	if (!IS_DOUBLE_EQUAL(netPosition, position.NetPosition))
	{
		LOG_FATAL << "NetPosition != YL+TL-YS-TS strategyId:" << position.StrategyID << " instrumentId:" << position.InstrumentID
			<< " TodayLong:" << position.TodayLong << " TodayShort:" << position.TodayShort
			<< " NetPosition:" << position.NetPosition;
	}
	memcpy(&pnlDaily_,&position, sizeof(pnlDaily_));
	LOG_INFO << "initPosition:" << pnlDaily_.toString();
}

double Position::aggregatedFee()
{
	return pnlDaily_.AggregatedFeeByRate;
}

StrategyInstrumentPNLDaily Position::OnRtnOrder(const Order &rtnOrder)
{
	std::string today = TradingDay::getToday();
	memcpy(pnlDaily_.TradingDay, today.c_str(), min(TradingDayLen, uint16_t(today.size())));
	updatePosition(rtnOrder);
	updatePnlInfo(rtnOrder);
	checkPnlInfo();
	return pnlDaily_;
}

// ����ģʽ���²�λ
void Position::updatePositionBySingle(const Order &rtnOrder)
{
	double coinSize = 1;
	if (strcmp(rtnOrder.ExchangeCode, "OKEX") == 0) {
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(rtnOrder.InstrumentID);
		coinSize = symbolInfo->CoinOrderSize;
	}
	if (rtnOrder.Direction == INNER_DIRECTION_Buy) {
		pnlDaily_.TodayLong += rtnOrder.Volume * coinSize;
		LOG_INFO << "updatePositionBySingle INNER_DIRECTION_Buy OrderRef: " << rtnOrder.OrderRef << ", fill volume: " << rtnOrder.Volume << ", coinSize: " << coinSize
			<< ", ExchangeCode: " << rtnOrder.ExchangeCode; 
	} else if (rtnOrder.Direction == INNER_DIRECTION_Sell) {
		pnlDaily_.TodayShort += rtnOrder.Volume * coinSize;
		LOG_INFO << "updatePositionBySingle INNER_DIRECTION_Sell OrderRef: " << rtnOrder.OrderRef << ", fill volume: " << rtnOrder.Volume << ", coinSize: " << coinSize
			<< ", ExchangeCode: " << rtnOrder.ExchangeCode; 
	} else {
		LOG_FATAL << "updatePositionBySingle WR Direction: " << rtnOrder.Direction;
	}
}

// ˫��ģʽ���²�λ
void Position::updatePositionByDouble(const Order &rtnOrder)
{
	double coinSize = 1;
	if (strcmp(rtnOrder.ExchangeCode, "OKEX") == 0) {
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(rtnOrder.InstrumentID);
		coinSize = symbolInfo->CoinOrderSize;
	}
	if (rtnOrder.Direction == INNER_DIRECTION_Buy && rtnOrder.Offset == INNER_OFFSET_CloseToday) {
		pnlDaily_.TodayLong -= rtnOrder.Volume * coinSize;
	} else if (rtnOrder.Direction == INNER_DIRECTION_Buy && rtnOrder.Offset == INNER_OFFSET_Open) {
		pnlDaily_.TodayLong += rtnOrder.Volume * coinSize;
	} else if (rtnOrder.Direction == INNER_DIRECTION_Sell && rtnOrder.Offset == INNER_OFFSET_CloseToday) {
		pnlDaily_.TodayShort -= rtnOrder.Volume * coinSize;
	} else if (rtnOrder.Direction == INNER_DIRECTION_Sell && rtnOrder.Offset == INNER_OFFSET_Open) {
		pnlDaily_.TodayShort += rtnOrder.Volume * coinSize;
	} else {
		LOG_FATAL << "updatePositionByDouble WR Direction: " << rtnOrder.Direction << ", Offset: " << rtnOrder.Offset;
	}
}
void Position::updatePosition(const Order &rtnOrder)
{
	double tempNet = pnlDaily_.NetPosition;
	if ((rtnOrder.ExchangeCode == Exchange_BINANCE) 
	|| (rtnOrder.ExchangeCode == Exchange_OKEX)
	|| (rtnOrder.ExchangeCode == Exchange_BYBIT)) {
		updatePositionBySingle(rtnOrder);
	} else {
		updatePositionByDouble(rtnOrder);
	}

	pnlDaily_.NetPosition = pnlDaily_.TodayLong - pnlDaily_.TodayShort;

	// calc EntryPrice
	if (IS_DOUBLE_EQUAL(pnlDaily_.NetPosition, 0)) {
		pnlDaily_.EntryPrice = 0;
	} else if (IS_DOUBLE_LESS_EQUAL(tempNet * pnlDaily_.NetPosition, 0)) {
		pnlDaily_.EntryPrice = rtnOrder.Price;
	} else if (IS_DOUBLE_GREATER(abs(pnlDaily_.NetPosition), abs(tempNet))) {
		pnlDaily_.EntryPrice = ((pnlDaily_.EntryPrice * abs(tempNet) + rtnOrder.Price * rtnOrder.Volume)) / 
			abs(pnlDaily_.NetPosition);
	}

	LOG_INFO << "updatePosition instrument: " <<pnlDaily_.InstrumentID
		<< ", orderRef: " << rtnOrder.OrderRef 
		<< ", exch: " << rtnOrder.ExchangeCode
		<<  ", NetPosition:  " << pnlDaily_.NetPosition << ", LONG: " << pnlDaily_.TodayLong
		<< ", short: " << pnlDaily_.TodayShort
		<< ", EntryPrice: " << pnlDaily_.EntryPrice 
		<< ", tempNet: " << tempNet
		<< ", volume: " << rtnOrder.Volume
		<< ", price: " << rtnOrder.Price;
}

void Position::updateUPnlInfo(const Order &rtnOrder)
{
	double coinSize = 1;
	if (strcmp(rtnOrder.ExchangeCode, "OKEX") == 0) {
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(rtnOrder.InstrumentID);
		coinSize = symbolInfo->CoinOrderSize;
	}
	if (rtnOrder.Direction == INNER_DIRECTION_Buy)
	{
		pnlDaily_.AvgBuyPrice = avgPrice(pnlDaily_.BuyQuantity, pnlDaily_.AvgBuyPrice, rtnOrder.Volume * coinSize, rtnOrder.Price);
		pnlDaily_.BuyQuantity += rtnOrder.Volume * coinSize;
		pnlDaily_.Turnover += rtnOrder.Volume * coinSize;
	}
	else if (rtnOrder.Direction == INNER_DIRECTION_Sell)
	{
		pnlDaily_.AvgSellPrice = avgPrice(pnlDaily_.SellQuantity, pnlDaily_.AvgSellPrice, rtnOrder.Volume * coinSize, rtnOrder.Price);
		pnlDaily_.SellQuantity += rtnOrder.Volume * coinSize;
		pnlDaily_.Turnover += rtnOrder.Volume * coinSize;
	}
	calcAggregateFee(rtnOrder);
	pnlDaily_.Profit = (instrument_->orderBook()->midPrice() - pnlDaily_.AvgBuyPrice) * pnlDaily_.BuyQuantity
		+ (pnlDaily_.AvgSellPrice - instrument_->orderBook()->midPrice()) * pnlDaily_.SellQuantity;
}

void Position::updateCPnlInfo(const Order &rtnOrder)
{
	double coinSize = 1;
	if (strcmp(rtnOrder.ExchangeCode, "OKEX") == 0) {
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(rtnOrder.InstrumentID);
		coinSize = symbolInfo->CoinOrderSize;
	}
	if (rtnOrder.Direction == INNER_DIRECTION_Buy)
	{
		// double temVol = rtnOrder.Volume * instrument_->getMultiplier() / rtnOrder.Price;
		pnlDaily_.AvgBuyPrice = avgPrice(pnlDaily_.BuyQuantity, pnlDaily_.AvgBuyPrice, rtnOrder.Volume, rtnOrder.Price);
		// pnlDaily_.BuyQuantity += rtnOrder.Volume * instrument_->getMultiplier() / rtnOrder.Price;
		pnlDaily_.BuyQuantity += rtnOrder.Volume;
	}
	else if (rtnOrder.Direction == INNER_DIRECTION_Sell)
	{
		// double temVol = rtnOrder.Volume * instrument_->getMultiplier() / rtnOrder.Price;
		pnlDaily_.AvgSellPrice = avgPrice(pnlDaily_.SellQuantity, pnlDaily_.AvgSellPrice, rtnOrder.Volume, rtnOrder.Price);
		// pnlDaily_.SellQuantity += rtnOrder.Volume * instrument_->getMultiplier() / rtnOrder.Price;
		pnlDaily_.SellQuantity += rtnOrder.Volume;
	}
	calcAggregateFee(rtnOrder);
	pnlDaily_.Profit = (instrument_->orderBook()->midPrice() - pnlDaily_.AvgBuyPrice) * pnlDaily_.BuyQuantity
		+ (pnlDaily_.AvgSellPrice - instrument_->orderBook()->midPrice()) * pnlDaily_.SellQuantity;
}

void Position::updatePnlInfo(const Order &rtnOrder)
{
	if (strcmp(rtnOrder.Category, INVERSE.c_str()) == 0) {
		updateCPnlInfo(rtnOrder);
	} else {
		updateUPnlInfo(rtnOrder);
	}
	LOG_INFO << "Position symbol: " << rtnOrder.InstrumentID
		<< ", timeInForce: " << rtnOrder.TimeInForce
		<< ", side: " <<  rtnOrder.Direction
		<< ", Profit: " <<  pnlDaily_.Profit
		<< ", orderRef: " << rtnOrder.OrderRef
		<< ", LimitPrice: " << rtnOrder.LimitPrice
		<< ", TradePrice: " << rtnOrder.Price
		<< ", MdPirce: " << instrument_->orderBook()->midPrice()
		<< ", TradeVolume: " << rtnOrder.Volume
		<< ", Category: " << rtnOrder.Category
		<< ", AvgBuyPrice: " << pnlDaily_.AvgBuyPrice
		<< ", AvgSellPrice: " << pnlDaily_.AvgSellPrice
		<< ", BuyQuantity: " << pnlDaily_.BuyQuantity
		<< ", SellQuantity: " << pnlDaily_.SellQuantity
		<< ", NetPosition: " << pnlDaily_.NetPosition
		<< ", AggregatedFee: " << pnlDaily_.AggregatedFee;
}
void Position::calcAggregateFee(const Order &rtnOrder)
{
	pnlDaily_.AggregatedFee = pnlDaily_.AggregatedFee + rtnOrder.Fee;
}

void Position::calcAggregateFeeByRate(const Order &rtnOrder)
{
	double coinSize = 1;
	if (strcmp(rtnOrder.ExchangeCode, "OKEX") == 0) {
		SymbolInfo *symbolInfo = InitialData::getSymbolInfo(rtnOrder.InstrumentID);
		coinSize = symbolInfo->CoinOrderSize;
	}
	// maker��
	if (FEETYPE_MAKER.compare(rtnOrder.MTaker) == 0)
	{
		if(instrument_->getInstrumentMakerFee())
			pnlDaily_.AggregatedFeeByRate += getTransactionFee(instrument_->getInstrumentMakerFee(), rtnOrder.Volume * coinSize, rtnOrder.Price);
		else
			pnlDaily_.AggregatedFeeByRate += getTransactionFee(instrument_->getMakerFee(), rtnOrder.Volume * coinSize, rtnOrder.Price);
	}
	// taker��
	else {
		if(instrument_->getInstrumentTakerFee())
			pnlDaily_.AggregatedFeeByRate += getTransactionFee(instrument_->getInstrumentTakerFee(), rtnOrder.Volume * coinSize, rtnOrder.Price);
		else
			pnlDaily_.AggregatedFeeByRate += getTransactionFee(instrument_->getTakerFee(), rtnOrder.Volume * coinSize, rtnOrder.Price);
	}

}
double Position::getTransactionFee(const SymbolTradingFee &tradingFee,double quantity, double price)
{
	if (FEEFORMAT_PERCENTAGE.compare(tradingFee.FeeFormat) == 0)
	{
		return price*quantity*(tradingFee.FeeRate);
	}
	return quantity*(tradingFee.FeeRate);
}
double Position::getTransactionFee(InstrumentTradingFee *tradingFee, double quantity, double price)
{
	if (FEEFORMAT_PERCENTAGE.compare(tradingFee->FeeFormat) == 0)
	{
		return price*quantity*(tradingFee->FeeRate);
	}
	return quantity*(tradingFee->FeeRate);
}
double Position::avgPrice(double lastQuantity, double lastPrice, double newQuantity, double newPrice)
{
	if (IS_DOUBLE_ZERO(lastQuantity + newQuantity))
	{
		return 0.0;
	}

	return (lastQuantity*lastPrice + newQuantity*newPrice) / (lastQuantity + newQuantity);
}

double Position::avgPriceOK(Instrument* instrument, char direction, double newQuantity, double newPrice)
{
	if (direction == INNER_DIRECTION_Buy)
	{
		double rtnOrderRatio = IS_DOUBLE_ZERO(pnlDaily_.AvgSellPrice) ? 0.0 : newPrice / pnlDaily_.AvgSellPrice;
		return avgPrice(pnlDaily_.BuyQuantity, pnlDaily_.AvgBuyPrice, newQuantity, rtnOrderRatio);
	}
	else if (direction == INNER_DIRECTION_Sell)
	{
		double rtnOrderRatio = IS_DOUBLE_ZERO(pnlDaily_.AvgBuyPrice) ? 0.0 : newPrice / pnlDaily_.AvgBuyPrice;
		return avgPrice(pnlDaily_.SellQuantity, pnlDaily_.AvgSellPrice, newQuantity, rtnOrderRatio);
	}
	else
	{
		return 0.0;
	}
}

void Position::checkPnlInfo()
{
	if(IS_DOUBLE_LESS(pnlDaily_.TodayLong, -0.0000001)
		|| IS_DOUBLE_LESS(pnlDaily_.TodayShort,-0.0000001)
		|| IS_DOUBLE_LESS(pnlDaily_.BuyQuantity, -0.0000001)
		|| IS_DOUBLE_LESS(pnlDaily_.SellQuantity, -0.0000001)
		|| IS_DOUBLE_LESS(pnlDaily_.AvgBuyPrice, 0.0)
		|| IS_DOUBLE_LESS(pnlDaily_.AvgSellPrice, 0.0)
		|| IS_DOUBLE_LESS(pnlDaily_.Turnover, 0.0))
	{
    string buffer = "pnlDaily error: ";
    buffer = buffer + pnlDaily_.toString();
		LOG_ERROR  << buffer.c_str();
	}
}
