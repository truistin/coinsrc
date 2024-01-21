#ifndef SPOT_BASE_STRATEGYPARAMETER_H
#define SPOT_BASE_STRATEGYPARAMETER_H
#include<unordered_map>
#include "spot/base/DataStruct.h"
using namespace std;


namespace spot
{
	namespace base
	{
		class StrategyParameter
		{
		public:
			StrategyParameter(int strategyID);
			
			// accessor
			double* getDouble(string key);
			int*    getInt(string key);
			const char* getString(string key);

			int* getIsForcedOpen() { return isForcedOpen_; }

			int* getCbMinSetOrderIntervalMs() { return cbMinSetOrderIntervalMs_; }
			double* getTcMaxPosition() { return tcMaxPosition_; }
			int* getCbPendingOrderTimeoutLimitSec() { return cbPendingOrderTimeoutLimitSec_; }
			int* getCbOrderBookStaleTimeLimitSec() { return cbOrderBookStaleTimeLimitSec_; }
			int* getTcMaxTrades() { return tcMaxTrades_; }

			int* getBianOrdersPerMin() { return bianOrdersPerMin_; }
			int* getBianOrdersPerTenSec() { return bianOrdersPerTenSec_; }
			int* getBianOrderSecStep() { return bianOrderSecStep_; }

			int* getOkxOrdersPer2Sec() { return okexOrdersPer2kMsec_; }
			int* getOkxOrderMsecStep() { return okexOrder2kMsecStep_; }

			int* getByReqOrdPers() { return byReqOrdPers_; }
			int* getByCanOrdPers() { return byCanOrdPers_; }

			//int* getTcMaxPosition() { return new int(100); }

			StrategyParam* getStrategyParam(string key);
			int* getCbMaxCancelAttempts() { return cbMaxCancelAttempts_; }

			// mutator
			bool setStrategyParameter(const StrategyParam &strategyParam);
			void initStrategyParameters();
			inline unordered_map<string, StrategyParam>& paramtersMap();
			bool IsExistDouble(string key);

		private:
			unordered_map<string, StrategyParam> strategyParamtersMap_;
			int strategyID_;

			int* isForcedOpen_;
			double* tcMaxPosition_;
			int* cbPendingOrderTimeoutLimitSec_;
			int* cbOrderBookStaleTimeLimitSec_;

			// strategycircuit
			int* cbMinSetOrderIntervalMs_;

			// tradingControl
			int* tcMaxTrades_;
			int* cbMaxCancelAttempts_;

			// bian order gap limit
			int* bianOrdersPerMin_; // orders sum per min
			int* bianOrdersPerTenSec_; // orders sum 10 secs, [0~10),[10,20)...[50,60)

    		int* bianOrderSecStep_; //10s

			int* okexOrdersPer2kMsec_; // ok cancel&order 60(/2s)
			int* okexOrder2kMsecStep_; 

			int* byReqOrdPers_;
			int* byCanOrdPers_;

		};
		static string getStrategyParamString(const StrategyParam &strategyParam)
		{
			char buff[128];
			memset(buff, 0, sizeof(buff));
			if (DOUBLETYPE.compare(strategyParam.ValueType) == 0)
			{
				SNPRINTF(buff, sizeof(buff), " Name=%s, Value=%f ", strategyParam.Name, strategyParam.ValueDouble);
			}
			else if (INTTYPE.compare(strategyParam.ValueType) == 0)
			{
				SNPRINTF(buff, sizeof(buff), " Name=%s, Value=%d ", strategyParam.Name, strategyParam.ValueInt);
			}
			else if (STRINGTYPE.compare(strategyParam.ValueType) == 0)
			{
				SNPRINTF(buff, sizeof(buff), " Name=%s, Value=%s ", strategyParam.Name, strategyParam.ValueString);
			}
			return string(buff);
		};
		unordered_map<string, StrategyParam>& StrategyParameter::paramtersMap()
		{
			return strategyParamtersMap_;
		}
	}
}
#endif