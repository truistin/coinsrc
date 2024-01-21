#include "spot/strategy/RmqWriter.h"
#include "spot/utility/Logging.h"
using namespace spot::strategy;

spsc_queue<QueueNode> * RmqWriter::rmqWriterQueue_;
RmqWriter::RmqWriter()
{
	rmqWriterQueue_ = new spsc_queue<QueueNode>();
};
RmqWriter::~RmqWriter()
{
	delete rmqWriterQueue_;
};
spsc_queue<QueueNode> * RmqWriter::getRmqWriterQueue()
{
	return rmqWriterQueue_;
}
void RmqWriter::enqueue(const QueueNode &node)
{
	rmqWriterQueue_->enqueue(node);
}

void RmqWriter::writeOrder(const Order &order)
{
#ifndef INIT_WITHOUT_RMQ
	Order *innerOrder = new Order();
	memcpy(innerOrder, &order, sizeof(Order));
	QueueNode node;
	node.Tid = TID_WriteOrder;
	node.Data = innerOrder;
	rmqWriterQueue_->enqueue(node);
#endif
}

void RmqWriter::writePNLDaily(const StrategyInstrumentPNLDaily& pnlDaily)
{
#ifndef INIT_WITHOUT_RMQ
	auto pnl = new StrategyInstrumentPNLDaily();
	memcpy(pnl, &pnlDaily, sizeof(StrategyInstrumentPNLDaily));
	QueueNode node;
	node.Tid = TID_WriteStrategyInstrumentPNLDaily;
	node.Data = pnl;
	rmqWriterQueue_->enqueue(node);
#endif
}

void RmqWriter::writeStrategyInstrumentTradeable(int strategyID, string instrumentID, bool tradeable)
{
#ifndef INIT_WITHOUT_RMQ
	auto strategySwitch = new StrategySwitch;
	strategySwitch->StrategyID = strategyID;
	memcpy(strategySwitch->InstrumentID,instrumentID.c_str(),sizeof(strategySwitch->InstrumentID));
	strategySwitch->Tradeable = tradeable;
	QueueNode node;
	node.Tid = TID_WriteStrategySwitch;
	node.Data = strategySwitch;
	rmqWriterQueue_->enqueue(node);
	LOG_INFO << "new strategy tradeable. strategyID=" << strategyID << " instrumentID=" << instrumentID.c_str() << " tradeable=" << tradeable;
#endif
}

void RmqWriter::writeStrategyParam(const StrategyParam& strategyParam)
{
#ifndef INIT_WITHOUT_RMQ
	auto param = new StrategyParam();
	memcpy(param, &strategyParam, sizeof(StrategyParam));
	QueueNode node;
	node.Tid = TID_WriteStrategyParam;
	node.Data = param;
	rmqWriterQueue_->enqueue(node);
	LOG_INFO << "StrategyParam change: " << strategyParam.StrategyID << " Name=" << strategyParam.Name;
#endif
}