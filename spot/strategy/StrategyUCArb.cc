#include "StrategyUCArb.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Measure.h"
#include "spot/base/InitialData.h"
#include "spot/cache/CacheAllocator.h"
#include "spot/utility/TradingDay.h"

using namespace spot::strategy;
// using namespace spot::risk;

Strategy *StrategyUCArb::Create(int strategyID, StrategyParameter *params) {
    char *buff = CacheAllocator::instance()->allocate(sizeof(StrategyUCArb));
    return new(buff) StrategyUCArb(strategyID, params);
}

StrategyUCArb::StrategyUCArb(int strategyID, StrategyParameter *params)
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

    bottom_thresh = *parameters()->getDouble("bottom_thresh");

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
        << ", bottom_thresh: " << bottom_thresh;

    cSize_ = 1;
	rSize_ = 1;

}

void StrategyUCArb::initInstrument() {
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
void StrategyUCArb::resetRcSize() {
	cSize_ = 1;
	rSize_ = 1;
}

void StrategyUCArb::qryPosition() {
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

void StrategyUCArb::init() {
    initInstrument();
    qryPosition();
}

int StrategyUCArb::getBuyPendingLen(SyInfo& sy) {
    int pendNum = 0;
    for (auto it : (*sy.strategyInstrument->buyOrders())) {
        for (auto iter : it.second->OrderList) {
            if (iter.OrderStatus == PendingNew && (strcmp(iter.MTaker, FEETYPE_MAKER.c_str()) == 0))
                pendNum++;
        }
    } 
    return pendNum;
}

int StrategyUCArb::getSellPendingLen(SyInfo& sy) {
    int pendNum = 0;
    for (auto it : (*sy.strategyInstrument->sellOrders())) {
        for (auto iter : it.second->OrderList) {
            if (iter.OrderStatus == PendingNew && (strcmp(iter.MTaker, FEETYPE_MAKER.c_str()) == 0))
                pendNum++;
        }
    }
    return pendNum;
}

int StrategyUCArb::getIocOrdPendingLen(SyInfo& sy) {
    int pendNum = 0;
    for (auto it : (*sy.strategyInstrument->sellOrders())) {
        for (auto iter : it.second->OrderList) {
            LOG_DEBUG << "getIocOrdPendingLen SELL  symbol: " << iter.InstrumentID
                << ", OrderStatus: " << iter.OrderStatus
                << ", MTaker: " << iter.MTaker
                << ", orderRef: " << iter.OrderRef;
            if (iter.OrderStatus == PendingNew && (strcmp(iter.TimeInForce, "IOC") == 0)) {
                pendNum++;
            }
                
        }
    }

    for (auto it : (*sy.strategyInstrument->buyOrders())) {
        for (auto iter : it.second->OrderList) {
            LOG_DEBUG << "getIocOrdPendingLen BUY  symbol: " << iter.InstrumentID
                << ", OrderStatus: " << iter.OrderStatus
                << ", MTaker: " << iter.MTaker
                << ", orderRef: " << iter.OrderRef;
            if (iter.OrderStatus == PendingNew && (strcmp(iter.TimeInForce, "IOC") == 0))
                {
                    pendNum++;
                }
                
        }
    }            

    LOG_DEBUG << "HEGET getIocOrdPendingLen sell size: "<< sy.strategyInstrument->sellOrders()->size() 
       << ", buy size: " << sy.strategyInstrument->buyOrders()->size() << ",pendNum: " << pendNum;
    return pendNum;
}

double StrategyUCArb::get_tol_delay() {
    if (sy1.trigger_source == TriggerSource::Trade)
        return trade_tolerance_seconds;
    else if (sy1.trigger_source == TriggerSource::BestPrice)
        return ob_tolerance_seconds;
    else if (sy1.trigger_source == TriggerSource::TPrice)
        return special_trade_tolerance_seconds;
    else
        return ob_tolerance_seconds;
}

bool StrategyUCArb::VaildCancelTime(Order& order, uint8_t loc)
{
    uint64_t now_ns= CURR_NSTIME_POINT;
    if (order.OrderStatus == PendingCancel || order.OrderStatus == PendingNew) {
        LOG_INFO << "VaildCancelTime symbol: " << order.InstrumentID << ", orderRef: "
            << order.OrderRef << ", status: " << order.OrderStatus << ", loc: " << loc;
        return false;
    }
    if (now_ns - order.TimeStamp < cancel_order_interval * 1e9) {
        LOG_INFO << "VaildCancelTime now_ns: " << now_ns << ", TimeStamp: " << order.TimeStamp
            << ", loc: " << loc;
        return false;
    }
    return true;
}

int StrategyUCArb::getPoPriceLevel(OrderByPriceMap*  priceMap) {
    int level = 0;
    for (auto it : (*priceMap)) {
        for (auto iter : it.second->OrderList) {
            if (strcmp(iter.TimeInForce, "GTX") == 0) {
                level++;
                break;
            }
        }
    } 
    return level;
}

// GTX pending orders
double StrategyUCArb::po_best_price(OrderByPriceMap*  priceMap, int side) {
    if (side == 0) {
        if (priceMap->size() != 0) {
            auto it = priceMap->rbegin();
            for (auto iter : it->second->OrderList) {
                if (strcmp(iter.TimeInForce, "GTX") == 0)
                    return iter.LimitPrice;
                else {
                    LOG_ERROR << "po_best_price error no buy GTX, PriceMap size: " << priceMap->size()
                        << ", size: " << it->second->OrderList.size() << ", orderRef: " << iter.OrderRef;
                    return kMinPrice;
                }
            }

        }
        else
            return kMinPrice;
    }
    else {
        if (priceMap->size() != 0) {
            auto it = priceMap->begin();
            // if (it->second.OrderList.size() > 1) {
            //     for (auto iter : it->second.OrderList) {
            //         LOG_FATAL << ""
            //     }             
            // }
            for (auto iter : it->second->OrderList) {
                if (strcmp(iter.TimeInForce, "GTX") == 0)
                    return iter.LimitPrice;
                else {
                    LOG_ERROR << "po_best_price error no sell GTX, PriceMap size: " << priceMap->size()
                        << ", size: " << it->second->OrderList.size() << ", orderRef: " << iter.OrderRef;
                    return kMaxPrice;
                }
            }
        }
        else
            return kMaxPrice;
    }
    // warning
    return 0;
}

bool StrategyUCArb::vaildPrice(SyInfo& sy) {
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

int StrategyUCArb::maker_action() {
    if (!vaildPrice(sy1) || !vaildPrice(sy2)) return 0;
    flag_send_order = 0;
    // if (!risk_->VaildByLimitOrdersSlidePers(cSize_)) {
    //     resetRcSize();
    //     return flag_send_order;}
    uint64_t now_ns= CURR_NSTIME_POINT;
    sy2.ask_fair_price = sy1.ask_price + sell_thresh * sy1.mid_price;
    sy2.bid_fair_price = sy1.bid_price + buy_thresh * sy1.mid_price;
    sy2.ask_place_price = sy2.ask_fair_price;
    sy2.bid_place_price = sy2.bid_fair_price;

    int64_t timeGap = now_ns - sy1.exch_ts;
    if (timeGap < tolerance_delay * 1e9) {
        double tol_sec = get_tol_delay();
        // # 有延迟，需要放宽
        if (now_ns - sy1.exch_ts >= tol_sec * 1e9) {
            sy2.ask_place_price *= (1 + delay_plus);
            sy2.bid_place_price *= (1 - delay_plus);
            LOG_WARN << "maker_action now_ns: " << now_ns << ", exch_ts: " << sy1.exch_ts
                << ", timeGap: " << timeGap << ", tol_sec: " << tol_sec
                << ", ask_place_price: " << sy2.ask_place_price << ", bid_place_price: " 
                << sy2.bid_place_price;
        }
    } else {
        //  需要避险
        sy2.ask_place_price *= (1 + disaster_tol_thresh);
        sy2.bid_place_price *= (1 - disaster_tol_thresh);
        LOG_WARN << "maker_action 2 now_ns: " << now_ns << ", exch_ts: " << sy1.exch_ts
            << ", timeGap: " << timeGap 
            << ", ask_place_price: " << sy2.ask_place_price << ", bid_place_price: " 
            << sy2.bid_place_price << ", disaster_tol_thresh: " << disaster_tol_thresh;
    }

    // # GTX check #s1 taker   s2 maker taker  s1:ba s2:by
    sy2.ask_place_price = max(sy2.ask_place_price, sy2.bid_price + sy2.prc_tick_size);
    sy2.bid_place_price = min(sy2.bid_place_price, sy2.ask_price - sy2.prc_tick_size);
    if (IS_DOUBLE_LESS(abs(sy2.ask_place_price - sy2.ask_last_place_price), batch_sensitivity * 0.5 * sy2.mid_price) &&
        IS_DOUBLE_LESS(abs(sy2.bid_place_price - sy2.bid_last_place_price), batch_sensitivity * 0.5 * sy2.mid_price)) return flag_send_order; // no need to adjust
    sy2.ask_last_place_price = sy2.ask_place_price;
    sy2.bid_last_place_price = sy2.bid_place_price;

    bool bid_continue = true; //# best_price buy 取最高 sell 取最低
    if (IS_DOUBLE_GREATER(po_best_price(buyPriceMap2, 0), sy2.bid_place_price * (1 + batch_sensitivity))) {  // 头部撤单 部分撤单，只撤无利可图单
        for(auto it = buyPriceMap2->rbegin(); it != buyPriceMap2->rend(); it++) {
            if (IS_DOUBLE_GREATER(it->first, sy2.bid_place_price * (1 + batch_sensitivity))) {
                for (auto& iter : it->second->OrderList) {
                    if (!VaildCancelTime(iter, 1)) continue;
                    sy2.strategyInstrument->cancelOrder(iter);
                    bid_continue = false;
                }
            } else break;
        }
    }
    if (getBuyPendingLen(sy2) != 0)
        bid_continue = false; //#有挂单

    bool ask_continue = true;
    // # ask order
    if (IS_DOUBLE_LESS(po_best_price(sellPriceMap2, 1), sy2.ask_place_price * (1 - batch_sensitivity))) { //  # 头部撤单
        for(auto it = sellPriceMap2->begin(); it != sellPriceMap2->end(); it++) {
            if (IS_DOUBLE_LESS(it->first, sy2.ask_place_price * (1 - batch_sensitivity))) {
                for (auto& iter : it->second->OrderList) {
                    if (!VaildCancelTime(iter, 2)) continue;
                    sy2.strategyInstrument->cancelOrder(iter);
                    ask_continue = false;
                }
            } else
                break;
        }
    }
    if (getSellPendingLen(sy2) != 0)
        ask_continue = false; //#有挂单

    double prc_tick_size = sy2.prc_tick_size;

    // if (IS_DOUBLE_LESS_EQUAL(sy2.mid_price, 0)) {
    //     LOG_FATAL << "mid_price: " << sy2.mid_price << ", ask_price: " << sy2.ask_price
    //         << ", bid_price: " << sy2.bid_price;
    // }
    double temp_maker_qty = round1(symbol2_maker_notional / sy2.mid_price, sy2.qty_tick_size, sy2.qty_decimal);

    // # 5 -> bybit po 版本，这个需要改？
    double maker_qty = min(temp_maker_qty, 5 * sy2.max_qty);

    
    if (bid_continue) {
        double ord_distance = max(prc_tick_size, round_up(sy2.bid_place_price * order_distance, prc_tick_size, sy2.prx_decimal));
        double worst_bid = sy2.bid_place_price - (order_level + level_tolerance) * ord_distance;
        double temp_bid1 = sy2.bid_place_price - ord_distance;
        int current_level = getPoPriceLevel(buyPriceMap2);
        if (IS_DOUBLE_GREATER(po_best_price(buyPriceMap2, 0), temp_bid1 + epsilon) && current_level < order_level) {  //# 尾部挂单
            auto it = buyPriceMap2->begin();
            double last_order_px = it->first; //# bid: 高->低   ask： 低->高
            for (int i = 1; (i < order_level - current_level + 1); i++) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                string Category = "linear";
                string CoinType = "SWAP";

                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                setOrder(sy2.strategyInstrument, INNER_DIRECTION_Buy,
                    last_order_px - i * order_distance,
                    maker_qty, order);
                flag_send_order += 1;
                LOG_INFO << "make action sy2 price: " << last_order_px - (i)*order_distance << ", side: " << INNER_DIRECTION_Buy
                    << ", qty: " << maker_qty 
                    << ", temp_maker_qty: " << temp_maker_qty
                    << ", buy_thresh: " << buy_thresh
                    << ", sell_thresh: " << sell_thresh
                    << ", timeGap: " << timeGap
                    << ", ask_place_price: " << sy2.ask_place_price // new
                    << ", bid_place_price: " << sy2.bid_place_price // new
                    << ", sy1.mid_price: " << sy1.mid_price
                    << ", sy2.mid_price: " << sy2.mid_price
                    << ", sy2.qty_tick_size: " << sy2.qty_tick_size
                    << ", sy2.qty_decimal: " << sy2.qty_decimal
                    << ", timeInForce: " << timeInForce
                    << ", triggerSource: " << sy2.trigger_source
                    << ", current_level: " << current_level;
            }
        }
        else if (current_level > order_level + level_tolerance) { // # 尾部撤单
            for (auto it = buyPriceMap2->begin(); it != buyPriceMap2->end(); it++) {
                if (IS_DOUBLE_GREATER(current_level, order_level + level_tolerance)) {
                    current_level -= 1;
                    for (auto& iter : it->second->OrderList) {
                        if (!VaildCancelTime(iter, 3)) continue;
                        sy2.strategyInstrument->cancelOrder(iter);
                    }
                }
                else
                    break;
            }
        }
        else if (current_level != 0 && IS_DOUBLE_LESS(po_best_price(buyPriceMap2, 0), worst_bid - epsilon)) { // 撤单
            for (auto it = buyPriceMap2->begin(); it != buyPriceMap2->end(); it++) {
                for (auto& iter : it->second->OrderList) {
                    if (!VaildCancelTime(iter, 4)) continue;
                    sy2.strategyInstrument->cancelOrder(iter);
                }
            }
        }

        else if (IS_DOUBLE_LESS(po_best_price(buyPriceMap2, 0), temp_bid1 - epsilon)) { //# 头部挂单
            if (current_level == 0) {
                for (int i = 0; i < order_level; i++) {
                    double order_px = round_down(sy2.bid_place_price - ord_distance * i, prc_tick_size, sy2.prx_decimal);
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(TimeInForceLen, uint16_t(timeInForce.size())));
                    string Category = "linear";
                    string CoinType = "SWAP";

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                    memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                    setOrder(sy2.strategyInstrument, INNER_DIRECTION_Buy,
                        order_px,
                        maker_qty, order);
                    // order = send_order(logic_acct_id2, order_px, sid2, INNER_DIRECTION_Buy, maker_qty, TimeInForce.PO, 10)
                    flag_send_order += 1;
                    LOG_INFO << "make action sy2 price: " << order_px
                        << ", buy_thresh: " << buy_thresh
                        << ", sell_thresh: " << sell_thresh
                        // << ", bid_place_price: " << sy1.bid_place_price
                        << ", ord_distance: " << ord_distance
                        << ", prc_tick_size: " << prc_tick_size
                        << ", prx_decimal: " << sy2.prx_decimal
                        << ", side: " << INNER_DIRECTION_Buy
                        << ", qty: " << maker_qty
                        << ", temp_maker_qty: " << temp_maker_qty
                        << ", timeGap: " << timeGap
                        << ", ask_place_price: " << sy2.ask_place_price // new
                        << ", bid_place_price: " << sy2.bid_place_price // new
                        << ", sy1.mid_price: " << sy1.mid_price
                        << ", sy2.mid_price: " << sy2.mid_price
                        << ", sy2.qty_tick_size: " << sy2.qty_tick_size
                        << ", sy2.qty_decimal: " << sy2.qty_decimal
                        << ", timeInForce: " << timeInForce
                        << ", triggerSource: " << sy2.trigger_source
                        << ", current_level: " << current_level;
                }
            }
            else {
                for (int i = 0; i < order_level; i++) {
                    double order_px = round_p(po_best_price(buyPriceMap2, 0) + ord_distance * (i + 1), sy2.prx_decimal);
                    if (IS_DOUBLE_LESS(order_px, sy2.bid_place_price + epsilon)) {
                        SetOrderOptions order;
                        order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                        string timeInForce = "GTX";
                        memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                        string Category = "linear";
                        string CoinType = "SWAP";

                        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                        memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                        memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                        flag_send_order += 1;
                        setOrder(sy2.strategyInstrument, INNER_DIRECTION_Buy,
                            order_px,
                            maker_qty, order);
                        LOG_INFO << "make action sy2 price: " << order_px 
                            << ", buy_thresh: " << buy_thresh
                            << ", sell_thresh: " << sell_thresh
                            << ", best_price(buyPriceMap2, 0): " << po_best_price(buyPriceMap2, 0) // 0 buy
                            << ", ord_distance: " << ord_distance
                            << ", i+1: " << (i + 1)
                            << ", prx_decimal: " << sy2.prx_decimal
                            << ", side: " << INNER_DIRECTION_Buy
                            << ", qty: " << maker_qty 
                            << ", temp_maker_qty: " << temp_maker_qty
                            << ", timeGap: " << timeGap
                            << ", ask_place_price: " << sy2.ask_place_price // new
                            << ", bid_place_price: " << sy2.bid_place_price // new
                            << ", sy1.mid_price: " << sy1.mid_price
                            << ", sy2.mid_price: " << sy2.mid_price
                            << ", sy2.qty_tick_size: " << sy2.qty_tick_size
                            << ", sy2.qty_decimal: " << sy2.qty_decimal
                            << ", timeInForce: " << timeInForce
                            << ", triggerSource: " << sy2.trigger_source;
                    else
                        break;
                    }
                }
            }
        }
    }

    if (ask_continue) {
        double ord_distance = max(prc_tick_size, round_up(sy2.ask_place_price * order_distance, prc_tick_size, sy2.prx_decimal));
        double worst_ask = sy2.ask_place_price + ( order_level + level_tolerance) * ord_distance;
        double temp_ask1 = sy2.ask_place_price + ord_distance;
        int current_level = getPoPriceLevel(sellPriceMap2);
        if (IS_DOUBLE_LESS(po_best_price(sellPriceMap2, 1), temp_ask1 - epsilon) && current_level < order_level) { //# 尾部挂单
            auto it = sellPriceMap2->rbegin();
            double last_order_px = it->first; //# bid: 高->低   ask： 低->高
            for (int i = 1; (i < order_level - current_level + 1); i++) {
                SetOrderOptions order;
                order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                string timeInForce = "GTX";
                memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                string Category = "linear";
                string CoinType = "SWAP";

                memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                setOrder(sy2.strategyInstrument, INNER_DIRECTION_Sell,
                    last_order_px + i * ord_distance,
                    maker_qty, order);
                flag_send_order += 1;
                LOG_INFO << "make action sy2 price: " << last_order_px + i * ord_distance << ", side: " << INNER_DIRECTION_Sell
                     << ", buy_thresh: " << buy_thresh
                    << ", sell_thresh: " << sell_thresh
                    << ", qty: " << maker_qty 
                    << ", temp_maker_qty: " << temp_maker_qty
                    << ", timeGap: " << timeGap
                    << ", ask_place_price: " << sy2.ask_place_price // new
                    << ", bid_place_price: " << sy2.bid_place_price // new
                    << ", sy1.mid_price: " << sy1.mid_price
                    << ", sy2.mid_price: " << sy2.mid_price
                    << ", sy2.qty_tick_size: " << sy2.qty_tick_size
                    << ", sy2.qty_decimal: " << sy2.qty_decimal
                    << ", timeInForce: " << timeInForce
                    << ", triggerSource: " << sy2.trigger_source
                    << ", current_level: " << current_level;
            }
        }
        else if (current_level > order_level + level_tolerance) {//  # 尾部撤单
            for(auto it = sellPriceMap2->rbegin(); it != sellPriceMap2->rend(); it++) {
                if (current_level > order_level + level_tolerance) {
                    current_level -= 1;
                    for(auto& iter : it->second->OrderList) {
                        if (!VaildCancelTime(iter, 5)) continue;
                        sy2.strategyInstrument->cancelOrder(iter);
                    }
                }
                else
                    break;
            }
        }
        else if (current_level != 0 && po_best_price(sellPriceMap2, 1) > worst_ask + epsilon) {  //# 撤单
            for(auto it = sellPriceMap2->rbegin(); it != sellPriceMap2->rend(); it++) {
                for(auto& iter : it->second->OrderList) {
                    if (!VaildCancelTime(iter, 6)) continue;
                    sy2.strategyInstrument->cancelOrder(iter);
                }
            }
        }
        else if (IS_DOUBLE_GREATER(po_best_price(sellPriceMap2, 1), temp_ask1 + epsilon)) {  //# 头部挂单
            if (current_level == 0) {
                for (int i = 0; i < order_level; i++) {
                    SetOrderOptions order;
                    order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                    string timeInForce = "GTX";
                    memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                    string Category = "linear";
                    string CoinType = "SWAP";

                    memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                    memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                    memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                    double order_px = round_up(sy2.ask_place_price + ord_distance * i, prc_tick_size, sy2.prx_decimal);
                    setOrder(sy2.strategyInstrument, INNER_DIRECTION_Sell,
                        order_px,
                        maker_qty, order);
                    // order = send_order(logic_acct_id2, order_px, sid2, INNER_DIRECTION_Sell, maker_qty, TimeInForce.PO, 10)
                    flag_send_order += 1;
                }
            }
            else {
                for (int i = 0; i < order_level; i++) {
                    double order_px = round_p(po_best_price(sellPriceMap2, 1) - ord_distance * (i + 1), sy2.prx_decimal);
                    if (IS_DOUBLE_GREATER(order_px, sy2.ask_place_price - epsilon)) {
                        SetOrderOptions order;
                        order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
                        string timeInForce = "GTX";
                        memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
                        string Category = "linear";
                        string CoinType = "SWAP";

                        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
                        memcpy(order.MTaker, FEETYPE_MAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_MAKER.size())));
                        memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

                        setOrder(sy2.strategyInstrument, INNER_DIRECTION_Sell,
                            order_px,
                            maker_qty, order);        
                        // order = send_order(logic_acct_id2, order_px, sid2, INNER_DIRECTION_Sell, maker_qty, TimeInForce.PO, 11)
                        flag_send_order += 1;
                    }
                    else
                        break;
                }
            }
        }
    }
    return flag_send_order;
}
// taker_action maker_action only exchange2 ,hedge only exchange1.
int StrategyUCArb::taker_action() {
    if (!vaildPrice(sy1) || !vaildPrice(sy2)) return 0;
    // risk check
    // if (!risk_->VaildByLimitOrdersSlidePers(rSize_)) {
    //     resetRcSize();
    //     return flag_send_order;
    // }
    flag_send_order = 0;
    uint64_t now_ns= CURR_NSTIME_POINT;
    if (now_ns - last_taker_time < symbol2_taker_time_gap * 1e9 ||
        getIocOrdPendingLen(sy2) > 5 * taker_level_limit) return flag_send_order;
    double eat_threshold = taker_threshold_perc * sy2.mid_price;

    // if (IS_DOUBLE_LESS_EQUAL(sy2.mid_price, 0)) {
    //     LOG_FATAL << "mid_price: " << sy2.mid_price << ", ask_price: " << sy2.ask_price
    //         << ", bid_price: " << sy2.bid_price;
    // }
    double temp_trade_qty = symbol2_taker_notional / sy2.mid_price;
    double trade_qty = min(round1(temp_trade_qty, sy2.qty_tick_size, sy2.qty_decimal), sy2.max_qty);

    // 有延迟，需要放宽
    double tol_sec = get_tol_delay();
    double delay = 0;
    if (now_ns - sy1.exch_ts >= tol_sec * 1e9)
        delay = delay_plus;

    double temp_buy_fair = round_down((sy1.bid_price + buy_thresh * sy1.mid_price - eat_threshold)* (1 - delay), sy2.prc_tick_size, sy2.prx_decimal);
    double temp_ask_fair = round_up((sy1.ask_price + sell_thresh * sy1.mid_price + eat_threshold) * (1 + delay), sy2.prc_tick_size, sy2.prx_decimal);
    if (IS_DOUBLE_GREATER_EQUAL(temp_buy_fair, sy2.ask_price)) {
        SetOrderOptions order;
        order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
        string timeInForce = "IOC";
        memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
        string Category = "linear";
        string CoinType = "SWAP";

        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
        memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

        setOrder(sy2.strategyInstrument, INNER_DIRECTION_Buy,
                        temp_buy_fair,
                        trade_qty,
                        order);
        // tag == 2?
        // order = send_order(logic_acct_id2, temp_buy_fair, sid2, INNER_DIRECTION_Buy, trade_qty, TimeInForce.IOC, 2);
        last_taker_time = now_ns;
        flag_send_order = 1;

        LOG_INFO << "take action sy2 temp_buy_fair side: " << INNER_DIRECTION_Buy
            << ", qty: " << trade_qty
            << ", temp_trade_qty: " << temp_trade_qty
            << ", symbol2_taker_notional: " << symbol2_taker_notional
            << ", sy2.mid_price: " << sy2.mid_price
            << ", sy2.qty_tick_size: " << sy2.qty_tick_size
            << ", sy2.qty_decimal: " << sy2.qty_decimal
            << ", sy2.max_qty: " << sy2.max_qty
            << ", temp_buy_fair: " << temp_buy_fair
            << ", sy1.bid_price: " << sy1.bid_price
            << ", sy1.mid_price: " << sy1.mid_price
            << ", sy2.prc_tick_size: " << sy2.prc_tick_size
            << ", sy2.prx_decimal: " << sy2.prx_decimal
            << ", timeInForce: " << timeInForce
            << ", bid_thresh: " << round_p(-10000 * buy_thresh, 1)
            << ", sell_thresh: " << round_p(10000 * sell_thresh, 1)
            << ", delta: " << delta_pos_notional
            << ", eat_threshold: " << eat_threshold
            << ", triggerprice: " << sy1.bid_price
            << ", source: " << sy1.trigger_source
            << ", sy1.real_pos: " << sy1.real_pos 
            << ", sy2.real_pos: " << sy2.real_pos
            << ", triggerSource: " << sy2.trigger_source;
            // << ", symbol1_pos: " << sy1.pos_notional // real pos * price
            // << ", symbol2_pos: " << sy2.pos_notional
            
    } else if (IS_DOUBLE_LESS_EQUAL(temp_ask_fair, sy2.bid_price)) {
        SetOrderOptions order;
        order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
        string timeInForce = "IOC";
        memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
        string Category = "linear";
        string CoinType = "SWAP";

        memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
        memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
        memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

        setOrder(sy2.strategyInstrument, INNER_DIRECTION_Sell,
                        temp_ask_fair,
                        trade_qty,
                        order);
        last_taker_time = now_ns;
        // sy2.ioc_pending[order.order_id.value] = now
        flag_send_order += 1;

        LOG_INFO << "take action sy2 temp_ask_fair side: " << INNER_DIRECTION_Sell
            << ", qty: " << trade_qty 
            << ", temp_trade_qty: " << temp_trade_qty
            << ", symbol2_taker_notional: " << symbol2_taker_notional
            << ", sy2.mid_price: " << sy2.mid_price
            << ", sy2.qty_tick_size: " << sy2.qty_tick_size
            << ", sy2.qty_decimal: " << sy2.qty_decimal
            << ", sy2.max_qty: " << sy2.max_qty
            << ", temp_ask_fair: " << temp_ask_fair
            << ", sy1.ask_price: " << sy1.ask_price
            << ", sy1.mid_price: " << sy1.mid_price
            << ", sy2.prc_tick_size: " << sy2.prc_tick_size
            << ", sy2.prx_decimal: " << sy2.prx_decimal
            << ", timeInForce: " << timeInForce 
            << ", bid_thresh: " << round_p(-10000 * buy_thresh, 1)
            << ", sell_thresh: " << round_p(10000 * sell_thresh, 1)
            << ", sy1.real_pos: " << sy1.real_pos
            << ", sy2.real_pos: " << sy2.real_pos
            << ", eat_threshold: " << eat_threshold
            << ", eat_threshold: " << eat_threshold << ", triggerprice: " <<sy1.ask_price <<", source: " << sy1.trigger_source
            << ", triggerSource: " << sy2.trigger_source;;
    }
     return flag_send_order;
}

