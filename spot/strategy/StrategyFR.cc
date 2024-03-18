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

    pre_sum_equity = 0;
    enable_maker = true;
}

void StrategyFR::qryPosition() {
    for (auto iter : strategyInstrumentList()) {
        Order order;
        order.QryFlag = QryType::QRYPOSI;
        order.setInstrumentID((void*)(iter->instrument()->getInstrumentID().c_str()), iter->instrument()->getInstrumentID().size());

        string Category = "linear";
        if (iter->instrument()->getInstrumentID().find("swap") != string::npos) {
            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        } else if (iter->instrument()->getInstrumentID().find("perp") != string::npos) {
            Category = "inverse";
            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        } else if (iter->instrument()->getInstrumentID().find("spot") != string::npos) {
            Category = LEVERAGE;
        } else {
            LOG_FATAL << "qryPosition fatal: " << iter->instrument()->getInstrumentID();
        }
        
        if (iter->instrument()->getInstrumentID().find("swap") != string::npos ||
            iter->instrument()->getInstrumentID().find("perp") != string::npos) {
            order.setExchangeCode((void*)(iter->instrument()->getExchange()), strlen(iter->instrument()->getExchange()));
            order.setCounterType((void*)(iter->instrument()->getExchange()), strlen(iter->instrument()->getExchange()));
            if (iter->query(order) != 0) {
                LOG_FATAL << "qryPosition symbol: " << iter->instrument()->getInstrumentID();
            }

            double coinSize = 1;
        
            if (strcmp(iter->instrument()->getExchange(), "OKEX") == 0) {
                SymbolInfo *symbolInfo = InitialData::getSymbolInfo(iter->instrument()->getInstrumentID());
                coinSize = symbolInfo->CoinOrderSize;
            }

            if (strcmp(gQryPosiInfo[iter->instrument()->getInstrumentID()].side, "NONE") == 0) { 
                LOG_INFO << "qryPosition long symbol: " << iter->instrument()->getInstrumentID() << ", TodayLong: " << iter->position().PublicPnlDaily().TodayLong 
                    << ", TodayShort: " << iter->position().PublicPnlDaily().TodayShort
                    << ", net: " << iter->position().getNetPosition(); 
                continue;
            }

            if (strcmp(gQryPosiInfo[iter->instrument()->getInstrumentID()].side, "BUY") == 0) {
                iter->position().PublicPnlDaily().TodayLong = gQryPosiInfo[iter->instrument()->getInstrumentID()].size * coinSize;
            }
            else if (strcmp(gQryPosiInfo[iter->instrument()->getInstrumentID()].side, "SELL") == 0) {
                iter->position().PublicPnlDaily().TodayShort = gQryPosiInfo[iter->instrument()->getInstrumentID()].size * coinSize;
            }
            
            iter->position().PublicPnlDaily().NetPosition = iter->position().PublicPnlDaily().TodayLong - iter->position().PublicPnlDaily().TodayShort;
                
            iter->position().PublicPnlDaily().EntryPrice = gQryPosiInfo[iter->instrument()->getInstrumentID()].entryPrice;
        
        } else if (iter->instrument()->getInstrumentID().find("spot") != string::npos) {
            order.setExchangeCode((void*)(iter->instrument()->getExchange()), strlen(iter->instrument()->getExchange()));
            order.setCounterType((void*)(iter->instrument()->getExchange()), strlen(iter->instrument()->getExchange()));
            if (iter->query(order) != 0) {
                LOG_FATAL << "qryPosition symbol: " << iter->instrument()->getInstrumentID();
            }

            auto it = BnApi::BalMap_.find(iter->instrument()->getInstrumentID());
            if (it == BnApi::BalMap_.end()) LOG_FATAL << "qry spot position error";
    
            double equity = it->second.crossMarginFree + it->second.crossMarginLocked - it->second.crossMarginBorrowed - it->second.crossMarginInterest;

            if (IS_DOUBLE_LESS(equity, 0)) {
                iter->position().PublicPnlDaily().TodayShort = equity;
                iter->position().PublicPnlDaily().NetPosition = equity;
            } else if (IS_DOUBLE_GREATER(equity, 0)) {
                iter->position().PublicPnlDaily().TodayLong = equity;
                iter->position().PublicPnlDaily().NetPosition = equity;
            }
        } else {
            LOG_FATAL << "qryPosition symbol fatal: " << iter->instrument()->getInstrumentID();
        }

        LOG_INFO << "qryPosition long symbol: " << iter->instrument()->getInstrumentID() << ", TodayLong: " << iter->position().PublicPnlDaily().TodayLong 
            << ", TodayShort: " << iter->position().PublicPnlDaily().TodayShort
            << ", net: " << iter->position().getNetPosition()
            << ", entryPrice: " << iter->position().PublicPnlDaily().EntryPrice;    
    }
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

        syInfo.fr_open_thresh = it.second.FrOpenThresh;
        syInfo.fr_close_thresh = it.second.FrCloseThresh;
        syInfo.close_flag = it.second.CloseFlag;

        syInfo.prc_tick_size = it.second.PreTickSize;
        syInfo.qty_tick_size = it.second.QtyTickSize;
        syInfo.max_delta_limit = it.second.MaxDeltaLimit;

        syInfo.force_close_amount = it.second.ForceCloseAmount;
        make_taker->insert({it.second.Symbol, syInfo});
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

    qryPosition();

    for (auto it : (*make_taker)) {
        it.second.ref = &(*make_taker)[it.second.ref_sy];
        if (SWAP == it.second.sy) it.second.ref->avg_price = it.second.avg_price;
    }
}

