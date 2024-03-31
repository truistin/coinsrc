#include "StrategyFR.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Measure.h"
#include "spot/base/InitialData.h"
#include "spot/cache/CacheAllocator.h"
#include "spot/utility/TradingDay.h"
#include "spot/bian/BnApi.h"
#include <spot/utility/MeasureFunc.h>

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

    pridict_borrow = new map<string, double>;
    make_taker = new map<string, sy_info>;
    symbol_map = new map<string, string>;

    pre_sum_equity = 0;
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
            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
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
            
            string sy = GetSPOTSymbol(iter->instrument()->getInstrumentID());
            auto it = BnApi::BalMap_.find(sy);
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

string StrategyFR::GetUMSymbol(string inst) {
    //btc_usdt_binance_swap --> btcusdt
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
    return cp;
}

string StrategyFR::GetCMSymbol(string inst) {
    //btc_usdt_binance_perp --> btcusdt_perp
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    cp.erase(std::remove(cp.begin(), cp.end(), '_'), cp.end());
    cp = cp  + "_perp";
    transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
    return cp;
}

string StrategyFR::GetSPOTSymbol(string inst) {
    //btc_usdt_binance_perp --> btcusdt_perp
    string cp = inst.substr(0, inst.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    cp = cp.substr(0, cp.find_last_of('_'));
    transform(cp.begin(), cp.end(), cp.begin(), ::toupper);
    return cp;
}

void StrategyFR::init() 
{
    MeasureFunc::addMeasureData(1, "StrategyFR time calc", 10000);
    for (auto iter : strategyInstrumentList()) {
        string sy = iter->instrument()->getInstrumentID();
        if (sy.find("swap") != std::string::npos) {
            symbol_map->insert({GetUMSymbol(sy), sy});
        } else if (sy.find("perp") != std::string::npos) {
            symbol_map->insert({GetCMSymbol(sy), sy});
        } else if (sy.find("spot") != std::string::npos) {
            symbol_map->insert({GetSPOTSymbol(sy), sy});
        }
    }

    for (auto it : InitialData::symbolInfoMap()) {
        sy_info syInfo;
        memcpy(syInfo.sy, it.second.Symbol, min(sizeof(syInfo.sy), sizeof(it.second.Symbol)));
        memcpy(syInfo.ref_sy, it.second.RefSymbol, min(sizeof(syInfo.ref_sy), sizeof(it.second.RefSymbol)));
        memcpy(syInfo.type, it.second.Type, min(sizeof(syInfo.type), sizeof(it.second.Type)));

        syInfo.long_short_flag = it.second.LongShort;
        syInfo.make_taker_flag = it.second.MTaker;
        syInfo.mv_ratio = it.second.MvRatio;
        syInfo.thresh = it.second.Thresh;

        syInfo.fr_open_thresh = it.second.OpenThresh;
        syInfo.close_thresh = it.second.CloseThresh;
        syInfo.close_flag = it.second.CloseFlag;

        syInfo.prc_tick_size = it.second.PreTickSize;
        syInfo.qty_tick_size = it.second.QtyTickSize;
        syInfo.min_amount = it.second.MinAmount;
        syInfo.fragment = it.second.Fragment;

        make_taker->insert({it.second.Symbol, syInfo});
    }

    for (auto iter : strategyInstrumentList()) {
        for (auto it : (*make_taker)) {
            if (it.first == iter->instrument()->getInstrumentID()) {
                it.second.inst = iter;
                it.second.sellMap = iter->sellOrders();
                it.second.buyMap = iter->buyOrders();
                break;
            }
        }
    }

    qryPosition();

    for (auto it : (*make_taker)) {
        it.second.ref = &(*make_taker)[it.second.ref_sy];
        LOG_INFO << "make_taker ref: " << it.second.sy << ", ref sy: " << it.second.ref_sy << ", ref: " << it.second.ref;
        if (SWAP == it.second.sy) it.second.ref->avg_price = it.second.avg_price;
    }
}

void StrategyFR::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "OnPartiallyFilledTradingLogic sy2 nullptr: " << rtnOrder.InstrumentID;
        return;
    }
    if (!check_min_delta_limit(sy1, (*sy2))) return;

    hedge(strategyInstrument);
    return;
}

void StrategyFR::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "OnFilledTradingLogic sy2 nullptr: " << rtnOrder.InstrumentID;
        return;
    }
    if (!check_min_delta_limit(sy1, (*sy2))) return;

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

