#include <spot/gateway/HuobFutureMdSpi.h>
#include <spot/utility/Logging.h>
#include <spot/gateway/MktDataStoreCallBack.h>
#include <spot/utility/Compatible.h>
#include <functional>
#include <spot/utility/gzipwrapper.h>
#include "spot/huobapi/HuobApi.h"

using namespace spot;
using namespace spot::gtway;
using namespace spot::utility;
using namespace spotrapidjson;
//#define HUOB_FUTURE_URL  "ws://172.18.6.16:8880/ws"
#define HUOB_FUTURE_URL  "wss://api.hbdm.com/ws"

const unsigned int OrderBookLevelMax = 5;
HuobFutureMdSpi::HuobFutureMdSpi(std::shared_ptr<ApiInfoDetail> &apiInfo, MktDataStoreCallBack* queueStore)
{
	apiInfo_ = apiInfo;
	queueStore_ = queueStore;
}

HuobFutureMdSpi::~HuobFutureMdSpi()
{

}
void HuobFutureMdSpi::Init()
{
	isHuobFutureMDConnected_ = false;
	websocketApi_ = new WebSocketApi();
	websocketApi_->SetUri(HUOB_FUTURE_URL);

	LOG_INFO << "frontMdAddr_:"<< HUOB_FUTURE_URL;
	//websocketApi_->Set_Protocol_Mode(true);
	websocketApi_->SetCompress(true);
	websocketApi_->SetCallBackOpen(std::bind(&HuobFutureMdSpi::com_callbak_open, this));
	websocketApi_->SetCallBackMessage(std::bind(&HuobFutureMdSpi::com_callbak_message, this, std::placeholders::_1));
	websocketApi_->SetCallBackClose(std::bind(&HuobFutureMdSpi::com_callbak_close, this));

	std::thread comThread(std::bind(&WebSocketApi::Run, websocketApi_));
	comThread.detach();

	std::thread timerThread(&HuobFutureMdSpi::Run, this);
	timerThread.detach();
}
void HuobFutureMdSpi::com_callbak_open()
{
	std::cout << "huob com_callbak_open" << std::endl;
	LOG_INFO << "huob com_callbak_open";
	subscribe();
	isHuobFutureMDConnected_ = true;
}

