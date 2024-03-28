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
    memset(&sy1, 0, sizeof(SyInfo));
    memset(&sy2, 0, sizeof(SyInfo));
    on_time_60 = CURR_MSTIME_POINT;
    risk_ = new spotRisk(this);
    last_taker_time = 0;
    delta_pos_notional = 0;
    enable_taker = *parameters()->getInt("enable_taker");
    enable_hedge = *parameters()->getInt("enable_hedge");
    enable_maker = *parameters()->getInt("enable_maker");
    symbol2_taker_time_gap = *parameters()->getDouble("symbol2_taker_time_gap");

    taker_level_limit = *parameters()->getInt("taker_level_limit");
    hedge_time_gap = *parameters()->getDouble("hedge_time_gap");
    hedge_pos_thresh2 = *parameters()->getDouble("hedge_pos_thresh2");
    hedge_taker_ratio = *parameters()->getDouble("hedge_taker_ratio");

    taker_threshold_perc = *parameters()->getDouble("taker_threshold_perc");
    taker_threshold_incr = *parameters()->getDouble("taker_threshold_incr");
    symbol2_taker_notional = *parameters()->getDouble("symbol2_taker_notional");
    max_delta_limit = *parameters()->getDouble("max_delta_limit");
    arbitrage_mode = *parameters()->getInt("arbitrage_mode");
    symbol2_multiplier = *parameters()->getDouble("symbol2_multiplier");
    symbol2_pos_adj = *parameters()->getDouble("symbol2_pos_adj");

    max_position_limit = *parameters()->getDouble("max_position_limit");
    disaster_tol_thresh = *parameters()->getDouble("disaster_tol_thresh");
    maker_threshold = *parameters()->getDouble("maker_threshold");
    thresh_max = *parameters()->getDouble("thresh_max");
    thresh_min = *parameters()->getDouble("thresh_min");
    batch_sensitivity = *parameters()->getDouble("batch_sensitivity");
    cancel_order_interval = *parameters()->getInt("cancel_order_interval");
    symbol2_maker_notional = *parameters()->getDouble("symbol2_maker_notional");

    order_distance = *parameters()->getDouble("order_distance");
    order_level = *parameters()->getInt("order_level");
    level_tolerance = *parameters()->getInt("level_tolerance");
    news = *parameters()->getInt("news");
    news_thresh = *parameters()->getDouble("news_thresh");
    ob_tolerance_seconds = *parameters()->getDouble("ob_tolerance_seconds");
    trade_tolerance_seconds = *parameters()->getDouble("trade_tolerance_seconds");
    special_trade_tolerance_seconds = *parameters()->getDouble("special_trade_tolerance_seconds");
    tolerance_delay = *parameters()->getDouble("tolerance_delay");
    delay_plus = *parameters()->getDouble("delay_plus");
    pos_adj_amount = *parameters()->getDouble("pos_adj_amount");
    step_thresh = *parameters()->getDouble("step_thresh");
    time_gap = *parameters()->getInt("time_gap");

    buy_open_flag = *parameters()->getInt("buy_open_flag");
    sell_open_flag = *parameters()->getInt("sell_open_flag");

    last_time_stamp = CURR_STIME_POINT;

    stop_trade = true;
    first_time60 = true;
    last_hedge_time = 0;
    symbol1 = parameters()->getString("symbol1");
    symbol2 = parameters()->getString("symbol2");
    memcpy(sy1.symbol, symbol1.c_str(), min(uint16_t(symbol1.size()), uint16_t(InstrumentIDLen)));
    memcpy(sy2.symbol, symbol2.c_str(), min(uint16_t(symbol2.size()), uint16_t(InstrumentIDLen)));

    exch1 = parameters()->getString("exch1");
    exch2 = parameters()->getString("exch2");
    kMinPrice = 0.0;
    kMaxPrice = 1000000000000.0;


    // ceil(abs(log10(self.si.qty_tick_size)))
    
    sy1.prc_tick_size = *parameters()->getDouble("sy1_prc_tick_size");
    sy2.prc_tick_size = *parameters()->getDouble("sy2_prc_tick_size");
    sy1.qty_tick_size = *parameters()->getDouble("sy1_qty_tick_size");
    sy2.qty_tick_size = *parameters()->getDouble("sy2_qty_tick_size");

    sy1.prx_decimal = ceil(abs(log10(sy1.prc_tick_size)));
    sy2.prx_decimal = ceil(abs(log10(sy2.prc_tick_size)));
    sy1.qty_decimal = ceil(abs(log10(sy1.qty_tick_size)));
    sy2.qty_decimal = ceil(abs(log10(sy2.qty_tick_size)));

    sy1.min_qty = *parameters()->getDouble("sy1_min_qty");
    sy2.min_qty = *parameters()->getDouble("sy2_min_qty");
    sy1.max_qty = *parameters()->getDouble("sy1_max_qty");
    sy2.max_qty = *parameters()->getDouble("sy2_max_qty");

    LOG_INFO << "initial arb: " << ", symbol1: " << symbol1 << ", symbol2: " << symbol2
        << ", exch1: " << exch1 << ", exch2: " << exch2
        << ", sy1.prx_decimal: " << sy1.prx_decimal
        << ", sy2.prx_decimal: " << sy2.prx_decimal
        << ", sy1.qty_decimal: " << sy1.qty_decimal
        << ", sy2.qty_decimal: " << sy2.qty_decimal
        << ", sy1.min_qty: " << sy1.min_qty
        << ", sy2.min_qty: " << sy2.min_qty
        << ", sy1.max_qty: " << sy1.max_qty
        << ", sy2.max_qty: " << sy2.max_qty
        << ", sy1.prc_tick_size: " << sy1.prc_tick_size
        << ", sy2.prc_tick_size: " << sy2.prc_tick_size
        << ", sy1.qty_tick_size: " << sy1.qty_tick_size
        << ", sy2.qty_tick_size: " << sy2.qty_tick_size
        << ", enable_taker: " << enable_taker
        << ", enable_hedge: " << enable_hedge
        << ", enable_maker: " << enable_maker
        << ", symbol2_taker_time_gap: " << symbol2_taker_time_gap
                
        << ", taker_level_limit: " << taker_level_limit
        << ", hedge_time_gap: " << hedge_time_gap
        << ", hedge_pos_thresh2: " << hedge_pos_thresh2
        << ", hedge_taker_ratio: " << hedge_taker_ratio

        << ", taker_threshold_perc: " << taker_threshold_perc
        << ", taker_threshold_incr: " << taker_threshold_incr
        << ", symbol2_taker_notional: " << symbol2_taker_notional
        << ", max_delta_limit: " << max_delta_limit
        << ", arbitrage_mode: " << arbitrage_mode
        << ", symbol2_multiplier: " << symbol2_multiplier
        << ", symbol2_pos_adj: " << symbol2_pos_adj

        << ", max_position_limit: " << max_position_limit
        << ", disaster_tol_thresh: " << disaster_tol_thresh
        << ", maker_threshold: " << maker_threshold
        << ", thresh_max: " << thresh_max
        << ", thresh_min: " << thresh_min
        << ", batch_sensitivity: " << batch_sensitivity
        << ", cancel_order_interval: " << cancel_order_interval
        << ", symbol2_maker_notional: " << symbol2_maker_notional

        << ", order_distance: " << order_distance
        << ", order_level: " << order_level
        << ", level_tolerance: " << level_tolerance
        << ", news: " << news
        << ", news_thresh: " << news_thresh
        << ", ob_tolerance_seconds: " << ob_tolerance_seconds
        << ", trade_tolerance_seconds: " << trade_tolerance_seconds
        << ", special_trade_tolerance_seconds: " << special_trade_tolerance_seconds
        << ", tolerance_delay: " << tolerance_delay
        << ", delay_plus: " << delay_plus
        << ", pos_adj_amount: " << pos_adj_amount
        << ", step_thresh: " << step_thresh
        << ", buy_open_flag: " << buy_open_flag
        << ", sell_open_flag: " << sell_open_flag
        << ", time_gap: " << time_gap;

    cSize_ = 1;
	rSize_ = 1;
}

