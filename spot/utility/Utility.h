#ifndef SPOT_UTILITY_UTILITY_H
#define SPOT_UTILITY_UTILITY_H

#include "spot/utility/Compatible.h"
#include<algorithm>
#include<time.h>
#include<vector>
#include<iostream>
#include<fstream>
#include<string>
#include<chrono>
#include <sstream>
#include <thread>
#include <iomanip>
#include <functional>
#ifdef WIN32
#include <io.h>
#include <direct.h>
#else	//__LINUX__
#include <unistd.h>
#include <sys/stat.h>
#include "tscns.h"
#endif

using namespace std;
using namespace std::chrono;

#define CURR_NSTIME_POINT duration_cast<chrono::nanoseconds>(system_clock::now().time_since_epoch()).count()
#define CURR_USTIME_POINT duration_cast<chrono::microseconds>(system_clock::now().time_since_epoch()).count()
#define CURR_MSTIME_POINT duration_cast<chrono::milliseconds>(system_clock::now().time_since_epoch()).count()
#define CURR_STIME_POINT duration_cast<chrono::seconds>(system_clock::now().time_since_epoch()).count()

#define EPS 0.0000000001
#define IS_DOUBLE_ZERO(d) (abs(d) < EPS)
#define IS_DOUBLE_EQUAL(a,b) IS_DOUBLE_ZERO((a)-(b))
#define IS_DOUBLE_NOT_EQUAL(a,b) (abs((a)-(b)) >= EPS)
#define IS_DOUBLE_GREATER(a,b) ((a)-(b) >= EPS)
#define IS_DOUBLE_GREATER_EQUAL(a,b) ((IS_DOUBLE_GREATER(a,b)) || (IS_DOUBLE_EQUAL(a,b)))
#define IS_DOUBLE_LESS(a,b) ((a)-(b) <= -EPS)
#define IS_DOUBLE_LESS_EQUAL(a,b) ((IS_DOUBLE_LESS(a,b)) || (IS_DOUBLE_EQUAL(a,b)))

#define IS_DOUBLE_NORMAL(x) (std::isnan(x)+(IS_DOUBLE_EQUAL(x, DBL_MAX))+(IS_DOUBLE_ZERO(x))+std::isinf(x))==0
#define SIGN(x) int(((x) > 0) - ((x) < 0))

#define SYMBOLLEN 6

typedef std::function<void(void* c, uint16_t length)> setMethod;
namespace spot
{
	namespace utility
	{
		static double epsilon = 1e-9;

		static double round_p(double p, int n) {
			double w = std::pow(10, n);
			return std::round(p * w) / w;
		}

		static double round_down(double num, double divisor, double decimal) {
			return round_p(std::floor(num / divisor + epsilon) * divisor, decimal);
		}

		static double round_up(double num, double divisor, double decimal) {
			return round_p(std::ceil(num / divisor - epsilon) * divisor, decimal);
		}

		static double round1(double num, double divisor, double decimal) {
			return round_p(int(num / divisor + epsilon) * divisor, decimal);
		}

		static void roundPrec(double& src, int bits)
		{
			stringstream ss;
			ss << fixed << setprecision(bits) << src;
			ss >> src;
			return;
		}

