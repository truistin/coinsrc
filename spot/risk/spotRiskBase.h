#ifndef SPOT_RISK_BASE_H
#define SPOT_RISK_BASE_H


namespace spot
{
	namespace risk
	{
		class spotRiskBase
		{
		public:
			spotRiskBase(){}

			virtual ~spotRiskBase() {}
			// virtual double calcRisk(const std::string& modelName, Instrument* optionInstrument, double underlyingMidPrice) = 0;
		};
	}
}
#endif
