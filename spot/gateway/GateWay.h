#ifndef SPOT_GTWAY_GATEWAY_H
#define SPOT_GTWAY_GATEWAY_H
#include <spot/base/DataStruct.h>
#include <spot/base/Adapter.h>
#include <functional>
#include <map>
#include <spot/utility/SPSCQueue.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include "spot/restful/spotcurl.h"
#include <spot/utility/Logging.h>
#include <spot/gateway/DistMdSpi.h>
#include <spot/gateway/ShmMdSpi.h>
#include <spot/gateway/ShmMtSpi.h>

using namespace moodycamel;
using namespace spot::gtway;
/*addSubInst 订阅合约需要在Init之前，行情登录成功后就自动订阅合约*/ 
namespace spot
{
namespace gtway
{
using namespace spot::base;
using namespace spot::utility;
class OrderStoreCallBack;

class GateWay: public base::Adapter
{
public:
  GateWay();
  virtual ~GateWay();
  void init();
  void initTdGtway();
  void initMdGtway();
  virtual int reqOrder(Order &order);
  virtual int cancelOrder(Order &order);
  virtual int query(const Order &order);
  typedef std::function<int(const Order &order)> RequestFun;
	inline ConcurrentQueue<InnerMarketData>* mdQueue();
	inline ConcurrentQueue<InnerMarketTrade>* mtQueue();
	inline ConcurrentQueue<InnerMarketData>* shfeUdpMdQueue();

	inline ConcurrentQueue<Order>* orderQueue();

	inline map<string, map<int, shared_ptr<ApiInfoDetail>>>* apiTdInfoDetailMap();
	inline map<string, map<int, shared_ptr<ApiInfoDetail>>>* apiMdInfoDetailMap();
	
	inline map<string, map<int, vector<string>>>* subInstMap();
	inline moodycamel::ConcurrentQueue<QueueNode>* mdLastPriceQueue();


  inline map<string,string>* exchangeCodeInvestorIdMap();
  inline map<string, SpotInitConfig> *spotInitConfigMap();
	void setReckMarketDataCallback(RecvMarketDataCallback marketDataCallback);
  void setRecvMarketTradeCallback(RecvMarketTradeCallback marketTradeCallback);
private:
  int i = 1;
  GateWayApiInfo apiInfo_;
  string sflowPath_;
  ConcurrentQueue<InnerMarketData>* mdQueue_;
  ConcurrentQueue<InnerMarketTrade>* mtQueue_;
  ConcurrentQueue<Order>* orderQueue_;


  map<std::pair<string, int>, map<int, RequestFun>*>*  requestFunMap_;
  
  map<string, map<int,shared_ptr<ApiInfoDetail>>> *apiTdInfoDetailMap_;
  map<string, map<int, shared_ptr<ApiInfoDetail>>> *apiMdInfoDetailMap_;

  map<string, map<int, vector<string>>> *subInstlMap_;
  ConcurrentQueue<QueueNode> *mdLastPriceQueue_;
  map<string, SpotInitConfig> *spotInitConfigMap_;//for spx
  RecvMarketDataCallback marketDataCallback_;
  RecvMarketTradeCallback marketTradeCallback_;
};

inline ConcurrentQueue<InnerMarketData>* GateWay::mdQueue()
{
	return mdQueue_;
}
inline ConcurrentQueue<InnerMarketTrade>* GateWay::mtQueue()
{
	return mtQueue_;
}

inline ConcurrentQueue<Order>* GateWay::orderQueue()
{
	return orderQueue_;
}

inline map<string, map<int, shared_ptr<ApiInfoDetail>>>* GateWay::apiTdInfoDetailMap()
{
	return apiTdInfoDetailMap_;
}

inline map<string, map<int, shared_ptr<ApiInfoDetail>>>* GateWay::apiMdInfoDetailMap()
{
	return apiMdInfoDetailMap_;
}
inline map<string, map<int, vector<string>>>* GateWay::subInstMap()
{
	return subInstlMap_;
}
inline ConcurrentQueue<QueueNode>* GateWay::mdLastPriceQueue()
{
	return mdLastPriceQueue_;
}

inline map<string, SpotInitConfig> *GateWay::spotInitConfigMap()
{
  return spotInitConfigMap_;
}
}
}
#endif