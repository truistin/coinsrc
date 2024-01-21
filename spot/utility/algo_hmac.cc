#include "algo_hmac.h"
//boost库引用文件
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <stdlib.h>

using namespace boost::archive::iterators;
using namespace std;
#ifdef WIN32
bool HmacEncode(const char * algo,const char * key,const char * input, unsigned char* output, unsigned int* output_length)
{
	const EVP_MD * engine = NULL;
	if (strcmp("sha512", algo) == 0) 
	{
		engine = EVP_sha512();
	}
	else if (strcmp("sha256", algo) == 0) 
	{
		engine = EVP_sha256();
	}
	else if (strcmp("sha1", algo) == 0) 
	{
		engine = EVP_sha1();
	}
	else if (strcmp("md5", algo) == 0) 
	{
		engine = EVP_md5();
	}
	else if (strcmp("sha224", algo) == 0) 
	{
		engine = EVP_sha224();
	}
	else if (strcmp("sha384", algo) == 0) 
	{
		engine = EVP_sha384();
	}
	else 
	{
		cout << "Algorithm " << algo << " is not supported by this program!" << endl;
		return false;
	}
	if (output == nullptr)
	{
		output = (unsigned char*)malloc(EVP_MAX_MD_SIZE);
	}
	HMAC_CTX *ctx = HMAC_CTX_new();
	HMAC_Init_ex(ctx, key, strlen(key), engine, NULL);
	HMAC_Update(ctx, (unsigned char*)input, strlen(input));        // input is OK; &input is WRONG !!!
	HMAC_Final(ctx, output, output_length);
	HMAC_CTX_free(ctx);

	return true;
}
#else
bool HmacEncode(const char * algo, const char * key, const char * input, unsigned char* output, unsigned int* output_length)
{
	const EVP_MD * engine = nullptr;
	if (strcmp("sha512", algo) == 0)
	{
		engine = EVP_sha512();
	}
	else if (strcmp("sha256", algo) == 0)
	{
		engine = EVP_sha256();
	}
	else if (strcmp("sha1", algo) == 0)
	{
		engine = EVP_sha1();
	}
	else if (strcmp("md5", algo) == 0)
	{
		engine = EVP_md5();
	}
	else if (strcmp("sha224", algo) == 0)
	{
		engine = EVP_sha224();
	}
	else if (strcmp("sha384", algo) == 0)
	{
		engine = EVP_sha384();
	}
	else
	{
		cout << "Algorithm " << algo << " is not supported by this program!" << endl;
		return false;
	}

	//HMAC_CTX ctx;
	//HMAC_CTX_init(&ctx);
	HMAC_CTX *ctx = HMAC_CTX_new();
	HMAC_Init_ex(ctx, key, strlen(key), engine, NULL);
	HMAC_Update(ctx, (unsigned char*)input, strlen(input));        // input is OK; &input is WRONG !!!  
	HMAC_Final(ctx, output, output_length);
	//HMAC_CTX_cleanup(ctx);
	HMAC_CTX_free(ctx);
	return true;
}

#endif
////URL encode
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	return y;
}
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			if (i + 2 < length)
			{	
				unsigned char high = FromHex((unsigned char)str[++i]);
				unsigned char low = FromHex((unsigned char)str[++i]);
				strTemp += high * 16 + low;
			}
		}
		else strTemp += str[i];
	}
	return strTemp;
}
//UTC time is need
std::string getUTCTime()
{
	auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* utm = gmtime(&tt);

	char utc_date[60] = { 0 };
	sprintf(utc_date, "%d-%02d-%02dT%02d:%02d:%02d",
		(int)utm->tm_year + 1900, (int)utm->tm_mon + 1, (int)utm->tm_mday,
		(int)utm->tm_hour, (int)utm->tm_min, (int)utm->tm_sec);
	return std::string(utc_date);
}

std::string getUTCMSTime()
{
	char * timestamp = new char[32];
	time_t t;
	time(&t);
	struct tm* ptm = gmtime(&t);
	strftime(timestamp, 32, "%FT%T.123Z", ptm);
	return string(timestamp);
}

////utf-8 encode
string string_To_UTF8(const std::string & str)
{
	int nwLen = mbstowcs(nullptr, str.c_str(), 0);
	if (nwLen == 0)
	{
		return "";
	}
	wchar_t * pwBuf = (wchar_t*)malloc(sizeof(wchar_t*)*(nwLen + 1));//一定要加1，不然会出现尾巴  

	//wchar_t * pwBuf = new wchar_t[nwLen + 1];
	memset(pwBuf, 0,sizeof(wchar_t)*(nwLen+1));
	nwLen = mbstowcs(pwBuf, str.c_str(), nwLen+1);

	int nLen = wcstombs(nullptr, pwBuf, 0);
	char * pBuf = (char*)malloc(sizeof(char*)*(nLen + 1));
	memset(pBuf, 0, sizeof(char)*(nLen + 1));
	wcstombs(pBuf, pwBuf, nLen + 1);

	std::string retStr(pBuf);
	free(pwBuf);
	free(pBuf);
	pwBuf = nullptr;
	pBuf = nullptr;

	return retStr;
}