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
    make_taker = new map<string, sy_info>;
    delta_mp = new map<string, double>;

    pre_sum_equity = 0;
}

void StrategyFR::init() 
{
    for (auto iter : strategyInstrumentList()) {
        last_price_map->insert({iter->instrument()->getInstrumentID(), 0});
    }

    for (auto it : InitialData::symbolInfoMap()) {
        sy_info syInfo;
        memcpy(syInfo.sy, it.second.Symbol, min(sizeof(syInfo.sy), sizeof(it.second.Symbol)));
        memcpy(syInfo.ref_sy, it.second.RefSymbol, min(sizeof(syInfo.ref_sy), sizeof(it.second.RefSymbol)));
        memcpy(syInfo.type, it.second.Type, min(sizeof(syInfo.type), sizeof(it.second.Type)));

        syInfo.long_short_flag = it.second.LongShort;
        syInfo.make_taker_flag = it.second.MTaker;
        syInfo.qty = it.second.OrderQty;
        syInfo.mv_ratio = it.second.MvRatio;
        syInfo.thresh = it.second.Thresh;
        syInfo.prc_tick_size = it.second.PreTickSize;
        syInfo.qty_tick_size = it.second.QtyTickSize;
        syInfo.max_delta_limit = it.second.MaxDeltaLimit;
        make_taker->insert({it.second.Symbol, syInfo});
        if (syInfo.make_taker_flag == 1) delta_mp->insert({it.second.Symbol, 0});
    }

    for (auto iter : strategyInstrumentList()) {
        for (auto it : (*make_taker)) {
            if (it.first == iter->instrument()->getInstrumentID()) {
                it.second.inst = it.second.inst;
                it.second.sellMap = it.second.inst->sellOrders();
                it.second.buyMap = it.second.inst->buyOrders();
                break;
            }
        }
    }

    for (auto it : (*make_taker)) {
        it.second.ref = &(*make_taker)[it.second.ref_sy];
    }
}

void StrategyFR::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    hedge(strategyInstrument);
    return;
}

void StrategyFR::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    hedge(strategyInstrument);
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
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_future_umimmr has no mkprice: " << symbol << ", markprice: " << (*last_price_map)[symbol];
        return 0;
    }
    double borrow = 0;
    if (IS_DOUBLE_GREATER(qty, 0)) { // 借usdt
        borrow = qty * price;
        IM = IM + borrow / ((*margin_leverage)[symbol] - 1) + (qty * price / um_leverage);         
    } else { // 借现贄1�7
        borrow = -qty;
        IM = IM + (price * (-qty) / ((*margin_leverage)[symbol] - 1)) + (-qty) * price / um_leverage;
    }

    order.borrow = borrow;

    if (IS_DOUBLE_GREATER(IM, sum_equity)) {
        LOG_INFO << "现货+合约的初始保证金 > 有效保证金，不可以下卄1�7: " << IM << ", sum_equity: " << sum_equity;
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
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_equity has no mkprice: " << order.sy << ", markprice: " << (*last_price_map)[order.sy];
        return 0;
    }

    double rate = collateralRateMap[order.sy];

    if (IS_DOUBLE_GREATER(order.qty, 0)) { // 现货做多＄1�7 合约做空
        double equity = order.qty * price * (1 + price_cent) * rate;
        double uswap_unpnl = order.qty * price - (1 + price_cent) * price * order.qty;
        sum_equity += equity - order.borrow + uswap_unpnl;
    } else { // 现货做空＄1�7 合约做多
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
            if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
                LOG_WARN << "BalMap calc_predict_equity has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
                return 0;
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
            if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
                LOG_WARN << "umWalletBalance calc_predict_equity has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
                return 0;
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
            if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
                LOG_WARN << "cmWalletBalance calc_predict_equity has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
                return 0;
            }
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        double price = (*last_price_map)[iter.symbol] * (1 + price_cent);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc has no mkprice: " << iter.symbol << ", markprice: " << (*last_price_map)[iter.symbol];
            return 0;
        }
        double avgPrice = (iter.entryPrice * iter.positionAmt + order.qty * (*last_price_map)[iter.symbol]) / (iter.positionAmt + order.qty);
        double uswap_unpnl = (price - avgPrice) * iter.positionAmt;
        sum_equity += uswap_unpnl;
    }

    for (auto iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        double price = (*last_price_map)[sy] * (1 + price_cent);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
            return 0;
        }
        double perp_size = 0;
        if (sy == "BTCUSD_PERP") {
            perp_size = 100;
        } else {
            perp_size = 10;
        }

        double quantity = perp_size * iter.positionAmt / iter.entryPrice + order.qty * perp_size / (*last_price_map)[sy];

        double turnover = perp_size * iter.positionAmt + order.qty * perp_size;

        double avgPrice = turnover / quantity;
        if (IS_DOUBLE_LESS_EQUAL(avgPrice, 0)) {
            avgPrice = (*last_price_map)[sy];
        }

        double cswap_unpnl = price * perp_size * iter.positionAmt * (1/avgPrice - 1/price);
        sum_equity += cswap_unpnl;
    }
    return sum_equity;
}

