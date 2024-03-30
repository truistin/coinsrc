#include "StrategyUCEasy.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Measure.h"
#include "spot/base/InitialData.h"
#include "spot/cache/CacheAllocator.h"
#include "spot/utility/TradingDay.h"

using namespace spot::strategy;
// using namespace spot::risk;

Strategy *StrategyUCEasy::Create(int strategyID, StrategyParameter *params) {
    char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyUCEasy));
    return new(buff) StrategyUCEasy(strategyID, params);
}

StrategyUCEasy::StrategyUCEasy(int strategyID, StrategyParameter *params)
        : StrategyCircuit(strategyID, params) {
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

	thresh_min = -0.2;
	thresh_max = 0.2;
}

void StrategyUCEasy::qryPosition() {
    for (auto iter : strategyInstrumentList()) {
        if (iter->instrument()->getInstrumentID() == sy1.symbol) {
            sy1.multiple = iter->instrument()->getMultiplier();
            LOG_INFO << "get sy1 symbol: " << sy1.symbol << ", multiple: " << sy1.multiple;

        } else if (iter->instrument()->getInstrumentID() == sy2.symbol) {
            sy2.multiple = iter->instrument()->getMultiplier();
            LOG_INFO << "get sy2 symbol: " << sy2.symbol << ", multiple: " << sy2.multiple;
        }
        Order order;
        order.QryFlag = QryType::QRYPOSI;
        order.setInstrumentID((void*)(iter->instrument()->getInstrumentID().c_str()), iter->instrument()->getInstrumentID().size());

        string Category = "linear";
        if (iter->instrument()->getInstrumentID().find("swap") != string::npos) {
            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        } else if (iter->instrument()->getInstrumentID().find("perp") != string::npos) {
            Category = "inverse";
            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        } else {
            LOG_FATAL << "qryPosition fatal: " << iter->instrument()->getInstrumentID();
        }
        cout << "qryPosition: " << iter->instrument()->getExchange() << endl;
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

        LOG_INFO << "qryPosition long symbol: " << iter->instrument()->getInstrumentID() << ", TodayLong: " << iter->position().PublicPnlDaily().TodayLong 
            << ", TodayShort: " << iter->position().PublicPnlDaily().TodayShort
            << ", net: " << iter->position().getNetPosition()
            << ", entryPrice: " << iter->position().PublicPnlDaily().EntryPrice;    
    }
}


void StrategyUCEasy::initSymbol(){
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
        syInfo.qty = it.second.OrderQty;
        syInfo.mv_ratio = it.second.MvRatio;
        syInfo.thresh = it.second.Thresh;

        syInfo.buy_thresh = it.second.OpenThresh;
        syInfo.sell_thresh = it.second.CloseThresh;
        syInfo.close_flag = it.second.CloseFlag;

        syInfo.prc_tick_size = it.second.PreTickSize;
        syInfo.qty_tick_size = it.second.QtyTickSize;
        syInfo.min_delta_limit = it.second.MinAmount;
        syInfo.max_delta_limit = it.second.MaxDeltaLimit;
        syInfo.pos_adj = it.second.PosAdj;

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
}

void StrategyUCEasy::init() {
    initSymbol();
    qryPosition();
    for (auto it : (*make_taker)) {
        it.second.ref = &(*make_taker)[it.second.ref_sy];
        if (SWAP == it.second.sy) it.second.ref->avg_price = it.second.avg_price;
    }
}

