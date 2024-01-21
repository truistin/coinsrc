#ifndef __WEBSOCKETAPI_H__
#define __WEBSOCKETAPI_H__
#include <string>

using namespace std;

#include "websocket.h"

class WebSocketApi
{
public:
	WebSocketApi();
	virtual ~WebSocketApi();
	void Run();
	void Close();
	WebSocket *pWebsocket;

	void SetCallBackOpen(const websocketpp_callbak_open &callbak_open);
	void SetCallBackClose(const websocketpp_callbak_close &callbak_close);
	void SetCallBackMessage(const websocketpp_callbak_message &callbak_message);
	void send(string &msg);
	void SetKey(string api_key, string secret_key);
	void SetUri(string uri);
	void SetCompress(bool isCompress);
	void SetOkCompress(bool isCompress);
	void Set_Protocol_Mode(bool isHttps);

	void UpdateCallBackMessageInWebSocket(const websocketpp_callbak_message &callbak_message);
protected:	
	bool   m_isHttps = true;
	bool   m_isCompress = false;
	bool   m_isOkCompress = false;
	string m_api_key;
	string m_secret_key;	
	string m_uri;
	void Emit(const char *channel,string &parameter);
	void Emit(const char *channel);
	void Emit(string &channel);
	void Remove(string channel);
private:
	websocketpp_callbak_open		m_callbak_open;
	websocketpp_callbak_close		m_callbak_close;
	websocketpp_callbak_message		m_callbak_message;
};
#endif /* __OKCOINWEBSOCKETAPI_H__ */