double StrategyFR::calc_predict_mm(order_fr& order, double price_cent)
{
    double sum_mm = 0;
    double price = (*last_price_map)[order.sy];
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_mm has no mkprice: " << order.sy << ", markprice: " << (*last_price_map)[order.sy];
        return 0;
    }
    double leverage = 0;
    if (margin_leverage->find(order.sy) == margin_leverage->end()) {
        leverage = (*margin_leverage)["default"];
    } else {
        leverage = (*margin_leverage)[order.sy];
        
    }

    if (IS_DOUBLE_GREATER(order.qty, 0)) { // 现货做多，合约做穄1�7
        sum_mm = sum_mm + order.borrow * (*margin_mmr)[leverage];
    } else { // 现货做空，合约做处1�7
        sum_mm = sum_mm + order.borrow * price * (*margin_mmr)[leverage];
    }

    for (auto it : BnApi::BalMap_) {
        double leverage = (*margin_mmr)[(*margin_leverage)[it.first]];
        double price = (*last_price_map)[it.first];
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "BalMap calc_predict_mm has no mkprice: " << it.first << ", markprice: " << (*last_price_map)[it.first];
            return 0;
        }
        string sy = it.first;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * 1; // 杠杆现货维持保证釄1�7
        } else {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * price;
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        string sy = iter.symbol;
        double price = (*last_price_map)[sy] * (1 + price_cent);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc calc_predict_mm has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
            return 0;
        }
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
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc calc_predict_mm has no mkprice: " << sy << ", markprice: " << (*last_price_map)[sy];
            return 0;
        }
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

double StrategyFR::calc_balance()
{
    double sum_usdt = 0;
    // um_account
    for (auto it : BnApi::UmAcc_->info_) {
        string sy = it.asset;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_usdt += it.crossWalletBalance + it.crossUnPnl;
        } else {
            sum_usdt += (it.crossWalletBalance + it.crossUnPnl) * (*last_price_map)[sy];
        }
    }

    // cm_account
    for (auto it : BnApi::CmAcc_->info_) {
        string sy = it.asset;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            LOG_FATAL << "";
        }
        sum_usdt += (it.crossWalletBalance + it.crossUnPnl) * (*last_price_map)[sy];
    }

    for (auto it : BnApi::BalMap_) {
        string sy = it.second.asset;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_usdt += it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginLocked - it.second.crossMarginInterest;
        } else {
            sum_usdt += (it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginLocked - it.second.crossMarginInterest) * (*last_price_map)[sy];;
        }

    }
    return sum_usdt;
}