void StrategyUCArb::over_max_delta_limit() {
    sy1.real_pos = sy1.strategyInstrument->position().getNetPosition();
    sy2.real_pos = sy2.strategyInstrument->position().getNetPosition();
    sy2.entry_price = sy2.strategyInstrument->position().PublicPnlDaily().EntryPrice;
    
    delta_pos_notional = sy1.real_pos * sy1.multiple + sy2.real_pos * sy2.entry_price;
    // delta_pos_notional = (sy1.real_pos + sy2.real_pos) * sy1.mid_price;
    if (IS_DOUBLE_GREATER(abs(delta_pos_notional), max_delta_limit * 10)) {
        enable_hedge = 0;
        enable_taker = 0;
        enable_maker = 0;
        LOG_INFO << "over_max_delta_limit delta: " << delta_pos_notional << ", max_delta_limit: " << max_delta_limit * 10;
        //  logger.info(f"Stop trade becasue over max delta limit, {delta_pos_notional}, {max_delta_limit * 10}")
    }
}
// taker_action maker_action only exchange2 ,hedge only exchange1.
void StrategyUCArb::hedge() {
    // uint64_t now_ns= CURR_NSTIME_POINT;
    // if (now_ns - last_hedge_time < hedge_time_gap * 1e9) return;
    // LOG_INFO << "StrategyUCArb::hedge(): " << delta_pos_notional << ", hedge_pos_thresh2: " << hedge_pos_thresh2;
    if (IS_DOUBLE_GREATER(abs(delta_pos_notional), hedge_pos_thresh2)) {
        //if sy1.ioc_pending: return
        if (getIocOrdPendingLen(sy1) != 0) {
            LOG_INFO << "getIocOrdPendingLen ERROR";
            return;}

        // double temp_taker_qty = round1(abs(delta_pos_notional) * hedge_taker_ratio / sy1.mid_price, sy1.qty_tick_size, sy1.qty_decimal);
        
        double temp_taker_qty = int(abs(delta_pos_notional)/ sy1.multiple);
        double taker_qty = max(sy1.min_qty, min(temp_taker_qty, sy1.max_qty));
        LOG_INFO << "getIocOrdPendingLen temp_taker_qty: " << temp_taker_qty << ", taker_qty: " << taker_qty
            << ", delta_pos_notional: " << delta_pos_notional << ", sy1.mid_price: " << sy1.mid_price;

        // double taker_qty =  abs(sy2.strategyInstrument->position().getNetPosition() - sy1.strategyInstrument->position().getNetPosition());
        // LOG_INFO << "hedge taker_qty: " << taker_qty << ", sy1.mid_price: " << sy1.mid_price
        //     << ", delta_pos_notional: " << delta_pos_notional;
        if (IS_DOUBLE_LESS(taker_qty * sy1.multiple, hedge_pos_thresh2)) {
            LOG_ERROR << "hedge temp_taker_qty: " << temp_taker_qty << ", taker_qty: " << taker_qty
                << ", delta_pos_notional: " << delta_pos_notional << ", sy1.mid_price: " << sy1.mid_price;
            return;
        }

        SetOrderOptions order;
        if (IS_DOUBLE_GREATER(delta_pos_notional, hedge_pos_thresh2)) {
            // if (!risk_->VaildBianOrdersCountbyDurationTenSec(rSize_) || !risk_->VaildBianOrdersCountbyDurationOneMin(rSize_)){
            //     resetRcSize();
            //     return;
            // }
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "IOC";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
            string Category = "inverse";
            string CoinType = "PERP";


            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
            memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));
            LOG_INFO << "hedge sy1 bid_price: " <<  sy1.bid_price << ", taker_qty: " << taker_qty
                << ", delta_pos_notional: " << delta_pos_notional <<  ", hedge_pos_thresh2: " << hedge_pos_thresh2;
            setOrder(sy1.strategyInstrument, INNER_DIRECTION_Sell,
                            sy1.bid_price,
                            taker_qty,
                            order);
        }
        else {
            // if (!risk_->VaildBianOrdersCountbyDurationTenSec(rSize_) || !risk_->VaildBianOrdersCountbyDurationOneMin(rSize_)){
            //     resetRcSize();
            //     return;
            // }            
            order.orderType = ORDERTYPE_LIMIT_CROSS; // ?
            string timeInForce = "IOC";
            memcpy(order.TimeInForce, timeInForce.c_str(), min(uint16_t(TimeInForceLen), uint16_t(timeInForce.size())));
            string Category = "inverse";
            string CoinType = "PERP";

            memcpy(order.Category, Category.c_str(), min(uint16_t(CategoryLen), uint16_t(Category.size())));
            memcpy(order.MTaker, FEETYPE_TAKER.c_str(), min(uint16_t(MTakerLen), uint16_t(FEETYPE_TAKER.size())));
            memcpy(order.CoinType, CoinType.c_str(), min(uint16_t(CoinTypeLen), uint16_t(CoinType.size())));

            LOG_INFO << "hedge sy1 ask_price: " <<  sy1.ask_price << ", taker_qty: " << taker_qty
                << ", delta_pos_notional: " << delta_pos_notional <<  ", hedge_pos_thresh2: " << hedge_pos_thresh2;
 
            setOrder(sy1.strategyInstrument, INNER_DIRECTION_Buy,
                            sy1.ask_price,
                            taker_qty,
                            order);
        }
    }
}

