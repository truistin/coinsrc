#include <spot/comm/MqDispatch.h>
#include <spot/utility/Logging.h>
#include <spot/utility/Measure.h>
#include <spot/comm/MqUtils.h>
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/rapidjson.h>
#include <spot/rapidjson/writer.h>
#include <spot/rapidjson/stringbuffer.h>
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>
#include <functional>
#include <map>
#include <list>
#include <spot/base/DataStruct.h>
#include <spot/base/MqStruct.h>
//#include <spot/base/BoostGZip.h>
#include <spot/base/MqDecode.h>
using namespace spot::base;
MqDispatch::MqDispatch(MqParam mqParam, MessageQue recvQue, MessageQue sendQue)
	: hostname_(mqParam.hostname),
	port_(mqParam.port),
	user_(mqParam.user),
	passwd_(mqParam.password),
	spotId_(mqParam.spotid),
	isRun_(true),
	heartbeat_(10),
	lastRecvTime_(0),
	lastSendTime_(0),
	recvMqQueue_(recvQue),
	sendMqQueue_(sendQue)
{
	RabbitMqProcess::instance()->setQueues(recvQue);
}
void MqDispatch::setRun(bool isRun)
{
	isRun_ = isRun;
}
void MqDispatch::setHeartBeat(int second)
{
	heartbeat_ = second;
}
int MqDispatch::connectMq()
{
	int res = RabbitMqProcess::instance()->init(hostname_, port_, user_, passwd_, spotId_);
	if (res)
	{
		return -1;
	}

	RabbitMqProcess::setRecvTimeOut(1, 0);
	lastRecvTime_ = CURR_STIME_POINT;
	lastSendTime_ = CURR_STIME_POINT;
	return 0;
}

void MqDispatch::sendTimeOut()
{
	base::QueueNode timeoutQueue;
	timeoutQueue.Tid = TID_TimeOut;
	recvMqQueue_->enqueue(timeoutQueue);
}
void MqDispatch::run()
{
	base::QueueNode queueNode;
	while (true)
	{
		try
		{
			int res = RabbitMqProcess::instance()->consumerRecv();
			if (res == 0)
			{
				lastRecvTime_ = CURR_STIME_POINT;
			}
			int nCount = 0;

			// 控制向mq发送的时间。
			while (nCount < 100 && sendMqQueue_->dequeue(queueNode))
			{
				sendMq(queueNode.Tid, queueNode.Data);
				++nCount;
				int64_t nowTime = CURR_STIME_POINT;
				if (nowTime - lastRecvTime_ >= heartbeat_)
				{
					LOG_WARN << "sendMq consume time(s):" << nowTime - lastRecvTime_;
					break;
				}
			}
			//处理超时 >3*为超时
			int64_t nowTime = CURR_STIME_POINT;
			if (nowTime - lastRecvTime_ > 3 * heartbeat_)
			{
				sendTimeOut();
				lastRecvTime_ = CURR_STIME_POINT;
			}
			//发送超时报文
			if (nowTime - lastSendTime_ >= heartbeat_)
			{
				base::QueueNode heartBeat;
				HeartBeat heart;
				heartBeat.Tid = TID_WriteHeartBeat;
				heartBeat.Data = &heart;
				sendMq(heartBeat.Tid, heartBeat.Data);
			}
		}
		catch (std::exception &ex)
		{
			LOG_ERROR << "exception:" << ex.what();
			sendTimeOut();
		}
		catch (...)
		{
			LOG_ERROR << "exception";
			sendTimeOut();
		}
	}

	return;
}

void MqDispatch::sendMq(int tid, void *body)
{
	int res = RabbitMqProcess::instance()->consumerSend(tid, body);
	if (res == 0)
	{
		lastSendTime_ = CURR_STIME_POINT;
	}
	else
	{
		LOG_ERROR << "consumerSend error  ";
	}
}




