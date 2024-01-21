#ifndef SPOT_UTILITY_MEASURE_H
#define SPOT_UTILITY_MEASURE_H
#include "spot/utility/Utility.h"
namespace spot
{
	namespace utility
	{
		class Measure
		{
		public:
			Measure(int row_, int column_, const char *filename_);
			~Measure();
			void add(int row_, int column_, long long time_);
			void calc();
		private:
			long long* m_buffer;
			int m_row;
			int m_column;
			string m_filename;
		};
		extern Measure *g_measure;
	}
}

#endif
