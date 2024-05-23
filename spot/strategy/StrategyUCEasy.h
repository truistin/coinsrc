#ifndef SPOT_STRATEGY_STRATEGYUCEASY_H
#define SPOT_STRATEGY_STRATEGYUCEASY_H

#include "spot/base/DataStruct.h"
#include "spot/strategy/Strategy.h"
#include "spot/strategy/StrategyInstrument.h"
#include "spot/strategy/StrategyCircuit.h"
#include "spot/risk/spotRisk.h"
using namespace std;
using namespace spot::risk;
namespace spot {
    namespace strategy {
		struct order_uc {
			public:
				order_uc() {
					sy = "";
					ref_sy = "";
					qty = 0;
					ref_qty = 0;
					borrow = 0;
				}
			public:
				string sy;
				string ref_sy;
				double qty;
				double ref_qty;
				double borrow;
		};

		struct uc_info {
			public:
				uc_info() {
					memset(this, 0, sizeof(uc_info));
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
				double avg_price;
				int make_taker_flag; // 1 maker
				int long_short_flag; // 1 short
				double min_amount;
				double fragment; // fragment 
				double force_close_amount; // adl 
				double prc_tick_size;
				double qty_tick_size;
				double qty;
				double mv_ratio;
				double step_thresh;
				double thresh; // arb close
				double buy_thresh;
				double sell_thresh;
				double pos_adj;
				int close_flag;
				int um_leverage;
				double price_ratio;
				int64_t exch_ts;
				double real_pos;
				uc_info* ref;
				StrategyInstrument *inst;
				OrderByPriceMap* sellMap; // pendingorders
				OrderByPriceMap* buyMap;
			public:
				void update(double askp, double bidp, double askv, double bidv, int64_t ts) {
					ask_p = askp;
					bid_p = bidp;
					ask_v = askv;
					bid_v = bidv;
					exch_ts = ts;
					mid_p = (askp + bidp) / 2;
				}
		};

        class StrategyUCEasy : public StrategyCircuit {
        public:
            static Strategy *Create(int strategyID, StrategyParameter *params);

            void init();

            virtual void
            OnRtnInnerMarketDataTradingLogic(const InnerMarketData &marketData, StrategyInstrument *strategyInstrument);

            virtual void OnPartiallyFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

            virtual void OnFilledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);

			virtual void OnTimerTradingLogic(){};

            virtual void OnRtnTradeTradingLogic(const InnerMarketTrade &marketTrade, StrategyInstrument *strategyInstrument){};
            virtual void OnCanceledTradingLogic(const Order &rtnOrder, StrategyInstrument *strategyInstrument);
			void OnForceCloseTimerInterval();
			bool over_max_delta_limit(uc_info& sy1, uc_info& sy2);
			bool check_min_delta_limit(uc_info& sy1, uc_info& sy2);
        private:
            StrategyUCEasy(int strategyID, StrategyParameter *params);
			void Mr_ClosePosition(StrategyInstrument *strategyInstrument);
			void Mr_Market_ClosePosition(StrategyInstrument *strategyInstrument);

			bool IsExistOrders(uc_info* sy, int side);
			void update_thresh(StrategyInstrument *strategyInstrument);
			void qryPosition();
			bool is_continue_mr(uc_info*, double qty);
			bool action_mr(double mr);
			void hedge(StrategyInstrument *strategyInstrument);

			double get_usdt_equity();
			double calc_predict_mm(uc_info& info, order_uc& order, double price_cent);
			double calc_predict_equity(uc_info& info, order_uc& order, double price_cent);
			double calc_future_uniMMR(uc_info& info, double qty);
			double calc_equity();
			double calc_mm();
			double calc_balance();
			void get_cm_um_brackets(string symbol, double val, double& mmr_rate, double& mmr_num);
			double calc_uniMMR();
			string GetCMSymbol(string inst);
			string GetUMSymbol(string inst);
			string GetSPOTSymbol(string inst);
			double getSpotAssetSymbol(string asset);
			bool make_continue_mr(double& mr);

		private:
			map<string, double>* margin_leverage;
			map<double, double>* margin_mmr;

		public:
			map<string, double>* last_price_map;
			double um_leverage;

			double price_ratio;
			map<string, double>* pridict_borrow;
			map<string, uc_info>* make_taker;
			map<string, string>* symbol_map;

			double disaster_tol_thresh;
			double thresh_min;
			double thresh_max;

			atomic<bool> en_maker;
		};
    }
}
#endif