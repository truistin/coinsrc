#pragma once

#include"curl.h"
#include <mutex>
#include <condition_variable>
#include "Uri.h"
#include <spot/utility/concurrentqueue.h>
#include <iostream>

using namespace std;
using namespace moodycamel;
class CurlMultiCurl
{
public:
    // typedef std::function<size_t(void*, size_t, size_t, void*)> FuncAsyn;
    // typedef std::function<size_t(const char*, size_t, size_t, void*)> FuncAsynHeader;

    CurlMultiCurl();
    static CurlMultiCurl& instance();
    virtual ~CurlMultiCurl();
    //全局初始化
    void GlobalInit();
    //全局反初始化
    void GlobleFint();
    //初始化
    void init();
    //反初始化
    void finit();
    //添加任务到队列
    void addUri(shared_ptr<Uri>& uri);

    static size_t call_wirte_func(void *buffer, size_t size, size_t nmemb, void *curl);
	static size_t header_callback(const char  *ptr, size_t size, size_t nmemb, void *curl);

public:
    // map store easyhand队列
    static map<string, ConcurrentQueue<CURL*>*>* m_QueEasyHandMap;    

private:
    //处理任务，循环从对列中获取数据，添加到muitihand
    void dealUri();
    //检查是否有任务完成
    void handUri();
    //读取已完成的任务进行解析
    void readUriResult();
    //从easyhand队列中获取easyhand，没有则新建一个
    CURL* GetCurl(shared_ptr<Uri>& uri);
    //新建一个easyhand
    CURL* CreateCurl(shared_ptr<Uri>& uri);
    //将使用完的easyhand放入队列
    // void PutCurl(CURL* curl);
    void PutCurl(CURL* curl, string exch);
    //将任务添加到mulitihand，执行任务
    void addUriToMultiRequest(shared_ptr<Uri>& uri);
    //给easyhand设置参数
    //int setUriParameter(CURL* easyhand, shared_ptr<Uri>& uri);

    static void get_response_code(CURL* curl, size_t& retcode);
    static long get_response_length(CURL* curl);
    static void get_response_contenttype(CURL* curl);
    void set_share_handle(CURL* & curl_handle);
    CURL* fillCurlInfo(shared_ptr<Uri>& uri, CURL* & curl);
    void resetBuffsize();
   // void setCallBack();
    void setParamFromUri(shared_ptr<Uri>& uri);

    bool m_bDebug;
    CURLM* m_pMultiHand;//多操作句柄
    //list<shared_ptr<Uri>> m_listUri;//任务列表
    //ConcurrentQueue<shared_ptr<Uri>> *mdQueue_;
    moodycamel::ConcurrentQueue<shared_ptr<Uri>> *uriQueue_;

    mutex m_taskMutex;//任务列表的锁
    mutex m_easyHandMutex;//easyhand队列的锁

    //list<CURL*> m_listEasyHand;// easyhand队列
    // moodycamel::ConcurrentQueue<CURL*>* m_QueEasyHand;// easyhand队列

    bool m_bRunning = true;//线程控制函数
    std::thread* m_uriAddThread;//投递任务的的线程
    std::thread* m_uriHandThread;//判断任务状态， 处理任务的线程
    //std::thread* m_uriReadThread;
    condition_variable m_conVarUri;
    //map<CURL*, std::shared_ptr<Uri>> m_mapRuningUri;//正在执行的任务
    static map<CURL*, shared_ptr<Uri>> m_mapRuningUri;//正在执行的任务
    static mutex m_runningUriMutex;
    mutex m_curlApiMutex;//多线程调用curl的接口时会出现崩溃，这里加个锁

    static mutex buffMutex;
    int m_curlnum = 0;//
    int m_successnum=0;
    int m_failednum = 0;
    int m_addmultFailed;

	long request_size;			//请求应该返回数据的大小
	long request_cursize;		//请求实际已经接收数据的大小

	char* curbuf;				//当前可用的数据buf比如去掉协议头的buf，
	long curbuf_size;			//当前buf中实际数据的大小

    //static bool isfirstwirte = true;
    string exchangeCode_;
    static size_t retcode;

    //CURLcode code;
    static string contenttype_str;
};