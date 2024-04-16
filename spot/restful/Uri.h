#ifndef __URL_H__
#define __URL_H__

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <algorithm>
#include "spotcurl.h"
#include <atomic>

using namespace std;

typedef void (*Url_CallBak)(void *_uri);

enum CURL_REQUEST_SAVE_TO
{
	CURL_REQUEST_SAVE_TO_MEMROY,
	CURL_REQUEST_SAVE_TO_FILE
};

enum HTTP_PROTOCOL
{
	HTTP_PROTOCOL_HTTP, HTTP_PROTOCOL_HTTPS 
};

enum HTTP_METHOD
{
  METHOD_GET, METHOD_POST, METHOD_PUT, METHOD_DELETE
};

enum HTTP_ENCODING
{
	NO_ENCDING, GZIP_ENCODING
};

enum BODY_FORMAT_TYPE
{
	FORM_TYPE, JSON_TYPE
};

class UriParam
{
public:
	UriParam();
	~UriParam();
	std::vector<pair<string, string>> m_items;

public:
	string Get(const string name) const;
	list<string> GetArray(const string name) const;
	string ToString();
	string ToJsonString(); 
	string ListToJsonString();

	void Clear();
	void Add(const char *name,const char *value);

	void AddNoEncode(const char *name,const char *value);

	void Set(const char *name,const char *value);
};

class Uri
{
public:
	/////////////////////////////////////
	typedef std::function<void(char *, uint64_t)> ResponseCallback;
	void clear();
	void Reset();
	void FreeBuf();

	void ClearParam(){ m_param.Clear(); };
	void AddParam(const string &strName, const string &strValue);
	void AddParam(const char *szName,const char *szValue);

	string GetSign(string &secret_key);
	string GetParamSet();
	string GetParamJson();
	string GetListJsonParam();
	void setResponseCallback(const ResponseCallback& cb);

	void ClearHeader(){ m_header.Clear(); };
	void AddHeader(const char *szName,const char *szValue);
	void AddHeader(const char *content);
	
	string AddParamToUrl(const string &strUrl);
	string GetUrl();
	int Request();
	
	void GetUTF8Result();
	void setUriClientOrdId(uint64_t orderId) {clientOrderId = orderId;}
	uint64_t getUriClientOrdId() {return clientOrderId;}

public:
	Uri();
	Uri& operator=(const Uri& other); 
	~Uri();
	CURL_REQUEST_SAVE_TO saveto;
	bool isinit;
	int type;
	string urlprotocolstr;
	string domain;
	string api;
	string url;
	HTTP_PROTOCOL protocol;
	HTTP_METHOD method;

	HTTP_ENCODING encoding;
	BODY_FORMAT_TYPE m_body_format;

	UriParam m_param;
	UriParam m_header;

	string result;

	CURLcode errcode;
	CURLMcode errMcode;
	string exchangeCode;


	CUrl* pcurl;

	long request_size;			//请求应该返回数据的大小
	long request_cursize;		//请求实际已经接收数据的大小

	char *curbuf;				//当前可用的数据buf比如去掉协议头的buf，
	long curbuf_size;			//当前buf中实际数据的大小
	curl_slist *httpHeaders;
	ResponseCallback cb_;

	string params;
	string urlstr;

	bool isfirstwrite;
	atomic<bool> isendwrite;
	size_t retcode;
	uint64_t clientOrderId;
	atomic<bool> isfinish;
};


#endif /* __URL_H__ */


