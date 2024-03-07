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
    margin_leverage->insert({"BTC", 10});
    margin_leverage->insert({"ETH", 10});
    margin_leverage->insert({"ETC", 10});
    margin_leverage->insert({"default", 5});

    margin_mmr = new map<double, double>;
    margin_mmr->insert({3, 0.1}); // {3:0.1, 5:0.08, 10:0.05}
    margin_mmr->insert({5, 0.08});
    margin_mmr->insert({10, 0.05});

    last_price_map = new map<string, double>;
    pridict_borrow = new map<string, double>;

    pre_sum_equity = 0;
}

void StrategyFR::init() 
{
    for (auto iter : strategyInstrumentList()) {
        last_price_map->insert({iter->instrument()->getInstrumentID(), 0});
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

double StrategyFR::get_usdt_equity()
{
    double equity = 0;
    equity = (BnApi::BalMap_["USDT"].crossMarginFree) + (BnApi::BalMap_["USDT"].crossMarginLocked) - (BnApi::BalMap_["USDT"].crossMarginBorrowed) - (BnApi::BalMap_["USDT"].crossMarginInterest);
    equity += (BnApi::BalMap_["USDT"].umWalletBalance) +  (BnApi::BalMap_["USDT"].umUnrealizedPNL);
    equity += (BnApi::BalMap_["USDT"].cmWalletBalance) + (BnApi::BalMap_["USDT"].cmUnrealizedPNL);
    return equity;
}

double StrategyFR::calc_future_uniMMR(string symbol, double qty)
{
    double usdt_equity = get_usdt_equity();
    double sum_equity = calc_equity();
    double IM = 0;

    order_fr order;
    order.sy = symbol;
    order.qty = qty;

    double price = (*last_price_map)[symbol];
    double borrow = 0;
    if (IS_DOUBLE_GREATER(qty, 0)) { // 借usdt
        borrow = qty * price;
        IM = IM + borrow / ((*margin_leverage)[symbol] - 1) + (qty * price / um_leverage);         
    } else { // 借现货
        borrow = -qty;
        IM = IM + (price * (-qty) / ((*margin_leverage)[symbol] - 1)) + (-qty) * price / um_leverage;
    }

    order.borrow = borrow;

    if (IS_DOUBLE_GREATER(IM, sum_equity)) {
        LOG_INFO << "现货+合约的初始保证金 > 有效保证金，不可以下单: " << IM << ", sum_equity: " << sum_equity;
        return 0;
    }

    double predict_equity = calc_predict_equity(order, price_ratio);
    double predict_mm = calc_predict_mm(order, price_ratio);
    double predict_mmr = predict_equity / predict_mm;
    return predict_mmr;

}

double StrategyFR::calc_predict_equity(order_fr& order, double price_cent)
{
    double sum_equity = 0;
    double price = (*last_price_map)[order.sy];
    double rate = collateralRateMap[order.sy];

    if (IS_DOUBLE_GREATER(order.qty, 0)) { // 现货做多， 合约做空
        double equity = order.qty * price * (1 + price_cent) * rate;
        double uswap_unpnl = order.qty * price - (1 + price_cent) * price * order.qty;
        sum_equity += equity - order.borrow + uswap_unpnl;
    } else { // 现货做空， 合约做多
        double qty = (-order.qty);
        double equity = qty * price - order.borrow * (1 + price_cent) * price;
        double uswap_unpnl = order.qty * price * (1 + price_cent) - qty * price;
        sum_equity = equity + uswap_unpnl;
    }

    for (auto it : BnApi::BalMap_) {
        double rate = collateralRateMap[it.first];

        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = (*last_price_map)[sy];
            } else {
                price = (*last_price_map)[sy] * (1 + price_cent);
            }
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double equity = it.second.umWalletBalance;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = (*last_price_map)[sy];
            } else {
                price = (*last_price_map)[sy] * (1 + price_cent);
            }
            sum_equity = sum_equity + equity * price;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double equity = it.second.cmWalletBalance;
            double price = 0;
            string sy = it.first;
            if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
                price = (*last_price_map)[sy];
            } else {
                price = (*last_price_map)[sy] * (1 + price_cent);
            }
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        double price = (*last_price_map)[iter.symbol] * (1 + price_cent);
        double uswap_unpnl = (price - iter.entryPrice) * iter.positionAmt;
        sum_equity += uswap_unpnl;
    }

    for (auto iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        double price = (*last_price_map)[sy] * (1 + price_cent);
        double perp_size = 0;
        if (sy == "BTCUSD_PERP") {
            perp_size = 100;
        } else {
            perp_size = 10;
        }
        double cswap_unpnl = price * perp_size * iter.positionAmt * (1 / iter.entryPrice - 1 / price);
        sum_equity += cswap_unpnl;
    }
    return sum_equity;
}