void StrategyUCArb::update_thresh() 
{
    // if (IS_DOUBLE_LESS_EQUAL(pos_adj_amount * symbol2_pos_adj, 0)) {
    //     LOG_FATAL << "pos_adj_amount: " << pos_adj_amount << ", symbol2_pos_adj: " << symbol2_pos_adj;
    // }    
    pos_adj = sy2.real_pos / pos_adj_amount * symbol2_pos_adj;

    buy_thresh = - min(max(maker_threshold + pos_adj, thresh_min), thresh_max);
    sell_thresh = min(max(maker_threshold - pos_adj, thresh_min), thresh_max);

    // if (IS_DOUBLE_GREATER(buy_thresh, (-bottom_thresh))) {
    //     buy_thresh = -bottom_thresh;
    // }

    // if (IS_DOUBLE_LESS(sell_thresh, bottom_thresh)) {
    //     sell_thresh = bottom_thresh;
    // }

    if (IS_DOUBLE_GREATER(abs(sy2.real_pos) * sy2.mid_price, max_position_limit)) {  //# 仓位cap
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

// 强平检查
void StrategyUCArb::OnForceCloseTimerInterval() {
    if (enable_hedge > 0) {
        LOG_DEBUG << "OnForceCloseTimerInterval: " << enable_hedge;
        hedge();
    }
}

//# qry_position_bal 查询保证金 # 60s
void StrategyUCArb::OnTimerTradingLogic()
{
    if (CURR_MSTIME_POINT - on_time_60 < 60000) return;
    on_time_60 = CURR_MSTIME_POINT;
    uint64_t now_ns = CURR_NSTIME_POINT;

    try {
        if (first_time60) {     
            if (IS_DOUBLE_ZERO(sy1.bid_price) || IS_DOUBLE_ZERO(sy2.bid_price)) return;
            
            over_max_delta_limit();
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

        over_max_delta_limit();

        LOG_INFO << "OnTimerTradingLogic time: " << now_ns
            << ", symbol1_bid: " << sy1.bid_price
            << ", symbol1_ask: " << sy1.ask_price
            << ", symbol2_bid: " << sy2.bid_price
            << ", symbol2_ask: " << sy2.ask_price
            << ", buy_thresh: " << -round_p(10000 * buy_thresh, 1)
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

        if (enable_hedge == 0)
            LOG_INFO << "enable_hedge is false";
        if (enable_taker == 0)
            LOG_INFO << "enable_taker is false";
        if (enable_maker == 0) {
            LOG_INFO << "---------Maker Action bid---------";
            if (buyPriceMap2->size() != 0) {
                for(auto it = buyPriceMap2->rbegin(); it != buyPriceMap2->rend(); it++) {
                    for (auto& iter : it->second->OrderList) {
                        LOG_INFO << "Bid pending: " << buyPriceMap2->size() << ", OrderRef: " << iter.OrderRef << ", LimitPrice: " << iter.LimitPrice
                            << ", VolumeTotalOriginal: " << iter.VolumeTotalOriginal;
                    }
                }
            }
            if (sellPriceMap2->size() != 0) {
                for(auto it = sellPriceMap2->rbegin(); it != sellPriceMap2->rend(); it++) {
                    for (auto& iter : it->second->OrderList) {
                        LOG_INFO << "Sell pending: "<< sellPriceMap2->size() << ", OrderRef: " << iter.OrderRef << ", LimitPrice: " << iter.LimitPrice
                            << ", VolumeTotalOriginal: " << iter.VolumeTotalOriginal;
                    }
                }
            }
        }
        LOG_INFO << "param stop_trade: " << stop_trade << ", enable_taker: " << enable_taker
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
            << ", delay_plus: " << delay_plus;
    }

    catch (std::exception ex) {
        LOG_ERROR << "OnTimerTradingLogic exception occur:" << ex.what();

    }
}

void StrategyUCArb::OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    over_max_delta_limit();
    if (enable_hedge > 0)
        hedge();
    update_thresh();
    LOG_INFO << "OnPartiallyFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
}

void StrategyUCArb::OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    over_max_delta_limit();
    if (enable_hedge > 0)
        hedge();
    update_thresh();
    LOG_INFO << "OnFilledTradingLogic fee: " << rtnOrder.Fee << ", f_prx: " << rtnOrder.Price << ", f_qty: " << rtnOrder.Volume
        << ", side: " << rtnOrder.Direction << ", localId: " << rtnOrder.OrderRef 
        << ", symbol: " << rtnOrder.InstrumentID
        << ", delta_pos: " << delta_pos_notional
        << ", sy1.real_pos: " << sy1.real_pos
        << ", sy2.real_pos: " << sy2.real_pos;
}

void StrategyUCArb::OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument)
{
try {
    uint64_t now_ns = CURR_NSTIME_POINT;
    if (strcmp(sy2.symbol, marketTrade.InstrumentID) == 0 || IS_DOUBLE_LESS_EQUAL(marketTrade.Price, 0)) return;
    if (sy1.exch_ts > marketTrade.EpochTime * 1000000) return;
    sy1.exch_ts = marketTrade.EpochTime * 1000000;
    sy1.recv_ts = now_ns;
    if ((marketTrade.Direction == INNER_DIRECTION_Buy && IS_DOUBLE_EQUAL(marketTrade.Price, sy1.ask_price)) || (marketTrade.Direction == INNER_DIRECTION_Sell && IS_DOUBLE_EQUAL(marketTrade.Price, sy1.bid_price))) return;
    if (news == 0) return;
    double ask = kMaxPrice;
    double bid = kMinPrice;
    // to do log side 是买的话打印 ob ask(没改之前)， 反之ob buy; exch(只收bybit的trade), 
    if (marketTrade.Direction == INNER_DIRECTION_Buy) {
        LOG_INFO << "trade logic before: " << marketTrade.ExchangeID
            << ", InstrumentID: " << marketTrade.InstrumentID
            << ", side: " << marketTrade.Direction
            << ", price: " << marketTrade.Price
            << ", ask1: " << sy1.ask_price
            << ", EpochTime: " << marketTrade.EpochTime * 1000000
            << ", tradedelay: " << (now_ns - marketTrade.EpochTime * 1000000);
    } else if (marketTrade.Direction == INNER_DIRECTION_Sell) {
        LOG_INFO << "trade logic before: " << marketTrade.ExchangeID
            << ", InstrumentID: " << marketTrade.InstrumentID
            << ", side: " << marketTrade.Direction
            << ", price: " << marketTrade.Price
            << ", bid1: " << sy1.bid_price
            << ", EpochTime: " << marketTrade.EpochTime * 1000000
            << ", tradedelay: " << (now_ns - marketTrade.EpochTime * 1000000);
    } else {
        LOG_FATAL << "side wrong";
    }
    if (marketTrade.Direction == INNER_DIRECTION_Sell) {
        bid = marketTrade.Price;
        ask = marketTrade.Price + sy1.prc_tick_size;
        if (1 == news)
            ask = max(ask, sy1.ask_price);
        else if (2 == news)
            ask = max(ask, min(round1(marketTrade.Price * (1 + news_thresh/10000), sy1.prx_decimal, sy1.prc_tick_size), sy1.ask_price));
        sy1.update_price(bid, ask, now_ns, TriggerSource::Trade);
    }
    else if (marketTrade.Direction == INNER_DIRECTION_Buy) {
        ask = marketTrade.Price;
        bid = marketTrade.Price - sy1.prc_tick_size;
        if (1 == news)
            bid = min(bid, sy1.bid_price);
        else if (2 == news)
            bid = min(bid, max(round1(marketTrade.Price * (1 - news_thresh/10000), sy1.prx_decimal, sy1.prc_tick_size), sy1.bid_price));
        sy1.update_price(bid, ask, now_ns, TriggerSource::Trade);
    }
    // to do log side 是买的话打印 ob ask（改了之后）， 反之ob buy; exch(只收bybit的trade), 

    LOG_INFO << "trade logic after: " << marketTrade.ExchangeID
        << ", InstrumentID: " << marketTrade.InstrumentID
        << ", side: " << marketTrade.Direction
        << ", price: " << marketTrade.Price
        << ", ask1: " << sy1.ask_price
        << ", bid1: " << sy1.bid_price
        << ", EpochTime: " << marketTrade.EpochTime * 1000000
        << ", tradedelay: " << (now_ns - marketTrade.EpochTime * 1000000);

    if (stop_trade) return;
    if ((enable_taker > 0) && (now_ns - sy1.exch_ts < tolerance_delay * 1e9)) {
        flag_send_order = taker_action();
        if (flag_send_order > 0)
            log_delay("tradeRtn taker", marketTrade.ExchangeID);
    }
    if (enable_maker > 0) {
        flag_send_order = maker_action();
        if (flag_send_order > 0)
            log_delay("tradeRtn maker", marketTrade.ExchangeID);
    }
    sy2.resetTriggerSource();
}
catch (std::exception ex) {
    LOG_ERROR << "OnRtnTradeTradingLogic exception occur:" << ex.what();
}

}

