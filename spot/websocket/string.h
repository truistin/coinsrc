
#ifndef __OKCOIN_STRING_H__
#define __OKCOIN_STRING_H__

#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include "md5.h"
#include <time.h>

using namespace std;

typedef unsigned char BYTE;

class str
{
public:
	//static string UnicodeToAnsi( const wstring& wstrSource);

	static string GetMD5(const char * pzsSource);
	static string GetMD5(const string &strSource);

	static BYTE toHex(const BYTE &x);
	static BYTE fromHex(const BYTE &x);
	static string URLEncode(const string &sIn);
	static string URLDecode(const string &sIn);

};

#endif /* __OKCOIN_STRING_H__ */