		static	const char digits[] = "9876543210123456789";
		static const char* zero = digits + 9;
		template<typename T>
		size_t convert(char buf[], T value)
		{
			T i = value;
			char* p = buf;
			do
			{
				int lsd = static_cast<int>(i % 10);
				i /= 10;
				*p++ = zero[lsd];
			} while (i != 0);

			if (value < 0)
			{
				*p++ = '-';
			}
			*p = '\0';
			std::reverse(buf, p);

			return p - buf;
		}
		template<typename T>
		size_t convertfill(char buf[], T value, char fill, uint16_t length)
		{
			T i = value;
			char* p = buf;
			do
			{
				int lsd = static_cast<int>(i % 10);
				i /= 10;
				*p++ = zero[lsd];
			} while (i != 0);

			if (value < 0)
			{
				*p++ = '-';
			}
			int len = p - buf;
			if (len >= length)
			{
				*p = '\0';
				std::reverse(buf, p);
			}
			else
			{
				int diff = length - len;
				while (diff-- >0)
				{
					*p++ = fill;
				}
				*p = '\0';
				std::reverse(buf, p);
			}
			return p - buf;
		}
		static const string getCurrentSystemTime()
		{
			auto tt = system_clock::to_time_t(system_clock::now());
			struct tm* ptm = localtime(&tt);
			char date[9] = { 0 };
			sprintf(date, "%02d:%02d:%02d", (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
			return string(date);
		}
		static const string getCurrSystemDate()
		{
			auto tt = system_clock::to_time_t(system_clock::now());
			struct tm* ptm = localtime(&tt);
			char date[9] = { 0 };
			sprintf(date, "%04d%02d%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday);
			return string(date);
		}
		static const string getCurrSystemDate2()
		{
			auto tt = system_clock::to_time_t(system_clock::now());
			struct tm* ptm = localtime(&tt);
			char date[11] = { 0 };
			sprintf(date, "%04d-%02d-%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday);
			return string(date);
		}
		static string convertToTimeString(int time)
		{
			int hour = time / 10000;
			int t = time % 10000;
			int minute = t / 100;
			t = t % 100;
			char buf[20];
			sprintf(buf, "%02d:%02d:%02d", hour, minute, t);
			return string(buf);
		}
		static int convertToTimeInt(const char* time)
		{
			string timeStr = time;
			//remove ':' from the string
			timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), ':'), timeStr.end());
			//remove all leading zeros except the last one - i.e. 00:00:00
			timeStr.erase(0, min(timeStr.find_first_not_of('0'), timeStr.size() - 1));

			return std::stoi(timeStr);
		}


		static string toFormattedSecondString(uint32_t second)
		{
			char buf[32] = { 0 };
			time_t seconds = static_cast<time_t>(second);

			struct tm tm_time;
#ifdef __LINUX__
			localtime_r(&seconds, &tm_time);
#endif

#ifdef WIN32
			_localtime64_s(&tm_time, &seconds);
#endif
			SNPRINTF(buf, sizeof(buf), "%02d:%02d:%02d", tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
			return buf;
		}

		static vector<string> split(const  string& sourcestr, const string& delimiter = ",")
		{
			vector<string> elems;
			size_t pos = 0;
			size_t len = sourcestr.length();
			size_t delim_len = delimiter.length();
			if (delim_len == 0) return elems;
			while (pos < len)
			{
				int find_pos = sourcestr.find(delimiter, pos);
				if (find_pos < 0)
				{
					elems.push_back(sourcestr.substr(pos, len - pos));
					break;
				}
				elems.push_back(sourcestr.substr(pos, find_pos - pos));
				pos = find_pos + delim_len;
			}
			return elems;
		}

		/*
		* The time must be in HH:MM:SS format
		*/
		static bool validateTimeFormat(const char* time)
		{
			if (strlen(time) != 8) return false;

			string timeStr = time;
			vector<string> timeVec = split(timeStr, ":");

			if (timeVec.size() != 3) return false;

			int hr = stoi(timeVec[0]);
			if (hr < 0 || hr >= 24) return false;

			int min = stoi(timeVec[1]);
			if (min < 0 || min >= 60) return false;

			int sec = stoi(timeVec[2]);
			if (sec < 0 || sec >= 60) return false;

			return true;
		}
		//tcp://120.136.172.92:30168
		static string parseAddPort(char * addrPort, int &port)
		{
			string strAddrPort(addrPort);
			char addr[50];
			int startPos = strAddrPort.find("//", 0);
			if (startPos != std::string::npos)
			{
				int endPos = strAddrPort.find(":", startPos);
				if (endPos != std::string::npos)
				{
					string strAddr = strAddrPort.substr(startPos + 2, (endPos - startPos - 2));
					string strPort = strAddrPort.substr(endPos + 1, (strAddrPort.length() - endPos - 1));

					memset(addr, 0, sizeof(addr));
					memcpy(addr, strAddr.c_str(), strAddr.length());
					port = atoi(strPort.c_str());
				}
			}
			return string(addr);
		}
		static unsigned long long get_thread_id(std::thread::id threadid)
		{
			stringstream ss;
			ss << threadid;
			uint64_t id = std::stoull(ss.str());

			return id;
		}
		static int getSpxFieldValue(char *source, int num, char *dest)
		{
			char *pch = strchr(source, '|');
			if (pch == nullptr)
			{
				return -1;
			}
			if (0 == num)
			{
				int len = pch - source;
				memcpy(dest, source, len);
				dest[len] = '\0';
				return len;
			}
			int now;
			int last;

			for (int i = 1; i <= num + 1; ++i)
			{
				last = pch - source;
				pch = strchr(pch + 1, '|');
				now = pch - source;
			}
			if (pch == nullptr || dest == nullptr)
			{
				return -1;
			}
			int len = now - last - 1;
			memcpy(dest, source + last + 1, len);
			dest[len] = '\0';
			//skip '|'
			return now + 1;
		}

		static int splitStr(char *source, vector<string> &vec, char deli)
		{
			char dest[1024];
			char *pch = strchr(source, deli);
			if (pch == nullptr)
			{
				return -1;
			}
			int count = 0;
			int len = pch - source;
			memcpy(dest, source, len);
			dest[len] = '\0';
			vec.push_back(dest);
			count++;
			int now;
			int last;

			while (pch != nullptr)
			{
				last = pch - source + 1;
				pch = strchr(pch + 1, deli);
				if (pch == nullptr)
				{
					//处理最后一段内容
					int srcLen = strlen(source);
					// last 从0 开始
					if (srcLen > last + 1)
					{
						int len = srcLen - last;
						memcpy(dest, source + last, len);
						dest[len] = '\0';
						vec.push_back(dest);
						count++;
					}
					return count;
				}
				now = pch - source;
				int len = now - last;
				memcpy(dest, source + last, len);
				dest[len] = '\0';
				vec.push_back(dest);
				count++;
			}
			return count;
		}

		static void joinSpxStr(char *source, const char *field)
		{
			int sourceLen = strlen(source);
			int fieldLen = 0;
			if (field != nullptr)
			{
				fieldLen = strlen(field);
				memcpy(source + sourceLen, field, fieldLen);
			}
			source[fieldLen + sourceLen] = '|';
			source[fieldLen + sourceLen + 1] = '\0';
		}


		static void deleteSuffixLetters(char *source, char deli)
		{
			char *pch = strchr(source, deli);
			if (pch == nullptr)
			{
				return;
			}
			else
			{
				int sumLen = strlen(source);
				int pos = pch - source;
				memset(pch, 0, sumLen - pos);
			}
		}


		static void strTrim(char *pStr)
		{
			char *pTmp = pStr;
			while (*pStr != '\0')
			{
				if (*pStr != ' ')
				{
					*pTmp++ = *pStr;
				}
				++pStr;
			}
			*pTmp = '\0';
		}

		/**
		* Trim any leading and trailing white space characters from the string.
		* Note that escape sequences and quotes are not handled.
		*
		* @param inStr - string to trim.
		* @return - string without leading or trailing white space
		*/
		#define UTILITY_STR_WHITESPACE " \t\n\r\v\f"
		static string trimWhiteSpace(const string &inStr)
		{
			string outStr;

			if (!inStr.empty())
			{
				string::size_type start = inStr.find_first_not_of(UTILITY_STR_WHITESPACE);

				// If there is only white space we return an empty string
				if (start != string::npos)
				{
					string::size_type end = inStr.find_last_not_of(UTILITY_STR_WHITESPACE);
					outStr = inStr.substr(start, end - start + 1);
				}
			}

			return outStr;
		}

		static bool getFilePath(const std::string folder)
		{
			std::string folder_builder;
			std::string sub;
			sub.reserve(folder.size());
			for (auto it = folder.begin(); it != folder.end(); ++it)
			{
				//cout << *(folder.end()-1) << endl;  
				const char c = *it;
				sub.push_back(c);
				if (c == '/' || it == folder.end() - 1)
				{
					folder_builder.append(sub);
					if (0 != ::access(folder_builder.c_str(), 0))
					{
						// this folder not exist  
#ifdef WIN32
						if (0 != ::mkdir(folder_builder.c_str()))
#else	//__LINUX__
						if (0 != ::mkdir(folder_builder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
#endif
						{
							// create failed  
							return false;
						}
					}
					sub.clear();
				}
			}
			return true;
		}

		static std::string epochToStr(uint64_t epoch)
		{
			struct tm tm_time;
			int kMicroSecondsPerSecond = 1000 * 1000;
			time_t seconds = static_cast<time_t>(epoch / kMicroSecondsPerSecond);
#ifdef __LINUX__
			localtime_r(&seconds, &tm_time);
#endif

#ifdef WIN32
			_localtime64_s(&tm_time, &seconds);
#endif
			int microseconds = static_cast<int>(epoch % kMicroSecondsPerSecond);
			char buf[32] = { 0 };
			SNPRINTF(buf, sizeof(buf), "%02d:%02d:%02d.%06d",
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);

			return buf;
		}

		static std::string epochToStr2(uint64_t epoch)
		{
			struct tm tm_time;
			int kMicroSecondsPerSecond = 1000 * 1000;
			time_t seconds = static_cast<time_t>(epoch / kMicroSecondsPerSecond);
#ifdef __LINUX__
			localtime_r(&seconds, &tm_time);
#endif

#ifdef WIN32
			_localtime64_s(&tm_time, &seconds);
#endif
			int microseconds = static_cast<int>(epoch % kMicroSecondsPerSecond);
			char buf[32] = { 0 };
			SNPRINTF(buf, sizeof(buf), "%04d%02d%02d %02d:%02d:%02d.%06d",
				tm_time.tm_year+1900, tm_time.tm_mon+1, tm_time.tm_mday,
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);

			return buf;
		}

		static long long getEpochTimeUsFromString(const std::string& dateTimeStr)
		{
			//dateTimeStr:20180329 21:00:00.021820
			std::vector<std::string> split = utility::split(dateTimeStr, ".");
			if (split.size() < 2)
				return 0;
			std::string timeStr = split[0];
			std::string nsStr = split[1];
			long long epoch = -1;
#ifdef __LINUX__
			struct tm t;
			strptime(timeStr.c_str(), "%Y%m%d %H:%M:%S", &t);
			epoch = std::mktime(&t);
#else
			std::tm t = {};
			timeStr.insert(6, "-");
			timeStr.insert(4, "-");
			timeStr = timeStr + "Z";
			std::istringstream ss(timeStr);

			if (ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S"))
			{
				epoch = std::mktime(&t);
			}
#endif
			if (epoch > 0)
				return epoch * 1000000 + stoi(nsStr);
			else
				return 0;
		}

		static void replaceAll(std::string& strSource, const std::string& strOld, const std::string& strNew)
		{
			int nPos = 0;
			while ((nPos = strSource.find(strOld, nPos)) != strSource.npos)
			{
				strSource.replace(nPos, strOld.length(), strNew);
				nPos += strNew.length();
			}
		}
		static int getCurrentHour()
		{
			struct tm tm_time;
			time_t seconds = static_cast<time_t>(CURR_MSTIME_POINT/1000);
#ifdef __LINUX__
			localtime_r(&seconds, &tm_time);
#else
			_localtime64_s(&tm_time, &seconds);
#endif
			return tm_time.tm_hour;
		}
		static void setThreadName(std::thread &thd, const char *name)
		{
#ifdef __LINUX__
			pthread_setname_np(thd.native_handle(), name);
#endif
		}

		class DoubleLess
		{
		public:
			bool operator()(const double& lhs, const double& rhs) const
			{
				return IS_DOUBLE_LESS(lhs, rhs);
			}
		};
		class DoubleMore
		{
		public:
			bool operator()(const double& lhs, const double& rhs) const
			{
				return IS_DOUBLE_GREATER(lhs, rhs);
			}
		};
		class CStringLess
		{
		public:
			bool operator()(char const *a, char const *b) const
			{
				return strcmp(a, b) < 0;
			}
		};
		class CStringSymbolLess
		{
		public:
			bool operator()(char const *a, char const *b) const
			{
				return strncmp(a, b, SYMBOLLEN) < 0;
			}
		};
		class Int32More
		{
		public:
			bool operator()(int32_t a, int32_t b) const
			{
				return a > b;
			}
		};
		class Int64More
		{
		public:
			bool operator()(int64_t a, int64_t b) const
			{
				return a > b;
			}
		};
		class CStringCmp
		{
		public:
			bool operator()(const char *s1, const char *s2) const
			{
				return strcmp(s1, s2) == 0;
			}
		};
		class CStringHash
		{
		public:
			size_t operator()(const char *str) const
			{
				//int strLength = strlen(str);
				//return std::_Hash_seq((unsigned char *)str, strLength);
				//BKDR hash algorithm
				int seed = 131;
				int hash = 0;
				while (*str)
				{
					hash = (hash * seed) + (*str);
					str++;
				}
				return hash & (0x7FFFFFFF);
			}
		};

	}
}

#endif
