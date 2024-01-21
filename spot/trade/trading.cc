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