double StrategyFR::calc_future_uniMMR(sy_info& info, double qty)
{
    double usdt_equity = get_usdt_equity();
    double sum_equity = calc_equity();
    double IM = 0;

    order_fr order;
    order.sy = info.sy;
    order.qty = qty;

    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_future_umimmr has no mkprice: " << info.sy << ", markprice: " << info.mid_p;
        return 0;
    }
    double borrow = 0;
    if ((SPOT == info.type && info.long_short_flag == 0) || (SWAP == info.type && info.long_short_flag == 1)) { // 锟斤拷usdt
        borrow = qty * price;
        IM = IM + borrow / ((*margin_leverage)[info.sy] - 1) + (qty * price / um_leverage);         
    } else { 
        borrow = qty;
        IM = IM + (price * (qty) / ((*margin_leverage)[info.sy] - 1)) + (qty) * price / um_leverage;
    }

    order.borrow = borrow;

    if (IS_DOUBLE_GREATER(IM, sum_equity)) {
        LOG_INFO << "IM: " << IM << ", sum_equity: " << sum_equity;
        return 0;
    }

    double predict_equity = calc_predict_equity(info, order, price_ratio);
    double predict_mm = calc_predict_mm(info, order, price_ratio);
    double predict_mmr = predict_equity / predict_mm;
    return predict_mmr;

}

double StrategyFR::calc_predict_equity(sy_info& info, order_fr& order, double price_cent)
{
    double sum_equity = 0;
    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_equity has no mkprice: " << order.sy << ", markprice: " << info.mid_p;
        return 0;
    }

    double rate = collateralRateMap[order.sy];

    if ((SPOT == info.type && info.long_short_flag == 0) || (SWAP == info.type && info.long_short_flag == 1)) { // 锟街伙拷锟斤拷锟斤拷锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�777777 锟斤拷约锟斤拷锟斤拷
        double equity = order.qty * price * (1 + price_cent) * rate;
        double uswap_unpnl = order.qty * price - (1 + price_cent) * price * order.qty;
        sum_equity += equity - order.borrow + uswap_unpnl;
    } else { // 锟街伙拷锟斤拷锟秸★拷1锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�777777 锟斤拷约锟斤拷锟斤拷
        double qty = (order.qty);
        double equity = qty * price - order.borrow * (1 + price_cent) * price;
        double uswap_unpnl = order.qty * price * (1 + price_cent) - qty * price;
        sum_equity = equity + uswap_unpnl;
    }

    for (auto it : BnApi::BalMap_) {
        if (symbol_map->find(it.first) == symbol_map->end()) continue;
        double rate = collateralRateMap[it.first];

        string sy = it.first;
        double price = 0;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            price = getSpotAssetSymbol(sy);
        } else {
            price = getSpotAssetSymbol(sy) * (1 + price_cent);
        }

        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "BalMap calc_predict_equity mkprice: " << sy << ", markprice: " << getSpotAssetSymbol(sy);
        } else {
            continue;
        }

        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double equity = it.second.umWalletBalance;
            sum_equity = sum_equity + equity * price;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double equity = it.second.cmWalletBalance;
            sum_equity = sum_equity + min(equity*price, equity*price*rate);
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        if (symbol_map->find(iter.symbol) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p * (1 + price_cent);
        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc mkprice: " << iter.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
        } else {
            continue;
        }
        double avgPrice = (iter.entryPrice * iter.positionAmt + order.qty * (*make_taker)[(*symbol_map)[iter.symbol]].mid_p) / (iter.positionAmt + order.qty);
        double uswap_unpnl = (price - avgPrice) * iter.positionAmt;
        sum_equity += uswap_unpnl;
    }

    for (auto iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        if (symbol_map->find(sy) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p * (1 + price_cent);
        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc mkprice: " << sy << ", markprice: " << (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
        } else {
            continue;
        }
        double perp_size = 0;
        if (sy == "BTCUSD_PERP") {
            perp_size = 100;
        } else {
            perp_size = 10;
        }

        double quantity = perp_size * iter.positionAmt / iter.entryPrice + order.qty * perp_size / (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;

        double turnover = perp_size * iter.positionAmt + order.qty * perp_size;

        double avgPrice = turnover / quantity;
        if (IS_DOUBLE_LESS_EQUAL(avgPrice, 0)) {
            avgPrice = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
        }

        double cswap_unpnl = price * perp_size * iter.positionAmt * (1/avgPrice - 1/price);
        sum_equity += cswap_unpnl;
    }
    return sum_equity;
}

