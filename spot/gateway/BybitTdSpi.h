#ifndef SPOT_BybitTdSpi_H
#define SPOT_BybitTdSpi_H
#include <set>
#include "spot/utility/Compatible.h"
#include <spot/base/DataStruct.h>
#include <spot/bybit/BybitApi.h>
#include "spot/utility/Mutex.h"
#include "spot/websocket/websocketapi.h"
#include "spot/restful/Uri.h"
#include "spot/base/AdapterCrypto.h"
#include "spot/restful/CurlMultiCurl.h"

//using namespace spot;
using namespace spot::base;
namespace spot
{
	namespace gtway
	{
		class OrderStoreCallBack;		
		class BybitTdSpi : public AdapterCrypto
		{
		public:
			BybitTdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore);
			~BybitTdSpi();
			void Init();
			void Run();
			int ReqOrderInsert(const Order &innerOrder);
			int ReqOrderCancel(const Order &innerOrder);
			int Query(const Order &order);
			void PutOrderToQueue(BybitWssRspOrder& bybitOrder);
            void com_callbak_open();
            void com_callbak_close();
            void com_callbak_message(const char *message);
			void com_login_message(const char *message);
			void com_subscribe_message(const char *message);
			void subscribe(const std::string &channel);

			int httpAsynReqCallback(char * result, uint64_t clientId);
			int httpAsynCancelCallback(char * result, uint64_t clientId);
			void auth();
			void uriPingOnHttp(char* message, uint64_t clientId);
			int httpQryOrderCallback(char * result, uint64_t clientId);

		private:
			std::mutex mutex_;
			MutexLock mutex1_;
			BybitApi *traderApi_;
			WebSocketApi *websocketApi_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			OrderStoreCallBack *orderStore_;
			map<uint64_t, Order> orderMap_;
			set<uint64_t> finishedOrders_;
			list<string> m_accountList;
		};
	}
}

#endif