double StrategyFR::calc_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {
        if (IS_DOUBLE_LESS_EQUAL((*last_price_map)[it.second.asset] , 0)) {
            LOG_WARN << "calc_equity asset has no mkprice: " << it.second.asset << ", markprice: " << (*last_price_map)[it.second.asset];
            return 0;
        }
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
        if (IS_DOUBLE_LESS_EQUAL((*last_price_map)[it.second.asset] , 0)) {
            LOG_WARN << "calc_mm asset has no mkprice: " << it.second.asset << ", markprice: " << (*last_price_map)[it.second.asset];
            return 0;
        }

        double leverage = 0; 
        string symbol = it.first;
        if (margin_leverage->find(symbol) == margin_leverage->end()) {
            leverage = (*margin_leverage)["default"];
        } else {
            leverage = (*margin_leverage)[symbol];
            
        }

        if (symbol == "USDT" || symbol == "USDC" || symbol == "BUSD") {
            double mm = it.second.crossMarginBorrowed * (*margin_mmr)[10];
            sum_mm += mm;
        } else {
            double mm = it.second.crossMarginBorrowed * (*margin_mmr)[leverage] * (*last_price_map)[it.second.asset];
            sum_mm += mm;
        }
    }

    for (auto it : BnApi::UmAcc_->info1_) {
        if (IS_DOUBLE_LESS_EQUAL((*last_price_map)[it.symbol] , 0)) {
            LOG_WARN << "calc_mm UmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*last_price_map)[it.symbol];
            return 0;
        }

        string symbol = it.symbol;
        double qty = it.positionAmt;
        double markPrice = (*last_price_map)[symbol];
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty) * markPrice, mmr_rate, mmr_num);
        double mm = abs(qty) * markPrice * mmr_rate - mmr_num;
        sum_mm += mm;
    }

    for (auto it : BnApi::CmAcc_->info1_) {
        if (IS_DOUBLE_LESS_EQUAL((*last_price_map)[it.symbol] , 0)) {
            LOG_WARN << "calc_mm CmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*last_price_map)[it.symbol];
            return 0;
        }
        string symbol = it.symbol;
        double qty = 0;

        if (symbol == "BTCUSD_PERP") {
            qty = it.positionAmt * 100;
        } else {
            qty = it.positionAmt * 10;
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

bool StrategyFR::over_max_delta_limit(sy_info& sy1, sy_info& sy2) 
{
    sy1.real_pos = sy1.inst->position().getNetPosition();
    sy2.real_pos = sy2.inst->position().getNetPosition();

    if (IS_DOUBLE_LESS(sy1.real_pos - sy2.real_pos, sy1.max_delta_limit)) {
        return false;
        LOG_INFO << "over_max_delta_limit sy1 real_pos: " << sy1.real_pos << ", sy2 real_pos: " << sy2.real_pos;
    }

    return true;
}

void StrategyFR::hedge(StrategyInstrument *strategyInstrument)
{
    string symbol = strategyInstrument->getInstrumentID();
    sy_info& sy1 = (*make_taker)[symbol];

    double delta_posi = sy1.real_pos + sy1.ref->real_pos;
    if (IS_DOUBLE_LESS(abs(delta_posi), sy1.max_delta_limit)) return;
     
    double taker_qty = abs(delta_posi);

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER_EQUAL(delta_posi, sy1.max_delta_limit)) {
        if (sy1.make_taker_flag == 1) {
            order.orderType = ORDERTYPE_MARKET; // ?
            
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, SPOT.c_str(), min(uint16_t(CategoryLen), uint16_t(SPOT.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.ref->inst, INNER_DIRECTION_Buy,
                            sy1.bid_p,
                            taker_qty,
                            order);
        } else {          
            order.orderType = ORDERTYPE_MARKET; // ?

            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, SPOT.c_str(), min(uint16_t(CategoryLen), uint16_t(SPOT.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
 
            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                            sy1.ask_p,
                            taker_qty,
                            order);
        }
    } else if (IS_DOUBLE_LESS_EQUAL(delta_posi, -sy1.max_delta_limit)) {
        if (sy1.make_taker_flag == 1) {
            order.orderType = ORDERTYPE_MARKET; // ?
            
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, SPOT.c_str(), min(uint16_t(CategoryLen), uint16_t(SPOT.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.ref->inst, INNER_DIRECTION_Sell,
                            sy1.bid_p,
                            taker_qty,
                            order);
        } else {          
            order.orderType = ORDERTYPE_MARKET; // ?

            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, SPOT.c_str(), min(uint16_t(CategoryLen), uint16_t(SPOT.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
 
            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                            sy1.ask_p,
                            taker_qty,
                            order);
        }
    }
}


void StrategyFR::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    int64_t ts = CURR_MSTIME_POINT;
    if (marketData.EpochTime - ts > 30) {
        LOG_WARN << "market data beyond time: " << marketData.InstrumentID << ", ts: " << marketData.EpochTime;
        return;
    }
    sy_info& sy1 = (*make_taker)[marketData.InstrumentID];
    sy1.update(marketData.AskPrice1, marketData.BidPrice1, marketData.AskVolume1, marketData.BidVolume1);

    if (sy1.make_taker_flag == 1) {
        sy_info& sy2 = (*make_taker)[sy1.ref_sy];
        double bal = calc_balance();
        if (sy1.long_short_flag == 1 && IS_DOUBLE_GREATER(sy1.mid_p - sy2.mid_p, 0)) {
            double spread_rate = (sy1.mid_p - sy2.mid_p) / sy2.mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, sy1.thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal) ||
                    !over_max_delta_limit(sy1, sy2)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.mid_p;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, marketData.AskVolume1 / 2);
                if (IS_DOUBLE_LESS(qty, sy1.max_delta_limit)) return;
                 qty = sy1.max_delta_limit;

                if (SPOT == sy1.type) {
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                    string Category = LEVERAGE;

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                    setOrder(sy1.inst, INNER_DIRECTION_Sell,
                        marketData.AskPrice1 + sy1.prc_tick_size,
                        qty, order);
                }

                if (SWAP == sy1.type) {
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                    string Category = LINEAR;

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                    setOrder(sy1.inst, INNER_DIRECTION_Sell,
                        marketData.AskPrice1 + sy1.prc_tick_size,
                        qty, order);
                } 
            }
        } else if (sy1.long_short_flag == 0 && IS_DOUBLE_LESS(sy1.mid_p - sy2.mid_p, 0)) {
                double spread_rate = (sy2.mid_p - sy1.mid_p) / sy1.mid_p;
                if (IS_DOUBLE_GREATER(spread_rate, sy1.thresh)) {
                    if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal) ||
                        !over_max_delta_limit(sy1, sy2)) {
                        LOG_WARN << "";
                        return;
                    }
                }

                double u_posi = abs(sy1.real_pos) * sy1.mid_p;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, marketData.BidVolume1 / 2);

                if (IS_DOUBLE_LESS(qty, sy1.max_delta_limit)) return;
                 qty = sy1.max_delta_limit;

                if (SPOT == sy1.type) {
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                    string Category = LEVERAGE;

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                    setOrder(sy1.inst, INNER_DIRECTION_Buy,
                        marketData.BidPrice1 - sy1.prc_tick_size,
                        qty, order);
                }

                if (SWAP == sy1.type) {
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                    string Category = LINEAR;

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                    setOrder(sy1.inst, INNER_DIRECTION_Buy,
                        marketData.BidPrice1 - sy1.prc_tick_size,
                        qty, order);
                } 
        }
    }

    return;
}