double StrategyFR::calc_predict_mm(sy_info& info, order_fr& order, double price_cent)
{
    double sum_mm = 0;
    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_mm has no mkprice: " << order.sy << ", markprice: " << info.mid_p;
        return 0;
    }
    double leverage = 0;
    if (margin_leverage->find(order.sy) == margin_leverage->end()) {
        leverage = (*margin_leverage)["default"];
    } else {
        leverage = (*margin_leverage)[order.sy];
        
    }

    if ((SPOT == info.type && info.long_short_flag == 0) || (SWAP == info.type && info.long_short_flag == 1)) { // 锟街伙拷锟斤拷锟洁，锟斤拷约锟斤拷锟絔1锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�777777
        sum_mm = sum_mm + order.borrow * (*margin_mmr)[leverage];
    } else { // 锟街伙拷锟斤拷锟秸ｏ拷锟斤拷约锟斤拷锟斤拷1锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�777777
        sum_mm = sum_mm + order.borrow * price * (*margin_mmr)[leverage];
    }

    for (auto it : BnApi::BalMap_) {
        if (symbol_map->find(it.first) == symbol_map->end()) continue;
        double leverage = (*margin_mmr)[(*margin_leverage)[it.first]];
        double price = getSpotAssetSymbol(it.first);
        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "BalMap calc_predict_mm mkprice: " << it.first << ", markprice: " << price;
        } else {
            continue;
        }
        string sy = it.first;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * 1; // 锟杰革拷锟街伙拷维锟街憋拷证锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�77771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�777777
        } else {
            sum_mm = sum_mm + it.second.crossMarginBorrowed + (*margin_mmr)[leverage] * price;
        }
    }

    for (auto iter : BnApi::UmAcc_->info1_) {
        string sy = iter.symbol;
        if (symbol_map->find(sy) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[sy]].mid_p * (1 + price_cent);
        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc calc_predict_mm mkprice: " << sy << ", markprice: " << (*make_taker)[(*symbol_map)[sy]].mid_p;
        } else {
            continue;
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
        if (symbol_map->find(sy) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[sy]].mid_p * (1 + price_cent);
        if (!IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc calc_predict_mm mkprice: " << sy << ", markprice: " << price;
        } else {
            continue;
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
            sum_usdt += (it.crossWalletBalance + it.crossUnPnl) * getSpotAssetSymbol(sy);
        }
    }

    // cm_account
    for (auto it : BnApi::CmAcc_->info_) {
        string sy = it.asset;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            LOG_FATAL << "";
        }
        sum_usdt += (it.crossWalletBalance + it.crossUnPnl) * getSpotAssetSymbol(sy);
    }

    for (auto it : BnApi::BalMap_) {
        string sy = it.second.asset;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_usdt += it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginLocked - it.second.crossMarginInterest;
        } else {
            sum_usdt += (it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginLocked - it.second.crossMarginInterest) * getSpotAssetSymbol(sy);
        }

    }
    return sum_usdt;
}

double StrategyFR::calc_equity()
{
    double sum_equity = 0;
    for (auto it : BnApi::BalMap_) {
        double price = getSpotAssetSymbol(it.second.asset);
        if (!IS_DOUBLE_LESS_EQUAL(price, 0)) {
            LOG_WARN << "calc_equity asset mkprice: " << it.second.asset << ", markprice: " << price;
        } else {
            continue;
        }
        if (!IS_DOUBLE_ZERO(it.second.crossMarginAsset) ||  !IS_DOUBLE_ZERO(it.second.crossMarginBorrowed)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.crossMarginFree + it.second.crossMarginLocked - it.second.crossMarginBorrowed - it.second.crossMarginInterest;
            double calc_equity = min(equity * price, equity * price * rate);
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.umWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.umUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.umWalletBalance +  it.second.umUnrealizedPNL;
            double calc_equity = equity * price; // should be swap symbol
            sum_equity += calc_equity;
        }

        if (!IS_DOUBLE_ZERO(it.second.cmWalletBalance) ||  !IS_DOUBLE_ZERO(it.second.cmUnrealizedPNL)) {
            double rate = collateralRateMap[it.second.asset];
            double equity = it.second.cmWalletBalance +  it.second.cmUnrealizedPNL;
            double calc_equity = min(equity * price, equity * price * rate); // should be perp symbol
            sum_equity += calc_equity;
        }
    }
    return sum_equity;
}
double StrategyFR::getSpotAssetSymbol(string asset)
{
    if (asset == "USDT" || asset == "USDC" || asset == "BUSD") {
        return 1;
    } 
    
    string sy = asset + "_usdt_binance_spot";
    transform(sy.begin(), sy.end(), sy.begin(), ::tolower);

    if (make_taker->find(sy) == make_taker->end()) return 0;
    return (*make_taker)[sy].mid_p;

}
double StrategyFR::calc_mm()
{
    double sum_mm = 0;
    // 锟街伙拷锟杰革拷mm
    for (auto it : BnApi::BalMap_) {
        double price = getSpotAssetSymbol(it.second.asset);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            if (make_taker->find(it.second.asset) == make_taker->end()) continue;
            LOG_ERROR << "calc_mm asset has no mkprice: " << it.second.asset << ", markprice: " << price;
            return -1;
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
            double mm = it.second.crossMarginBorrowed * (*margin_mmr)[leverage] * price;
            sum_mm += mm;
        }
    }

    for (auto it : BnApi::UmAcc_->info1_) {
        if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
        if (IS_DOUBLE_LESS_EQUAL((*make_taker)[(*symbol_map)[it.symbol]].mid_p , 0)) {
            LOG_ERROR << "calc_mm UmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[it.symbol]].mid_p;
            return -1;
        }

        string symbol = it.symbol;
        double qty = it.positionAmt;
        double markPrice = (*make_taker)[(*symbol_map)[symbol]].mid_p;
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty) * markPrice, mmr_rate, mmr_num);
        double mm = abs(qty) * markPrice * mmr_rate - mmr_num;
        sum_mm += mm;
    }

    for (auto it : BnApi::CmAcc_->info1_) {
        if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
        if (IS_DOUBLE_LESS_EQUAL((*make_taker)[(*symbol_map)[it.symbol]].mid_p , 0)) {
            LOG_ERROR << "calc_mm CmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[it.symbol]].mid_p;
            return -1;
        }
        string symbol = it.symbol;
        double qty = 0;

        if (symbol == "BTCUSD_PERP") {
            qty = it.positionAmt * 100;
        } else {
            qty = it.positionAmt * 10;
        }

        double markPrice = (*make_taker)[(*symbol_map)[symbol]].mid_p;
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
    if (uniAccount_mm == 0) return 999;
    if (uniAccount_mm == -1) return 8;
    return (uniAccount_equity)/(uniAccount_mm);
}

