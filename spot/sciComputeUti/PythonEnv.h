#ifndef SPOT_PYTHON_ENV_H
#define SPOT_PYTHON_ENV_H

namespace spot
{
	namespace base
	{
		class PythonEnv
		{
		public:
			PythonEnv();
			static PythonEnv& instance();
			void init();

		private:
			~PythonEnv();
			bool isInit_;
		};
	}
}
#endif