#ifndef __OkSwapApi_H__
#define __OkSwapApi_H__

#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include "spot/utility/Utility.h"
#include "spot/restful/UriFactory.h"
#include "spot/restful/Uri.h"
#include "OkSwapStruct.h"
#include "spot/base/MqStruct.h"
#include "spot/base/InstrumentManager.h"
#include "spot/base/AdapterCrypto.h"
#include "spot/restful/CurlMultiCurl.h"

using namespace std;
using namespace spot::base;
//using namespace spot::gtway;

#define OKX_DOMAIN "https://www.okx.com"
class OkSwapApi
{
public:
	OkSwapApi() {};
	OkSwapApi(string api_key, string secret_key, string passphrase, AdapterCrypto* adapt);
	~OkSwapApi();

	int SetPrivateParams(std::shared_ptr<Uri> uri);
	string CreateSignature(string& timestamp, string& method, string& requestPath, string& body, const char* encode_mode);
	
	void OrderInsert(const Order* innerOrder,const string& uuid);
	string  OrderCancel(const string& instrument_id, const string& order_id);
	int QryPosiBySymbol(const Order &order);
	int CancelAllOrders();

	// bool QryPosiInfo();
	static void AddInstrument(const char *instrument);
    void ConvertQuantityAndPrice(const Order* order, std::shared_ptr<Uri> m_uri);
	void uriReqCallbackOnHttp(char* message, uint64_t clientId);
	void uriCanCallbackOnHttp(char* message, uint64_t clientId);
	// bool QryAsyPosiMPriceInfo();
	// void uriQryMarkPCallbackOnHttp(char* result, uint64_t clientId);
	// void uriQryPosiCallbackOnHttp(char* result, uint64_t clientId);


private:
	string m_api_key;
	string m_secret_key;
	string m_passphrase;
	Uri m_uri;
	UriFactory urlprotocol;
	map<string, int> queryFailedCountMap;
	map<string, int> markPxQueryFailedCountMap;

	static vector<string> okTickerToInstrumentMap_;
	bool cancelAll;
	AdapterCrypto *adapter_;

public:
	// Func funcQryPosi;
	// Func funcQryMarkPrice;
};
#endif /* __OkSwapApi_H__ */