bool StrategyFR::is_continue_mr(sy_info* info, double qty)
{
    double mr = calc_future_uniMMR(*info, qty);
    if (IS_DOUBLE_GREATER(mr, 2)) {
        return true;
    }
    return false;
}

bool StrategyFR::check_min_delta_limit(sy_info& sy1, sy_info& sy2) 
{
    sy1.real_pos = sy1.inst->position().getNetPosition();
    sy2.real_pos = sy2.inst->position().getNetPosition();

    if (SWAP == sy1.sy) {
        sy2.avg_price = sy1.inst->position().PublicPnlDaily().EntryPrice;
    } else {
        sy1.avg_price = sy2.inst->position().PublicPnlDaily().EntryPrice;
    }
    if (IS_DOUBLE_LESS(abs((sy1.real_pos + sy2.real_pos) * sy1.mid_p), sy1.min_amount)) {
        return false;
        LOG_INFO << "check_min_delta_limit sy1 real_pos: " << sy1.real_pos << ", sy2 real_pos: " << sy2.real_pos;
    }

    return true;
}

void StrategyFR::hedge(StrategyInstrument *strategyInstrument)
{
    string symbol = strategyInstrument->getInstrumentID();
    if (make_taker->find(symbol) == make_taker->end()) return;
    sy_info& sy1 = (*make_taker)[symbol];
    sy_info* sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "hedge sy2 nullptr: " << symbol;
        return;
    }
    double delta_posi = sy1.real_pos + sy2->real_pos;
    if (sy1.make_taker_flag == 0) {
        if (IS_DOUBLE_LESS(abs(delta_posi * sy1.mid_p), sy1.min_amount)) return;
    } else if(sy2->make_taker_flag == 0) {
        if (IS_DOUBLE_LESS(abs(delta_posi * sy2->mid_p), sy2->min_amount)) return;
    }
    if (IS_DOUBLE_GREATER(abs(delta_posi) * sy1.mid_p, 3 * sy1.fragment)) {
        LOG_FATAL <<  "more than 3 * fragment " << symbol << ", delta_posi: " << delta_posi;
        return;
    }
    double taker_qty = abs(delta_posi);

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER_EQUAL(delta_posi, 0)) {
        // sy1 maker open_short  sy1.pos < 0 delta_pos > 0 sy2.close_long
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, sy1.qty_tick_size)) {
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            taker_qty = round1(taker_qty, sy2->qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy2->mid_p, sy2->min_amount)) return;
            
            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                            sy2->ask_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker close long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->ask_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        // sy1 maker open_long sy1.pos > 0 delta_pos > 0 sy2.open_short
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, sy1.qty_tick_size)) {          
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            taker_qty = round1(taker_qty, sy2->qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy2->mid_p, sy2->min_amount)) return;

            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                            sy2->ask_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker open short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->ask_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        // sy2 maker open_short sy2.pos<0 delta_pos>0 sy1.close_long
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, sy2->qty_tick_size)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            taker_qty = round1(taker_qty, sy1.qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy1.mid_p, sy1.min_amount)) return;

            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                            sy1.ask_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy1 taker close long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.ask_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        // sy2 maker open_long sy2.pos>0 delta_pos>0 sy1.open_short
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, sy2->qty_tick_size)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            taker_qty = round1(taker_qty, sy1.qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy1.mid_p, sy1.min_amount)) return;

            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                            sy1.ask_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy1 taker open short: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.ask_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        }
    } else if (IS_DOUBLE_LESS_EQUAL(delta_posi, 0)) {
        // sy1 maker open_short sy1.pos<0 delta_pos<0 sy2 open_long
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, sy1.qty_tick_size)) {
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            taker_qty = round1(taker_qty, sy2->qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy2->mid_p, sy2->min_amount)) return;

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                            sy2->bid_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker open long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        //sy1 maker open_long sy1.pos>0 delta_pos<0 sy2 close_short
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, sy1.qty_tick_size)) {          
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy2->type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            taker_qty = round1(taker_qty, sy2->qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy2->mid_p, sy2->min_amount)) return;

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                            sy2->bid_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        //sy2 maker open_short sy2.pos<0 delta_pos<0 sy1 open_long
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, sy2->qty_tick_size)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            taker_qty = round1(taker_qty, sy1.qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy1.mid_p, sy1.min_amount)) return;

            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                            sy1.bid_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy1 taker open long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Buy
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.bid_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        //sy2 maker open_long sy2.pos>0 delta_pos<0 sy1 close_short
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, sy2->qty_tick_size)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (SPOT == sy1.type)
                memcpy(order.Category, LEVERAGE.c_str(), min(uint16_t(CategoryLen), uint16_t(LEVERAGE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            taker_qty = round1(taker_qty, sy1.qty_tick_size, qty_decimal);
            if (IS_DOUBLE_LESS(taker_qty * sy1.mid_p, sy1.min_amount)) return;

            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                            sy1.bid_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy1 taker close short: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Buy
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.bid_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        }
    }
}

