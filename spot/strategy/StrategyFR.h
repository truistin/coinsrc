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

		struct sy_info {
			public:
				sy_info() {
					memset(this, 0, sizeof(sy_info));
				}
			public:
				char sy[40];
				char ref_sy[40];
				char type[10];
				double mid_p;
				double ask_p;
				double bid_p;
				double ask_v;
				double bid_v;
				int make_taker_flag;
				int long_short_flag;
				double prc_tick_size;
				double qty_tick_size;
				double qty;
				double mv_ratio;
				double thresh;
				int64_t exch_ts;
				double real_pos;
				StrategyInstrument *inst;
				OrderByPriceMap* sellMap;
				OrderByPriceMap* buyMap;
			public:
				update(double askp, double bidp, double askv, double bidv) {
					ask_p = askp;
					bid_p = bidp;
					ask_v = askv;
					bid_v = bidv;
					mid_p = (askp + bidp) / 2;
				}
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
			bool over_max_delta_limit();
        private:
            StrategyFR(int strategyID, StrategyParameter *params);

			double get_usdt_equity();
			double calc_predict_mm(order_fr& order, double price_cent);
			double calc_predict_equity(order_fr& order, double price_cent);
			double calc_future_uniMMR(string symbol, double qty);
			double calc_equity();
			double calc_mm();
			double calc_balance();
			void get_cm_um_brackets(string symbol, double val, double& mmr_rate, double& mmr_num);

		private:
			map<string, double>* margin_leverage;
			map<double, double>* margin_mmr;

		public:
			map<string, double>* last_price_map;
			double um_leverage;

			double pre_sum_equity;
			double price_ratio;
			map<string, double>* pridict_borrow;
			map<string, sy_info>* make_taker;
			map<string, double>* delta_mp;
		};
    }
}
#endif