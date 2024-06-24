#include <boost/program_options.hpp>
#include "spot/utility/ReadIni.h"
#include "spot/strategy/StrategyCooker.h"
#include <spot/comm/AsyncLogging.h>
#include "spot/gateway/GateWay.h"
#include "spot/comm/MqDispatch.h"
#include "spot/strategy/StrategyTimer.h"
#include "spot/utility/TradingDay.h"
#include "spot/strategy/ConfigParameter.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/strategy/StrategyFactory.h"
#include "spot/base/InitialData.h"
#include "spot/base/MeasureOrderToAck.h"
#include "spot/websocket/websocket.h"
#include "spot/base/InstrumentManager.h"

#include "spot/shmmd/ShmMtServer.h"
#include "spot/base/InitJsonFile.h"
#include "spot/cache/CacheAllocator.h"

#ifdef __LINUX__
#include <pthread.h>
#endif
using namespace spot::utility;
using namespace spot::base;
using namespace spot::strategy;
using namespace spot::gtway;
using namespace spot::shmmd;

static void signalHandler( int signum )
{
	cout << "signalHandler throw websocketpp exception";
	LOG_WARN << "signalHandler throw websocketpp exception";
	// std::logic_error e("Bad Connection");
	// throw std::exception(e);
}

void fillTables()
{
	// TableInfo btcInfo("BTCUSDT", 10, 5);
	// TableInfo btcPerpInfo("BTCUSD_PERP", 10, 5);

	// TableInfo bnbInfo("BNBUSDT", 10, 5);
	// TableInfo bnbPerpInfo("BNBUSD_PERP", 9, 5);

	TableInfo ethInfo("ETHUSDT", 20, 5);
	TableInfo ethPerpInfo("ETHUSD_PERP", 20, 5);

	TableInfo btcInfo("BTCUSDT", 20, 5);
	TableInfo btcPerpInfo("BTCUSD_PERP", 20, 5);

	TableInfo bnbInfo("BNBUSDT", 20, 5);
	TableInfo bnbPerpInfo("BNBUSD_PERP", 20, 5);

	TableInfo solInfo("SOLUSDT", 20, 5);
	TableInfo solPerpInfo("SOLUSD_PERP", 20, 5);

	bnbInfo.data = new double*[bnbInfo.rows];
	for (int i = 0; i < bnbInfo.rows; ++i) {  
		bnbInfo.data[i] = new double[bnbInfo.cols];
		memset(bnbInfo.data[i], 0, sizeof(double) * bnbInfo.cols);  
	}  
 

	bnbPerpInfo.data = new double*[bnbPerpInfo.rows];
	for (int i = 0; i < bnbPerpInfo.rows; ++i) {  
		bnbPerpInfo.data[i] = new double[bnbPerpInfo.cols];
		memset(bnbPerpInfo.data[i], 0, sizeof(double) * bnbPerpInfo.cols);  
	} 

	btcInfo.data = new double*[btcInfo.rows];
	for (int i = 0; i < btcInfo.rows; ++i) {  
		btcInfo.data[i] = new double[btcInfo.cols];
		memset(btcInfo.data[i], 0, sizeof(double) * btcInfo.cols);  
	}  
 

	btcPerpInfo.data = new double*[btcPerpInfo.rows];
	for (int i = 0; i < btcPerpInfo.rows; ++i) {  
		btcPerpInfo.data[i] = new double[btcPerpInfo.cols];
		memset(btcPerpInfo.data[i], 0, sizeof(double) * btcPerpInfo.cols);  
	}  

	ethInfo.data = new double*[ethInfo.rows];
	for (int i = 0; i < ethInfo.rows; ++i) {  
		ethInfo.data[i] = new double[ethInfo.cols];  
		memset(ethInfo.data[i], 0, sizeof(double) * ethInfo.cols);  
	}

	ethPerpInfo.data = new double*[ethPerpInfo.rows];
	for (int i = 0; i < ethPerpInfo.rows; ++i) {  
		ethPerpInfo.data[i] = new double[ethPerpInfo.cols];  
		memset(ethPerpInfo.data[i], 0, sizeof(double) * ethPerpInfo.cols);  
	}  

	mmr_table.push_back(bnbInfo);
	mmr_table.push_back(bnbPerpInfo);
	mmr_table.push_back(btcInfo);
	mmr_table.push_back(btcPerpInfo);
	mmr_table.push_back(ethInfo);
	mmr_table.push_back(ethPerpInfo);
}