void StrategyFR::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto& sy2 = sy1.ref;
    over_max_delta_limit(sy1, sy2);

    hedge(strategyInstrument);
    return;
}

void StrategyFR::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto& sy2 = sy1.ref;
    over_max_delta_limit(sy1, sy2);

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

double StrategyFR::calc_uniMMR()
{
    double uniAccount_equity = calc_equity();
    double uniAccount_mm = calc_mm();
    return (uniAccount_equity)/(uniAccount_mm);
}

int StrategyFR::is_continue_mr(string symbol, double qty)
{
    double mr = calc_future_uniMMR(symbol, qty);
    if (IS_DOUBLE_GREATER(mr, 9)) {
        return 2;
    }
    else return INT_MIN;
}

bool StrategyFR::over_max_delta_limit(sy_info& sy1, sy_info& sy2) 
{
    sy1.real_pos = sy1.inst->position().getNetPosition();
    sy2.real_pos = sy2.inst->position().getNetPosition();

    if (SWAP == sy1.sy) {
        sy2.avg_price = sy1.avg_price;
    } else {
        sy1.avg_price = sy2.avg_price;
    }
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
    sy_info* sy2 = sy1.ref;

    double delta_posi = sy1.real_pos + sy2->real_pos;
    if (IS_DOUBLE_LESS(abs(delta_posi), sy1.max_delta_limit)) return;
    if (IS_DOUBLE_GREATER(abs(delta_posi), sy1.force_close_amount))
        LOG_FATAL << "force close symbol: " << symbol << ", delta_posi: " << delta_posi;
     
    double taker_qty = abs(delta_posi);

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER_EQUAL(delta_posi, sy1.max_delta_limit)) {
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, 0)) {
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                            sy2->ask_p,
                            taker_qty,
                            order);
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, 0)) {          
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                            sy2->ask_p,
                            taker_qty,
                            order);
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                            sy1.ask_p,
                            taker_qty,
                            order);
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                            sy1.ask_p,
                            taker_qty,
                            order);
        }
    } else if (IS_DOUBLE_LESS_EQUAL(delta_posi, -sy1.max_delta_limit)) {
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, 0)) {
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                            sy2->bid_p,
                            taker_qty,
                            order);
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, 0)) {          
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                            sy2->bid_p,
                            taker_qty,
                            order);
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                            sy1.bid_p,
                            taker_qty,
                            order);
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                            sy1.bid_p,
                            taker_qty,
                            order);
        }
    }
}