// flag 1 arb , 0 fr
//close arb_thresh/fr_thresh 锟斤拷锟斤拷锟斤拷锟斤拷为锟斤拷maker锟斤拷锟斤拷  maker锟斤拷taker 锟斤拷锟斤拷芨叨锟斤拷锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7777(at most larger than taker)锟斤拷锟斤拷maker锟斤拷锟洁，锟斤拷锟斤拷maker锟斤拷taker锟斤拷锟斤拷要锟竭讹拷锟斤拷(at least large than taker)
bool StrategyFR::ClosePosition(const InnerMarketData &marketData, sy_info& sy, int closeflag)
{
    bool flag = false;

    double thresh = sy.close_thresh;
    string stType = "ArbClose";
    if (closeflag == 0) { 
        stType = "FrClose";
    }

    double bal = 0;
    if (closeflag == 0) {
        bal = calc_balance();
    }

    sy_info* sy2 = sy.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "ClosePosition sy2 nullptr: " << sy.sy;
        return false;
    }
    double delta_posi = sy.real_pos + sy2->real_pos;
    if (IS_DOUBLE_GREATER(abs(delta_posi) * sy.mid_p, 3 * sy.fragment)) {
        LOG_FATAL <<  "more than 3 * fragment " << sy.sy << ", delta_posi: " << delta_posi;
        return false;
    }
    if (sy.make_taker_flag == 1) { // sy1 maker
        if ((sy.long_short_flag == 1) && IS_DOUBLE_LESS(sy.real_pos, sy.qty_tick_size)) { // sy1 short
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p;
            if (IS_DOUBLE_LESS(spread_rate, thresh)) {
                if (IsCancelExistOrders(&sy, INNER_DIRECTION_Buy)) return false;
                if (closeflag == 0 && IS_DOUBLE_LESS(abs(sy.real_pos) * sy.avg_price, sy.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return false;
                }

                double u_posi = abs(sy.real_pos) * sy.avg_price;
                double qty = min((u_posi - bal * sy.mv_ratio) / sy.mid_p, sy2->bid_v / 2);
                qty = min (qty, sy.fragment/sy.mid_p);

                double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
                qty = round1(qty, sy.qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy.mid_p, sy.min_amount)) return false;
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

                // double qty = std::min(sy.real_pos, sy2->bid_v / 2);

                setOrder(sy.inst, INNER_DIRECTION_Buy,
                    marketData.BidPrice1 - sy.prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy maker close short : " << sy.sy << ", sy order side: " << INNER_DIRECTION_Buy
                    << ", sy maker_taker_flag: " << sy.make_taker_flag
                    << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                    << ", sy category: " << sy.type << ", sy order price: "
                    << marketData.BidPrice1 - sy.prc_tick_size << ", sy order qty: " << qty;                

                flag = true;
            }
        } else if ((sy.long_short_flag == 0) && IS_DOUBLE_GREATER(sy.real_pos, sy.qty_tick_size)) { //sy1 long
            if (IsCancelExistOrders(&sy, INNER_DIRECTION_Sell)) return false;
            double spread_rate = (sy.mid_p - sy2->mid_p) / sy2->mid_p; 

            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                if (closeflag == 0 && IS_DOUBLE_LESS(abs(sy.real_pos) * sy.avg_price, sy.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return false;
                }

                double u_posi = abs(sy.real_pos) * sy.avg_price;
                double qty = min((u_posi - bal * sy.mv_ratio) / sy.mid_p, sy2->ask_v / 2);
                qty = min (qty, sy.fragment/sy.mid_p);

                double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
                qty = round1(qty, sy.qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy.mid_p, sy.min_amount)) return false;

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

                // double qty = std::min(sy.real_pos, sy2->ask_v / 2);

                setOrder(sy.inst, INNER_DIRECTION_Sell,
                    marketData.AskPrice1 + sy.prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy maker close long: " << sy.sy << ", sy order side: " << INNER_DIRECTION_Sell
                    << ", sy maker_taker_flag: " << sy.make_taker_flag
                    << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                    << ", sy category: " << sy.type << ", sy order price: "
                    << marketData.AskPrice1 + sy.prc_tick_size << ", sy order qty: " << qty;    

                flag = true;
            }
        }
    } else if (sy2->make_taker_flag == 1) { // sy2 maker
        if ((sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, sy.qty_tick_size)) { //sy2 short
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Buy)) return false;
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p;

            if (IS_DOUBLE_LESS(spread_rate, thresh)) {

                if (closeflag == 0 && IS_DOUBLE_LESS(abs(sy2->real_pos) * sy2->avg_price, sy2->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return false;
                }

                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((u_posi - bal * sy2->mv_ratio) / sy2->mid_p, sy.bid_v / 2);
                qty = min (qty, sy2->fragment/sy2->mid_p);

                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);

                
                if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) return false;

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

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy2 maker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;   

                flag = true;
            }
        }  else if ((sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, sy.qty_tick_size)) { // sy2 long
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Sell)) return false;
            double spread_rate = (sy2->mid_p - sy.mid_p) / sy.mid_p; 

            if (IS_DOUBLE_GREATER(spread_rate, thresh)) {
                if (closeflag == 0 && IS_DOUBLE_LESS(abs(sy2->real_pos) * sy2->avg_price, sy2->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return false;
                }


                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((u_posi - bal * sy2->mv_ratio) / sy2->mid_p, sy.ask_v / 2);
                qty = min (qty, sy2->fragment/sy2->mid_p);

                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) return false;

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

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy2 maker close long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->ask_p + sy2->prc_tick_size << ", sy2 order qty: " << qty;  

                flag = true;
            }
        }
    }
    return flag;
}

