#ifndef SPOT_BASE_MEASUREORDERTOACK_H
#define SPOT_BASE_MEASUREORDERTOACK_H
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
		class MeasureOrderToAck
		{
		public:
			MeasureOrderToAck();
			~MeasureOrderToAck();
			static MeasureOrderToAck& instance();
			void init(spsc_queue<QueueNode> *sendQueue);
			void addColumnName(string name);
			void sendMetricData(const Order *order);
		private:
			spsc_queue<QueueNode> *sendQueue_;
			std::vector<std::string> columnNameVec_;			
			QueueNode queueNode_;		
		};

	}
}
#endif