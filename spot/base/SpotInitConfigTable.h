#ifndef SPOT_BASE_SPOTINITCONFIG_TABLE_H
#define SPOT_BASE_SPOTINITCONFIG_TABLE_H
#include "spot/base/DataStruct.h"
#include "spot/base/MqStruct.h"
#include "spot/utility/Utility.h"
#include<map>
#include<list>

using namespace spot::utility;
using namespace spot::base;

class SpotInitConfigTable
{
public:
	SpotInitConfigTable();
	~SpotInitConfigTable();
	static SpotInitConfigTable& instance();
	void add(std::list<SpotInitConfig> &configList);	

	int getMarketDataQueueMode();
	int getSpotmdSleepInterval();
	int getIsOkSimParam_();
	int getIsBaSimParam_();
	int getIsBySimParam_();
	string getValueByKey(string key);
	int getMqWriteMQ();
	int getSubAll();


	inline int measureEnable();

private:
	map<std::string, SpotInitConfig> initConfigMap_;
	int marketDataQueueMode_;//0:Log,1:IndexServer,2:CffexOffset
	int spotmdSleepIntervalus_;

	int measureEnable_;//tick2order度量开关。0：关闭；1：simple；2：detail;3:order2ack
	int TimerInterval_; //position risk
	int ForceCloseTimerInterval_; //force closing
	int IsOkSimParam_;
	int IsBaSimParam_;
	int IsBySimParam_;
	int MqWriteMQ_;
};

inline int SpotInitConfigTable::measureEnable()
{
	return measureEnable_;
}
#endif