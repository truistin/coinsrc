#include <spot/gateway/ShmMtSpi.h>
#include <spot/utility/Logging.h>

using namespace spot::gtway;

ShmMtSpi::ShmMtSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore)
	: queueStore_(queStore)
{
	apiInfo_ = apiInfo;
	marketTradeCallback_ = nullptr;
}


ShmMtSpi::~ShmMtSpi()
{
}

void ShmMtSpi::Init()
{
	std::vector<std::string> instruments;
	for (auto it = subInstMap_.begin(); it != subInstMap_.end(); ++it) {
		instruments.push_back(it->first);
		LOG_INFO << "ShmMtSpi frontMdAddr_: " << apiInfo_->frontMdAddr_
			<< ", instrument: " << it->first;
	}

	if (!init(apiInfo_->frontMdAddr_, instruments)) {
		LOG_FATAL << "ShmMtClient init failed!";
	}
	start();
}

void ShmMtSpi::AddSubInst(const char * inst)
{
	subInstMap_[inst] = inst;
	LOG_INFO << "ShmMtSpi::AddSubInst: " << inst;
}

void ShmMtSpi::onInnerMarketTrade(const spot::base::InnerMarketTrade& mt)
{
	if (marketTradeCallback_)
	{
		marketTradeCallback_(const_cast<spot::base::InnerMarketTrade&>(mt));
	}
	else 
	{
		queueStore_->storeHandle(const_cast<spot::base::InnerMarketTrade*>(&mt));
	}
}

void ShmMtSpi::setRecvMarketTradeCallback(RecvMarketTradeCallback marketTradeCallback)
{
	marketTradeCallback_ = marketTradeCallback;
}
