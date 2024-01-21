#ifndef SPOT_BASE_RISK_H
#define SPOT_BASE_RISK_H
namespace spot
{
	namespace base
	{
		class Risk
		{
		public:
			virtual ~Risk() {};
//			virtual double exposure() = 0;
//			virtual double pnlGross() = 0;
//			virtual double pnlNet() = 0;
			virtual double aggregatedFee() = 0;
		};
	}
}
#endif