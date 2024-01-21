#ifndef SPOT_COMM_MQDISPATCH_H
#define SPOT_COMM_MQDISPATCH_H
#include "spot/comm/RabbitMqProcess.h"

class MqDispatch
{
public:
	MqDispatch(MqParam mqParam, MessageQue recvQue, MessageQue sendQue);
	int connectMq();
	void run();
	void setRun(bool isRun);
	void setHeartBeat(int second);
	void sendMq(int tid, void *body);
	void sendTimeOut();	
private:
	string hostname_;
	int port_;
	string user_;
	string passwd_;
	int spotId_;
	bool ttl_;
	MessageQue recvMqQueue_;
	MessageQue sendMqQueue_;
	MessageQueueMPMC mdQueue_;
	bool isRun_;
	int heartbeat_;
	int64_t  lastRecvTime_;
	int64_t  lastSendTime_;
};


#endif
