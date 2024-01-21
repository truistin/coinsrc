#include "spot/strategy/StrategyCooker.h"
#include "spot/utility/Utility.h"
#include "spot/utility/Logging.h"
#include "spot/base/ParametersManager.h"
#include "spot/strategy/StrategyFactory.h"
#include "spot/strategy/Initializer.h"
#include "spot/strategy/RmqWriter.h"
#include "spot/base/MeasureLatencyCache.h"
#include "spot/strategy/MarketDataManager.h"
#include "spot/base/InitialData.h"
#include "spot/base/MqDecode.h"
#include "spot/shmmd/ShmManagement.h"

using namespace spot;
using namespace spot::base;
using namespace spot::strategy;

using namespace spot::shmmd;

StrategyCooker::StrategyCooker()
{	
	rmqReadQueue_ = new spsc_queue<QueueNode>;
	odrCtrler_ = new OrderController();
}

StrategyCooker::~StrategyCooker()
{
	delete rmqReadQueue_;
	delete odrCtrler_;
}
void StrategyCooker::setGatewayQueue(ConcurrentQueue<InnerMarketData>* mdQueue, ConcurrentQueue<InnerMarketTrade> *mtQueue, ConcurrentQueue<Order>* orderQueue)
{
	mdQueue_ = mdQueue;
	mtQueue_ = mtQueue;
	orderQueue_ = orderQueue;
}
void StrategyCooker::run()
{
	if(!initStrategyInstrument())
	{
		LOG_FATAL << "initialize failed";
	}
	StrategyFactory::initStrategy();
#ifndef INIT_WITHOUT_RMQ
	sendInitFinished();
#endif
	cout << "init finished. spotID=" << InitialData::getSpotID() << endl;
	sendStrategyInstrumentTradeable();

	//check trading period before marketdata coming
	runStrategy();
}

void StrategyCooker::sendStrategyInstrumentTradeable()
{
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		RmqWriter::writeStrategyInstrumentTradeable(strategyIter.first, "",strategyIter.second->isTradeable());
		for(auto instrumentIter : strategyIter.second->strategyInstrumentList())
		{
			RmqWriter::writeStrategyInstrumentTradeable(strategyIter.first, instrumentIter->getInstrumentID(), instrumentIter->isTradeable());
		}
	}
}

void StrategyCooker::runStrategy()
{
	// InnerMarketTrade mt;
	Order rtnOrder;
	// QueueNode rmqNode;
	while (true)
	{
		try
		{
			// handleMarketData();
			// handleMarketTrade(mt);
			handleRtnOrder(rtnOrder);
			// handleRmqData(rmqNode);	
			handleTimerEvent();
			handleShmData();

		}
		catch (std::overflow_error ex)
		{
			turnOffAllStrategy();
			LOG_ERROR << "overflow_error:" << ex.what();
		}
		catch (exception ex)
		{
			fprintf(stderr, "%s\n", ex.what());
			turnOffAllStrategy();			
			LOG_ERROR << "exception:" << ex.what();
		}
		catch (...)
		{
			turnOffAllStrategy();
			LOG_ERROR << "unknown exception";
		}
	}
}
void StrategyCooker::handleTimerEvent()
{
	if (gCircuitBreakTimerActive)
	{	
		handleCircuitBreakCheck(); //曝险定时器
		//restart timer
		gCircuitBreakTimerActive = false;
	}
	if (gForceCloseTimeIntervalActive)
	{
		handleForceCloseTimerInterval(); // 强平定时器
		gForceCloseTimeIntervalActive = false;
	}
	// if (gCancelOrderActive)
	// {
	// 	handleCheckCancelOrder();
	// 	gCancelOrderActive = false;
	// }
}