// flag 1 arb , 0 fr
bool StrategyFR::ClosePosition(const InnerMarketData &marketData, sy_info& sy, int closeflag)
{
    bool flag = false;

    double thresh = sy.thresh;
    string stType = "ArbClose";
    if (closeflag == 0) {
        thresh = sy.fr_close_thresh;
        stType = "FrClose";
    }

    sy_info* sy2 = sy.ref;
    if (sy.make_taker_flag == 1) {
        if ((sy.long_short_flag == 1) && IS_DOUBLE_LESS(sy.real_pos, 0) &&
            IS_DOUBLE_LESS(sy.mid_p - sy2->mid_p, 0)) {
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy.real_pos, marketData.BidVolume1);

                setOrder(sy.inst, INNER_DIRECTION_Buy,
                    marketData.BidPrice1 - sy.prc_tick_size,
                    qty, order);

                flag = true;
            }
        } else if ((sy.long_short_flag == 0) && IS_DOUBLE_GREATER(sy.real_pos, 0) &&
            IS_DOUBLE_GREATER(sy.mid_p - sy2->mid_p, 0)) {
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p; 
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy.real_pos, marketData.AskVolume1);

                setOrder(sy.inst, INNER_DIRECTION_Sell,
                    marketData.AskPrice1 + sy.prc_tick_size,
                    qty, order);

                flag = true;
            }
        }
    } else if (sy2->make_taker_flag == 1) {
        if ((sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0) &&
            IS_DOUBLE_LESS(sy2->mid_p, sy.mid_p)) {
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy2->real_pos, sy2->bid_v);

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                flag = true;
            }
        }  else if ((sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0) &&
            IS_DOUBLE_GREATER(sy2->mid_p, sy.mid_p)) {
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p; 
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy2->real_pos, sy2->ask_v);

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
                    qty, order);

                flag = true;
            }
        }
    }
    return flag;
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

    if (sy1.close_flag) {
        ClosePosition(marketData, sy1, 0);
        return;
    }

    if (ClosePosition(marketData, sy1, 1)) return;

    if (!enable_maker) return;
    sy_info* sy2 = sy1.ref;
    double bal = calc_balance();
    if (sy1.make_taker_flag == 1) {
        if (sy1.long_short_flag == 1 && IS_DOUBLE_GREATER(sy1.mid_p, sy2->mid_p)) {
            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, sy1.fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, marketData.AskVolume1 / 2);
                if (IS_DOUBLE_LESS(qty, sy1.max_delta_limit)) return;
                if (is_continue_mr(sy1.sy, qty) != 2) return;
                //  qty = sy1.max_delta_limit;

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy1.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy1.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                string stType = "FrOpen";
                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                setOrder(sy1.inst, INNER_DIRECTION_Sell,
                    marketData.AskPrice1 + sy1.prc_tick_size,
                    qty, order);
            }

        } else if (sy1.long_short_flag == 0 && IS_DOUBLE_LESS(sy1.mid_p, sy2->mid_p)) {
            double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, sy1.fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, marketData.BidVolume1 / 2);

                if (IS_DOUBLE_LESS(qty, sy1.max_delta_limit)) return;
                if (is_continue_mr(sy1.sy, qty) != 2) return;

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy1.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy1.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                string stType = "FrOpen";
                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                setOrder(sy1.inst, INNER_DIRECTION_Buy,
                    marketData.BidPrice1 - sy1.prc_tick_size,
                    qty, order);
            }
        }
    } else if (sy2->make_taker_flag == 1) {
        if (sy2->long_short_flag == 0 &&  IS_DOUBLE_GREATER(sy1.mid_p, sy2->mid_p)) {
            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, sy1.thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.ref->real_pos) * sy1.ref->mid_p, sy1.ref->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                string stType = "FrOpen";
                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy2->bid_v / 2);
                if (is_continue_mr(sy2.sy, qty) != 2) return;

                if (IS_DOUBLE_LESS(qty, sy2.max_delta_limit)) return;

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);
            }
        } else if (sy2->long_short_flag == 1 && IS_DOUBLE_LESS(sy1.mid_p, sy2->mid_p)) {
            double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, sy1.thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.ref->real_pos) * sy1.ref->mid_p, sy1.ref->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                string stType = "FrOpen";
                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy2->ask_v / 2);
                if (is_continue_mr(sy2.sy, qty) != 2) return;

                if (IS_DOUBLE_LESS(qty, sy2.max_delta_limit)) return;

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
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

