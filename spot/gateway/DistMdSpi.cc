#include <spot/gateway/DistMdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/base/InitialData.h>
#include <spot/shmmd/ShmMdServer.h>
#include <spot/distributor/DistServer.h>

using namespace spot::gtway;
using namespace spot::shmmd;
using namespace spot::distributor;

DistMdSpi::DistMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack *queStore)
	: queueStore_(queStore)
{
	apiInfo_ = apiInfo;
}


DistMdSpi::~DistMdSpi()
{

}

void DistMdSpi::Init()
{
	std::vector<std::string> instruments;
	for (auto it = subInstMap_.begin(); it != subInstMap_.end(); ++it) {
		instruments.push_back(it->first);
	}

	char spotid[256] = { 0 };
	sprintf(spotid, "%d", InitialData::getSpotID());
	if (!init(apiInfo_->frontMdAddr_, instruments, spotid)) {
		LOG_FATAL << "DistMdSpi init failed!";
	}
	start();
}

void DistMdSpi::AddSubInst(const char * inst)
{
	subInstMap_[inst] = inst;
	LOG_INFO << "DistMdSpi::AddSubInst: " << inst;
}

void DistMdSpi::onMsg(const spot::base::InnerMarketData* innerMktData)
{
	spot::base::InnerMarketData* pData = const_cast<spot::base::InnerMarketData*>(innerMktData);
	pData->EpochTime = CURR_USTIME_POINT;
	if (ShmMdServer::instance().isInit())
	{
		ShmMdServer::instance().update(pData);
	}
	if (DistServer::instance().isInit())
	{
		DistServer::instance().update(pData);
	}
	if (!ShmMdServer::instance().isInit() && !DistServer::instance().isInit())
	{
		queueStore_->storeHandle(const_cast<spot::base::InnerMarketData*>(pData));
	}
}