void StrategyCooker::handleCircuitBreakCheck()
{
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		LOG_DEBUG << "handleCircuitBreakCheck: StrategyCircuit check for strategy: " << strategyIter.first;
		strategyIter.second->OnTimer();
	}
}
void StrategyCooker::handleForceCloseTimerInterval()
{
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		LOG_DEBUG << "handleForceCloseTimerInterval: timer trigger for strategy: " << strategyIter.first;
		strategyIter.second->OnForceCloseTimerInterval();
	}
}
void StrategyCooker::handleCheckCancelOrder()
{
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{	
		//LOG_DEBUG << "timer trigger cancel order. for strategy: " << strategyIter.first;
		strategyIter.second->OnCheckCancelOrder();
	}
}
void StrategyCooker::handleMarketData()
{
	InnerMarketData md;
	auto ret = mdQueue_->try_dequeue(md);
	if (ret)
	{
		handleMarketData(md);
	}
}

void StrategyCooker::handleMarketData(InnerMarketData &md)
{
	LOG_DEBUG << "StrategyCooker handle MarketData contractId:" << md.InstrumentID
		<< " TradingDay:" << md.TradingDay
		<< " ExchangeID:" << md.ExchangeID
		<< " EpochTime:" << md.EpochTime
		<< " UpdateMillisec:" << md.UpdateMillisec 
		// << " Volume:" << md.Volume 
		<< " LastPrice:" << md.LastPrice
		<< " BidPrice1:" << md.BidPrice1 << " BidVolume1:" << md.BidVolume1
		<< " AskPrice1:" << md.AskPrice1 << " AskVolume1:" << md.AskVolume1
		<< " MessageID:" << md.MessageID;
		// << " BidPrice2:" << md.BidPrice2 << " BidVolume2:" << md.BidVolume2
		// << " AskPrice2:" << md.AskPrice2 << " AskVolume2:" << md.AskVolume2
		// << " BidPrice3:" << md.BidPrice3 << " BidVolume3:" << md.BidVolume3
		// << " AskPrice3:" << md.AskPrice3 << " AskVolume3:" << md.AskVolume3
		// << " BidPrice4:" << md.BidPrice4 << " BidVolume4:" << md.BidVolume4
		// << " AskPrice4:" << md.AskPrice4 << " AskVolume4:" << md.AskVolume4
		// << " BidPrice5:" << md.BidPrice5 << " BidVolume5:" << md.BidVolume5
		// << " AskPrice5:" << md.AskPrice5 << " AskVolume5:" << md.AskVolume5;

	gStrategyThreadMsgID = md.MessageID;

	MarketDataManager::instance().OnRtnInnerMarketData(md);
//	gTickToOrderMeasure->sendMetricData();
//	gStrategyThreadMsgID = 0;
}

void StrategyCooker::handleMarketTrade(InnerMarketTrade &mt)
{
	auto ret = mtQueue_->try_dequeue(mt);
	if (ret)
	{
		LOG_DEBUG << "StrategyCooker handleMarketTrade InstrumentID:" << mt.InstrumentID
			<< " ExchangeID:" << mt.ExchangeID
			<< " Tid:" << mt.Tid
			<< " Direction:" << mt.Direction
			<< " Price:" << mt.Price
			<< " Volume:" << mt.Volume
			<< " Turnover:" << mt.Turnover
			<< " MessageID:" << mt.MessageID
			<< " TradingDay:" << mt.TradingDay
			<< " EpochTime:" << mt.EpochTime
			<< " UpdateMillisec:" << mt.UpdateMillisec;
			// << " UpdateTime:" << mt.UpdateTime;

		//gStrategyThreadMsgID = mt.MessageID;
		//gTickToOrderMeasure->addMeasurePoint(0, mt.EpochTime);

		MarketDataManager::instance().OnRtnInnerMarketTrade(mt);
		//gTickToOrderMeasure->sendMetricData();
		//gStrategyThreadMsgID = 0;
	}
}
void StrategyCooker::handleRtnOrder(Order &rtnOrder)
{
		auto ret = orderQueue_->try_dequeue(rtnOrder);
		if (ret)
		{

			LOG_INFO << "handleRtnOrder OrderRef: " << rtnOrder.OrderRef
				<< ", Instrument: " << rtnOrder.InstrumentID
				<< ", ExchangeCode: " << rtnOrder.ExchangeCode
				<< ", OrderSysID: " << rtnOrder.OrderSysID
				<< ", Volume: " << rtnOrder.Volume
				<< ", VolumeFilled: " << rtnOrder.VolumeFilled
				<< ", VolumeRemained: " << rtnOrder.VolumeRemained
				<< ", LimitPrice: " << rtnOrder.LimitPrice
				<< ", Price: " << rtnOrder.Price
				<< ", VolumeTotalOriginal: " << rtnOrder.VolumeTotalOriginal
				<< ", OrderStatus: " << rtnOrder.OrderStatus;
			odrCtrler_->OnRtnOrder(rtnOrder);
		}
}
void StrategyCooker::handleRmqData(QueueNode &node)
{
	auto ret = rmqReadQueue_->dequeue(node);
	if (ret)
	{
		LOG_INFO << "handleRmqData recvmsg Tid:" << node.Tid;
		switch (node.Tid)
		{
		case TID_TimeOut:
			LOG_ERROR << "CB Alarm - heartbeat timeout";
			setAllStrategyTradeable(false);
			break;
		case TID_StrategyParam:
			updateStrategyParam(node);
			break;
		case TID_StrategySwitch:
			updateStrategySwitch(node);
			break;
		case TID_MarketData:
			updateMarketData(node);
			break;
		default:
			string buffer = "invalid Tid:";
			buffer = buffer + std::to_string(node.Tid);
			LOG_ERROR << buffer.c_str();
			break;
		}
	}
}

