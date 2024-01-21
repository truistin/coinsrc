#ifndef __OKCOINWEBSOCKETAPI_H__
#define __OKCOINWEBSOCKETAPI_H__
#include <string>

using namespace std;

#include "websocket.h"
#include "websocketapi.h"

class OKCoinWebSocketApiCom:public WebSocketApi
{
public:
	OKCoinWebSocketApiCom(string api_key,string secret_key, string url)
	{
		SetKey(api_key, secret_key);
		SetUri(url);
	};
	~OKCoinWebSocketApiCom(){};

	string ok_sub_futureusd_X_depth_Y_Z(string name, string type, string depth);
	string ok_sub_futureusd_X_ticker_Y(string name, string type);
	string ok_sub_futureusd_X_trade_Y(string name, string type);

	string ok_sub_futureusd_X_index(string name);

	string ok_sub_spot_X_depth_Y(string name, string depth);
	string ok_sub_spot_X_ticker(string name);
	string ok_sub_spot_X_deals(string name);

	void send_heartbeat();
	void send_v5_heartbeat();
};

#endif /* __OKCOINWEBSOCKETAPI_H__ */