void StrategyUCArb::OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument)
{
    uint64_t now_ns = CURR_NSTIME_POINT;
    if (strcmp(sy2.symbol, marketData.InstrumentID) == 0) {
        if (sy2.exch_ts > marketData.EpochTime * 1000000) {
            LOG_ERROR << "OnRtnInnerMarketDataTradingLogic sy2 exch_ts: " << sy2.exch_ts
                << ", symbol: " << sy2.symbol
                << ", EpochTime: " << marketData.EpochTime * 1000000 << ", trigger_source: " << sy2.trigger_source;    
            return;
        }
        LOG_INFO << "marketData exch: " << marketData.ExchangeID 
            << ", symbol: " << marketData.InstrumentID
            << ", ask1: " << marketData.AskPrice1 
            << ", bid1: " << marketData.BidPrice1 
            << ", EpochTime: " << marketData.EpochTime * 1000000
            << ", marketdelay: " << (now_ns - marketData.EpochTime * 1000000);
        sy2.update_price(marketData.BidPrice1, marketData.AskPrice1, now_ns, TriggerSource::BestPrice);
    }

    if (strcmp(sy1.symbol, marketData.InstrumentID) == 0) {
        if (sy1.exch_ts > marketData.EpochTime* 1000000) {
            LOG_DEBUG << "OnRtnInnerMarketDataTradingLogic sy1 exch_ts: " << sy1.exch_ts
                << ", symbol: " << marketData.InstrumentID
                << ", EpochTime: " << marketData.EpochTime * 1000000 << ", trigger_source: " << sy1.trigger_source;  
            return;
        }
        sy1.exch_ts = marketData.EpochTime * 1000000;
        sy1.recv_ts = now_ns;

        if (IS_DOUBLE_EQUAL(marketData.BidPrice1, sy1.bid_price) && IS_DOUBLE_EQUAL(marketData.AskPrice1, sy1.ask_price)) {
            // LOG_ERROR << "OnRtnInnerMarketDataTradingLogic BidPrice1: " << marketData.BidPrice1
            //     << ", AskPrice1: " << marketData.AskPrice1;
            return;
        }
        LOG_INFO << "marketData exch: " << marketData.ExchangeID 
            << ", symbol: " << marketData.InstrumentID
            << ", ask1: " << marketData.AskPrice1 
            << ", bid1: " << marketData.BidPrice1 
            << ", EpochTime: " << marketData.EpochTime * 1000000
            << ", marketdelay: " << (now_ns - marketData.EpochTime * 1000000);
        sy1.update_price(marketData.BidPrice1, marketData.AskPrice1, now_ns, TriggerSource::BestPrice);
    }

    if (stop_trade) return;
    if (enable_taker > 0 && (now_ns - sy1.exch_ts) < tolerance_delay * 1e9) {
        flag_send_order = taker_action();
        if (flag_send_order > 0)
            log_delay("MarketRtn taker", marketData.ExchangeID);
    }
    if (enable_maker > 0) {
        int flag_send_order = maker_action();
        if (flag_send_order > 0)
            log_delay("MarketRtn maker", marketData.ExchangeID);
    }
    sy2.resetTriggerSource();
}

