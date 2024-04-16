#ifndef __NET_CURL_H__
#define __NET_CURL_H__

#include <list>
#include <map>
#include <string>
#include <functional>
#include <atomic>

using namespace std;


#ifdef __LINUX__
#include "curl.h"
#endif

#ifdef WIN32
#include <Windows.h>
#include "curl/win_vc10/include/curl.h"
#endif

extern string gCurlProxyAddr;

class CUrl
{
public:
	typedef std::function<void(char *)> CurlResponseCallback;
	CUrl();
	~CUrl();
	static CUrl &GetCUrl();

	static int global_init();
	static void global_cleanup();
	
	void set_share_handle(CURL* curl_handle);

	int initapi();


	/*
	 * 将请求返回的结果打印输出到屏幕.
	 */

	static long call_wirte_func(void *buffer, int size, int nmemb, void *uri);

	static size_t header_callback(const char  *ptr, size_t size, size_t nmemb, void *uri);

	void setCurlResponseCallback(CurlResponseCallback cb);
	


	/*
	 * 执行 API.
	 */
	int request(void *_uri);

	void get_response_code();
	void get_response_length();
	void get_response_contenttype();

	bool isheadread;//协议头是否已经读取过

	CURLcode code;
	long retcode;
	CURL *curl;
	struct curl_slist *cookies;
	list<string> cookielist;

	bool dataverify;

	string m_sessionid;
	string file_name;

	long file_size ;	
	long request_size;
	string contenttype_str;

	bool isfirstwrite;
	CurlResponseCallback cb_;
	CURLM * multi_handle;


};

#endif /* __NET_CURL_H__ */
