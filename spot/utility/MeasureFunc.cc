#include"spot/utility/MeasureFunc.h"
#include<memory>
#include<string.h>
#include<vector>
#include "spot/utility/Logging.h"

std::map<int, MeasureDataInfo*> MeasureFunc::measureDataInfoMap_;

MeasureFunc::MeasureFunc(int measureId)
	:measureId_(measureId)
{
	startTime_ = CURR_USTIME_POINT;
}
MeasureFunc::~MeasureFunc()
{
	auto iter = measureDataInfoMap_.find(measureId_);
	if (iter == measureDataInfoMap_.end())
		return;
	auto endTime = CURR_USTIME_POINT;
	auto measureInfo = iter->second;
	if (measureInfo->count != 0)
	{
		if (measureInfo->count % measureInfo->measureCount != measureInfo->measureCount-1)
		{
			measureInfo->measureSum += endTime - startTime_;
		}
		else
		{
			LOG_INFO << measureInfo->measureFunc << " MeasureData.latency:" << (double)measureInfo->measureSum / measureInfo->count;
			measureInfo->count = -1;
			measureInfo->measureSum = 0;
		}
	}
	else
	{
		//init
		measureInfo->measureSum = endTime - startTime_;
	}
	measureInfo->count++;
}
void MeasureFunc::addMeasureData(int mid, string mfunc, int mcount)
{
	MeasureDataInfo *measureInfo = new MeasureDataInfo(mid,mfunc,mcount);
	measureDataInfoMap_[mid] = measureInfo;
}