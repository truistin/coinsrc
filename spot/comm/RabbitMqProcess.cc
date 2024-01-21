#include <spot/comm/RabbitMqProcess.h>
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
#include <spot/base/BoostGZip.h>
#include <spot/base/MqDecode.h>
using namespace spot::base;

struct timeval RabbitMqProcess::tval_ = { 1, 0 };

RabbitMqProcess::RabbitMqProcess()
{ 
	socket_ = nullptr;
	recvConnection_ = nullptr;
}
void RabbitMqProcess::setQueues(MessageQue recvQue)
{
	recvMqQue_ = recvQue;
	//mdQueue_ = mdQueue;
}
RabbitMqProcess* RabbitMqProcess::instance() {
	static RabbitMqProcess instance_;
	return &instance_;
};
void RabbitMqProcess::setRecvTimeOut(unsigned sec, unsigned usec)
{
	tval_.tv_sec = sec;
	tval_.tv_usec = usec;
}


int RabbitMqProcess::init(string hostname, int port, string user, string passwd, int spotId)
{
	int res = init_rabbitmq(recvConnection_, hostname, port, user, passwd);
	if (res)
	{
		LOG_FATAL << "init_rabbitmq fatal";
		return -1;
	}
	res = init_rabbitmq(sendConnection_, hostname, port, user, passwd);
	if (res)
	{
		LOG_FATAL << "init_rabbitmq fatal";
		return -1;
	}

	recvQueName_ = "ServerToSpot" + std::to_string(spotId);
	sendQueName_ = "SpotToServer" + std::to_string(spotId);

	res = queue_purge(recvConnection_, recvQueName_);
	if (res)
	{
		LOG_FATAL << "queue_purge failed " << recvQueName_;
		return -1;
	}
	else
	{
		LOG_INFO << "restart purge queue = " << recvQueName_;
	}

	props_._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_PRIORITY_FLAG;
	props_.content_type = amqp_cstring_bytes("text/plain");
	props_.delivery_mode = 2; /* persistent delivery mode */
	props_.priority = 0; //default is 0, we have 3 levels 0,1,2 - 0 is lowest priority, 2 is highest priority

	//0,1,0 auto ack 0,0,0 no auto ack
	amqp_basic_consume(recvConnection_, 1, amqp_bytes_malloc_dup(amqp_cstring_bytes(recvQueName_.c_str())), amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
//	amqp_basic_consume(recvConnection_, 1, amqp_bytes_malloc_dup(amqp_cstring_bytes(recvQueName_.c_str())), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);

	res = die_on_amqp_error(amqp_get_rpc_reply(recvConnection_), "Consuming");
	if (res)
	{
		LOG_FATAL << "amqp_get_rpc_reply - consuming failed" << sendQueName_;
		return -1;
	}
	return 0;
}
int RabbitMqProcess::consumerRecv()
{
	static int count = 0;
	amqp_envelope_t envelope;
	amqp_maybe_release_buffers(recvConnection_);
	amqp_rpc_reply_t ret = amqp_consume_message(recvConnection_, &envelope, &tval_, 0);
	int res = die_on_amqp_error(ret, "consume receive message");
	if (res)
	{
		return -1;
	}
	handleMqMessage(static_cast<const char *>(envelope.message.body.bytes), envelope.message.body.len);

	amqp_destroy_envelope(&envelope);
	return 0;
}

int RabbitMqProcess::publishMeseage(string msg, int priority) 
{
	mutex_.lock();
	amqp_bytes_t pubMsg;
	string enBuf;

	props_.priority = priority; //default is 0, we have 3 levels 0,1,2 - 0 is lowest priority, 2 is highest priority
	LOG_DEBUG << "priority: " << props_.priority << " QueueName:" << sendQueName_;	
	pubMsg.bytes = (void *)msg.c_str();
	pubMsg.len = msg.length();
	int res = amqp_basic_publish(sendConnection_,
		1,
		amqp_cstring_bytes(""),
		amqp_cstring_bytes(sendQueName_.c_str()),
		0,
		0,
		&props_,
		pubMsg);
	mutex_.unlock();
	if (res < 0)
	{
		string buffer = "publish error: ";
		buffer = buffer + amqp_error_string2(res);		
		LOG_ERROR << buffer.c_str();
	}

	return 0;
}
int RabbitMqProcess::consumerSend(int tid, void *body)
{
	int res = 0;
	switch (tid)
	{
	case TID_Init:
	{
		StrategyInit *init = static_cast<StrategyInit*>(body);
		res = publishMeseage(init->toJson(), RMQ_Priority_High);
		delete init;
		break;
	}
	case TID_InitFinished:
	{
		StrategyFinish finish;
		res = publishMeseage(finish.toJson(), RMQ_Priority_High);
		break;
	}
	case TID_WriteStrategyInstrumentPNLDaily:
	{
		StrategyInstrumentPNLDaily *pnlDaily = static_cast<StrategyInstrumentPNLDaily*>(body);
		res = publishMeseage(pnlDaily->toJson(), RMQ_Priority_High);
		delete pnlDaily;
		break;
	}
	case TID_WriteOrder:
	{
		Order *order = static_cast<Order*>(body);
		res = publishMeseage(order->toJson(), RMQ_Priority_Medium);
		delete order;
		break;
	}
	case TID_WriteHeartBeat:
	{
		HeartBeat *heartBeat = static_cast<HeartBeat*>(body);
		res = publishMeseage(heartBeat->toJson(), RMQ_Priority_Low);
		break;
	}
	case TID_WriteStrategySwitch:
	{
		StrategySwitch *strategySwitch = static_cast<StrategySwitch*>(body);
		res = publishMeseage(strategySwitch->toJson(), RMQ_Priority_High);
		delete strategySwitch;
		break;
	}
	case TID_WriteStrategyParam:
	{
		StrategyParam *updateStrategyParams = static_cast<StrategyParam*>(body);
		res = publishMeseage(updateStrategyParams->toJson(), RMQ_Priority_High);
		delete updateStrategyParams;
		break;
	}
	case TID_WriteMetricData:
	{
		MetricData *metricData = static_cast<MetricData*>(body);
		res = publishMeseage(metricData->toJson(), RMQ_Priority_Low);
		delete metricData;
		break;
	}
	//case TID_WriteMqMkData:
	//{
	//	MarketData *mqMarketData = static_cast<MarketData*>(body);
	//	res = publishMeseage(mqMarketData->toJson(), RMQ_Priority_High);
	//	delete mqMarketData;
	//	break;
	//}

	case TID_WriteAlarm:
	{
		Alarm *alarm = static_cast<Alarm*>(body);
		res = publishMeseage(alarm->toJson(), RMQ_Priority_Alarm);
		LOG_WARN << alarm->toJson();
		delete alarm;
		break;
	}

	default:
	{
		res = -1;
		LOG_ERROR << "write tid not found: " << tid;
		break;
	}
	}

	if (res < 0)
		return -1;
	return 0;
}

void RabbitMqProcess::handleMqMessage(const char *body, unsigned int len)
{
	string recvData(body, len);
	LOG_DEBUG << "recv len:" << len << " data:" << recvData;
	spotrapidjson::Document doc;
	doc.Parse(body, len);
	if (doc.HasParseError())
	{
		LOG_ERROR << "json parse error";
	}
	string strTitle;
	if (doc.HasMember("Title"))
	{
		spotrapidjson::Value& title = doc["Title"];
		strTitle = title.GetString();
	}
	else
	{
		LOG_ERROR << "cann't find Title";
	}
	//Heartbeat:无需处理
	if (strTitle.compare("Heartbeat") == 0 || strTitle.compare("EndTimeBinner") == 0)
		return;

	QueueNode queueNode;
	if (doc.HasMember("Content"))
	{
		spotrapidjson::Value& content = doc["Content"];
		spotrapidjson::Value arrayValue = content.GetArray();
		queueNode = decode(strTitle, arrayValue);
	}  //InitFinish,EndTimeBinner：没有Content
	else if (strTitle.compare("InitFinish") == 0)
	{
		queueNode.Tid = TID_InitFinished;
		queueNode.Data = nullptr;
	}
	if (strTitle.compare("StrategySwitch") == 0)
	{
		LOG_INFO << "recv StrategySwitch from rmq";
	}
	else if (strTitle.compare("StrategyParam") == 0)
	{
		LOG_INFO << "recv UpdateStrategyParams from rmq";
	}
	recvMqQue_->enqueue(queueNode);
}