void StrategyCooker::handleShmData()
{
	ShmManagement::instance().loop();
}

void StrategyCooker::setAllStrategyTradeable(bool tradeable)
{
	LOG_DEBUG << " tradeable=" << tradeable;
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		if(tradeable)
		{
			strategyIter.second->turnOnStrategy();
		}
		else
		{
			strategyIter.second->turnOffStrategy();
		}
	}
}

void StrategyCooker::updateStrategyParam(const QueueNode& node)
{
	auto paramList = reinterpret_cast<list<StrategyParam>*>(node.Data);
	if(!checkStrategyParams(paramList))
	{
		return;
	}
	
	for (auto updateParam : *paramList)
	{
		LOG_INFO << "recv update parameters:" << getStrategyParamString(updateParam);
		if (strcmp(updateParam.Name, "orderSpacing") == 0)
		{
			auto strategy = StrategyFactory::getStrategy(updateParam.StrategyID);
			if (strategy && strategy->hasUnfinishedOrder())
			{
				LOG_WARN << "CB Alarm " << updateParam.StrategyID << " - strategy has unfinished order,so cann't update orderSpacing.StrategyID:" << updateParam.StrategyID;
				continue;
			}
		}
		StrategyParam param;
		convertStrategyParam(updateParam,param);
		auto parameter = ParametersManager::getStrategyParameters(param.StrategyID);
		if (parameter)
		{
			parameter->setStrategyParameter(param);
			LOG_INFO << "updated parameters:" << getStrategyParamString(updateParam);
			RmqWriter::writeStrategyParam(updateParam);
		}
		else
		{
			LOG_WARN << "updateStrategyParam failed. cann't find StrategyID:" << updateParam.StrategyID;
		}
	}
}
bool StrategyCooker::checkStrategyParams(list<StrategyParam>* paramList)
{
	for (auto param : *paramList)
	{
		auto parameter = ParametersManager::getStrategyParameters(param.StrategyID);
		if (!parameter)
		{
			LOG_WARN << "checkStrategyParams cann't find StartegyID:" << param.StrategyID;
			return false;
		}
		auto oldParam = parameter->getStrategyParam(param.Name);
		if (!oldParam)
		{
			LOG_WARN << "checkStrategyParams cann't find paramName:" << param.Name;
			return false;
		}
		if(strcmp(oldParam->ValueType,param.ValueType) != 0)
		{
			LOG_WARN << "checkStrategyParams invalid ValueType:" << param.ValueType
					  << " old ValueType:" << oldParam->ValueType;
			return false;
		}	
	}
	return true;
}

