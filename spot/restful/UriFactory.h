
#ifndef __CURL_PROTOCOL_H__
#define __CURL_PROTOCOL_H__

#include <map>
#include <unordered_map>
using namespace std;
#include "Uri.h"

enum HTTP_SERVER_TYPE
{
	HTTP_SERVER_TYPE_CN,		//国内站
	HTTP_SERVER_TYPE_COM,		//国际站
};

enum HTTP_PROTOCL_TYPE
{
	HTTP_PROTOCL_TYPE_UNKONWN		= 0	,
														
	// 永续合约 ok API
	// HTTP_API_TYPE_CANCEL_BATCH_INFO,
	HTTP_API_TYPE_OKX_POSITION_INFO ,
	HTTP_API_TYPE_OKX_MARK_PRICE_INFO ,
	HTTP_API_TYPE_OKX_SWAP_FUTURE_ORDER_CANCEL ,
	HTTP_API_TYPE_OKX_SWAP_FUTURE_ORDER_INSERT ,
	HTTP_API_TYPE_SWAP_FUTURE_ORDER_INFO ,

	// bian
	HTTP_API_TYPE_BIAN_POSITION_INFO ,
	HTTP_API_TYPE_BIAN_MPRICE_FUNDINGRATE_INFO ,
	HTTP_API_TYPE_BIAN_SWAP_FUTURE_ORDER_CANCEL ,
	HTTP_API_TYPE_BIAN_SWAP_FUTURE_ORDER_INSERT ,
};

#define URL_PROTOCOL_HTTPS					"https://"
#define URL_PROTOCOL_HTTP					"http://"


#define URL_PROTOCOL_CN						"https://"
#define URL_DOMAIN_CN						"www.okcoin.cn"


#define URL_MAIN_COM						"https://www.okx.com"
#define URL_PROTOCOL_COM					"https://"
#define URL_DOMAIN_COM						"www.okx.com"

// okv5版本
// 永续合约 positon API
// #define HTTP_API_CANCEL_BATCH_ORDERS		"/api/v5/trade/cancel-batch-orders"	//POST
#define HTTP_API_OKX_POSITION		"/api/v5/account/positions"	//GET 合约下单
#define HTTP_API_OKX_MARKPX		"/api/v5/public/mark-price"	//GET MARKPX
#define HTTP_API_OKX_SWAP_FUTURE_ORDER_CANCEL "/api/v5/trade/cancel-order" //POST 合约撤单
#define HTTP_API_OKX_SWAP_FUTURE_ORDER_INSERT "/api/v5/trade/order" //POST 合约下单
#define HTTP_API_SWAP_FUTURE_ORDER_INFO "/api/v5/trade/order" //查询订单信息 GET

// bian information
#define HTTP_API_BIAN_POSITION		"/fapi/v2/positionRisk"	//GET 合约下单
#define HTTP_API_BIAN_MARKPX_FUNDINGRATE		"/fapi/v1/premiumIndex"	//GET MARKPX
#define HTTP_API_BIAN_SWAP_FUTURE_ORDER_CANCEL "/fapi/v1/order" //DELETE 合约撤单
#define HTTP_API_BIAN_SWAP_FUTURE_ORDER_INSERT "/fapi/v1/order" //POST 合约下单

class UriFactory
{
public:
	UriFactory();
	virtual ~UriFactory();

	int InitApi(HTTP_SERVER_TYPE type);

	void GetUrl(Uri &uri,unsigned int http_protocl_type);

	std::unordered_map<unsigned int,Uri> m_urllist;
	string domain;
	int AddApi(unsigned int http_protocl_type,HTTP_PROTOCOL http_protocol,string api,HTTP_METHOD http_method);

};

#endif /* __CURL_PROTOCOL_H__ */