double StrategyFR::calc_predict_mm(order_fr& order, double price_cent)
{
    double sum_mm = 0;
    double price = (*last_price_map)[order.sy];

    double leverage = 0;
    if (margin_leverage.find(order.sy) == margin_leverage.end()) {
        leverage = (*margin_leverage)["default"];
    } else {
        leverage = (*margin_leverage)[order.sy];
        
    }

    if (IS_DOUBLE_GREATER(order.qty, 0)) { // 现货做多，合约做空
        sum_mm = sum_mm + order.borrow * (*margin_mmr)[leverage];
    } else { // 现货做空，合约做多
        sum_mm = sum_mm + order.borrow * price * (*margin_mmr)[leverage];
    }

    for (auto it : BnApi::BalMap_) {
        double leverage = margin_mmr[margin_leverage[it.first]];
        double price = (*last_price_map)[it.first];
        string sy = it.first;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * 1; // 杠杆现货维持保证金
        } else {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * price;
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        string sy = iter.symbol;
        double price = (*last_price_map)[iter.symbol] * (1 + price_cent);
        double qty = iter.positionAmt;
        if (sy == order.sy) {
            qty = qty + order.qty;
        }
        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(iter.symbol, abs(qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + abs(qty) * price * mmr_rate -  mmr_num;
    }

    for (auto iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        double price = (*last_price_map)[iter.symbol] * (1 + price_cent);
        double qty = 0;
        if (sy == "BTCUSD_PERP") {
            qty = iter.positionAmt * 100 / price;
        } else {
            qty = iter.positionAmt * 10 / price;
        }
        double mmr_rate = 0;
        double mmr_num = 0;
        qty = iter.positionAmt;
        get_cm_um_brackets(iter.symbol, abs(qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + (abs(qty) * mmr_rate -  mmr_num) * price;
    }

    return sum_mm;

}

double StrategyFR::calc_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {
        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double calc_equity = min(equity * (*last_price_map)[it.second.asset], equity * (*last_price_map)[it.second.asset] * rate);
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.umWalletBalance +  it.second.umUnrealizedPNL;
            double calc_equity = equity * (*last_price_map)[it.second.asset]; // should be swap symbol
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.cmWalletBalance +  it.second.cmUnrealizedPNL;
            double calc_equity = min(equity * (*last_price_map)[it.second.asset], equity * (*last_price_map)[it.second.asset] * rate); // should be perp symbol
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
            leverage = (*margin_leverage)["default"];
        } else {
            leverage = (*margin_leverage)[symbol];
            
        }

        if (symbol == "USDT" || symbol == "USDC" || symbol == "BUSD") {
            double mm = it.second.crossMarginBorrowed * margin_mmr[10];
            sum_mm += mm;
        } else {
            double mm = it.second.crossMarginBorrowed * (*margin_mmr)[leverage] * (*last_price_map)[it.second.asset];
            sum_mm += mm;
        }
    }

    for (auto it : BnApi::UmAcc_->info1_) {
        string symbol = it.symbol;
        double qty = gQryPosiInfo[symbol].size;
        double markPrice = (*last_price_map)[symbol];
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty) * markPrice, mmr_rate, mmr_num);
        double mm = abs(qty) * markPrice * mmr_rate - mmr_num;
        sum_mm += mm;
    }

    for (auto it : BnApi::CmAcc_->info1_) {
        string symbol = it.symbol;
        double qty = 0;

        if (symbol == "BTCUSD_PERP") {
            qty = gQryPosiInfo[symbol].size * 100;
        } else {
            qty = gQryPosiInfo[symbol].size * 10;
        }

        double markPrice = (*last_price_map)[symbol];
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty), mmr_rate, mmr_num);
        double mm = (abs(qty) * mmr_rate - mmr_num) * markPrice;
        sum_mm += mm;
    }
    return sum_mm;

}

void StrategyFR::get_cm_um_brackets(string symbol, double val, double& mmr_rate, double& mmr_num)
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