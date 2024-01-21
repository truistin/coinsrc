#ifndef SPOT_BianTdSpi_H
#define SPOT_BianTdSpi_H
#include <set>
#include "spot/utility/Compatible.h"
#include <spot/base/DataStruct.h>
#include <spot/bian/BnApi.h>
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
		class BianTdSpi : public AdapterCrypto
		{
		public:
			BianTdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore);
			~BianTdSpi();
			void Init();
			void Run();
			int ReqOrderInsert(const Order &innerOrder);
			int ReqOrderCancel(const Order &innerOrder);
			int Query(const Order &order);
			void PutOrderToQueue(BianWssRspOrder& bianOrder);
            void com_callbak_open();
            void com_callbak_close();
            void com_callbak_message(const char *message);
            void PostponeListenKey();
			void getListkey();
			int httpAsynReqCallback(char * result, uint64_t clientId);
			int httpAsynCancelCallback(char * result, uint64_t clientId);
			void httpAsynPostponeListenKey(char * result, uint64_t clientId);
			void uriPingOnHttp(char* message, uint64_t clientId);
			int httpQryOrderCallback(char * result, uint64_t clientId);

		private:
			std::mutex mutex_;
			MutexLock mutex1_;
			BnApi *traderApi_;
			WebSocketApi *websocketApi_;
			WebSocketApi *websocketApi1_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			OrderStoreCallBack *orderStore_;
			map<uint64_t, Order> orderMap_;
			set<uint64_t> finishedOrders_;
			list<string> m_accountList;

			std::string listenKey_;
		};
	}
}

#endif