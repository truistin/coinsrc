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
    um_leverage = *parameters()->getDouble("um_leverage");
    price_ratio = *parameters()->getDouble("price_ratio");

    margin_leverage = new map<string, double>; // {'BTC':10, 'ETH':10,'ETC':10,'default':5}
    margin_leverage.insert({"BTC", 10});
    margin_leverage.insert({"ETH", 10});
    margin_leverage.insert({"ETC", 10});
    margin_leverage.insert({"default", 5});

    margin_mmr = new map<string, double>;
    margin_mmr.insert({3, 0.1}); // {3:0.1, 5:0.08, 10:0.05}
    margin_mmr.insert({5, 0.08});
    margin_mmr.insert({10, 0.05});

    last_price_map = new map<string. double>;
    pridict_borrow = new map<string, double>;

    pre_sum_equity = 0;
}

void StrategyFR::init() 
{
    for (auto iter : strategyInstrumentList()) {
        last_price_map.insert({iter->instrument()->getInstrumentID(), 0});
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

double StrategyFR::predict_spot_IM(string symbol, double qty)
{
    double borrow = BalMap_["USDT"] - qty * last_price_map[symbol];
    if (IS_DOUBLE_LESS(borrow, 0)) {
        borrow = -borrow;
    }
    pridict_borrow.insert({symbol, borrow});
    return (borrow / (margin_leverage[symbol] - 1));
}

double StrategyFR::predict_um_IM(string symbol, double qty)
{
    return (qty * last_price_map[symbol] / um_leverage);
}

void StrategyFR::get_usdt_equity()
{
    double equity = 0;
    equity = Decimal(BalMap_["USDT"].crossMarginFree) + Decimal(BalMap_["USDT"].crossMarginLocked) - Decimal(BalMap_["USDT"].crossMarginBorrowed) - Decimal(BalMap_["USDT"].crossMarginInterest);
    equity += Decimal(BalMap_["USDT"].umWalletBalance) +  Decimal(BalMap_["USDT"].umUnrealizedPNL);
    equity += Decimal(BalMap_["USDT"].cmWalletBalance) + Decimal(BalMap_["USDT"].cmUnrealizedPNL);
    return equity
}

void StrategyFR::calc_future_uniMMR(string symbol, double qty)
{
    double usdt_equity = get_usdt_equity();
    double sum_equity = calc_equity();
    double IM = 0;

    order_fr order;
    order.sy = symbol;
    order.qty = qty;

    double IM = 0;
    double price = last_price_map[symbol];
    double borrow = 0;
    if (IS_DOUBLE_GREAT(qty, 0)) { // 借usdt
        borrow = qty * price;
        IM = IM + borrow / (margin_leverage[symbol] - 1) + (qty * price / um_leverage);         
    } else { // 借现货
        borrow = -qty;
        // IM += Decimal(abs(qty)) / (self.margin_leverage[key] - 1) * price + Decimal(abs(qty)) * price / self.uswap_leverage[key]
        IM = IM + (price * (-qty) / (margin_leverage[symbol] - 1)) + (-qty) * price / um_leverage;
    }

    if (IS_DOUBLE_GREAT(IM, sum_equity)) {
        LOG_INFO << "现货+合约的初始保证金 > 有效保证金，不可以下单: " << IM << ", sum_equity: " << sum_equity;
        return;
    }

    double predict_equity = calc_predict_equity(order);
    double predict_mm = calc_predict_mm(order);
    double predict_mmr = predict_equity / predict_mm;
    return predict_mmr;

}

void StrategyFR::calc_predict_equity(order_fr& order)
{
    double sum_equity = 0;
    double price = last_price_map[order.symbol];
    double rate = collateralRateMap[order.symbol];

    if (IS_DOUBLE_GREATER(order.qty, 0)) { // 现货做多， 合约做空
        double equity = order.qty * price * (1 + price_ratio) * rate;
        double uswap_unpnl = order.qty * price - (1 + price_ratio) * price * order.qty;
        sum_equity += equity - order.borrow + uswap_unpnl;
    } else { // 现货做空， 合约做多
        double qty = (-order.qty);
        double equity = qty * price - order.borrow * (1 + price_ratio) * price;
        double uswap_unpnl = order.qty * price * (1 + price_ratio) - qty * price;
        sum_equity = equity + uswap_unpnl;
    }

    for (auto it : BnApi::BalMap_) {
        double rate = collateralRateMap[it.first];

        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = last_price_map[sy];
            } else {
                price = last_price_map[sy] * (1 + price_ratio);
            }
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double equity = it.second.umWalletBalance;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = last_price_map[sy];
            } else {
                price = last_price_map[sy] * (1 + price_ratio);
            }
            sum_equity = sum_equity + equity * price;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double equity = it.second.cmWalletBalance;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = last_price_map[sy];
            } else {
                price = last_price_map[sy] * (1 + price_ratio);
            }
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }
    }

    


}

void StrategyFR::calc_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {
        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double calc_equity = min(equity * last_price_map[it.second.asset], equity * last_price_map[it.second.asset] * rate);
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.umWalletBalance +  it.second.umUnrealizedPNL;
            double calc_equity = equity * last_price_map[it.second.asset]; // should be swap symbol
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.second.cmWalletBalance +  it.second.cmUnrealizedPNL;
            double calc_equity = min(equity * last_price_map[it.second.asset], equity * last_price_map[it.second.asset] * rate); // should be perp symbol
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
            double mm = it.second.crossMarginBorrowed * margin_mmr[leverage] * last_price_map[it.second.asset];
            sum_mm += mm;
        }
    }

    for (auto it : BnApi::UmMap_) {
        string symbol = it.asset;
        double qty = gQryPosiInfo[symbol].size;
        double markPrice = last_price_map[symbol];
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

        double markPrice = last_price_map[symbol];
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

void StrategyFR::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{

    return;
}

void StrategyFR::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    return;
}