void StrategyCooker::updateStrategySwitch(const QueueNode &node)
{
	auto straSwitchList = reinterpret_cast<list<StrategySwitch>*>(node.Data);
	for (auto straSwitch : *straSwitchList)
	{
		auto tradeable = (straSwitch.Tradeable == 1);
		LOG_INFO << "StrategyCooker::updateStrategySwitch " << straSwitch.toString();
		if (strlen(straSwitch.InstrumentID) == 0)
		{
			setStrategyTradeable(straSwitch.StrategyID, tradeable);
		}
		else
		{
			setStrategyInstrumentTradeable(straSwitch.StrategyID, straSwitch.InstrumentID, tradeable);
		}

		RmqWriter::writeStrategyInstrumentTradeable(straSwitch.StrategyID, straSwitch.InstrumentID, tradeable);

	}
}

void StrategyCooker::updateMarketData(const QueueNode& node)
{
	auto marketDataList = reinterpret_cast<list<MarketData>*>(node.Data);
	for (auto marketData : *marketDataList)
	{
		InnerMarketData md;
		memset(&md, 0, sizeof(md));
		memcpy(&md, &marketData, sizeof(MarketData));

		handleMarketData(md);
	}
}

bool StrategyCooker::setStrategyTradeable(int strategyID, bool tradeable)
{
	LOG_DEBUG << "strategyID=" << strategyID << " tradeable=" << tradeable;
	auto strategyMap = StrategyFactory::strategyMap();
	auto strategyIter = strategyMap.find(strategyID);
	if (strategyIter == strategyMap.end())
	{
		LOG_ERROR << "cann't find strategyID:" << strategyID;
		return false;
	}
	auto ret = false;
	if (tradeable)
	{
		ret = strategyIter->second->turnOnStrategy();
	}
	else
	{
		ret = strategyIter->second->turnOffStrategy();
	}
	return ret;
}

bool StrategyCooker::setStrategyInstrumentTradeable(int strategyID, string instrumentID, bool tradeable)
{
	auto strategyMap = StrategyFactory::strategyMap();
	auto strategyIter = strategyMap.find(strategyID);
	if (strategyIter == strategyMap.end())
	{
		LOG_ERROR << "cann't find strategyID:" << strategyID;
		RmqWriter::writeStrategyInstrumentTradeable(strategyID, instrumentID, !tradeable);
		return false;
	}
	auto ret = false;
	if (tradeable)
	{
		ret = strategyIter->second->turnOnInstrument(instrumentID);
	}
	else
	{
		ret = strategyIter->second->turnOffInstrument(instrumentID);
	}
	return ret;
}
void StrategyCooker::turnOffAllStrategy()
{
	LOG_INFO << "StrategyCooker::turnOffAllStrategy";
	auto strategyMap = StrategyFactory::strategyMap();
	for(auto strategy : strategyMap)
	{
		strategy.second->turnOffStrategy();
	}
}

void StrategyCooker::setAdapter(Adapter *adapter)
{
	odrCtrler_->setAdapter(adapter);
}

void StrategyCooker::sendInitFinished()
{
	QueueNode node;
	node.Tid = TID_InitFinished;
	node.Data = nullptr;
	RmqWriter::enqueue(node);
	LOG_INFO << "spot_hft init finished spotID=" << InitialData::getSpotID();
}

bool StrategyCooker::initStrategyInstrument()
{
	auto strategyMap = StrategyFactory::strategyMap();
	for (auto strategyIter : strategyMap)
	{
		list<string> instrumentList;
		Initializer::getInstrumentListByStrategyID(strategyIter.first, instrumentList);
		if(instrumentList.size() ==0)
		{
			LOG_FATAL <<  "cann't find instrument. strategyID:" << strategyIter.first;
			return false;
		}
		for (auto instrumentIDIter : instrumentList)
		{
			strategyIter.second->addStrategyInstrument(instrumentIDIter.c_str(), odrCtrler_);
		}	 
	}
	return true;
}

void StrategyCooker::convertStrategyParam(const StrategyParam& updateParam, StrategyParam& param)
{
	param.StrategyID = updateParam.StrategyID;
	memcpy(param.Name, updateParam.Name, sizeof(param.Name));
	memcpy(param.ValueType, updateParam.ValueType, sizeof(param.ValueType));
	memcpy(param.ValueString, updateParam.ValueString, sizeof(param.ValueString));
	param.ValueInt = updateParam.ValueInt;
	param.ValueDouble = updateParam.ValueDouble;
}
