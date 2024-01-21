#ifndef SPOT_UTILITY_MEASUREFUNC_H
#define SPOT_UTILITY_MEASUREFUNC_H
#include "spot/utility/Utility.h"
#include <map>
struct MeasureDataInfo
{
	int measureId;
	string measureFunc;
	int measureCount;
	uint64_t measureSum;	
	int count;
	MeasureDataInfo(int mid, string mfunc, int mcount = 100)
	{
		measureId = mid;
		measureFunc = mfunc;
		measureCount = mcount;
		count = 0;
		measureSum = 0;
	}
};
class MeasureFunc
{
public:	
	MeasureFunc(int measureId);
	~MeasureFunc();	
	static void addMeasureData(int mid, string mfunc, int mcount = 100);
private:
	uint64_t startTime_;	
	int measureId_;
	static std::map<int, MeasureDataInfo*> measureDataInfoMap_;
};


#endif