int main(int argc, char* argv[])
{
#ifdef __LINUX__
	signal(SIGPIPE, SIG_IGN);
	// signal(SIGABRT, SIG_IGN);
	signal(SIGABRT, signalHandler);
#endif
	RmqWriter rmqWriter;

	//local configuration i.e. flow folder, logging level, and rabbitmq
	ConfigParameter config("../config/spot.ini");
	// TradingDay::init(config.startTime());
	// TradingTime::init(config.startTime());		
#ifndef INIT_WITHOUT_RMQ
	initTickToOrderMeasure(RmqWriter::getRmqWriterQueue());
#endif
	//setup logging
	getFilePath("../log/");
	char *buff = CacheAllocator::instance()->allocate(sizeof(AsyncLogging));

	g_asyncLog = new (buff) AsyncLogging("../log/spot", 500 * 1000 * 1000, config.logLevel(),config.logMqLevel());
	g_asyncLog->connectMq(config.mqParam());
	g_asyncLog->start();

	fillTables();

	LOG_INFO << "Trading:Main g_asyncLog start finish. time:" << getCurrentSystemTime();
	//setup gateway
	buff = CacheAllocator::instance()->allocate(sizeof(GateWay));
	GateWay *gateway = new (buff) GateWay();
	LOG_INFO << "test1: " << getCurrentSystemTime();

	Initializer initializer(config.spotID());
	//setup MQ dispatcher
	StrategyCooker cooker;
#ifndef INIT_WITHOUT_RMQ
	MqDispatch mqDispatch(config.mqParam(), cooker.getRmqReadQueue(), RmqWriter::getRmqWriterQueue());
	mqDispatch.connectMq();
	std::thread mqthread(std::bind(&MqDispatch::run, &mqDispatch));
#endif

#ifdef INIT_WITHOUT_RMQ
	InitJsonFile jsonFile("../config/", cooker.getRmqReadQueue());
	jsonFile.getInitData();
#endif

	//get init info		
	initializer.init(cooker.getRmqReadQueue());
	LOG_INFO << "Trading:Main initializer init finish";

	//init gateway and setup
	config.init(gateway);
	gateway->setReckMarketDataCallback(std::bind(&MarketDataManager::OnRtnInnerMarketData, &(MarketDataManager::instance()), std::placeholders::_1));
	gateway->setRecvMarketTradeCallback(std::bind(&MarketDataManager::OnRtnInnerMarketTrade, &(MarketDataManager::instance()), std::placeholders::_1));

	gateway->init();

	LOG_INFO << "Trading:Main gateway init finish";

	cooker.setGatewayQueue(gateway->mdQueue(), gateway->mtQueue(), gateway->orderQueue());
	cooker.setAdapter(gateway);		
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //start cooker threads
    std::thread spinnerThread(std::bind(&StrategyCooker::run, &cooker));
	
	StrategyTimer timer;
	std::thread timerThread(std::bind(&StrategyTimer::start, &timer));

#ifdef __LINUX__
	pthread_setname_np(timerThread.native_handle(), "StrategyTimer");
	pthread_setname_np(pthread_self(), "Main");
	pthread_setname_np(spinnerThread.native_handle(), "Cooker");
#endif
	spinnerThread.join();
	timerThread.join();

#ifndef INIT_WITHOUT_RMQ
	pthread_setname_np(mqthread.native_handle(), "MqDispatch");
	mqthread.join();
#endif

	return 0;
}