void StrategyFR::Mr_Market_ClosePosition(StrategyInstrument *strategyInstrument)
{
    sy_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];
    string stType = "FrClose";

    sy_info* sy2 = sy.ref;

    SetOrderOptions order;
    order.orderType = ORDERTYPE_MARKET; // ?

    if (SPOT == sy.type) {
        string Category = LEVERAGE;
        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else if (SWAP == sy.type) {
        string Category = LINEAR;
        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else {
        LOG_FATAL << "";
    }

    memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

    memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

    double qty = sy.real_pos;

    if (IS_DOUBLE_GREATER(qty, 0)) {
        setOrder(sy.inst, INNER_DIRECTION_Sell,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    } else {
        setOrder(sy.inst, INNER_DIRECTION_Buy,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    }

    SetOrderOptions order1;
    order1.orderType = ORDERTYPE_MARKET; // ?

    if (SPOT == sy2->type) {
        string Category = LEVERAGE;
        memcpy(order1.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else if (SWAP == sy2->type) {
        string Category = LINEAR;
        memcpy(order1.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else {
        LOG_FATAL << "";
    }

    memcpy(order1.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

    memcpy(order1.StType, stType.c_str(), min(sizeof(order1.StType) - 1, stType.size()));

    double qty = sy2->real_pos;

    if (IS_DOUBLE_GREATER(qty, 0)) {
        setOrder(sy2->inst, INNER_DIRECTION_Sell,
            sy2->bid_p - sy2->prc_tick_size,
            abs(qty), order1);
    } else {
        setOrder(sy2->inst, INNER_DIRECTION_Buy,
            sy2->bid_p - sy2->prc_tick_size,
            abs(qty), order1);
    }




}

// flag 1 arb , 0 fr
void StrategyFR::Mr_ClosePosition(StrategyInstrument *strategyInstrument)
{
    sy_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];

    double thresh = sy.fr_close_thresh;
    string stType = "FrClose";

    sy_info* sy2 = sy.ref;
    if (sy.make_taker_flag == 1) {
        if ((sy.long_short_flag == 1) && IS_DOUBLE_LESS(sy.real_pos, 0) &&
            IS_DOUBLE_LESS(sy.mid_p - sy2->mid_p, 0)) {
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy.real_pos, sy.bid_v);

                setOrder(sy.inst, INNER_DIRECTION_Buy,
                    sy.bid_p - sy.prc_tick_size,
                    qty, order);
            }
        } else if ((sy.long_short_flag == 0) && IS_DOUBLE_GREATER(sy.real_pos, 0) &&
            IS_DOUBLE_GREATER(sy.mid_p - sy2->mid_p, 0)) {
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p; 
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy.real_pos, sy.ask_v);

                setOrder(sy.inst, INNER_DIRECTION_Sell,
                    sy.ask_p + sy.prc_tick_size,
                    qty, order);
            }
        }
    } else if (sy2->make_taker_flag == 1) {
        if ((sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0) &&
            IS_DOUBLE_LESS(sy2->mid_p, sy.mid_p)) {
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p;
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy2->real_pos, sy2->bid_v);

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);
            }
        }  else if ((sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0) &&
            IS_DOUBLE_GREATER(sy2->mid_p, sy.mid_p)) {
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p; 
            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (SPOT == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy2->real_pos, sy2->ask_v);

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
                    qty, order);
            }
        }
    }
}

void StrategyFR::OnTimerTradingLogic() 
{
    over_max_delta_limit();
    double mr = calc_uniMMR();

    if (IS_DOUBLE_GREATER(mr, 3) && IS_DOUBLE_LESS(mr, 6)) {
        for (auto iter : strategyInstrumentList()) {
            Mr_ClosePosition(iter);
            enable_maker = false;
        }
    } else if (IS_DOUBLE_LESS(mr, 3)) {
        for (auto iter : strategyInstrumentList()) {
            Mr_Market_ClosePosition(tier);
            enable_maker = false;
        }
    } else if (IS_DOUBLE_GREATER(mr, 9)) {
        enable_maker = true;
    }
    // for (auto it : (*make_taker)) {
    //     if (SPOT == it.second.type) {
    //         auto iter = BnApi::BalMap_.find(it.first);
    //         if (iter == BnApi::BalMap_.end()) LOG_FATAL << "";
    //         double qty = iter->second.crossMarginFree + iter->second.crossMarginLocked - iter->second.crossMarginLocked - iter->second.crossMarginInterest;
    //         if (IS_DOUBLE_GREATER(abs(it.second.real_pos - qty), it.second.max_delta_limit)) {
    //             LOG_WARN << "";
    //         }
    //     }

    //     if (SWAP == it.second.type) {
    //         bool flag = false;
    //         for (auto iter : BnApi::UmAcc_->info1_) {
    //             if (it.first == iter.symbol) {
    //                 if (IS_DOUBLE_GREATER(abs(it.second.real_pos - iter.positionAmt), it.second.max_delta_limit)) {
    //                     LOG_WARN << "";
    //                 }
    //                 flag = true;
    //             }
    //         }
    //         if (!flag) LOG_FATAL << "";
    //     }

    //     if (PERP == it.second.type) {
    //         for (auto iter : BnApi::CmAcc_->info1_) {
    //             if (it.first == iter.symbol) LOG_FATAL << "";
    //         }
    //     }
    // }
    

}