bool StrategyUCEasy::vaildPrice(SyInfo& sy) {
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

bool StrategyUCEasy::IsCancelExistOrders(sy_info* sy)
{
    if (sy->buyMap->size() != 0) {
        for (auto it : (*sy->buyMap)) {
            for (auto iter : it.second->OrderList) {
                if (strcmp(iter.TimeInForce, "GTX") == 0) sy->inst->cancelOrder(iter);
            }
                
        }
        return true;
    }

    if (sy->sellMap->size() != 0) {
        for (auto it : (*sy->sellMap)) {
            for (auto iter : it.second->OrderList)
                if (strcmp(iter.TimeInForce, "GTX") == 0) sy->inst->cancelOrder(iter);
        }
        return true;
    }

    return false;
}

void StrategyUCEasy::Mr_Market_ClosePosition(StrategyInstrument *strategyInstrument)
{
    sy_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];
    string stType = "Mr_Markeet_Close";

    sy_info* sy2 = sy.ref;

    SetOrderOptions order;
    order.orderType = ORDERTYPE_MARKET; // ?

    if (PERP == sy.type) {
        string Category = INVERSE;
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
        qty = min (qty, sy2.fragment/sy2.mid_p);

        double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
        qty = round1(qty, sy2.qty_tick_size, qty_decimal);

        setOrder(sy.inst, INNER_DIRECTION_Sell,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    } else {
        double qty = std::min(sy.real_pos, sy2->bid_v / 2);
        qty = min (qty, sy2.fragment/sy2.mid_p);

        double qty_decimal = ceil(abs(log10(sy.qty_tick_size)));
        qty = round1(qty, sy2.qty_tick_size, qty_decimal);

        setOrder(sy.inst, INNER_DIRECTION_Buy,
            sy.bid_p - sy.prc_tick_size,
            abs(qty), order);
    }
}

// flag 1 arb , 0 fr
//close arb_thresh/fr_thresh   maker taker(at most larger than taker)    (at least large than taker)
void StrategyUCEasy::Mr_ClosePosition(StrategyInstrument *strategyInstrument)
{
    sy_info& sy = (*make_taker)[strategyInstrument->getInstrumentID()];

    sy_info* sy2 = sy.ref;
    if (sy.make_taker_flag == 1) {
        if (IS_DOUBLE_LESS(sy.real_pos, 0)) {
            if (IsCancelExistOrders(&sy)) return;

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

            double qty = std::min(sy.real_pos, sy2->bid_v / 2);

            setOrder(sy.inst, INNER_DIRECTION_Buy,
                sy.bid_p - sy.prc_tick_size,
                qty, order);
            
            LOG_INFO << "Mr_ClosePosition sy maker close short : " << sy.sy << ", sy order side: " << INNER_DIRECTION_Buy
                << ", sy maker_taker_flag: " << sy.make_taker_flag
                << ", sy long_short_flag: " << sy.long_short_flag << ", sy real_pos: " << sy.real_pos
                << ", sy category: " << sy.type << ", sy order price: "
                << sy.bid_p - sy.prc_tick_size << ", sy order qty: " << qty;    
        } else if (IS_DOUBLE_GREATER(sy.real_pos, 0)) {
            if (IsCancelExistOrders(&sy)) return;

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

            double qty = std::min(sy.real_pos, sy2->ask_v / 2);
            qty = min (qty, sy.fragment/sy2.mid_p)

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
        if (IS_DOUBLE_LESS(sy2->real_pos, 0)) {
            if (IsCancelExistOrders(&sy)) return;
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

            double qty = std::min(sy2->real_pos, sy.bid_v / 2);

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                sy2->bid_p - sy2->prc_tick_size,
                qty, order);

            LOG_INFO << "Mr_ClosePosition sy2 maker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;   
        }  else if (IS_DOUBLE_GREATER(sy2->real_pos, 0)) {
            if (IsCancelExistOrders(&sy)) return;
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

void StrategyUCEasy::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    MeasureFunc f(1);
    int64_t ts = CURR_MSTIME_POINT;
    if (marketData.EpochTime - ts > 30) {
        LOG_WARN << "market data beyond time: " << marketData.InstrumentID << ", ts: " << marketData.EpochTime;
        return;
    }
    sy_info& sy1 = (*make_taker)[marketData.InstrumentID];
    sy1.update(marketData.AskPrice1, marketData.BidPrice1, marketData.AskVolume1, marketData.BidVolume1);

    double mr = 0;
    if (!make_continue_mr(mr)) {
        action_mr(mr);
        return;
    }

    sy_info* sy2 = sy1.ref;
    double bal = calc_balance();
    if (sy1.make_taker_flag == 1) {
            if (IsCancelExistOrders(&sy1)) return;

            if (IS_DOUBLE_GREATER(abs(sy1.real_pos + sy2->real_pos), sy1.max_delta_limit)) return;
            
            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;

            if (IS_DOUBLE_GREATER(spread_rate, sy1.sell_thresh)) {
                if (IS_DOUBLE_GREATER(-sy1.real_pos * sy1.avg_price, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->ask_v / 2);
                qty = min (qty, sy2.fragment/sy2.mid_p)

                double qty_decimal = ceil(abs(log10(sy2.qty_tick_size)));
                qty = round1(qty, sy2.qty_tick_size, qty_decimal);


                if (!is_continue_mr(&sy1, qty)) return;
                //  qty = sy1.pos_thresh;

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (PERP == sy1.type) {
                    string Category = INVERSE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy1.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                setOrder(sy1.inst, INNER_DIRECTION_Sell,
                    marketData.AskPrice1 + sy1.prc_tick_size,
                    qty, order);

                LOG_INFO << "MarketDataTradingLogic sy1 maker sell: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                    << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                    << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                    << ", sy1 category: " << sy1.type << ", sy1 order price: "
                    << marketData.AskPrice1 + sy1.prc_tick_size << ", sy1 order qty: " << qty;    
            
            } else if (IS_DOUBLE_LESS(spread_rate, sy1.buy_thresh)) {
                if (IS_DOUBLE_GREATER(sy1.real_pos * sy1.avg_price, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->ask_v / 2);

                qty = min (qty, sy2.fragment/sy2.mid_p)

                double qty_decimal = ceil(abs(log10(sy2.qty_tick_size)));
                qty = round1(qty, sy2.qty_tick_size, qty_decimal);

                if (!is_continue_mr(&sy1, qty)) return;
                //  qty = sy1.pos_thresh;

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (PERP == sy1.type) {
                    string Category = INVERSE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy1.type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                setOrder(sy1.inst, INNER_DIRECTION_Buy,
                    marketData.BidPrice1 + sy1.prc_tick_size,
                    qty, order);

                LOG_INFO << "MarketDataTradingLogic sy1 maker sell: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                    << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                    << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                    << ", sy1 category: " << sy1.type << ", sy1 order price: "
                    << marketData.AskPrice1 + sy1.prc_tick_size << ", sy1 order qty: " << qty;    
            
            }
    } else if (sy2->make_taker_flag == 1) {
            if (IsCancelExistOrders(sy2)) return;

            if (IS_DOUBLE_GREATER(abs(sy2->real_pos + sy1.real_pos), sy2->max_delta_limit)) return;

            double spread_rate = (sy2->mid_p - sy1.mid_p) / sy1.mid_p;

            if (IS_DOUBLE_LESS(spread_rate, sy2->buy_thresh)) {
                if (IS_DOUBLE_GREATER(sy2->real_pos * sy2->avg_price, sy2->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (PERP == sy2->type) {
                    string Category = INVERSE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy1.bid_v / 2);
                qty = min (qty, sy2.fragment/sy2.mid_p)

                double qty_decimal = ceil(abs(log10(sy2.qty_tick_size)));
                qty = round1(qty, sy2.qty_tick_size, qty_decimal);

                if (!is_continue_mr(sy2, qty)) return;

                setOrder(sy2->inst, INNER_DIRECTION_Buy,
                    sy2->bid_p - sy2->prc_tick_size,
                    qty, order);

                LOG_INFO << "ClosePosition sy2 maker open long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                    << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                    << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                    << ", sy2 category: " << sy2->type << ", sy2 order price: "
                    << sy2->bid_p - sy2->prc_tick_size << ", sy2 order qty: " << qty;  
            
            } else if (IS_DOUBLE_GREATER(spread_rate, sy1.sell_thresh)) {
                if (IS_DOUBLE_GREATER(-sy2->real_poss * sy2->avg_price, sy2->mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));

                if (PERP == sy2->type) {
                    string Category = INVERSE;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else if (SWAP == sy2->type) {
                    string Category = LINEAR;
                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                } else {
                    LOG_FATAL << "";
                }

                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));

                double u_posi = abs(sy2->real_pos) * sy2->avg_price;
                double qty = min((bal * sy2->mv_ratio - u_posi) / sy2->mid_p, sy1.ask_v / 2);

                qty = min (qty, sy2.fragment/sy2.mid_p)

                double qty_decimal = ceil(abs(log10(sy2.qty_tick_size)));
                qty = round1(qty, sy2.qty_tick_size, qty_decimal);

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
    
    return;
}

bool StrategyUCEasy::check_min_delta_limit(sy_info& sy1, sy_info& sy2) 
{
    sy1.real_pos = sy1.inst->position().getNetPosition();
    sy2.real_pos = sy2.inst->position().getNetPosition();
    sy2.avg_price = sy2.strategyInstrument->position().PublicPnlDaily().EntryPrice;
    sy1.avg_price = sy1.strategyInstrument->position().PublicPnlDaily().EntryPrice;

    double delta_pos_notional = sy1.real_pos * sy1.multiple + sy2.real_pos * sy2.entry_price;
    if (IS_DOUBLE_LESS(delta_pos_notional, sy1.min_delta_limit)) {
        return false;
        LOG_INFO << "check_min_delta_limit sy1 real_pos: " << sy1.real_pos << ", sy2 real_pos: " << sy2.real_pos;
    }

    return true;
}

void StrategyUCEasy::hedge(StrategyInstrument *strategyInstrument)
{
    string symbol = strategyInstrument->getInstrumentID();
    sy_info& sy1 = (*make_taker)[symbol];
    sy_info* sy2 = sy1.ref;

    double delta_posi = sy1.real_pos * sy1.multiple + sy2.real_pos * sy2.entry_price;
    if (IS_DOUBLE_LESS(abs(delta_posi), sy1.min_delta_limit)) return;
    if (IS_DOUBLE_GREATER(abs(delta_posi) * sy1.mid_p, sy1.fragment * 5))
        LOG_FATAL << "force close symbol: " << symbol << ", delta_posi: " << delta_posi;
     
    double taker_qty = int(abs(delta_posi) / sy1.multiple);

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER_EQUAL(delta_posi, sy1.min_delta_limit)) {
        if ((sy1.make_taker_flag == 1)) {        
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (PERP == sy2->type)
                memcpy(order.Category, INVERSE.c_str(), min(uint16_t(CategoryLen), uint16_t(INVERSE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Sell,
                            sy2->ask_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker open short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->ask_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        } else if ((sy2->make_taker_flag == 1)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (PERP == sy1.type)
                memcpy(order.Category, INVERSE.c_str(), min(uint16_t(CategoryLen), uint16_t(INVERSE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

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
    } else if (IS_DOUBLE_LESS_EQUAL(delta_posi, -sy1.min_delta_limit)) {
        if ((sy1.make_taker_flag == 1)) {        
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy2->type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (PERP == sy2->type)
                memcpy(order.Category, INVERSE.c_str(), min(uint16_t(CategoryLen), uint16_t(INVERSE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

            setOrder(sy2->inst, INNER_DIRECTION_Buy,
                            sy2->bid_p,
                            taker_qty,
                            order);
            LOG_INFO << "hedge sy2 taker close short: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;

        } else if ((sy2->make_taker_flag == 1)) { 
            order.orderType = ORDERTYPE_MARKET; // ?
            if (SWAP == sy1.type) 
                memcpy(order.Category, LINEAR.c_str(), min(uint16_t(CategoryLen), uint16_t(LINEAR.size())));
            else if (PERP == sy1.type)
                memcpy(order.Category, INVERSE.c_str(), min(uint16_t(CategoryLen), uint16_t(INVERSE.size())));

            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));

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

void StrategyUCEasy::update_thresh(StrategyInstrument *strategyInstrument) 
{
    string symbol = strategyInstrument->getInstrumentID();
    sy_info* sy = &(*make_taker)[symbol];
    if (sy->make_taker_flag == 0) {
        sy = sy->ref;
    }
    
    double pos_adj = sy->real_pos / sy->pos_adj;
    double step_thresh = abs(sy->buy_thresh);

    double buy_thresh = - min(max(sy->thresh + pos_adj, thresh_min), thresh_max);
    double sell_thresh = min(max(sy->thresh - pos_adj, thresh_min), thresh_max);

    sy->buy_thresh = buy_thresh + step_thresh;
    sy->sell_thresh = sell_thresh + step_thresh;

    LOG_INFO << "update_thresh maker_threshold: " << sy->thresh 
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: "  << sy2.real_pos
        << ", pos_adj: " << pos_adj 
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
    check_min_delta_limit();
    update_thresh(strategyInstrument);
    hedge(strategyInstrument);
    LOG_INFO << "OnPartiallyFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
}

void StrategyUCEasy::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    check_min_delta_limit();
    update_thresh(strategyInstrument);
    hedge(strategyInstrument);
    LOG_INFO << "OnFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", symbol: " << rtnOrder.InstrumentID
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
}

void StrategyUCEasy::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    hedge(strategyInstrument);
}
