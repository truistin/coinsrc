#ifndef SPOT_STRATEGY_STRATEGYFR_H
#define SPOT_STRATEGY_STRATEGYFR_H

#include "spot/base/DataStruct.h"
#include "spot/strategy/Strategy.h"
#include "spot/strategy/StrategyInstrument.h"
#include "spot/strategy/StrategyCircuit.h"
#include "spot/risk/spotRisk.h"
using namespace std;
using namespace spot::risk;
namespace spot {
    namespace strategy {
		struct order_fr {
			public:
				order_fr() {
					sy = "";
					qty = 0;
					borrow = 0;
				}
			public:
				string sy;
				double qty;
				double borrow;
		};
        class StrategyFR : public StrategyCircuit {
        public:
            static Strategy *Create(int strategyID, StrategyParameter *params);

            void init();

            virtual void
            OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument);

            virtual void OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

            virtual void OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

			virtual void OnTimerTradingLogic(){};

            virtual void OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument);
            virtual void OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			void OnForceCloseTimerInterval(){};
        private:
            StrategyFR(int strategyID, StrategyParameter *params);

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
			double bottom_thresh;

			OrderByPriceMap*  buyPriceMap1;
			OrderByPriceMap*  sellPriceMap1;
			OrderByPriceMap*  buyPriceMap2;
			OrderByPriceMap*  sellPriceMap2;
			map<string, double>* margin_leverage;
			map<double, double>* margin_mmr;

		public:
			map<string, double>* last_price_map;
			uint32_t cSize_;
			uint32_t rSize_;
			// make: postonly pending   take :ioc pending   hedge: ioc pending
			//ioc: ( sy1: hedge , sy2:take_action)
			spotRisk* risk_; // taker_action maker_action only exchange2 ,hedge only exchange1.
			uint64_t on_time_60;
			double um_leverage;

			double pre_sum_equity;
			double price_ratio;
			map<string, double>* pridict_borrow;
		};
    }
}
#endif