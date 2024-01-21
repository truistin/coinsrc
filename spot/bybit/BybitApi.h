#ifndef __Bybit_API_H__
#define __Bybit_API_H__
#include <string>
#include "spot/restful/Uri.h"
#include "spot/bybit/BybitStruct.h"
#include "spot/base/Instrument.h"
#include "spot/base/AdapterCrypto.h"

using namespace spot::base;
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTP_DELETE 2
#define ENCODING_GZIP 0

#ifdef __TESTMODE__
#define Bybit_DOMAIN "api-testnet.bybit.com"
#else
#define Bybit_DOMAIN "api.bybit.com"
#endif //__TESTMODE__

class BybitApi	
{
public:
	BybitApi() {};
	BybitApi(string api_key, string secret_key,string customer_id, AdapterCrypto* adapt);
	~BybitApi();	
	void Init();

	bool GetServerTime(int type = 0);

	uint64_t ReqOrderInsert(const Order &order);
	int CancelOrder(const Order &innerOrder, int type = 0);
	int CancelAllOrders();

	// bool QryInfo();
    // bool QryPosiInfo();

	uint64_t GetCurrentNonce();

	static void AddInstrument(const char *instrument);

	static string GetDepthChannel(string instrument_id);
	static string GetTradeChannel(string instrument_id);
	static string GetTickerChannel(string instrument_id);

	static string GetDepthInstrumentID(string channel);
	static string GetTradeInstrumentID(string channel);
	static string GetTickerInstrumentID(string channel);

	//btcusdt@depth5
	static std::map<string, string> GetDepthInstrumentMap();
	static std::map<string, string> GetTradeInstrumentMap();
	static std::map<string, string> GetTickerInstrumentMap();

	static string GetCurrencyPair(string inst);

	void uriQryPosiCallbackOnHttp(char* result, uint64_t clientId);
	void uriQryMarkPCallbackOnHttp(char* result, uint64_t clientId);
	void QryAsynInfo();
	
	void uriReqCallbackOnHttp(char* message, uint64_t clientId);
	void uriCanCallbackOnHttp(char* message, uint64_t clientId);
	int QryPosiBySymbol(const Order &order);
	void UpdateAccount();
	int QryAsynOrder(const Order& order);

private:
	void uriQryOrderOnHttp(char* message, uint64_t clientId);
	void SetPrivateParams(int mode, Uri& m_uri, int encode_mode = -1);
	string CreateSignature(int http_mode, string secret_key, Uri& m_uri, const char* encode_mode);
    void ConvertQuantityAndPrice(const Order &order, Uri& m_uri);
private:
	string m_api_key;
	string m_secret_key;
	string m_customer_id;
	string m_passphrase;
	Uri m_uri;
	uint64_t m_nonce;	
	int64_t m_timeLagency;
	InnerMarketData m_field;
	map<string ,int> queryFailedCountMap;
	map<string ,int> posiQueryFailedCountMap;
	static std::map<string, string> depthToInstrumentMap_;
	static std::map<string, string> tradeToInstrumentMap_;
	static std::map<string, string> tickerToInstrumentMap_;

	static std::map<string, string> originSymbolToSpotSymbol_;
	static std::vector<string> categoryVec_;

	AdapterCrypto *adapter_;
	bool cancelAll;
};
#endif