void StrategyUCEasy::initInstrument() {
    for (auto it : strategyInstrumentList())
	{
		it->tradeable(true);
	}
    for (auto iter : strategyInstrumentList()) {
        if (strcmp(sy1.symbol, iter->instrument()->getInstrumentID().c_str()) == 0) {
            sy1.strategyInstrument = iter;
            buyPriceMap1 = sy1.strategyInstrument->buyOrders();
            sellPriceMap1 = sy1.strategyInstrument->sellOrders();
        }
        if (strcmp(sy2.symbol, iter->instrument()->getInstrumentID().c_str()) == 0) {
            sy2.strategyInstrument = iter;
            buyPriceMap2 = sy2.strategyInstrument->buyOrders();
            sellPriceMap2 = sy2.strategyInstrument->sellOrders();
        }
    }
}
void StrategyUCEasy::resetRcSize() {
	cSize_ = 1;
	rSize_ = 1;
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
        syInfo.pos_thresh = it.second.PosThresh;
        syInfo.max_delta_limit = it.second.MaxDeltaLimit;
        syInfo.ord_capital = it.second.OrdCapital;

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
}

void StrategyUCEasy::init() {
    initInstrument();
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

    if (ClosePosition(marketData, sy1, 1)) return;

    double mr = 0;
    if (!make_continue_mr(mr)) {
        action_mr(mr);
        return;
    }

    sy_info* sy2 = sy1.ref;
    double bal = calc_balance();
    if (sy1.make_taker_flag == 1) {
            if (IsCancelExistOrders(&sy1, INNER_DIRECTION_Sell)) return;
            if (IsCancelExistOrders(&sy1, INNER_DIRECTION_Buy)) return;

            if (IS_DOUBLE_GREATER(abs(sy1.real_pos + sy2->real_pos), sy1.max_delta_limit)) return;
            
            double spread_rate = (sy1.mid_p - sy2->mid_p) / sy2->mid_p;

            if (IS_DOUBLE_GREATER(spread_rate, sy1.sell_thresh)) {
                if (IS_DOUBLE_GREATER(-sy1.real_pos * sy1.avg_price, sy1.mv_ratio * bal)) {
                    LOG_WARN << "";
                    return;
                }

                double u_posi = abs(sy1.real_pos) * sy1.avg_price;
                double qty = min((bal * sy1.mv_ratio - u_posi) / sy1.mid_p, sy2->ask_v / 2);
                if (IS_DOUBLE_LESS(qty, sy1.pos_thresh)) return;
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
                if (IS_DOUBLE_LESS(qty, sy1.pos_thresh)) return;
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
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Buy)) return;
            if (IsCancelExistOrders(sy2, INNER_DIRECTION_Sell)) return;

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
                if (!is_continue_mr(sy2, qty)) return;

                if (IS_DOUBLE_LESS(qty, sy2->pos_thresh)) return;

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
                if (!is_continue_mr(sy2, qty)) return;

                if (IS_DOUBLE_LESS(qty, sy2->pos_thresh)) return;

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
    if (IS_DOUBLE_GREATER(abs(delta_posi), sy1.force_close_amount))
        LOG_FATAL << "force close symbol: " << symbol << ", delta_posi: " << delta_posi;
     
    double taker_qty = int(abs(delta_posi) / sy1.multiple);

    SetOrderOptions order;
    if (IS_DOUBLE_GREATER_EQUAL(delta_posi, sy1.min_delta_limit)) {
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, 0)) {
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
            LOG_INFO << "hedge sy2 taker close long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Sell
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->ask_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, 0)) {          
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
        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0)) { 
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
            LOG_INFO << "hedge sy1 taker close long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Sell
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.ask_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;

        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) { 
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
        if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 1) && IS_DOUBLE_LESS(sy1.real_pos, 0)) {
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
            LOG_INFO << "hedge sy2 taker open long: " << sy2->sy << ", sy2 order side: " << INNER_DIRECTION_Buy
                << ", sy2 maker_taker_flag: " << sy2->make_taker_flag
                << ", sy2 long_short_flag: " << sy2->long_short_flag << ", sy2 real_pos: " << sy2->real_pos
                << ", sy1 real_pos: " << sy1.real_pos << ", sy2 category: " << sy2->type << ", sy2 order price: "
                << sy2->bid_p << ", sy2 order qty: " << taker_qty << ", delta_posi: " << delta_posi;
        } else if ((sy1.make_taker_flag == 1) && (sy1.long_short_flag == 0) && IS_DOUBLE_GREATER(sy1.real_pos, 0)) {          
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

        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 1) && IS_DOUBLE_LESS(sy2->real_pos, 0)) { 
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
            LOG_INFO << "hedge sy1 taker open long: " << sy1.sy << ", sy1 order side: " << INNER_DIRECTION_Buy
                << ", sy1 maker_taker_flag: " << sy1.make_taker_flag
                << ", sy1 long_short_flag: " << sy1.long_short_flag << ", sy1 real_pos: " << sy1.real_pos
                << ", sy2 real_pos: " << sy2->real_pos << ", sy1 category: " << sy1.type << ", sy1 order price: "
                << sy1.bid_p << ", sy1 order qty: " << taker_qty << ", delta_posi: " << delta_posi;

        } else if ((sy2->make_taker_flag == 1) && (sy2->long_short_flag == 0) && IS_DOUBLE_GREATER(sy2->real_pos, 0)) { 
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

void StrategyUCEasy::update_thresh() 
{
    // if (IS_DOUBLE_LESS_EQUAL(pos_adj_amount * symbol2_pos_adj, 0)) {
    //     LOG_FATAL << "pos_adj_amount: " << pos_adj_amount << ", symbol2_pos_adj: " << symbol2_pos_adj;
    // }    
    pos_adj = sy2.real_pos / pos_adj_amount * symbol2_pos_adj;

    buy_thresh = - min(max(maker_threshold + pos_adj, thresh_min), thresh_max);
    sell_thresh = min(max(maker_threshold - pos_adj, thresh_min), thresh_max);

    buy_thresh = buy_thresh + step_thresh;
    sell_thresh = sell_thresh + step_thresh;

    if (IS_DOUBLE_GREATER(abs(sy2.real_pos) * sy2.mid_price, max_position_limit)) {  //# äťä˝cap
        if (IS_DOUBLE_GREATER(sy2.real_pos, 0))
            buy_thresh  -= disaster_tol_thresh;
        else
            sell_thresh += disaster_tol_thresh;
    }
    else if (IS_DOUBLE_GREATER(abs(delta_pos_notional), max_delta_limit)) {
        if (IS_DOUBLE_GREATER(delta_pos_notional, 0))
            buy_thresh -= disaster_tol_thresh;
        else
            sell_thresh += disaster_tol_thresh;
    }

    LOG_INFO << "update_thresh maker_threshold: " << maker_threshold 
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: "  << sy2.real_pos << ", pos_adj_amount: " << pos_adj_amount << ", symbol2_pos_adj: " << symbol2_pos_adj
        << ", pos_adj: " << pos_adj 
        << ", buy_thresh: " << buy_thresh
        << ", sell_thresh: " << sell_thresh
        << ", delta_pos_notional: " << delta_pos_notional;
}

void StrategyUCEasy::OnForceCloseTimerInterval() 
{
    for (auto iter : strategyInstrumentList()) {
        hedge(iter);
    }
}

void StrategyUCEasy::OnTimerTradingLogic()
{
    if (CURR_MSTIME_POINT - on_time_60 < 60000) return;
    on_time_60 = CURR_MSTIME_POINT;
    uint64_t now_ns = CURR_NSTIME_POINT;

    try {
        if (first_time60) {     
            if (IS_DOUBLE_ZERO(sy1.bid_price) || IS_DOUBLE_ZERO(sy2.bid_price)) return;
            
            check_min_delta_limit();
            update_thresh();
            LOG_INFO << "start_trade delta_pos: " << delta_pos_notional
                << ", arbitrage_mode: " << arbitrage_mode
                << ", sy1.bid_price: " << sy1.bid_price
                << ", sy2.bid_price: " << sy2.bid_price
                << ", sy1.real_pos: " << sy1.real_pos
                << ", sy2.real_pos: " << sy2.real_pos;
            first_time60 = false;
        }
        stop_trade = false;
        double spread = 0;
        // if (IS_DOUBLE_LESS_EQUAL(sy2.mid_price, 0)) {
        //     LOG_FATAL << "mid_price: " << sy2.mid_price << ", ask_price: " << sy2.ask_price
        //         << ", bid_price: " << sy2.bid_price;
        // }
        if (IS_DOUBLE_ZERO(sy2.mid_price))
            spread = 0;
        else
            spread = round_p((sy1.mid_price * symbol2_multiplier - sy2.mid_price) / sy2.mid_price, 4);

        check_min_delta_limit();

        LOG_INFO << "OnTimerTradingLogic time: " << now_ns
            << ", symbol1_bid: " << sy1.bid_price
            << ", symbol1_ask: " << sy1.ask_price
            << ", symbol2_bid: " << sy2.bid_price
            << ", symbol2_ask: " << sy2.ask_price
            << ", buy_thresh: " << round_p(10000 * buy_thresh, 1)
            << ", sell_thresh: " <<  round_p(10000 * sell_thresh, 1)
            << ", delta_adj: " << 0
            << ", pos_adj: " <<  round_p(10000 * pos_adj, 1)
            << ", symbol2 bid_quote: " <<  po_best_price(buyPriceMap2, 0)
            << ", symbol2 ask_quote: " <<  po_best_price(sellPriceMap2, 1)
            << ", fair_bid: " << sy2.bid_fair_price
            << ", fair_ask: " << sy2.ask_fair_price
            << ", sy1.real_pos: " << sy1.real_pos
            << ", sy2.real_pos: " << sy2.real_pos
            << ", sy1.mid_price: " << sy1.mid_price
            << ", delta_pos_notional: " << delta_pos_notional
            << ", spread: " << spread;
    }

    catch (std::exception ex) {
        LOG_ERROR << "OnTimerTradingLogic exception occur:" << ex.what();

    }
}

void StrategyUCEasy::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    check_min_delta_limit();
    update_thresh();
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
    update_thresh();
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
