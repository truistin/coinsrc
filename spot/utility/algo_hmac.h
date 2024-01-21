#ifndef _ALGO_HMAC_H_
#define _ALGO_HMAC_H_

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include "openssl/sha.h"
#include <openssl/hmac.h>
#include <sys/timeb.h>
#include <time.h> 
#include <chrono>

bool HmacEncode(const char * algo, const char * key, const char * input, unsigned char* output, unsigned int* output_length);
unsigned char ToHex(unsigned char x);
unsigned char FromHex(unsigned char x);
std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);
std::string string_To_UTF8(const std::string & str);
std::string getUTCTime();
std::string getUTCMSTime();
#endif