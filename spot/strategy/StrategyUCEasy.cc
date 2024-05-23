#include "StrategyUCEasy.h"
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

Strategy *StrategyUCEasy::Create(int strategyID, StrategyParameter *params) {
    char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyUCEasy));
    return new(buff) StrategyUCEasy(strategyID, params);
}

StrategyUCEasy::StrategyUCEasy(int strategyID, StrategyParameter *params)
        : StrategyCircuit(strategyID, params) 
{
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
    make_taker = new map<string, uc_info>;
    symbol_map = new map<string, string>;

}

void StrategyUCEasy::qryPosition() {
    for (const auto& iter : strategyInstrumentList()) {
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

            BnApi::BalMap_mutex_.lock();
            auto it = BnApi::BalMap_.find(sy);
            if (it == BnApi::BalMap_.end()) LOG_FATAL << "qry spot position error";
    
            double equity = it->second.crossMarginFree + it->second.crossMarginLocked - it->second.crossMarginBorrowed - it->second.crossMarginInterest;
            LOG_INFO << "FR spot qeuity: " << equity << ", crossMarginFree: " << it->second.crossMarginFree << ", crossMarginLocked: "
                << it->second.crossMarginLocked << ", crossMarginBorrowed: " << it->second.crossMarginBorrowed << ", crossMarginInterest: " << it->second.crossMarginInterest;
            BnApi::BalMap_mutex_.unlock();

            if (IS_DOUBLE_LESS_EQUAL(equity, 0)) {
                iter->position().PublicPnlDaily().TodayShort = -equity;
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


void StrategyUCEasy::init() 
{
    MeasureFunc::addMeasureData(1, "StrategyUCEasy time calc", 10000);
    for (const auto& iter : strategyInstrumentList()) {
        string sy = iter->instrument()->getInstrumentID();
        if (sy.find("swap") != std::string::npos) {
            symbol_map->insert({GetUMSymbol(sy), sy});
        } else if (sy.find("perp") != std::string::npos) {
            symbol_map->insert({GetCMSymbol(sy), sy});
        } else if (sy.find("spot") != std::string::npos) {
            symbol_map->insert({GetSPOTSymbol(sy), sy});
        }
	
		iter->tradeable(true);
    }

    for (const auto& it : InitialData::symbolInfoMap()) {
        uc_info syInfo;
        memcpy(syInfo.sy, it.second.Symbol, min(sizeof(syInfo.sy), sizeof(it.second.Symbol)));
        memcpy(syInfo.ref_sy, it.second.RefSymbol, min(sizeof(syInfo.ref_sy), sizeof(it.second.RefSymbol)));
        memcpy(syInfo.type, it.second.Type, min(sizeof(syInfo.type), sizeof(it.second.Type)));

        syInfo.long_short_flag = it.second.LongShort;
        syInfo.make_taker_flag = it.second.MTaker;
        syInfo.mv_ratio = it.second.MvRatio;
        
        syInfo.close_flag = it.second.CloseFlag;

        if (!IS_DOUBLE_NORMAL(it.second.PreTickSize)) LOG_FATAL << "PreTickSize ERR: " << it.second.PreTickSize;        
        syInfo.prc_tick_size = it.second.PreTickSize;

        if (!IS_DOUBLE_NORMAL(it.second.QtyTickSize)) LOG_FATAL << "QtyTickSize ERR: " << it.second.QtyTickSize;        
        syInfo.qty_tick_size = it.second.QtyTickSize;

        if (!IS_DOUBLE_NORMAL(it.second.MinAmount)) LOG_FATAL << "MinAmount ERR: " << it.second.MinAmount;        
        syInfo.min_amount = it.second.MinAmount;

        if (!IS_DOUBLE_NORMAL(it.second.Fragment)) LOG_FATAL << "Fragment ERR: " << it.second.Fragment;        
        syInfo.fragment = it.second.Fragment;

        if (it.second.UmLeverage == 0) LOG_FATAL << "UmLeverage ERR: " << it.second.UmLeverage;        
        syInfo.um_leverage = it.second.UmLeverage;

        if (!IS_DOUBLE_NORMAL(it.second.PriceRatio)) LOG_FATAL << "PriceRatio ERR: " << it.second.PriceRatio;        
        syInfo.price_ratio = it.second.PriceRatio;

        if (!IS_DOUBLE_NORMAL(it.second.Thresh)) LOG_FATAL << "Thresh ERR: " << it.second.Thresh;        
        syInfo.thresh = it.second.Thresh;
        
        if (!IS_DOUBLE_NORMAL(it.second.OpenThresh)) LOG_FATAL << "step_thresh ERR: " << it.second.step_thresh;        
        syInfo.step_thresh = it.second.OpenThresh;

        if (!IS_DOUBLE_NORMAL(it.second.PosAdj)) LOG_FATAL << "step_thresh ERR: " << it.second.PosAdj;        
        syInfo.pos_adj = it.second.PosAdj;

        make_taker->insert({it.second.Symbol, syInfo});
    }

    for (const auto& iter : strategyInstrumentList()) {
        for (auto& it : (*make_taker)) {
            if (it.first == iter->instrument()->getInstrumentID()) {
                it.second.inst = iter;
                it.second.sellMap = iter->sellOrders();
                it.second.buyMap = iter->buyOrders();
                break;
            }
        }
    }

    qryPosition();

    for (auto& it : (*make_taker)) {
        it.second.ref = &(*make_taker)[it.second.ref_sy];
        it.second.real_pos = it.second.inst->position().getNetPosition();
        LOG_INFO << "make_taker ref: " << it.second.sy << ", ref sy: " << it.second.ref_sy << ", ref: " << it.second.ref
            << ", inst: " << it.second.inst
            << ", real pos: " << it.second.real_pos
            << ", first: " << it.first;
    }

    for (const auto& iter : (*make_taker)) {
        std::string str = iter.first;
        orderForm ord;
        memcpy(ord.symbol, str.c_str(), min(sizeof(ord.symbol), str.size()));
        ord.qty_decimal = ceil(abs(log10(iter.second.qty_tick_size)));
        ord.price_decimal = ceil(abs(log10(iter.second.prc_tick_size)));
        orderFormMap.insert({str, ord});
    }
}


bool StrategyUCEasy::vaildPrice(uc_info& sy) {
    if (!IS_DOUBLE_NORMAL(sy.mid_price) 
        || !IS_DOUBLE_NORMAL(sy.ask_price)
        || !IS_DOUBLE_NORMAL(sy.bid_price)) {
            LOG_ERROR << "vaildPrice mid_price: " << sy.mid_price
                << ", ask_price: " << sy.ask_price
                << ", bid_price: " << sy.bid_price;
            return false;
        }
    return true;

}

bool StrategyUCEasy::IsExistOrders(uc_info* sy, double px, int side)
{
    bool flag = false;
    if (side == INNER_DIRECTION_Buy) {
        if (sy->buyMap->size() != 0) {
            for (const auto& it : (*sy->buyMap)) {
                // if (IS_DOUBLE_EQUAL(it.first, px)) {
                //     LOG_INFO << "buy map has same px symbol: " << it.first << ", px: " << px << ", size: " << sy->buyMap->size();
                //     continue; 
                // }
                for (const auto& iter : it.second->OrderList) {
                    flag = true;
                    if (strcmp(iter.TimeInForce, "GTX") == 0 && strcmp(iter.InstrumentID, sy->sy) == 0) {
                        // LOG_INFO << "buy OrderList info: " << iter.InstrumentID << ", orderRef: " 
                        //     << iter.OrderRef << ", status: " << iter.OrderStatus << ", order list: " << it.second->OrderList.size();
                        if (IS_DOUBLE_EQUAL(iter.LimitPrice, px)) {
                            LOG_INFO << "buy map has same px symbol: " << it.first << ", px: " << px << ", orderRef: " << iter.OrderRef
                            << ", map size: " << sy->buyMap->size()
                                << ", order list: " << it.second->OrderList.size();
                            continue; 
                        }
                        if (!VaildCancelTime(iter, 1)) continue;
                        sy->inst->cancelOrder(iter);
                    }           
                }
            }
        }
    } else if (side == INNER_DIRECTION_Sell) {
        if (sy->sellMap->size() != 0) {
            for (const auto& it : (*sy->sellMap)) {
                // if (IS_DOUBLE_EQUAL(it.first, px)) {
                //     LOG_INFO << "sell map has same px symbol: " << it.first << ", px: " << px << ", size: " << sy->sellMap->size();
                //     continue; 
                // }
                for (const auto& iter : it.second->OrderList) {
                    if (strcmp(iter.TimeInForce, "GTX") == 0 && strcmp(iter.InstrumentID, sy->sy) == 0) {
                        flag = true;
                        // LOG_INFO << "sell OrderList info: " << iter.InstrumentID << ", orderRef: " 
                        //     << iter.OrderRef << ", status: " << iter.OrderStatus << ", order list: " << it.second->OrderList.size();
                        if (IS_DOUBLE_EQUAL(iter.LimitPrice, px)) {
                            LOG_INFO << "sell map has same px symbol: " << it.first << ", px: " << px << ", orderRef: " << iter.OrderRef
                                << ", map size: " << sy->sellMap->size()
                                << ", order list: " << it.second->OrderList.size();
                            continue; 
                        }
                        if (!VaildCancelTime(iter, 2)) continue;
                        sy->inst->cancelOrder(iter);
                    }
                }
            }
        }
    } else {
        LOG_FATAL << "side fatal: " << side;
    }
    return flag;

}

void StrategyUCEasy::Mr_Market_ClosePosition(StrategyInstrument *strategyInstrument)
{
    LOG_INFO << "before market_close_freeze_time:" << market_close_freeze_time << ", curr_mstime_point:" << CURR_MSTIME_POINT;
    if (market_close_freeze_time > CURR_MSTIME_POINT) return;
    market_close_freeze_time = CURR_MSTIME_POINT + 1000;
    LOG_INFO << "after market_close_freeze_time:" << market_close_freeze_time;
    uc_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];
    string stType = "Mr_Market_Close";

    if (abs(sy.real_pos) * sy.mid_p < sy.min_amount) return;
    uc_info* sy2 = sy.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "Mr_Market_ClosePosition sy2 nullptr: " << sy.sy;
        return;
    }
    SetOrderOptions order;
    order.orderType = ORDERTYPE_MARKET; // ?

    if (AssetType_Spot == sy.type) {
        string Category = LEVERAGE;
        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else if (AssetType_FutureSwap == sy.type) {
        string Category = LINEAR;
        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
    } else {
        LOG_FATAL << "Mr_Market_ClosePosition type error: " << sy.type;
    }

    memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

    memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

    if (IS_DOUBLE_GREATER_EQUAL(sy.real_pos, 0) && (sy.long_short_flag == 0)) {
        double qty = std::min(sy.real_pos, sy2->ask_v / 2);
        qty = std::min(qty, sy.fragment/sy.bid_p);
        double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
        qty = round1(qty, sy.qty_tick_size, qty_decimal);

        setOrder(sy.inst, INNER_DIRECTION_Sell,
            sy.bid_p,
            abs(qty), order);
    } else if (IS_DOUBLE_LESS(sy.real_pos, 0) && (sy.long_short_flag == 1)) {
        double qty = std::min(abs(sy.real_pos), sy2->bid_v / 2);
        qty = std::min(qty, sy.fragment/sy.bid_p);
        double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
        qty = round1(qty, sy.qty_tick_size, qty_decimal);

        setOrder(sy.inst, INNER_DIRECTION_Buy,
            sy.ask_p,
            abs(qty), order);
    }
}

// flag 1 arb , 0 fr
//close arb_thresh/fr_thresh   maker taker(at most larger than taker)    (at least large than taker)
void StrategyUCEasy::Mr_ClosePosition(StrategyInstrument *strategyInstrument)
{
    uc_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];
    string stType = "MrClose";

    uc_info* sy2 = sy.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "Mr_ClosePosition sy2 nullptr: " << sy.sy;
        return;
    }
    if (sy.make_taker_flag == 1) {
        if ((sy.long_short_flag == 1) && IS_DOUBLE_LESS(sy.real_pos, 0)) {
            if (IsExistOrders(&sy, sy.bid_p - sy.prc_tick_size, INNER_DIRECTION_Buy)) return;
                // if (getBuyPendingLen(sy) != 0) return;
                if (abs(sy.real_pos) * sy.mid_p < sy.min_amount) return;

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (AssetType_Spot == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (AssetType_FutureSwap == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "Mr_ClosePosition type error: " << sy.type;
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(abs(sy.real_pos), sy2->bid_v / 2);
                qty = std::min (qty, sy.fragment/sy.bid_p);

                double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
                qty = round1(qty, sy.qty_tick_size, qty_decimal);

                setOrder(sy.inst, INNER_DIRECTION_Buy,
                    sy.bid_p - sy.prc_tick_size,
                    qty, order);
                
                LOG_INFO << "Mr_ClosePosition sy maker close short : " << sy.sy << ", sy order side: " << INNER_DIRECTION_Buy
                    << ", sy maker_taker_flag: " << sy.make_taker_flag
                    << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                    << ", sy category: " << sy.type << ", sy order price: "
                    << sy.bid_p - sy.prc_tick_size << ", sy order qty: " << qty;    

        } else if ((sy.long_short_flag == 0) && IS_DOUBLE_GREATER(sy.real_pos, 0)) {
            if (IsExistOrders(&sy, sy.ask_p + sy.prc_tick_size, INNER_DIRECTION_Sell)) return;
                // if (getSellPendingLen(sy) != 0) return;
                if (abs(sy.real_pos) * sy.mid_p < sy.min_amount) return;
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (AssetType_Spot == sy.type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (AssetType_FutureSwap == sy.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "Mr_ClosePosition type error: " << sy.type;
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy.real_pos, sy2->ask_v / 2);
                qty = std::min(qty, sy.fragment/sy.ask_p);
                double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
                qty = round1(qty, sy.qty_tick_size, qty_decimal);

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
            if (IsExistOrders(sy2, sy2->bid_p - sy2->prc_tick_size, INNER_DIRECTION_Buy)) return;
                // if (getBuyPendingLen(*sy2) != 0) return;
                if (abs(sy2->real_pos) * sy2->mid_p < sy2->min_amount) return;
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (AssetType_Spot == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (AssetType_FutureSwap == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "Mr_ClosePosition type error: " << sy2->type;
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(abs(sy2->real_pos), sy.bid_v / 2);
                qty = std::min(qty, sy2->fragment/sy2->bid_p);
                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "Mr_ClosePosition sy2 maker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;   
            
        }  else if ((sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) {
            if (IsExistOrders(sy2, sy2->ask_p + sy2->prc_tick_size, INNER_DIRECTION_Sell)) return;
                // if (getSellPendingLen(*sy2) != 0) return;
                if (abs(sy2->real_pos) * sy2->mid_p < sy2->min_amount) return;
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (AssetType_Spot == sy2->type) {
                    string Category = LEVERAGE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (AssetType_FutureSwap == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "Mr_ClosePosition type error: " << sy2->type;
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                memcpy(order.StType, stType.c_str(), min(sizeof(order.StType) - 1, stType.size()));

                double qty = std::min(sy2->real_pos, sy.ask_v / 2);
                qty = std::min(qty, sy2->fragment/sy2->ask_p);
                double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
                qty = round1(qty, sy2->qty_tick_size, qty_decimal);


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

bool StrategyUCEasy::action_mr(double mr)
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

double StrategyUCEasy::calc_mm()
{
    double sum_mm = 0;
    // �ֻ��ܸ�mm
    BnApi::BalMap_mutex_.lock();
    for (const auto& it : BnApi::BalMap_) {
        double price = getSpotAssetSymbol(it.second.asset);
        if (!IS_DOUBLE_NORMAL(price)) continue;

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
        LOG_INFO << "calc_mm asset:" << it.second.asset << ", crossMarginBorrowed:" << it.second.crossMarginBorrowed;
    }
    BnApi::BalMap_mutex_.unlock();

    BnApi::UmAcc_mutex_.lock();
    for (const auto& it : BnApi::UmAcc_->info1_) {
        if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
        if (IS_DOUBLE_LESS_EQUAL((*make_taker)[(*symbol_map)[it.symbol]].mid_p , 0)) {
            LOG_ERROR << "calc_mm UmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[it.symbol]].mid_p
                << ", second: " << (*symbol_map)[it.symbol];
            return -1;
        }

        string symbol = it.symbol;
        double qty = abs(it.positionAmt);
        double markPrice = (*make_taker)[(*symbol_map)[symbol]].mid_p;
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty) * markPrice, mmr_rate, mmr_num);
        double mm = abs(qty) * markPrice * mmr_rate - mmr_num;
        LOG_INFO << "calc_mm um:" << symbol << ", markPrice:" << markPrice << ", mmr_rate:" << mmr_rate << ", mmr_num:" << mmr_num << ", mm:" << mm;
        sum_mm += mm;
    }
    BnApi::UmAcc_mutex_.unlock();

    BnApi::CmAcc_mutex_.lock();
    for (const auto& it : BnApi::CmAcc_->info1_) {
        if (symbol_map->find(it.symbol) == symbol_map->end()) continue;
        if (IS_DOUBLE_LESS_EQUAL((*make_taker)[(*symbol_map)[it.symbol]].mid_p , 0)) {
            LOG_ERROR << "calc_mm CmAcc asset has no mkprice: " << it.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[it.symbol]].mid_p;
            return -1;
        }
        string symbol = it.symbol;
        double qty = 0;

        if (symbol == "BTCUSD_PERP") {
            qty = abs(it.positionAmt) * 100;
        } else {
            qty = abs(it.positionAmt) * 10;
        }

        double markPrice = (*make_taker)[(*symbol_map)[symbol]].mid_p;
        double mmr_rate;
        double mmr_num;
        get_cm_um_brackets(symbol, abs(qty), mmr_rate, mmr_num);
        double mm = (abs(qty) * mmr_rate - mmr_num) * markPrice;
        sum_mm += mm;
    }
    BnApi::CmAcc_mutex_.unlock();
    return sum_mm;
}

double StrategyUCEasy::calc_equity()
{
    double sum_equity = 0;
    BnApi::BalMap_mutex_.lock();
    for (const auto& it : BnApi::BalMap_) {
        double price = getSpotAssetSymbol(it.second.asset);
        if (IS_DOUBLE_LESS_EQUAL(price, 0)) continue;

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
    BnApi::BalMap_mutex_.unlock();
    return sum_equity;
}

double StrategyUCEasy::calc_uniMMR()
{
    double uniAccount_equity = calc_equity();
    double uniAccount_mm = calc_mm();
    LOG_INFO << "uniAccount_equity:" << uniAccount_equity << ", uniAccount_mm:" << uniAccount_mm;
    if (uniAccount_mm == 0) return 999;
    if (uniAccount_mm == -1) return 8;
    return (uniAccount_equity)/(uniAccount_mm);
}

bool StrategyUCEasy::make_continue_mr(double& mr)
{
    mr = calc_uniMMR();
    LOG_INFO << "MR_VAL: " << mr;
    if (IS_DOUBLE_GREATER(mr, 9)) {
        return true;
    }
    return false;
}

double StrategyUCEasy::calc_predict_mm(uc_info& info, order_uc& order)
{
    double sum_mm = 0;
    double usdt_equity = get_usdt_equity();

    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_mm has no mkprice: " << order.sy << ", markprice: " << info.mid_p;
        return 0;
    }
    double leverage = 0;
    string symbol = GetSPOTSymbol(order.sy);
    if (margin_leverage->find(order.sy) == margin_leverage->end()) {
        leverage = (*margin_leverage)["default"];
    } else {
        leverage = (*margin_leverage)[symbol];
    }

    if ((AssetType_Spot == info.type && info.long_short_flag == 0) || (AssetType_FutureSwap == info.type && info.long_short_flag == 1)) { // 
        if (IS_DOUBLE_GREATER(usdt_equity, 0)) {
            usdt_equity = usdt_equity - order.borrow;
            if (IS_DOUBLE_LESS(usdt_equity, 0)) 
                sum_mm = sum_mm + abs(usdt_equity) * (*margin_mmr)[leverage];
        } else {
            sum_mm = sum_mm + order.borrow * (*margin_mmr)[leverage];
        }
    } else { // 
        sum_mm = sum_mm + order.borrow * price * (1 + info.price_ratio) * (*margin_mmr)[leverage];
    }

    // LOG_INFO << "calc_predict_mm sy: " << order.sy << ", price: " << info.mid_p << ", mmr: " << (*margin_mmr)[leverage] << ", sum: " << sum_mm;

    BnApi::BalMap_mutex_.lock();
    for (const auto& it : BnApi::BalMap_) {
        if (symbol_map->find(it.first) == symbol_map->end()) continue;
        double leverage = (*margin_mmr)[(*margin_leverage)[it.first]];
        double price = getSpotAssetSymbol(it.first);
        if (!IS_DOUBLE_NORMAL(price)) continue;

        string sy = it.first;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            sum_mm = sum_mm + it.second.crossMarginBorrowed * (*margin_mmr)[leverage] * 1; // 
        } else {
            sum_mm = sum_mm + it.second.crossMarginBorrowed * (*margin_mmr)[leverage] * price * (1 + info.price_ratio);
        }
    }
    BnApi::BalMap_mutex_.unlock();

    bool firstFlag = true;
    BnApi::UmAcc_mutex_.lock();
    for (const auto& iter : BnApi::UmAcc_->info1_) {
        string sy = iter.symbol;
        auto sy_it = symbol_map->find(sy);
        if (sy_it == symbol_map->end()) continue;
        
        double price = (*make_taker)[(*symbol_map)[sy]].mid_p * (1 + (*make_taker)[(*symbol_map)[sy]].price_ratio);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc calc_predict_mm mkprice: " << sy << ", markprice: " << (*make_taker)[(*symbol_map)[sy]].mid_p;
            continue;
        }
        double qty = abs(iter.positionAmt); // positionAmt positive or negitive

        if (sy_it->second == order.sy || sy_it->second == order.ref_sy) {
            qty = qty + order.qty;  // forever +, cause we assume the direction of all orders are the same
            firstFlag = false;
        }

        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(iter.symbol, abs(qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + abs(qty) * price * mmr_rate -  mmr_num;
        LOG_INFO << "calc_predict_mm swap sy: " << order.sy << ", price: " << (*make_taker)[order.sy].mid_p << ", mmr: " << mmr_rate << ", mmr_num: " << mmr_num
            << ", iter.positionAmt: " << iter.positionAmt << ", sy_it->second: " << sy_it->second  << ", iter.symbol: " << iter.symbol << ", ori qty: " << abs(iter.positionAmt)
            << ", qty: " << qty << ",sum_mm: " << sum_mm;

    }

    if (firstFlag && (order.sy.find("swap") != string::npos)) {
        double price = (*make_taker)[order.sy].mid_p * (1 + (*make_taker)[order.sy].price_ratio);
        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(order.sy, abs(order.qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + abs(order.qty) * price * mmr_rate -  mmr_num;
        LOG_INFO << "calc_predict_mm sy first: " << order.sy << ", mid_p: " << (*make_taker)[order.sy].mid_p << ", mmr: " << mmr_rate << ", mmr_num: " << mmr_num
            << ", price: " << price << ", price_ratio: " << (*make_taker)[order.sy].price_ratio;
    } else if (firstFlag && (order.ref_sy.find("swap") != string::npos)) {
        double price = (*make_taker)[order.ref_sy].mid_p * (1 + (*make_taker)[order.ref_sy].price_ratio);
        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(order.ref_sy, abs(order.qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + abs(order.qty) * price * mmr_rate -  mmr_num;   
        LOG_INFO << "calc_predict_mm sy second: " << order.sy << ", mid_p: " << (*make_taker)[order.sy].mid_p << ", mmr: " << mmr_rate << ", mmr_num: " << mmr_num
            << ", price: " << price << ", price_ratio: " << (*make_taker)[order.sy].price_ratio;
    }
    BnApi::UmAcc_mutex_.unlock();

    bool firstFlag_perp = true;
    BnApi::CmAcc_mutex_.lock();
    for (const auto& iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        auto sy_it = symbol_map->find(sy);
        if (sy_it == symbol_map->end()) continue;

        firstFlag_perp = false;
        
        double price = (*make_taker)[(*symbol_map)[sy]].mid_p * (1 + (*make_taker)[(*symbol_map)[sy]].price_ratio);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc calc_predict_mm mkprice: " << sy << ", markprice: " << price;
            continue;
        }
        double qty = 0;
        if (sy == "BTCUSD_PERP") {
            qty = abs(iter.positionAmt) * 100 / price;
        } else {
            qty = abs(iter.positionAmt) * 10 / price;
        }
        if (sy_it->second == order.sy || sy_it->second == order.ref_sy) {
            qty = qty + order.qty;  // forever +, cause we assume the direction of all orders are the same
            firstFlag_perp = false;
        }

        double mmr_rate = 0;
        double mmr_num = 0;
        qty = abs(iter.positionAmt);
        get_cm_um_brackets(iter.symbol, abs(qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + (abs(qty) * mmr_rate -  mmr_num) * price;
        LOG_INFO << "calc_predict_mm perp sy: " << order.sy << ", price: " << (*make_taker)[order.sy].mid_p << ", mmr: " << mmr_rate << ", mmr_num: " << mmr_num
            << ", iter.positionAmt: " << iter.positionAmt << ", sy_it->second: " << sy_it->second  << ", order.sy: " << order.sy << ", ori qty: " << abs(iter.positionAmt)
            << ", qty: " << qty << ",sum_mm: " << sum_mm;
    }

    if (firstFlag_perp && (order.sy.find("perp") != string::npos)) {
        double price = (*make_taker)[order.sy].mid_p * (1 + (*make_taker)[order.sy].price_ratio);
        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(order.sy, abs(order.qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + (abs(order.qty) * mmr_rate -  mmr_num) * price;
    } else if (firstFlag_perp && (order.ref_sy.find("perp") != string::npos)) {
        double price = (*make_taker)[order.ref_sy].mid_p * (1 + (*make_taker)[order.ref_sy].price_ratio);
        double mmr_rate = 0;
        double mmr_num = 0;
        get_cm_um_brackets(order.ref_sy, abs(order.qty) * price, mmr_rate, mmr_num);
        sum_mm = sum_mm + (abs(order.qty) * mmr_rate -  mmr_num) * price;
    }
    BnApi::CmAcc_mutex_.unlock();
    
    return sum_mm;

}

double StrategyUCEasy::calc_predict_equity(uc_info& info, order_uc& order)
{
    double sum_equity = 0;
    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_predict_equity has no mkprice: " << order.sy << ", markprice: " << info.mid_p;
        return 0;
    }

    double rate = collateralRateMap[GetSPOTSymbol(order.sy)];

    if ((AssetType_Spot == info.type && info.long_short_flag == 0) || (AssetType_FutureSwap == info.type && info.long_short_flag == 1)) { 
        double equity = order.qty * price * (1 + info.price_ratio) * rate;
        double uswap_unpnl = order.qty * price - (1 + info.price_ratio) * price * order.qty;
        sum_equity += equity - order.borrow + uswap_unpnl;
    } else { 
        double qty = (order.qty);
        double equity = qty * price - order.borrow * (1 + info.price_ratio) * price;
        double uswap_unpnl = order.qty * price * (1 + info.price_ratio) - qty * price;
        sum_equity = equity + uswap_unpnl;
    }

    BnApi::BalMap_mutex_.lock();
    for (const auto& it : BnApi::BalMap_) {
        string str(it.first);
        if (str != "USDT" && str != "USDC" && str != "BUSD" && symbol_map->find(it.first) == symbol_map->end()) continue;
        double rate = collateralRateMap[it.first];

        string sy = it.first;
        double price = 0;
        if (sy == "USDT" || sy == "USDC" || sy == "BUSD") {
            price = getSpotAssetSymbol(sy);
        } else {
            price = getSpotAssetSymbol(sy) * (1 + (*make_taker)[(*symbol_map)[sy]].price_ratio);
        }

        if (!IS_DOUBLE_NORMAL(price)) {
            LOG_WARN << "BalMap calc_predict_equity mkprice: " << sy << ", markprice: " << getSpotAssetSymbol(sy);
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
    BnApi::BalMap_mutex_.unlock();

    BnApi::UmAcc_mutex_.lock();
    for (const auto& iter : BnApi::UmAcc_->info1_) {
        if (symbol_map->find(iter.symbol) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p * (1 + (*make_taker)[(*symbol_map)[iter.symbol]].price_ratio);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "UmAcc mkprice: " << iter.symbol << ", markprice: " << (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
            continue;
        }
        double avgPrice = (iter.entryPrice * abs(iter.positionAmt) + order.qty * (*make_taker)[(*symbol_map)[iter.symbol]].mid_p) / (abs(iter.positionAmt) + order.qty);
        double uswap_unpnl = (price - avgPrice) * abs(iter.positionAmt);
        sum_equity += uswap_unpnl;
    }
    BnApi::UmAcc_mutex_.unlock();

    BnApi::CmAcc_mutex_.lock();
    for (const auto& iter : BnApi::CmAcc_->info1_) {
        string sy = iter.symbol;
        if (symbol_map->find(sy) == symbol_map->end()) continue;
        double price = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p * (1 + (*make_taker)[(*symbol_map)[iter.symbol]].price_ratio);
        if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
            LOG_WARN << "CmAcc mkprice: " << sy << ", markprice: " << (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
            continue;
        }
        double perp_size = 0;
        if (sy == "BTCUSD_PERP") {
            perp_size = 100;
        } else {
            perp_size = 10;
        }

        double quantity = perp_size * abs(iter.positionAmt) / iter.entryPrice + order.qty * perp_size / (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;

        double turnover = perp_size * abs(iter.positionAmt) + order.qty * perp_size;

        double avgPrice = turnover / quantity;
        if (IS_DOUBLE_LESS_EQUAL(avgPrice, 0)) {
            avgPrice = (*make_taker)[(*symbol_map)[iter.symbol]].mid_p;
        }

        double cswap_unpnl = price * perp_size * abs(iter.positionAmt) * (1/avgPrice - 1/price);
        sum_equity += cswap_unpnl;
    }
    BnApi::CmAcc_mutex_.unlock();
    return sum_equity;
}

double StrategyUCEasy::calc_future_uniMMR(uc_info& info, double qty)
{
    double sum_equity = calc_equity();
    double IM = 0;

    order_uc order;
    order.sy = info.sy;
    order.qty = qty;
    order.ref_sy = info.ref_sy;

    double price = info.mid_p;
    if (IS_DOUBLE_LESS_EQUAL(price , 0)) {
        LOG_WARN << "calc_future_umimmr has no mkprice: " << info.sy << ", markprice: " << info.mid_p;
        return 0;
    }
    double borrow = 0;
    string symbol = GetSPOTSymbol(info.sy);
    if ((AssetType_Spot == info.type && info.long_short_flag == 0) || (AssetType_FutureSwap == info.type && info.long_short_flag == 1)) { // 
        borrow = qty * price;
        IM = IM + borrow / ((*margin_leverage)[symbol] - 1) + (qty * price / info.um_leverage); // the spot and swap need has the same um_leverage val      
    } else { 
        borrow = qty;
        IM = IM + (price * (qty) / ((*margin_leverage)[symbol] - 1)) + (qty) * price / info.um_leverage;
    }

    order.borrow = borrow;

    if (IS_DOUBLE_GREATER(IM, sum_equity)) {
        LOG_INFO << "IM: " << IM << ", sum_equity: " << sum_equity;
        return 0;
    }

    double predict_equity = calc_predict_equity(info, order);
    double predict_mm = calc_predict_mm(info, order);
    double predict_mmr = predict_equity / predict_mm;
    LOG_DEBUG << "calc_future_uniMMR: " << predict_mmr 
        // << ", calc mr: " << calc_uniMMR()
        << ", predict_equity: " << predict_equity << ", predict_mm: " << predict_mm << ", query mr: " << BnApi::accInfo_->uniMMR
        << ", IM: " << IM << ", qty: " << qty;

    return predict_mmr;

}


bool StrategyUCEasy::is_continue_mr(uc_info* info, double qty)
{
    double mr = calc_future_uniMMR(*info, qty);
    if (IS_DOUBLE_GREATER(mr, 2)) {
        return true;
    }
    return false;
}

void StrategyUCEasy::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    MeasureFunc f(1);
    int64_t ts = CURR_MSTIME_POINT;

    if (ts - marketData.EpochTime > 300) {
        LOG_WARN << "market data beyond time: " << marketData.InstrumentID << ", EpochTime: " << marketData.EpochTime
            << ", ts: " << ts << ", gap: " << marketData.EpochTime - ts;
        return;
    }
    if (make_taker->find(marketData.InstrumentID) == make_taker->end()) {
        LOG_FATAL << "make take find sy err: " << marketData.InstrumentID;
        return;
    }

    uc_info& sy1 = (*make_taker)[marketData.InstrumentID];
    sy1.update(marketData.AskPrice1, marketData.BidPrice1, marketData.AskVolume1, marketData.BidVolume1, marketData.UpdateMillisec);
    uc_info* sy2 = sy1.ref;

    if (!vaildAllSymboPrice(30000)) return;

    LOG_INFO << "symbol1: " << sy1.sy << ", sy1 close_flag: " << sy1.close_flag << ", sy1 maker_taker_flag: " << sy1.make_taker_flag << ", sy1 long_short_flag: " << sy1.long_short_flag
         << ", sy1 real_pos: " << sy1.real_pos << ", sy1 mid_p: " << sy1.mid_p;
    LOG_INFO << "symbol2: " << sy2->sy << ", sy2 close_flag: " << sy2->close_flag << ", sy2 maker_taker_flag: " << sy2->make_taker_flag << ", sy2 long_short_flag: " << sy2->long_short_flag
         << ", sy2 real_pos: " << sy2->real_pos << ", sy2 mid_p: " << sy2->mid_p;

    double delta_posi = 0;

    if (sy1.make_taker_flag) {
        delta_posi = sy1.real_pos * sy1.mid_p + sy2->real_pos * sy2->multiple;
        double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;
        LOG_INFO << "symbol1: " << sy1.sy << ", spread_rate: " << spread_rate << "(" << sy1.mid_p << " - " << sy2->mid_p << ")" << "/" << sy2->mid_p << ", sy1 fr_open_thresh: " << sy1.fr_open_thresh;
    } 
    if (sy2->make_taker_flag ) {
        delta_posi = sy1.real_pos * sy1.multiple + sy2->real_pos * sy2->mid_p;
         double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;
         LOG_INFO << "symbol2: " << sy2->sy << ", spread_rate: " << spread_rate << "(" << sy2->mid_p << " - " << sy1.mid_p << ")" << "/" << sy1.mid_p << ", sy1 buy_thresh: " << sy1.buy_thresh << ", sy1 sell_thresh: " << sy1.sell_thresh;
    }

    double mr = 0;
    if (!make_continue_mr(mr)) {
        action_mr(mr);
        return;
    }

    if (sy2 == nullptr) {
        LOG_ERROR << "OnRtnInnerMarketDataTradingLogic sy2 nullptr: " << sy1.sy;
        return;
    }
    double bal = calc_balance();
    
    if (IS_DOUBLE_GREATER(abs(delta_posi), 3 * sy1.fragment)) {
        LOG_FATAL <<  "more than 3 * fragment " << sy1.sy << ", delta_posi: " << delta_posi;
        return;
    }


    if (sy1.make_taker_flag == 1) { //sy1 maker
        double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;

        if (IS_DOUBLE_GREATER(spread_rate, sy1.sell_thresh)) {
            if (IsExistOrders(&sy1, marketData.AskPrice1 + sy1.prc_tick_size, INNER_DIRECTION_Sell)) return;
            if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal)) {
                LOG_WARN << "MarketDataTradingLogic sy1 real_pos: " << sy1.real_pos << ", sy1 mid_p: " << sy1.mid_p
                    << ", mv_ratio: " << sy1.mv_ratio << ", bal: " << bal;
                return;
            }

            double u_posi = abs(sy1.real_pos) * sy1.mid_p;
            double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->ask_v / 2);
            qty = min (qty, sy1.fragment/sy1.mid_p);

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            qty = round1(qty, sy1.qty_tick_size, qty_decimal);

            if (IS_DOUBLE_LESS(qty * sy1.mid_p, sy1.min_amount)) {
                LOG_WARN << "MarketDataTradingLogic sy1 min_amount: " << sy1.min_amount << ", sy1 mid_p: " << sy1.mid_p
                    << ", qty: " << qty;
                return;
            }

            if (!is_continue_mr(&sy1, qty)) {
                return;
            }
            //  qty = sy1.min_amount;

            SetOrderOptions order;
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "GTX";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

            if (AssetType_Spot == sy1.type) {
                string Category = LEVERAGE;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else if (AssetType_FutureSwap == sy1.type) {
                string Category = LINEAR;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else {
                LOG_FATAL << "MarketDataTradingLogic type error: " << sy1.type;
            }

            memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Sell,
                marketData.AskPrice1 + sy1.prc_tick_size,
                qty, order);

            LOG_INFO << "MarketDataTradingLogic sy1 maker open sell: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << marketData.AskPrice1 + sy1.prc_tick_size << ", sy1 order qty: " << qty
                << ", bal: " << bal << ", spread_rate: " << spread_rate;    
        
        } else if (IS_DOUBLE_LESS(spread_rate, sy1.buy_thresh)) {
            if (IsExistOrders(&sy1, marketData.BidPrice1 - sy1.prc_tick_size, INNER_DIRECTION_Buy)) return;
            if (IS_DOUBLE_GREATER(abs(sy1.real_pos) * sy1.mid_p, sy1.mv_ratio * bal)) {
                LOG_WARN << "MarketDataTradingLogic sy1 real_pos: " << sy1.real_pos << ", sy1 mid_p: " << sy1.mid_p
                    << ", mv_ratio: " << sy1.mv_ratio << ", bal: " << bal;
                return;
            }

            double u_posi = abs(sy1.real_pos) * sy1.mid_p;
            double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->bid_v / 2);
            qty = min (qty, sy1.fragment/sy1.mid_p);

            double qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
            qty = round1(qty, sy1.qty_tick_size, qty_decimal);

            if (IS_DOUBLE_LESS(qty * sy1.mid_p, sy1.min_amount)) {
                LOG_WARN << "MarketDataTradingLogic sy1 min_amount: " << sy1.min_amount << ", sy1 mid_p: " << sy1.mid_p
                    << ", qty: " << qty;
                return;
            }

            if (!is_continue_mr(&sy1, qty)) {
                return;
            }

            SetOrderOptions order;
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "GTX";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

            if (AssetType_Spot == sy1.type) {
                string Category = LEVERAGE;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else if (AssetType_FutureSwap == sy1.type) {
                string Category = LINEAR;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else {
                LOG_FATAL << "MarketDataTradingLogic type error: " << sy1.type;
            }

            memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

            setOrder(sy1.inst, INNER_DIRECTION_Buy,
                marketData.BidPrice1 - sy1.prc_tick_size,
                qty, order);

            LOG_INFO << "MarketDataTradingLogic sy1 maker open long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Buy
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << marketData.BidPrice1 - sy1.prc_tick_size << ", sy1 order qty: " << qty
                << ", bal: " << bal << ", spread_rate: " << spread_rate;  
        }
    } else if (sy2->make_taker_flag == 1) { //sy2 maker
        double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;

        if (IS_DOUBLE_LESS(spread_rate, sy2->buy_sell)) {
            if (IsExistOrders(sy2, sy2->bid_p - sy2->prc_tick_size, INNER_DIRECTION_Buy)) return;
            if (IS_DOUBLE_GREATER(abs(sy2->real_pos) * sy2->mid_p, sy2->mv_ratio * bal)) {
                LOG_WARN << "MarketDataTradingLogic sy2 make symbol: " << sy2->sy
                    << ", make mid px: " << sy2->mid_p << ", mv_ratio: " << sy2->mv_ratio
                    <<", bal: " << bal << ", make real_pos: " << sy2->real_pos;
                return;
            }

            SetOrderOptions order;
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "GTX";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

            if (AssetType_Spot == sy2->type) {
                string Category = LEVERAGE;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else if (AssetType_FutureSwap == sy2->type) {
                string Category = LINEAR;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else {
                LOG_FATAL << "MarketDataTradingLogic type error: " << sy2->type;
            }

            memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

            double u_posi = abs(sy2->real_pos) * sy2->mid_p;
            double qty = min((bal * sy2->mid_p - u_posi) / sy2->mid_p, sy1.bid_v / 2);
            qty = min (qty, sy2->fragment/sy2->mid_p);

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            qty = round1(qty, sy2->qty_tick_size, qty_decimal);

            if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) {
                LOG_WARN << "MarketDataTradingLogic sy2 min_amount: " << sy2->min_amount << ", sy2 mid_p: " << sy2->mid_p
                    << ", qty: " << qty;
                return;
            }
            if (!is_continue_mr(sy2, qty)) {
                return;
            }

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                sy2->bid_p - sy2->prc_tick_size,
                qty, order);

            LOG_INFO << "ClosePosition sy2 maker open long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty
                << ", bal: " << bal << ", spread_rate: " << spread_rate; 

        } else if (IS_DOUBLE_GREATER(spread_rate, sy2->sell_thrsh)) {
            if (IsExistOrders(sy2, sy2->ask_p + sy2->prc_tick_size, INNER_DIRECTION_Sell)) return;
            if (IS_DOUBLE_GREATER(abs(sy2->real_pos) * sy2->mid_p, sy2->mv_ratio * bal)) {
                LOG_WARN << "MarketDataTradingLogic sy2 make symbol: " << sy2->sy
                    << ", make mid px: " << sy2->mid_p << ", mv_ratio: " << sy2->mv_ratio
                    <<", bal: " << bal << ", make real_pos: " << sy2->real_pos;
                return;
            }

            SetOrderOptions order;
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "GTX";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

            if (AssetType_Spot == sy2->type) {
                string Category = LEVERAGE;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else if (AssetType_FutureSwap == sy2->type) {
                string Category = LINEAR;
                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            } else {
                LOG_FATAL << "MarketDataTradingLogic type error: " << sy2->type;
            }

            memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

            double u_posi = abs(sy2->real_pos) * sy2->mid_p;
            double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy1.ask_v / 2);
            qty = min (qty, sy2->fragment/sy2->mid_p);

            double qty_decimal = ceil(abs(log10(sy2->qty_tick_size)));
            qty = round1(qty, sy2->qty_tick_size, qty_decimal);

            if (IS_DOUBLE_LESS(qty * sy2->mid_p, sy2->min_amount)) {
                LOG_WARN << "MarketDataTradingLogic sy2 min_amount: " << sy2->min_amount << ", sy2 mid_p: " << sy2->mid_p
                    << ", qty: " << qty;
                return;
            }
            if (!is_continue_mr(sy2, qty)) {
                return;
            }

            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                sy2->ask_p + sy2->prc_tick_size,
                qty, order);
            
            LOG_INFO << "ClosePosition sy2 maker open short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->ask_p + sy2->prc_tick_size << ", sy2 order qty: " << qty
                << ", bal: " << bal << ", spread_rate: " << spread_rate; 
        }
    }
    return;
}

bool StrategyUCEasy::check_min_delta_limit(uc_info& sy1, uc_info& sy2) 
{
    sy1.real_pos = sy1.inst->position().getNetPosition();
    sy2.real_pos = sy2.inst->position().getNetPosition();
    sy2.avg_price = sy2.strategyInstrument->position().PublicPnlDaily().EntryPrice;
    sy1.avg_price = sy1.strategyInstrument->position().PublicPnlDaily().EntryPrice;

    double delta_pos_notional = 0;

    if (sy1.make_taker_flag == 1) delta_pos_notional = sy1.real_pos * sy1.avg_price + sy2.real_pos * sy2.multiple;
    else delta_pos_notional = sy1.real_pos * sy1.multiple + sy2.real_pos * sy2.avg_price;

    if (IS_DOUBLE_LESS(delta_pos_notional, sy1.min_amount)) {
        return false;
        LOG_INFO << "check_min_delta_limit sy1 real_pos: " << sy1.real_pos << ", sy2 real_pos: " << sy2.real_pos;
    }

    return true;
}

int StrategyUCEasy::getIocOrdPendingLen(uc_info& sy) {
    int pendNum = 0;
    for (auto it : (*sy.inst->sellOrders())) {
        for (auto iter : it.second->OrderList) {
            LOG_DEBUG << "getIocOrdPendingLen SELL  symbol: " << iter.InstrumentID
                << ", OrderStatus: " << iter.OrderStatus
                << ", MTaker: " << iter.MTaker
                << ", orderRef: " << iter.OrderRef;
            if (iter.OrderStatus == PendingNew && (iter.OrderType == ORDERTYPE_MARKET)) {
                pendNum++;
            }
                
        }
    }

    for (auto it : (*sy.inst->buyOrders())) {
        for (auto iter : it.second->OrderList) {
            LOG_DEBUG << "getIocOrdPendingLen BUY  symbol: " << iter.InstrumentID
                << ", OrderStatus: " << iter.OrderStatus
                << ", MTaker: " << iter.MTaker
                << ", orderRef: " << iter.OrderRef;
            if (iter.OrderStatus == PendingNew && (iter.OrderType == ORDERTYPE_MARKET))
                {
                    pendNum++;
                }
                
        }
    }            
    if (pendNum > 0)
        LOG_INFO << "HEGET getIocOrdPendingLen sell size: "<< sy.inst->sellOrders()->size() 
        << ", buy size: " << sy.inst->buyOrders()->size() << ",pendNum: " << pendNum;
    return pendNum;
}


void StrategyUCEasy::hedge(StrategyInstrument *strategyInstrument)
{
    string symbol = strategyInstrument->getInstrumentID();
    if (make_taker->find(symbol) == make_taker->end()) return;
    uc_info& sy1 = (*make_taker)[symbol];
    uc_info* sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_ERROR << "hedge sy2 nullptr: " << symbol;
        return;
    }
    double delta_posi = 0;
    if (sy1.make_taker_flag == 0) {
        delta_posi = sy1.real_pos * sy1.multiple + sy2->real_pos * sy2->mid_p;
        if (IS_DOUBLE_LESS(abs(delta_posi), sy1.min_amount)) return;
    } else if(sy2->make_taker_flag == 0) {
        delta_posi = sy1.real_pos * sy1.mid_p + sy2->real_pos * sy2->multiple;
        if (IS_DOUBLE_LESS(abs(delta_posi), sy2->min_amount)) return;
    }
    
    if (IS_DOUBLE_GREATER(abs(delta_posi), 3 * sy1.fragment)) {
        LOG_FATAL <<  "more than 3 * fragment " << symbol << ", delta_posi: " << delta_posi;
        return;
    }

    double taker_qty = int(abs(delta_posi)/ sy1.multiple);
    taker_qty = max(sy1.min_qty, min(taker_qty, sy1.max_qty));

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER(delta_posi, 0)) {
        // sy1 maker open_short  sy1.pos < 0 delta_pos > 0 sy2.close_long ???
        if ((sy1.make_taker_flag == 1)) {
            if (getIocOrdPendingLen(*sy2) != 0) 
                return; 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (AssetType_FutureSwap == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (AssetType_Spot == sy2->type)
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
        } else if ((sy2->make_taker_flag == 1)) { 
            if (getIocOrdPendingLen(sy1) != 0)
                return; 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (AssetType_FutureSwap == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (AssetType_Spot == sy1.type)
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
    } else if (IS_DOUBLE_LESS(delta_posi, 0)) {
        // sy1 maker open_short sy1.pos<0 delta_pos<0 sy2 open_long ?�?�
        if ((sy1.make_taker_flag == 1)) {
            if (getIocOrdPendingLen(*sy2) != 0)
                return; 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (AssetType_FutureSwap == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (AssetType_Spot == sy2->type)
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
        } else if ((sy2->make_taker_flag == 1)) { 
            if (getIocOrdPendingLen(sy1) != 0)
                return; 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (AssetType_FutureSwap == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (AssetType_Spot == sy1.type)
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
        //sy2 maker open_long sy2.pos>0 delta_pos<0 sy1 close_short ???
        }
    }
    }
}

void StrategyUCEasy::update_thresh(StrategyInstrument *strategyInstrument) 
{
    string symbol = strategyInstrument->getInstrumentID();
    uc_info* sy = &(*make_taker)[symbol];
    if (sy->make_taker_flag == 0) {
        sy = sy->ref;
    }
    
    double pos_adj = sy->real_pos / sy->pos_adj;

    double buy_thresh = - min(max(sy->thresh + pos_adj, thresh_min), thresh_max);
    double sell_thresh = min(max(sy->thresh - pos_adj, thresh_min), thresh_max);

    sy->buy_thresh = buy_thresh + sy->step_thresh;
    sy->sell_thresh = sell_thresh + sy->step_thresh;

    LOG_INFO << "update_thresh maker_threshold: " << sy->thresh 
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: "  << sy2.real_pos
        << ", pos_adj: " << pos_adj
        << ", step_thresh: " << sy->step_thresh
        << ", buy_thresh: " << buy_thresh
        << ", sell_thresh: " << sell_thresh;
}

void StrategyUCEasy::OnForceCloseTimerInterval() 
{
    for (auto iter : strategyInstrumentList()) {
        hedge(iter);
        update_thresh(iter);
    }
}

void StrategyUCEasy::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_FATAL << "OnPartiallyFilledTradingLogic sy2 nullptr: " << rtnOrder.InstrumentID;
    }
    update_thresh(strategyInstrument);
    if (!check_min_delta_limit(sy1, (*sy2))) return;
    hedge(strategyInstrument);

    LOG_INFO << "OnPartiallyFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
    return;
}

void StrategyUCEasy::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    auto& sy1 = (*make_taker)[strategyInstrument->getInstrumentID()];
    auto sy2 = sy1.ref;
    if (sy2 == nullptr) {
        LOG_FATAL << "OnFilledTradingLogic sy2 nullptr: " << rtnOrder.InstrumentID;
    }

    update_thresh(strategyInstrument);

    if (!check_min_delta_limit(sy1, (*sy2))) return;

    hedge(strategyInstrument);

    LOG_INFO << "OnFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", symbol: " << rtnOrder.InstrumentID
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
    return;
}

void StrategyUCEasy::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    hedge(strategyInstrument);
}
