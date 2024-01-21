#ifndef SPOT_SPOTHUOBTRADERSPI_H
#define SPOT_SPOTHUOBTRADERSPI_H
#include <set>
#include "spot/utility/Compatible.h"
#include <spot/base/DataStruct.h>
#include <spot/huobapi/HuobApi.h>
#include "spot/utility/Mutex.h"
using namespace spot;
using namespace spot::base;
namespace spot
{
	namespace gtway
	{
		class OrderStoreCallBack;		
		class HuobTraderSpi
		{
		public:
			HuobTraderSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore);
			~HuobTraderSpi();
			void SetIsCryptoFuture(bool isCryptoFuture);
			void Init();
			void Run();
			int getFutureOrderOffsetType(char direction, char offset, Instrument *instrument);
			string getFutureConstractCode(Instrument *inst);
			string getSpotConstractCode(Instrument *inst);
			
			int testOrderInsert(Order &innerOrder);
			int ReqOrderInsert(const Order &innerOrder);
			int ReqOrderCancel(const Order &innerOrder);
			void UpdateUserOrder(uint64_t order_id, Order& order, bool isByOrderId);
			void UpdateUserOrderMap(map<uint64_t, Order>& orderMap, bool isByOrderId);
			void PutOrderToQueue(HuobOrder &hbOrder, Order &order, bool isByOrderId);

		private:
			void convert2OrderMapBySymbol(map<uint64_t, Order>& orderMap, map<std::string, map<uint64_t, Order*>>& queryOrderMap);
			void updateUserOrderMapBySymbol(const std::string& symbol, map<uint64_t, Order*>& orderMap, bool isByOrderId);
			void queryOrderStatus(bool isByOrderId);

		private:
			bool	isCryptoFuture_;
			HuobApi *traderApi_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			OrderStoreCallBack *orderStore_;
			map<uint64_t, Order> orderMap_; //by OrderSysId
			set<uint64_t> finishedOrders_; //by OrderSysId

			map<uint64_t, Order> orderMap2_; //by OrderRef
			set<uint64_t> finishedOrders2_; //by OrderRef
			unsigned int queryIndex_;

			MutexLock mutex_;
			list<string> m_accountList;
		};
	}
}

#endif