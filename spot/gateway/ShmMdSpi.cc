#include <spot/base/InitialData.h>
#include <spot/gateway/ShmMdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/distributor/DistServer.h>
using namespace spot::distributor;
using namespace spot::gtway;

ShmMdSpi::ShmMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore)
	: queueStore_(queStore)
{
	apiInfo_ = apiInfo;
	pServer_ = nullptr;
	marketDataCallback_ = nullptr;
}

ShmMdSpi::~ShmMdSpi()
{

}

void ShmMdSpi::Init()
{
	string multiCastIpPort;
	string interfaceIp;
	auto configMap = InitialData::spotInitConfigMap();
	auto iter = configMap.find("MCAST.multiCastIpPort");
	if (iter != configMap.end())
	{
		multiCastIpPort = iter->second.FieldValue;
	}
	iter = configMap.find("MCAST.interfaceIp");
	if (iter != configMap.end())
	{
		interfaceIp = iter->second.FieldValue;
	}
	LOG_INFO << "multicast config, MCAST.multiCastIpPort=" << multiCastIpPort << ", MCAST.interfaceIp=" << interfaceIp;
	if (!multiCastIpPort.empty() && !interfaceIp.empty()) {
		pServer_ = new UdpMcastServer(multiCastIpPort, interfaceIp);
	}

	std::vector<std::string> instruments;
	for (auto it = subInstMap_.begin(); it != subInstMap_.end(); ++it) {
		instruments.push_back(it->first);
		LOG_INFO << "ShmMdSpi frontMdAddr_: " << apiInfo_->frontMdAddr_
			<< ", instrument: " << it->first;
	}
	
	
	while (!init(apiInfo_->frontMdAddr_, instruments)) 
	{
		LOG_ERROR << "ShmMdClient init failed!";
		// SLEEP(3000);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}
	start();
}

void ShmMdSpi::AddSubInst(const char * inst)
{
	subInstMap_[inst] = inst;
	LOG_DEBUG << "ShmMdSpi::AddSubInst: " << inst;
}

void ShmMdSpi::onInnerMarketData(const spot::base::InnerMarketData& md)
{
	if (marketDataCallback_)
	{
		marketDataCallback_(const_cast<spot::base::InnerMarketData&>(md));
	}
	else if (DistServer::instance().isInit())
	{
		DistServer::instance().update(&md);
	}
	else if (pServer_) {
		pServer_->sendMsg(reinterpret_cast<const char*>(&md), sizeof(InnerMarketData));
	}
	else 
	{
		queueStore_->storeHandle(const_cast<spot::base::InnerMarketData*>(&md));
	}
}
void ShmMdSpi::setReckMarketDataCallback(RecvMarketDataCallback marketDataCallback)
{
	marketDataCallback_ = marketDataCallback;
}

