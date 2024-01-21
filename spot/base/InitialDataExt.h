#ifndef SPOT_BASE_INITIALDATAEXT_H
#define SPOT_BASE_INITIALDATAEXT_H
#include <map>
#include <list>
#include <string>
#include "spot/base/MqStruct.h"
#include "spot/base/InitialData.h"
using namespace std;
namespace spot
{
	namespace base
	{
		//extend functions for initial data
		class InitialDataExt
		{
		public:
			InitialDataExt() {};
			~InitialDataExt() {};
			static void addSpotTdInfo(SpotTdInfo &spotTdInfo);
			static void addSpotMdInfo(SpotMdInfo &spotMdInfo);

			static const string& getTdInterfaceType(const string& exchangeCode);
			static int getTdInterfaceID(const string& exchangeCode, int routeKey);
			static int getTdInterfaceIDVecSize(const string& exchangeCode);

			static list<string> getTdExchangeList(string& interfaceType);
			static list<string> getMdExchangeList(string& interfaceType, int interfaceId);
			static string getInvestorId(string exchangeCode);
			static int exchangeCodeInvestorIdMapSize();
			static map<string, vector<int>> exchangeTdInterfaceIDMap_;

		private:
			//exchange to trading interface map
			static map<string, string> exchangeTdInterfaceTypeMap_;
			//interface to exchange list map
			static map<string, map<int, list<string>>> mdInterfaceExhangeMap_;
			static map<string, list<string>> tdInterfaceExhangeMap_;
			//exchange to investorid map
			static map<string, string> exchangeCodeInvestorIdMap_;


		};
	}
}
#endif
