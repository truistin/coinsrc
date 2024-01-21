#ifndef SPOT_STRATEGY_CONFIGPARAMETER_H
#define SPOT_STRATEGY_CONFIGPARAMETER_H
#include "spot/utility/Compatible.h"
#include "spot/strategy/Strategy.h"
#include "spot/utility/ReadIni.h"
#include "spot/gateway/GateWay.h"
#include "spot/comm/MqDispatch.h"
using namespace spot;
using namespace spot::base;
using namespace spot::utility;
using namespace spot::gtway;
namespace spot
{
	namespace strategy
	{
		class ConfigParameter
		{
		public:
			ConfigParameter(const char* iniFileName);
		    ~ConfigParameter();
			void init(GateWay *gateway);
			void initTdConfig(GateWay *gateway);
			void initMdConfig(GateWay *gateway);
			inline int spotID();
			inline int rabbitPort();
			inline int logLevel();
			inline int logMqLevel();
			inline int logNanoLevel();
			inline int startTime();
			inline std::string rabbitAddr();
			inline std::string rabbitUser();
			inline std::string rabbitPassword();
			inline MqParam mqParam();
			inline std::string distKey();
			inline std::string shmKey();
			inline std::string tradeShmKey();

		private:
			ReadIni *readIni_;
			int spotID_;
			int rabbitPort_;
			int logLevel_;
			int logMqLevel_;
			int logNanoLevel_;
			int startTime_;

			std::string rabbitAddr_;
			std::string rabbitUser_;
			std::string rabbitPassword_;
			MqParam mqParam_;
			std::string distKey_;
			std::string shmKey_;
			std::string tradeShmKey_;

		};
		inline int ConfigParameter::startTime()
		{
			return startTime_;
		}
		inline int ConfigParameter::spotID()
		{
			return spotID_;
		}
		inline int ConfigParameter::rabbitPort()
		{
			return rabbitPort_;
		}
		inline int ConfigParameter::logLevel()
		{
			return logLevel_;
		}
		inline int ConfigParameter::logMqLevel()
		{
			return logMqLevel_;
		}
		inline int ConfigParameter::logNanoLevel()
		{
			return logNanoLevel_;
		}
		inline std::string ConfigParameter::rabbitAddr()
		{
			return rabbitAddr_;
		}
		inline std::string ConfigParameter::rabbitUser()
		{
			return rabbitUser_;
		}
		inline std::string ConfigParameter::rabbitPassword()
		{
			return rabbitPassword_;
		}
		inline MqParam ConfigParameter::mqParam()
		{
			return mqParam_;
		}
		inline std::string ConfigParameter::distKey()
		{
			return distKey_;
		}
		inline std::string ConfigParameter::shmKey()
		{
			return shmKey_;
		}
		inline std::string ConfigParameter::tradeShmKey()
		{
			return tradeShmKey_;
		}

	}
}
#endif