bool StrategyFR::IsCancelExistOrders(sy_info* sy, int side)
{
    if (side == INNER_DIRECTION_Buy) {
        if (sy->buyMap->size() != 0) {
            for (auto it : (*sy->buyMap)) {
                for (auto iter : it.second->OrderList)
                    sy->inst->cancelOrder(iter);
            }
            return true;
        }
    } else if (side == INNER_DIRECTION_Sell) {
        if (sy->sellMap->size() != 0) {
            for (auto it : (*sy->sellMap)) {
                for (auto iter : it.second->OrderList)
                    sy->inst->cancelOrder(iter);
            }
            return true;
        }
    } else {
        LOG_FATAL << "side fatal: " << side;
    }
    return false;

}

//open fr_thresh 锟斤拷锟斤拷锟斤拷锟斤拷为锟斤拷maker锟斤拷锟斤拷, maker锟斤拷taker 锟斤拷锟斤拷要锟竭讹拷锟斤拷(at least larger than taker)锟斤拷锟斤拷maker锟斤拷锟洁，锟斤拷锟斤拷maker锟斤拷taker锟斤拷锟斤拷芨叨锟斤拷锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7777(at least large than taker)
void StrategyFR::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    MeasureFunc f(1);
    int64_t ts = CURR_MSTIME_POINT;
    if (marketData.EpochTime - ts > 30) {
        LOG_WARN << "market data beyond time: " << marketData.InstrumentID << ", ts: " << marketData.EpochTime;
        return;
    }
    if (make_taker->find(marketData.InstrumentID) == make_taker->end()) return;
    sy_info& sy1 = (*make_taker)[marketData.InstrumentID];
    sy1.update(marketData.AskPrice1, marketData.BidPrice1, marketData.AskVolume1, marketData.BidVolume1, marketData.UpdateMillisec);

    if (!sy1.close_flag) { //fr close
        ClosePosition(marketData, sy1, 0);
        return;
    } 
    // arb close
    if(ClosePosition(marketData, sy1, 1)) return;

    double mr = 0;
    if (!make_continue_mr(mr)) {
        action_mr(mr);
        return;
    }

    sy_info* sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "OnRtnInnerMarketDataTradingLogic sy2 nullptr: " << sy1.sy;
        return;
    }
    double bal = calc_balance();
    double delta_posi = sy1.real_pos + sy2->real_pos;
    if (IS_DOUBLE_GREATER(abs(delta_posi) * sy1.mid_p, 3 * sy1.fragment)) {
        LOG_FATAL <<  "more than 3 * fragment " << sy1.sy << ", delta_posi: " << delta_posi;
        return;
    }


    if (sy1.make_taker_flag == 1) { //sy1 maker
        // sy1 short
        if (sy1.long_short_flag == 1 && IS_DOUBLE_LESS(sy1.real_pos * sy1.mid_p, sy1.min_amount)) { 
            if (IsCancelExistOrders(&sy1, INNER_DIRECTION_Sell)) return;

            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;

            if (IS_DOUBLE_GREATER(spread_rate, sy1.fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.avg_price, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->ask_v / 2);
                qty = min (qty, sy1.fragment/sy1.mid_p);

                double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
                qty = round1(qty, sy1.qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy1.mid_p, sy1.min_amount)) return;

                if (!is_continue_mr(&sy1, qty)) return;
                //  qty = sy1.min_delta_limit;

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

                LOG_INFO << "MarketDataTradingLogic sy1 maker open sell: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                    << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                    << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                    << ", sy1 category: " << sy1.type << ", sy1 order price: "
                    << marketData.AskPrice1 + sy1.prc_tick_size << ", sy1 order qty: " << qty;    
            
            }
        //sy1 long
        } else if (sy1.long_short_flag == 0 && IS_DOUBLE_GREATER(sy1.real_pos * sy1.mid_p, -sy1.min_amount)) {
            if (IsCancelExistOrders(&sy1, INNER_DIRECTION_Buy)) return;

            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;

            if (IS_DOUBLE_LESS(spread_rate, sy1.fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.avg_price, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->bid_v / 2);
                qty = min (qty, sy1.fragment/sy1.mid_p);

                double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
                qty = round1(qty, sy1.qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy1.mid_p, sy1.min_amount)) return;

                if (!is_continue_mr(&sy1, qty)) return;

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

                LOG_INFO << "MarketDataTradingLogic sy1 maker open long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Buy
                    << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                    << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                    << ", sy1 category: " << sy1.type << ", sy1 order price: "
                    << marketData.BidPrice1 - sy1.prc_tick_size << ", sy1 order qty: " << qty;    
            }
        }
    } else if (sy2->make_taker_flag == 1) { //sy2 maker
        //sy2 long
        if (sy2->long_short_flag == 0 && IS_DOUBLE_GREATER(sy2->real_pos * sy2->mid_p, -sy2->min_amount)) { 
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Buy)) return;

            double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;

            if (IS_DOUBLE_LESS(spread_rate, sy2->fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy2->real_pos) * sy2->avg_price, sy2->mv_ratio * bal)) {
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
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy1.bid_v / 2);
                qty = min (qty, sy2->fragment/sy2->mid_p);

                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) return;
                if (!is_continue_mr(sy2, qty)) return;

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy2 maker open long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;  

            }
        // sy2 short
        } else if (sy2->long_short_flag == 1 && IS_DOUBLE_LESS(sy2->real_pos * sy2->mid_p, sy2->min_amount)) { 
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Sell)) return;

            double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;

            if (IS_DOUBLE_GREATER(spread_rate, sy2->fr_open_thresh)) {
                if (IS_DOUBLE_GREATER(abs(sy2->real_pos) * sy2->avg_price, sy2->mv_ratio * bal)) {
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
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy1.ask_v / 2);
                qty = min (qty, sy2->fragment/sy2->mid_p);

                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);

                if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) return;
                if (!is_continue_mr(sy2, qty)) return;

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
                    qty, order);
                
                LOG_INFO << "ClosePosition sy2 maker open short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->ask_p + sy2->prc_tick_size << ", sy2 order qty: " << qty;  
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
    string stType = "Mr_Market_Close";

    sy_info* sy2 = sy.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "Mr_Market_ClosePosition sy2 nullptr: " << sy.sy;
        return;
    }
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

    if (IS_DOUBLE_GREATER(sy.real_pos, 0)) {
        double qty = std::min(sy.real_pos, sy2->ask_v / 2);
        setOrder(sy.inst, INNER_DIRECTION_Sell,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    } else {
        double qty = std::min(sy.real_pos, sy2->bid_v / 2);
        setOrder(sy.inst, INNER_DIRECTION_Buy,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    }
}

// flag 1 arb , 0 fr
//close arb_thresh/fr_thresh 锟斤拷锟斤拷锟斤拷锟斤拷为锟斤拷maker锟斤拷锟斤拷  maker锟斤拷taker 锟斤拷锟斤拷芨叨锟斤拷锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7771锟�1锟�71锟�1锟�771锟�1锟�71锟�1锟�7777(at most larger than taker)锟斤拷锟斤拷maker锟斤拷锟洁，锟斤拷锟斤拷maker锟斤拷taker锟斤拷锟斤拷要锟竭讹拷锟斤拷(at least large than taker)
void StrategyFR::Mr_ClosePosition(StrategyInstrument *strategyInstrument)
{
    sy_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];
    string stType = "FrClose";

    sy_info* sy2 = sy.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "Mr_ClosePosition sy2 nullptr: " << sy.sy;
        return;
    }
    if (sy.make_taker_flag == 1) {
        if ((sy.long_short_flag == 1) && IS_DOUBLE_LESS(sy.real_pos, 0)) {
            if (IsCancelExistOrders(&sy, INNER_DIRECTION_Buy)) return;

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

                double qty = std::min(sy.real_pos, sy2->bid_v / 2);

                setOrder(sy.inst, INNER_DIRECTION_Buy,
                    sy.bid_p - sy.prc_tick_size,
                    qty, order);
                
                LOG_INFO << "Mr_ClosePosition sy maker close short : " << sy.sy << ", sy order side: " << INNER_DIRECTION_Buy
                    << ", sy maker_taker_flag: " << sy.make_taker_flag
                    << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                    << ", sy category: " << sy.type << ", sy order price: "
                    << sy.bid_p - sy.prc_tick_size << ", sy order qty: " << qty;    

        } else if ((sy.long_short_flag == 0) && IS_DOUBLE_GREATER(sy.real_pos, 0)) {
            if (IsCancelExistOrders(&sy, INNER_DIRECTION_Sell)) return;

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

                double qty = std::min(sy.real_pos, sy2->ask_v / 2);

                setOrder(sy.inst, INNER_DIRECTION_Sell,
                    sy.ask_p + sy.prc_tick_size,
                    qty, order);

                LOG_INFO << "Mr_ClosePosition sy maker close long: " << sy.sy << ", sy order side: " << INNER_DIRECTION_Sell
                    << ", sy maker_taker_flag: " << sy.make_taker_flag
                    << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                    << ", sy category: " << sy.type << ", sy order price: "
                    << sy.ask_p + sy.prc_tick_size << ", sy order qty: " << qty;    
            
        }
    } else if (sy2->make_taker_flag == 1) {
        if ((sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0)) {
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Buy)) return;

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

                double qty = std::min(sy2->real_pos, sy.bid_v / 2);

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "Mr_ClosePosition sy2 maker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;   
            
        }  else if ((sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) {
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Sell)) return;

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

                double qty = std::min(sy2->real_pos, sy.ask_v / 2);

                setOrder(sy2->inst, INNER_DIRECTION_Sell,
                    sy2->ask_p + sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "Mr_ClosePosition sy2 maker close long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->ask_p + sy2->prc_tick_size << ", sy2 order qty: " << qty;  

            
        }
    }
}

