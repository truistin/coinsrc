#include "RabbitMqTest.h"
#include <spot/comm/MqDispatch.h>
#include <memory>
#include <spot/utility/LogFile.h>
#include <spot/utility/Logging.h>


void asyncOutput(const char* msg, int len)
{
  fprintf(stderr, "%s", msg);
}
int main(int argc, char **argv)
{
  spot::Logger::setOutput(asyncOutput);
  spot::Logger::setLogLevel(spot::Logger::LogLevel::DEBUG);
  MessageQue recvQue = new spsc_queue<base::QueueNode>;
  MessageQue sendQue = new spsc_queue<base::QueueNode>;
  MessageQueueMPMC lastPriceQue = new moodycamel::ConcurrentQueue<base::QueueNode>;
  MqDispatch mqDispatch("192.168.1.149", 5672, "spotmq", "root", 1, recvQue,sendQue,lastPriceQue);
  mqDispatch.connectMq();
  mqDispatch.run();
  return 0;
}

