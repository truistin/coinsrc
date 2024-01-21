#ifndef SPOT_BASE_PARAM_H
#define SPOT_BASE_PARAM_H
#include<iostream>
namespace spot
{
	namespace base
	{
		class Param
		{
		public:
			virtual ~Param() {};
		};
		template<typename T>
		class ParamType : public Param
		{
		public:
			ParamType(T value)
			{
				pvalue_ = new T(value);
			}
			~ParamType()
			{
				delete pvalue_;
			}
			T* getValue()
			{
				return pvalue_;
			};
			void setValue(T value)
			{
				*pvalue_ = value;
			};
		private:
			T* pvalue_;
		};
	}
}
#endif