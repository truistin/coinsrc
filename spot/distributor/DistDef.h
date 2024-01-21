#ifndef SPOT_DIST_DEF_H
#define SPOT_DIST_DEF_H

#define DIST_MAX_BUFF_LEN 2*1024
#define DIST_MAX_EPOLL_SIZE 1024

#include <string>
#include <sstream>
#include <spot/base/DataStruct.h>
using namespace spot::base;

namespace spot
{
	namespace distributor
	{
		//DistMsgType
		enum
		{
			Dist_ConnReqMsg = 1,
			Dist_ConnRspMsg = 2,
			Dist_SubReqMsg = 3,
			Dist_SubRspMsg = 4,
			Dist_UnsubReqMsg = 5,
			Dist_UnsubRspMsg = 6,
			Dist_InnerMarketData = 7,
		};

		struct DistMsgHeader
		{
			int msgType;
			int msgLen;
			char msgBody[1];
		};

		struct DistConnReqMsg
		{
			int msgType;
			int msgLen;
			char tagStr[1024];
			uint64_t epochTime;

			DistConnReqMsg() {
				msgType = Dist_ConnReqMsg;
				msgLen = sizeof(DistConnReqMsg) - 2 * sizeof(int);
				tagStr[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "ConnReqMsg={";
				os << "tagStr=" << tagStr;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistConnRspMsg
		{
			int msgType;
			int msgLen;
			char tagStr[1024];
			int code;
			char msg[1024];
			uint64_t epochTime;

			DistConnRspMsg() {
				msgType = Dist_ConnRspMsg;
				msgLen = sizeof(DistConnRspMsg) - 2 * sizeof(int);
				tagStr[0] = 0;
				code = 0;
				msg[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "ConnRspMsg={";
				os << "tagStr=" << tagStr;
				os << " code=" << code;
				os << " msg=" << msg;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistSubReqMsg
		{
			int msgType;
			int msgLen;
			char subStr[1024];
			uint64_t epochTime;

			DistSubReqMsg() {
				msgType = Dist_SubReqMsg;
				msgLen = sizeof(DistSubReqMsg) - 2 * sizeof(int);
				subStr[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "SubReqMsg={";
				os << "subStr=" << subStr;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistSubRspMsg
		{
			int msgType;
			int msgLen;
			int code;
			char msg[1024];
			uint64_t epochTime;

			DistSubRspMsg() {
				msgType = Dist_SubRspMsg;
				msgLen = sizeof(DistSubRspMsg) - 2 * sizeof(int);
				code = 0;
				msg[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "SubRspMsg={";
				os << "code=" << code;
				os << " msg=" << msg;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistUnsubReqMsg
		{
			int msgType;
			int msgLen;
			char subStr[1024];
			uint64_t epochTime;

			DistUnsubReqMsg() {
				msgType = Dist_UnsubReqMsg;
				msgLen = sizeof(DistUnsubReqMsg) - 2 * sizeof(int);
				subStr[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "UnsubReqMsg={";
				os << "subStr=" << subStr;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistUnsubRspMsg
		{
			int msgType;
			int msgLen;
			int code;
			char msg[1024];
			uint64_t epochTime;

			DistUnsubRspMsg() {
				msgType = Dist_UnsubRspMsg;
				msgLen = sizeof(DistUnsubRspMsg) - 2 * sizeof(int);
				code = 0;
				msg[0] = 0;
				epochTime = 0;
			}
			std::string toString() const {
				ostringstream os;
				os << "UnsubRspMsg={";
				os << "code=" << code;
				os << " msg=" << msg;
				os << " epochTime=" << epochTime;
				os << "}";
				return os.str();
			}
		};

		struct DistInnerMarketData
		{
			int msgType;
			int msgLen;
			InnerMarketData data;

			DistInnerMarketData() {
				msgType = Dist_InnerMarketData;
				msgLen = sizeof(DistInnerMarketData) - 2 * sizeof(int);
			}
			std::string toString() const {
				ostringstream os;
				os << "InnerMarketData={";
				os << "InstrumentID=" << data.InstrumentID;
				os << " MessageID=" << data.MessageID;
				os << " EpochTime=" << data.EpochTime;
				os << "}";
				return os.str();
			}
		};

		union DistMsg {
			int msgType;
			DistConnReqMsg connReqMsg;
			DistConnRspMsg connRspMsg;
			DistSubReqMsg subReqMsg;
			DistSubRspMsg subRspMsg;
			DistUnsubReqMsg unsubReqMsg;
			DistUnsubRspMsg unsubRspMsg; 
			DistInnerMarketData marketData;
		};

	}
}

#endif //SPOT_DIST_DEF_H
