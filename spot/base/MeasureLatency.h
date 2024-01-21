#ifndef SPOT_BASE_MEASURELATENCY_H
#define SPOT_BASE_MEASURELATENCY_H
#include<unordered_map>
#include<vector>
#include<memory>
#include<string>
#include<map>
#include<functional>
#include "spot/utility/SPSCQueue.h"
#include "spot/base/DataStruct.h"
using namespace spot::utility;
namespace spot
{
	namespace base
	{
		class TimePoint
		{
		public:
			TimePoint(const char* currPoint, const char* lastPoint, uint64_t epochTime, uint64_t epochTimeDiff)
				:currPoint_(currPoint), lastPoint_(lastPoint), epochTime_(epochTime), epochTimeDiff_(epochTimeDiff),isPrinted(false)
			{
			}
			std::string currPoint_;
			std::string lastPoint_;
			uint64_t epochTime_;
			uint64_t epochTimeDiff_;
			bool isPrinted;
		};

		typedef std::map<int, std::vector<TimePoint*>> TimePointMap;

		class MeasureLatency
		{
		public:
			MeasureLatency();
			~MeasureLatency();
			static void init(spsc_queue<QueueNode> *rmqQueue);
			static void addTickToOrderTimePoint(int messageID,const char* currPoint,uint64_t epochTime = 0);
			static void addLastTickToOrderTimePoint(int messageID, const char* currPoint, const char* gateWay,const char* exchangeID, int strategyID, uint64_t epochTime = 0);
			static void addOrderToTradeTimePoint(int orderRef, const char* currPoint, uint64_t epochTime = 0);
			static void addLastOrderToTradeTimePoint(int orderRef, const char* currPoint, const char* gateWay, const char* exchangeID, int strategyID, uint64_t epochTime = 0);
			static void writeLog(int keyID, const char* gateWay, const char* exchangeID, int strategyID, std::vector<TimePoint*> &vec);
			//可选，默认两个都度量
			static void setMeasureFlag(bool measureTickToOrder, bool measureOrderToTrade);
			//可选，默认MAXINT,count:度量数据条数
			static void setMeasureCount(int count);
			//可选，默认不分段统计，sectionNumber:分段个数；gap:每段间隔
			static void setSectionNumber(int sectionNumber, int gap);
			//可选，默认显示最大的10条记录
			static void setTopNumber(unsigned int top = 10);
			//把数据导出到csv文件
			static void exportToFile();
			static void exportTickToOrder();
			static void exportOrderToTrade();
			static void statistics(TimePointMap &timePointMap);
		private:
			static TimePointMap tickToOrderMap_;
			static TimePointMap orderToTradeMap_;
			static bool measureTickToOrder_;
			static bool measureOrderToTrade_;
			static int measureCount_;
			static std::vector<int> sectionCountVec_;
			static std::map<uint64_t, uint64_t, std::greater<uint64_t>> diffEpochTimeMap_;
			static int gap_;
			static unsigned int top_;
			static spsc_queue<QueueNode> *rmqQueue_;
		};
	}
}
#endif