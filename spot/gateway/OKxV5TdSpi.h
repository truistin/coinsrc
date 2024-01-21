#ifndef SPOT_SPOTOKEV5TRADERSPI_H
#define SPOT_SPOTOKEV5TRADERSPI_H

#include <spot/base/DataStruct.h>
#include "spot/base/Instrument.h"
#include "spot/rapidjson/document.h"
#include "spot/rapidjson/rapidjson.h"
#include "spot/utility/Mutex.h"
#include "spot/okex/OkSwapApi.h"
#include <spot/gateway/OrderStoreCallBack.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <set>
#include "spot/websocket/websocketapi.h"
#include "spot/base/AdapterCrypto.h"

//using namespace spot;
using namespace spot::base;
using namespace spot::strategy;
namespace spot
{
	namespace gtway
	{
		class OrderStoreCallBack;
		class OKxV5TdSpi : public AdapterCrypto
		{
		public:
			OKxV5TdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, OrderStoreCallBack *orderStore);
			~OKxV5TdSpi();
			void Init();
			void Run();
			int ReqOrderInsert(const Order &innerOrder);
			int ReqOrderCancel(const Order &innerOrder);
			int Query(const Order &order);
			void PutOrderToQueue(OKxWssSwapOrder &queryOrder, Order &preOrder);
			void com_callbak_open();
            void com_callbak_close();
            void com_callbak_message(const char *message);
			void com_login_message(const char *message);
			void com_subscribe_message(const char *message);
			int httpAsynReqCallback(char * result, uint64_t clientId);
			int httpAsynCancelCallback(char * result, uint64_t clientId);

			string CreateSignature(string& timestamp, string& method, string& requestPath, 
	string& sk, const char* encode_mode);

			OkSwapApi *GetTradeApi() { return traderApi_; }
			void auth();
			void subscribe(const std::string &channel);

		public:
			spotrapidjson::Document documentCOM_;
			WebSocketApi *okWebsocketApi_;
			OkSwapApi *traderApi_;
			std::shared_ptr<ApiInfoDetail> apiInfo_;
			OrderStoreCallBack *orderStore_;
			MutexLock mutex1_;
			std::mutex mutex_;
		private:
			bool loginFlag_;
		};
		
	}
}

#endif