void StrategyFR::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    hedge(strategyInstrument);
    return;
}

void StrategyFR::OnForceCloseTimerInterval() 
{
    for (auto iter : strategyInstrumentList()) {
        hedge(iter);
    }
}

void StrategyFR::OnTimerTradingLogic() 
{
    for (auto it : (*make_taker)) {
        if (SPOT == it.second.type) {
            auto iter = BnApi::BalMap_.find(it.first);
            if (iter == BnApi::BalMap_.end()) LOG_FATAL << "";
            double qty = iter.second.crossMarginFree + iter.second.crossMarginLocked - iter.second.crossMarginLocked - iter.second.crossMarginInterest;
            if (IS_DOUBLE_GREATER(abs(it.second.real_pos - qty), it.second.max_delta_limit)) {
                LOG_WARN << "";
            }
        }

        if (SWAP == it.second.type) {
            bool flag = false;
            for (auto iter : BnApi::UmAcc_->info1_) {
                if (strcmp(it.first, iter.symbol) == 0) {
                    if (IS_DOUBLE_GREATER(abs(it.second.real_pos - iter.positionAmt), it.second.max_delta_limit)) {
                        LOG_WARN << "";
                    }
                    flag = true;
                }
            }
            if (!flag) LOG_FATAL << "";
        }
    }
    
    for (auto it : BnApi::UmAcc_->info1_) {
        if (PERP == it.second.type) LOG_FATAL << "";
    }

}

