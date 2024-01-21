#ifndef SPOT_STRATEGY_STRATEGYBTCARB1_H
#define SPOT_STRATEGY_STRATEGYBTCARB1_H

#include "spot/base/DataStruct.h"
#include "spot/strategy/Strategy.h"
#include "spot/strategy/StrategyInstrument.h"
#include "spot/strategy/StrategyCircuit.h"
#include "spot/risk/spotRisk.h"
using namespace std;
using namespace spot::risk;
namespace spot {
    namespace strategy {
		enum TriggerSource {
			Unknown = 1,
    		BestPrice,
			Trade,
    		TPrice		
		};
		struct SyInfo
		{
			char symbol[InstrumentIDLen+1];
			uint64_t mp_update_t; // mark price updatetime
			uint64_t update_ts; // liq price
			double entry_price;
			double real_pos;
			double bid_price;
			double ask_price;
			double mid_price;

			double mark_price;
			double liq_price;

			double max_qty;
			double min_qty;
        	uint64_t last_taker_time;
        	double prx_decimal;
        	double qty_decimal;
			double prc_tick_size;
			double qty_tick_size;

			double bid_place_price;
			double ask_place_price;
			double bid_fair_price;
			double ask_fair_price;
			double bid_last_place_price;
			double ask_last_place_price;
			double sy_avg_midprice;
			int64_t mid_num;

			// double pos_notional;
			// double realizedpnl;
			// double coin_pos;
			// double avg_entry_price;

			uint64_t recv_ts;
			uint64_t exch_ts;
			
			double multiple;
			int trigger_source;

			StrategyInstrument *strategyInstrument;
			
			void update_price(double bid_p, double ask_p, uint64_t ts, int tp)
			{
				// LOG_INFO << "update_price: " << symbol << ", trigger_source: " << trigger_source;
				bid_price = bid_p;
				ask_price = ask_p;
				mid_price = (bid_p + ask_p) / 2.0;
				sy_avg_midprice = (sy_avg_midprice + mid_price) / (++mid_num);
				recv_ts = ts;
				trigger_source = tp;
			}
			void resetTriggerSource()
			{
				trigger_source = TriggerSource::Unknown;
			}

			SyInfo()
			{
				memset(this, 0, sizeof(SyInfo));
			}
		};
		
        class StrategyUCArb1 : public StrategyCircuit {
        public:
            static Strategy *Create(int strategyID, StrategyParameter *params);

            void init();

            virtual void
            OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument);

            virtual void OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

            virtual void OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

			virtual void OnTimerTradingLogic();

            virtual void OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument);
            virtual void OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			void OnForceCloseTimerInterval();
			int getSellPendingLen(SyInfo& sy);
			int getBuyPendingLen(SyInfo& sy);

        private:
            StrategyUCArb1(int strategyID, StrategyParameter *params);
			int getPoPriceLevel(OrderByPriceMap*  priceMap);
			void qryPosition();
			void resetRcSize();
			int taker_action();
			int maker_action();
			int getIocOrdPendingLen(SyInfo& sy);
			double get_tol_delay();
			double po_best_price(OrderByPriceMap*  priceMap, int side);
			void over_max_delta_limit();
			void hedge();
			void update_thresh();
			void log_delay(string s, string exch);
			void initInstrument();
			bool vaildPrice(SyInfo& sy);
			bool VaildCancelTime(Order& order, uint8_t loc);

		private:
			bool first_time60;
			int	enable_taker;
			int	enable_hedge;
			int	enable_maker;
			double	symbol2_taker_time_gap;
			int	taker_level_limit;
			double	hedge_time_gap;
			double	hedge_pos_thresh2;
			double	hedge_taker_ratio;
			double	taker_threshold_perc;
			double	taker_threshold_incr;
			double	symbol2_taker_notional;
			double	max_delta_limit;
			int	arbitrage_mode;
			double	symbol2_multiplier;
			double	symbol2_pos_adj;
			double	max_position_limit;
			double	disaster_tol_thresh;
			double	maker_threshold;
			double	thresh_max;
			double	thresh_min;
			double	batch_sensitivity;
			int	cancel_order_interval;
			double	symbol2_maker_notional;
			double	order_distance;
			int	order_level;
			int	level_tolerance;
			int	news;
			double	news_thresh;
			double	ob_tolerance_seconds;
			double	trade_tolerance_seconds;
			double	special_trade_tolerance_seconds;
			double	tolerance_delay;
			double	delay_plus;

			int flag_send_order; 
			uint64_t last_hedge_time;
			uint64_t last_taker_time;
			SyInfo sy1;
			SyInfo sy2;
			bool ready;
			double buy_thresh;
			double sell_thresh;

			double delta_pos_notional;

			double kMinPrice;
			double kMaxPrice;

			double pos_adj;
			string symbol1;
			string symbol2;
			string exch1;
			string exch2;
			int trigger_source;
			bool stop_trade;
			double pos_adj_amount;

			OrderByPriceMap*  buyPriceMap1;
			OrderByPriceMap*  sellPriceMap1;
			OrderByPriceMap*  buyPriceMap2;
			OrderByPriceMap*  sellPriceMap2;

			atomic<double> step_thresh;

			uint64_t last_time_stamp;

		public:
			uint32_t cSize_;
			uint32_t rSize_;
			// make: postonly pending   take :ioc pending   hedge: ioc pending
			//ioc: ( sy1: hedge , sy2:take_action)
			spotRisk* risk_; // taker_action maker_action only exchange2 ,hedge only exchange1.
			uint64_t on_time_60;


		};
    }
}
#endif