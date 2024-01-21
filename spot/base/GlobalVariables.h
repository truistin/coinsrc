#ifndef SPOT_BASE_GLOBALVARIABLES_H
#define SPOT_BASE_GLOBALVARIABLES_H
#include <spot/utility/Compatible.h>
#include <spot/base/DataStruct.h>
class GlobalVariables
{
public:
	GlobalVariables()
	{
		mergeMode_ = MergeMode::Default;
		strategyThreadMsgID_ = 0;
	};
	~GlobalVariables(){};
	static GlobalVariables& instance()
	{
		static GlobalVariables gvInstance_;
		return gvInstance_;
	};
	void setMergeMode(MergeMode mergeMode)
	{
		mergeMode_ = mergeMode;
	}
	MergeMode getMergeMode()
	{
		return mergeMode_;
	}
	void setStrategyThreadMsgID(int msgId)
	{
		strategyThreadMsgID_ = msgId;
	}
	int getStrategyThreadMsgID()
	{
		return strategyThreadMsgID_;
	}
private:
	MergeMode mergeMode_;
	int strategyThreadMsgID_;
};

#endif //SPOT_BASE_VERSION_H
