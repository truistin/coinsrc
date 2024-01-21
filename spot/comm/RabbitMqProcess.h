#ifndef SPOT_COMM_RABBITMQ_PROCESS_H
#define SPOT_COMM_RABBITMQ_PROCESS_H
#include <spot/base/DataStruct.h>
#include <spot/base/MqStruct.h>
#include <spot/utility/SPSCQueue.h>
#include <spot/utility/concurrentqueue.h>
#include <spot/rapidjson/document.h>
#include <spot/rapidjson/rapidjson.h>
#include <amqp.h>
#include <memory>
using namespace spot;
using namespace spot::utility;

#define RMQ_Priority_Low  0
#define RMQ_Priority_Medium 1 
#define RMQ_Priority_High  2
#define RMQ_Priority_Alarm  3

typedef spsc_queue<base::QueueNode>* MessageQue;
typedef moodycamel::ConcurrentQueue<base::QueueNode>* MessageQueueMPMC;
struct MqParam
{
	string hostname;
	int port;
	string user;
	string password;
	int spotid;
	int compressMode;

};
class RabbitMqProcess
{
public:
	RabbitMqProcess();
	void setQueues(MessageQue recvQue);
	int init(string hostname, int port, string user, string passwd, int spotId);
	static void setRecvTimeOut(unsigned sec, unsigned msec);
	int consumerRecv();
	void handleMqMessage(const char *body, unsigned int len);
	int consumerSend(int tid, void *body);
	int publishMeseage(string msg, int priority);

	amqp_connection_state_t recvConnection_;
	amqp_connection_state_t sendConnection_;
	static RabbitMqProcess* instance();
private:
	amqp_basic_properties_t props_;
	MessageQue recvMqQue_;
	MessageQueueMPMC mdQueue_;
	amqp_socket_t *socket_;
	static struct timeval tval_;
	string sendQueName_;
	string recvQueName_;

	static RabbitMqProcess* instance_;
	std::mutex mutex_;
};
#endif
