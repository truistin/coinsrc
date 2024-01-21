#ifndef __HUOB_API_H__
#define __HUOB_API_H__
#include <string>
#include "spot/restful/Uri.h"
#include "spot/huobapi/HuobStruct.h"
#include "spot/base/Instrument.h"

using namespace spot::base;
#define HTTP_GET 0
#define HTTP_POST 1
class HuobApi	
{
public:
	HuobApi() {};
	HuobApi(string api_key, string secret_key,string customer_id);
	~HuobApi();		

	uint64_t		GetCurrentNonce();
	static void		AddInstrument(Instrument *instrument, bool isFuture);
	static string	GetDepthChannel(Instrument *instrument, bool isFuture);
	static string	GetTradeChannel(Instrument *instrument, bool isFuture);
	static string	GetInstrumentID(string channel,bool isLong = true);
	//现货接口
	/***************************************************************/
	bool GetSymbols();
	bool GetDepthMarket(string inst, int type = 0);

	bool GetAccounts(list<string>* accountList);
	bool GetAccountBalance(string accountID);

	bool GetOrder(string orderID, HuobOrder& order);
	bool GetOpenOrders(string symbol, list<HuobOrder>& order_list);
	bool GetUserTransactions(string inst, list<HuobTransaction> &transaction_list);
	uint64_t ReqOrderInsert(HuobOrder &order);
	bool CancelOrder(string &order_id);

	static std::map<string, string> GetChannelMap();
	static string GetCurrencyPair(string inst);
	/***************************************************************/

	//期货接口
	/***************************************************************/
	bool		GetFutureConstracts();//done
	bool		GetFutureConstractIndex(string inst);//done
	bool		GetFutureContractPriceLimit(string inst);//done

	bool		GetFutureContractOpenInterest(string inst);//done
	bool		GetUserFutureTransactions(string inst, list<HuobFutureTransaction> &transaction_list);//done
	bool		GetFutureHistoryTrade(string inst, list<HuobTransaction> &transaction_list);

	bool		GetFutureDepthMarket(string inst, int type = 0);//done
	bool		GetFutureAccounts(string inst);//done
	bool		GetFuturePosition(string inst);//done

	bool		GetFutureOrder(string orderID, HuobOrder& order, bool isByOrderId);//done
	bool		GetFutureOrderList(const std::string& symbol, list<string> orderIDList, list<HuobOrder>& orderList, bool isByOrderId);
	bool		GetFutureOrderDetail(string inst, string orderID, HuobOrderDetail& order);
	bool		GetFutureOpenOrder(string orderID, HuobOrder& order);
	bool		GetFutureTradeOrder(string orderID, HuobOrder& order);

	uint64_t	ReqFutureOrderInsert(HuobOrder &order);//Done
	bool		CancelFutureOrder(const std::string &order_id, const std::string& symbol);//Done

	uint64_t	ReqBatchFutureOrderInsert(list<HuobOrder> &order_list);
	bool		CancelBatchFutureOrder(list<string> &order_id_list);//Done

	static std::map<string, string> GetFutureLongChannelMap();
	static std::map<string, string> GetFutureShortChannelMap();
	static string					GetFutureCurrencyPair(Instrument *inst);
	static string					GetFutureConstractCode(string inst);
	/**************************************************************/

private:
	void SetPrivateParams(int mode, bool isFuture, int body_format = FORM_TYPE);
	string CreateSignature(int http_mode, string secret_key, const char* encode_mode);
private:
	
	string m_api_key;
	string m_secret_key;
	string m_customer_id;
	Uri m_uri;
	uint64_t m_nonce;	
	InnerMarketData m_field;
	static std::map<string, string> channelToInstrumentMap_;
	static std::map<string, string> futureChannelToLongInstrumentMap_;
	static std::map<string, string> futureChannelToShortInstrumentMap_;
};
#endif
