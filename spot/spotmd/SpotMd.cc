#include <iostream>
#include <boost/program_options.hpp>
#include "thread"
#include "spot/utility/Utility.h"
#include "spot/utility/SPSCQueue.h"
#include "spot/base/DataStruct.h"
#include "spot/utility/ReadIni.h"
#include "spot/strategy/StrategyCooker.h"
#include <spot/utility/LogFile.h>
#include <spot/utility/Logging.h>
#include <spot/comm/AsyncLogging.h>
#include "spot/gateway/GateWay.h"
#include "spot/strategy/Initializer.h"
#include "spot/comm/MqDispatch.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/InstrumentManager.h"
#include "spot/utility/TradingTime.h"
#include "spot/utility/TradingDay.h"
#include "spot/strategy/ConfigParameter.h"
#include "spot/base/InitialData.h"
#include "spot/base/InitialDataExt.h"
#include <spot/gateway/MktDataQueStore.h>
#include <spot/base/MqDecode.h>
#include "spot/utility/Compatible.h"
#include "spot/base/SpotInitConfigTable.h"
#include "MdInterface.h"
#include "spot/base/InitJsonFile.h"
#include "spot/shmmd/ShmManagement.h"
#include "spot/cache/CacheAllocator.h"

using namespace spot;
using namespace spot::utility;
using namespace spot::base;
using namespace spot::strategy;
using namespace spot::gtway;
using namespace moodycamel;


map<string, string> symbolExChangeMap;
int main(int argc, char* argv[])
{
#ifdef __LINUX__
	signal(SIGPIPE, SIG_IGN);
	signal(SIGABRT, SIG_IGN);
#endif
	RmqWriter writer;
	try
	{
		// boost::program_options::options_description opts("spotmd options");
		// opts.add_options()
		// 	("help", "just a help info")
		// 	("version", "to show version info");
		// boost::program_options::variables_map vm;
		
		// boost::program_options::store(parse_command_line(argc, argv, opts), vm);
		
		// if (vm.count("help"))
		// {
		// 	std::cout << opts << endl;
		// 	return 0;
		// }
		// else if (vm.count("version"))
		// {
		// 	//std::cout << _SpotVersion_ << std::endl;
		// 	return 0;
		// }

		//Get Config
		ConfigParameter config("../config/spot.ini");
		// TradingDay::init(config.startTime());
		// TradingTime::init(config.startTime());
		//setup logging
		getFilePath("../log/");
		char *buff = CacheAllocator::instance()->allocate(sizeof(AsyncLogging));
		g_asyncLog = new (buff) AsyncLogging("../log/spotmd", 500 * 1000 * 1000, config.logLevel(), config.logMqLevel(), 3, true);
		g_asyncLog->connectMq(config.mqParam());
		g_asyncLog->start();

		//LOG_INFO << "Version:" << _SpotVersion_;
#if defined(__USE_TSCNS__) && defined(__LINUX__)
		TSCNS::instance().init();
		LOG_INFO << "tscns init";
#endif		
		StrategyCooker cooker;
#ifdef INIT_WITHOUT_RMQ
		InitJsonFile jsonFile("../config/", cooker.getRmqReadQueue());
		jsonFile.getInitData();
#endif

		//setup MQ dispatcher
#ifndef INIT_WITHOUT_RMQ
		MqDispatch mqDispatch(config.mqParam(), cooker.getRmqReadQueue(), RmqWriter::getRmqWriterQueue());
		mqDispatch.connectMq();
		std::thread mqthread(std::bind(&MqDispatch::run, &mqDispatch));
		pthread_setname_np(mqthread.native_handle(), "MQ");
#endif	
		
		Initializer initializer(config.spotID());
		initializer.init(cooker.getRmqReadQueue(),true);
		
		MarketDataQueueMode mdQueueMode = MarketDataQueueMode::Log;
		MdInterface::instance().init(initializer, config);		
#ifdef __Debug_Mode__
		ShmManagement::instance().start();
#endif
#ifdef __LINUX__
		pthread_setname_np(pthread_self(), "Main");
#endif
#ifndef INIT_WITHOUT_RMQ
		QueueNode rmqNode;
		while (true)
		{
			try
			{
				cooker.handleRmqData(rmqNode);
			}
			catch (...)
			{
				LOG_WARN << "handleRmqData exception";
			}
			// SLEEP(1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
#endif
	// cause websocket will throws exception
	}
	// catch (boost::program_options::error_with_no_option_name &ex)
	// {
	// 	cout << "exit main thread " << ex.what() << endl;
	// 	LOG_ERROR << "exit main thread " << ex.what() << __FILE__ << " " << __LINE__;
	// }
	catch (exception& ex)
	{
		cout << "exit main thread " << ex.what() << endl;
		LOG_ERROR << "exit main thread " << ex.what() << __FILE__ << " " << __LINE__;
	}
	catch (...)
	{
		cout << "exit main thread " << endl;
		LOG_ERROR << "exit main thread " << __FILE__ << " " << __LINE__;
	}
	// getchar();
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000000));
	}
	// pthread_exit(nullptr); // main thread exit but sub thread not exit
	return 0;
}