void HuobFutureMdSpi::com_callbak_close()
{
	std::cout << "huob com_callbak_close" << std::endl;
	LOG_INFO << "huob com_callbak_close";
	isHuobFutureMDConnected_ = false;
}
void HuobFutureMdSpi::subscribe()
{
	map<string, string> channelMap = HuobApi::GetFutureLongChannelMap();

	for (auto iter : channelMap)
	{
		//¶©ÔÄÆÚ»õ
		LOG_INFO << "subscribe channel:" << iter.first;
		subscribe(iter.first);
	}
}
void HuobFutureMdSpi::subscribe(string channel)
{	
	string cmd = "{\"sub\":\"";
	cmd += channel;
	cmd += "\",\"id\":\"";
	cmd += "LiangZhan123";
	cmd += "\"}";
	websocketApi_->send(cmd);
}
void HuobFutureMdSpi::com_callbak_message(const char *message)
{
	try
	{
		spotrapidjson::Document documentCOM_;
		documentCOM_.Parse<0>(message);

		if (documentCOM_.HasParseError())
		{
			LOG_ERROR << "Parsing to document failure";
			return;
		}

		if (documentCOM_.HasMember("ping"))
		{
			spotrapidjson::Value& eventNode = documentCOM_["ping"];
			int64_t ping = eventNode.GetInt64();
			std::ostringstream oss;
			oss << ping; std:string pongAsString(oss.str());
			pongAsString = "{\"pong\":" + pongAsString + "}";

			LOG_INFO << "pongAsString: " << pongAsString;
			websocketApi_->send(pongAsString);
			return;
		}

		if (documentCOM_.HasMember("ch"))
		{
			spotrapidjson::Value& channelNode = documentCOM_["ch"];
			string channel = channelNode.GetString();
			map<string, string> channelMap = HuobApi::GetFutureLongChannelMap();
			if (channelMap.find(channel) != channelMap.end())
			{
				if (string::npos != channel.find(depth_channel_name))
				{
					InnerMarketData field_;
					memset(&field_, 0, sizeof(field_));
					field_.LevelType = MDLEVELTYPE_DEFAULT;
					strcpy(field_.ExchangeID, Exchange_HUOB.c_str());
					field_.EpochTime = CURR_USTIME_POINT;
					strcpy(field_.UpdateTime, getCurrentSystemTime().c_str());
					string inst = HuobApi::GetInstrumentID(channel,true);
					strcpy(field_.InstrumentID, inst.c_str());

					if (documentCOM_.HasMember("tick"))
					{
						spotrapidjson::Value& tickNode = documentCOM_["tick"];
						if (tickNode.HasMember("asks"))
						{
							spotrapidjson::Value& asks = tickNode["asks"];
							auto askSize = min(OrderBookLevelMax, asks.Size());
							for (unsigned int i = 0; i < askSize; ++i)
							{
								spotrapidjson::Value& priceNode = asks[i][0];
								double price = priceNode.GetDouble();
								field_.setAskPrice(i + 1, price);
								spotrapidjson::Value& volumeNode = asks[i][1];
								double volume = volumeNode.GetDouble();
								field_.setAskVolume(i + 1, volume);
							}
						}

						if (tickNode.HasMember("bids"))
						{
							spotrapidjson::Value& bids = tickNode["bids"];
							auto bidSize = min(OrderBookLevelMax, bids.Size());
							for (unsigned int i = 0; i < bidSize; ++i)
							{
								spotrapidjson::Value& priceNode = bids[i][0];
								double price = priceNode.GetDouble();
								field_.setBidPrice(i + 1, price);
								spotrapidjson::Value& volumeNode = bids[i][1];
								double volume = volumeNode.GetDouble();
								field_.setBidVolume(i + 1, volume);
							}
						}
					}
					queueStore_->storeHandle(&field_);
				}
				if (string::npos != channel.find(trade_channel_name))
				{
					InnerMarketTrade trade_;
					memset(&trade_, 0, sizeof(trade_));
					if (documentCOM_.HasMember("tick"))
					{
						spotrapidjson::Value& tickNode = documentCOM_["tick"];
						if (tickNode.HasMember("data"))
						{
							spotrapidjson::Value& dataNode = tickNode["data"];
							if (dataNode.IsArray())
							{
								for (int i = 0; i < dataNode.Size(); ++i)
								{
									spotrapidjson::Value& tradeNode = dataNode[i];

									strcpy(trade_.ExchangeID, Exchange_HUOB.c_str());
									trade_.EpochTime = CURR_USTIME_POINT;
									strcpy(trade_.UpdateTime, getCurrentSystemTime().c_str());
									string inst = HuobApi::GetInstrumentID(channel,true);
									strcpy(trade_.InstrumentID, inst.c_str());
									trade_.MessageID = getMessageID();

									if (tradeNode.HasMember("tradeId"))
									{
										trade_.Tid = stoll(tradeNode["tradeId"].GetString());
									}
									if (tradeNode.HasMember("direction"))
									{
										if (0 == strcmp(tradeNode["direction"].GetString(), "buy"))
										{
											trade_.Direction = INNER_DIRECTION_Buy;
										}
										else
										{
											trade_.Direction = INNER_DIRECTION_Sell;
										}
									}
									if (tradeNode.HasMember("price"))
									{
										if (tradeNode["price"].IsString())
										{
											trade_.Price = stod(tradeNode["price"].GetString());
										}
										else
										{
											trade_.Price = tradeNode["price"].GetDouble();
										}
									}
									if (tradeNode.HasMember("amount"))
									{
										if (tradeNode["amount"].IsString())
										{
											trade_.Volume = stod(tradeNode["amount"].GetString());
										}
										else
										{
											trade_.Volume = tradeNode["amount"].GetDouble();
										}
									}

								}
							}
							else if (dataNode.IsObject())
							{
								cout << "Is is object";
								return;
							}

						}
					}
					queueStore_->storeHandle(&trade_);
				}
			}
			else
			{
				LOG_INFO << "not market channel: " << channel;
			}
		}
		else
		{
			LOG_INFO << "recv non-ch msg:" << message;
		}
	}
	catch (exception ex)
	{
		LOG_WARN << "HuobFutureMdSpi::com_callbak_message exception occur:" << ex.what();
	}
}
void HuobFutureMdSpi::AddSubInst(Instrument *inst)
{
	bool isFuture = true;
	HuobApi::AddInstrument(inst, isFuture);
}

void HuobFutureMdSpi::Run()
{
	while (true)
	{
		SLEEP(29000);
		if (isHuobFutureMDConnected_) {
			std::ostringstream oss;
			oss << CURR_MSTIME_POINT;
			std:string pingAsString(oss.str());
			pingAsString = "{\"ping\":" + pingAsString + "}";			
			websocketApi_->send(pingAsString);
			LOG_INFO << "send:" << pingAsString;
		}
	}
}