bool StrategyFR::make_continue_mr(double& mr)
{
    mr = calc_uniMMR();
    if (IS_DOUBLE_GREATER(mr, 9)) {
        return true;
    }
    return false;
}

bool StrategyFR::action_mr(double mr)
{
    if (IS_DOUBLE_GREATER(mr, 3) && IS_DOUBLE_LESS(mr, 6)) {
        for (auto iter : strategyInstrumentList()) {
            Mr_ClosePosition(iter);
        }
        return false;
    } else if (IS_DOUBLE_LESS(mr, 3)) {
        for (auto iter : strategyInstrumentList()) {
            Mr_Market_ClosePosition(iter);
            return false;
        }
    } else if (IS_DOUBLE_GREATER(mr, 9)) {
        return true;
    }
    return false;
} 

void StrategyFR::OnTimerTradingLogic() 
{
    double mr = calc_uniMMR();
    LOG_INFO << "calc mr: " << mr << ", query mr: " << BnApi::accInfo_->uniMMR;
    // action_mr(mr);
    // mr 锟斤拷询锟饺斤拷
    // position 锟饺斤拷
    // 
    
    for (auto it : (*symbol_map)) {
        LOG_INFO << "onTime symbol_map: " << it.first << ", value: " << it.second;
    }

    for (auto iter : strategyInstrumentList()) {
        string sy = iter->instrument()->getInstrumentID();
        double net = iter->position().getNetPosition(); 

        if (sy.find("spot") != string::npos) {
            string asset = GetSPOTSymbol(sy);
            auto it = BnApi::BalMap_.find(asset);
            if (it == BnApi::BalMap_.end()) LOG_FATAL << "qry spot position error: " << asset;
            double equity = it->second.crossMarginFree + it->second.crossMarginLocked - it->second.crossMarginBorrowed - it->second.crossMarginInterest;

            LOG_INFO << "fr onTime spot sy: " << asset
                << ", mem val: " << net << ", qry pos val: " << equity;
        }

        if (sy.find("swap") != string::npos) {
            // string asset = GetUMSymbol(sy);
            bool flag = false;
            for (auto it : BnApi::UmAcc_->info1_) { 
                if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
                if (sy == (*symbol_map)[it.symbol]) {
                    flag = true;
                    LOG_INFO << "fr onTime swap sy: " << it.symbol << ", mem val: " << net
                        << ", qry pos val: " << it.positionAmt;
                    break;
                }
            }
            if (!flag) LOG_FATAL << "onTime um can't find symbol: " << sy;
        }

        if (sy.find("perp") != string::npos) {
            // string asset = GetCMSymbol(sy);
            bool flag = false;
            for (auto it : BnApi::CmAcc_->info1_) { 
                if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
                if (sy == (*symbol_map)[it.symbol]) {
                    flag = true;
                    LOG_INFO << "fr onTime perp sy: " << it.symbol << ", mem val: " << net
                        << ", qry pos val: " << it.positionAmt;
                    break;
                }
            }
            if (!flag) LOG_FATAL << "onTime cm can't find symbol: " << sy;
        }

    }

}

