#include "StrategyFR.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Measure.h"
#include "spot/base/InitialData.h"
#include "spot/cache/CacheAllocator.h"
#include "spot/utility/TradingDay.h"
#include "spot/bian/BnApi.h"

using namespace spot::strategy;
// using namespace spot::risk;

Strategy *StrategyFR::Create(int strategyID, StrategyParameter *params) {
    char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyFR));
    return new(buff) StrategyFR(strategyID, params);
}

StrategyFR::StrategyFR(int strategyID, StrategyParameter *params)
        : StrategyCircuit(strategyID, params) 
{
    margin_leverage = new map<string, double>; // {'BTC':10, 'ETH':10,'ETC':10,'default':5}
    margin_leverage.insert({"BTC", 10});
    margin_leverage.insert({"ETH", 10});
    margin_leverage.insert({"ETC", 10});
    margin_leverage.insert({"default", 5});

    margin_mmr = new map<string, double>;
    margin_mmr.insert({3, 0.1}); // {3:0.1, 5:0.08, 10:0.05}
    margin_mmr.insert({5, 0.08});
    margin_mmr.insert({10, 0.05});


    mrkprice_map = new map<string. double>;
}

void StrategyFR::init() 
{
    for (auto iter : strategyInstrumentList()) {
        mrkprice_map.insert({iter->instrument()->getInstrumentID(), 0});
    }
}

void StrategyFR::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument)
{
    return;
}

void StrategyFR::calc_spot_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {
        if (!IS_DOUBLE_ZERO(it.crossMarginAsset, 0) ||  !IS_DOUBLE_ZERO(it.crossMarginBorrowed)) {
            double rate = collateralRateMap[it.asset];
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double calc_equity = min(equity * marketTrade.Price, equity * marketTrade.Price * rate);
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.umWalletBalance, 0) ||  !IS_DOUBLE_ZERO(it.umUnrealizedPNL)) {
            double rate = collateralRateMap[it.asset];
            double equity = it.umWalletBalance +  it.umUnrealizedPNL;
            double calc_equity = equity * marketTrade.Price;
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.cmWalletBalance, 0) ||  !IS_DOUBLE_ZERO(it.cmUnrealizedPNL)) {
            double rate = collateralRateMap[it.asset];
            double equity = it.cmWalletBalance +  it.cmUnrealizedPNL;
            double calc_equity = min(equity * marketTrade.Price, equity * marketTrade.Price * rate);
            sum_equity += calc_equity;
        }

    }
    return sum_equity;
}

double StrategyFR::calc_mm()
{
    double sum_mm = 0;
    // 现货杠杆mm
    for (auto it : BnApi::BalMap_) {
        double leverage = 0; 
        string symbol = it.asset;
        if (margin_leverage.find(symbol) == margin_leverage.end()) {
            leverage = margin_leverage["default"];
        } else {
            leverage = margin_leverage[symbol];
            
        }

        if (symbol == "USDT" || symbol == "USDC" || symbol == "BUSD") {
            double mm = it.second.crossMarginBorrowed * margin_mmr[10];
            sum_mm += mm;
        } else {
            double mm = it.second.crossMarginBorrowed * margin_mmr[leverage] * mrkprice_map[it.second.asset];
            sum_mm += mm;
        }
    }

    for (auto it : BnApi::UmMap_) {
        string symbol = it.asset;
        double qty = gQryPosiInfo[symbol].size;
        double markPrice = mrkprice_map[symbol];
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty) * markPrice, mmr_rate, mmr_num);
        double mm = abs(qty) * markPrice * mmr_rate - mmr_num;
        sum_mm += mm
    }

    for (auto it : BnApi::CmMap_) {
        string symbol = it.asset;
        double qty = 0;

        if (symbol == "BTCUSD_PERP") {
            qty = gQryPosiInfo[symbol].size * 100;
        } else {
            qty = gQryPosiInfo[symbol].size * 10;
        }

        double markPrice = mrkprice_map[symbol];
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty), mmr_rate, mmr_num);
        double mm = (abs(qty) * mmr_rate - mmr_num) * markPrice;
        sum_mm += mm;
    }
    return sum_mm;

}

void get_cm_um_brackets(string symbol, double val, double& mmr_rate, double& mmr_num)
{
    bool flag = false;
    for (auto it : mmr_table) {
        if (symbol == it.table_name) {
            double** data = it.data;
            for (int i = 0; i < it.rows; ++i) {  
                if (IS_DOUBLE_GREATER_EQUAL(val, data[i][0]) && IS_DOUBLE_LESS_EQUAL(val, data[i][1])) {
                    mmr_rate = data[i][3];
                    mmr_num = data[i][4];
                    return;
                }
            }  
        }
    }

    if (!flag) {
        LOG_FATAL << ""; 
    }
}

void StrategyFR::calc_spot_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {

    }
    return sum_equity;
}


void StrategyFR::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{

    return;
}

void StrategyFR::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}