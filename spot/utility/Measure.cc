#include"spot/utility/Measure.h"
#include<memory>
#include<string.h>
#include<vector>

using namespace spot;
using namespace spot::utility;

Measure * spot::utility::g_measure;

Measure::Measure(int row_, int column_,const char *filename_)
{
	m_row = row_;
	m_column = column_;
	m_filename = filename_;
	m_buffer = reinterpret_cast<long long*>(malloc(sizeof(long long)*row_*column_));
}
Measure::~Measure()
{
	free(m_buffer);
}
void Measure::add(int row_, int column_, long long time_)
{
	if (row_ > m_row-1 || column_ > m_column-1)
	{
		printf("measure finished\n");
		exit(1);
		printf("ignore data row=%d,column=%d\n", row_, column_);		
	}
	memcpy(m_buffer + row_*m_column + column_, &time_, sizeof(time_));
	if (row_ == m_row - 1 && column_ == m_column - 1)
	{
		calc();
	}
}
void Measure::calc()
{
	ofstream file;
	file.open(m_filename);
	vector<long long> vet_sum(m_column);
	//文件头
	for (int col = 0; col < m_column; col++)
	{
		vet_sum[col] = 0;
		file << "t" << col << ",";
	}
	for (int col = 1; col < m_column; col++)
	{
		file << "t" << col << "-t" << col - 1 << ",";
	}
	file << "t" << m_column - 1 << "-t0" << endl;

	for (int row = 0; row < m_row; row++)
	{	
		for (int col = 0; col < m_column; col++)
		{
			long long *t = m_buffer + row*m_column + col;
			file << *t << ",";
		}
		for (int col = 1; col < m_column; col++)
		{
			long long *t1 = m_buffer + row*m_column + col-1;
			long long *t2 = m_buffer + row*m_column + col;
			file << *t2-*t1 << ",";
			vet_sum[col - 1] += *t2 - *t1;
		}
			long long dif = *(m_buffer + row*m_column + m_column - 1) - *(m_buffer + row*m_column);
			file << dif << endl;
			vet_sum[m_column - 1] += dif;
	}

	//求平均值
	for (int col = 0; col < m_column; col++)
	{		
		file << "-,";
	}
	for (int col = 0; col < m_column; col++)
	{
		long long avg = vet_sum[col] / m_row;
		file << avg <<",";
	}

	printf("count=%d,sum=%lld,avg=%f ns\n", m_row, vet_sum[m_column-1], (double)vet_sum[m_column - 1]/m_row);
	file.close();
}