void StrategyUCArb::OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument)
{
    if (rtnOrder.InstrumentID == symbol1) {
        LOG_INFO << "OnCanceledTradingLogic symbol: " << rtnOrder.InstrumentID << ", orderRef: "
            << rtnOrder.OrderRef << ", status: " << rtnOrder.OrderStatus;
        hedge();
    }
}


void StrategyUCArb::log_delay(string s, string exch)
{
    uint64_t now_ns = CURR_NSTIME_POINT;
    double delay_ts = now_ns - sy1.exch_ts;
    string day = TradingDay::getToday();
    LOG_INFO << "monitorlog flag: " << s 
            << ", exch: " << exch
            << ", date: " << day << ", now: " << now_ns << ", delay: " << delay_ts
            << ", delay_ms: " << delay_ts/1e6
            << ", stop_trade: " << stop_trade
            << ", buy_thresh: " << buy_thresh
            << ", sell_thresh: " << sell_thresh
            <<", ask1: " << sy1.ask_price
            <<  ", bid1: " << sy1.bid_price
            << ", sy2 midPrice: " << sy2.mid_price
            << ", tolerance_delay: " << tolerance_delay
            << ", flag_send_order: " << flag_send_order
            <<  ", exchange_ts: " << sy1.exch_ts
            <<  ", md_recv_ts: " << sy1.recv_ts;
}

