#include <spot/utility/LogStream.h>
#include <spot/utility/Compatible.h>
#include <spot/utility/Milo.h>
#include <algorithm>
#include <limits>
#include <type_traits>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <spot/utility/Utility.h>

#pragma warning(disable:4996)

using namespace spot;
using namespace spot::detail;
#ifdef __LINUX__
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wtautological-compare"
#else
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#endif

namespace spot
{
namespace detail
{

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20,"not = 20");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17,"not = 17");

// Efficient Integer to String Conversions, by Matthew Wilson.
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

size_t convertDouble(char buf[], double value)
{
	char* p = buf;
	if (!std::isnan(value))
	{
		auto ivalue = static_cast<int>(value + EPS);
		auto dvalue = static_cast<int>((value-ivalue + EPS)*1e8);
		
		int j = 0;
		do
		{
			int lsd = dvalue % 10;
			dvalue /= 10;
			*p++ = zero[lsd];
			j++;
		} while (j < 8);
		*p++ = '.';
		do
		{
			int lsd = ivalue % 10;
			ivalue /= 10;
			*p++ = zero[lsd];	
		} while (ivalue != 0);
		if (value < 0)
		{
			*p++ = '-';
		}
	}
	else
	{
		*p++ = 'n';
		*p++ = 'a';
		*p++ = 'n';
	}
	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
  uintptr_t i = value;
  char* p = buf;

  do
  {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

}
}

template<int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
  *cur_ = '\0';
  return data_;
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

void LogStream::staticCheck()
{
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10," more limits double");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10, " more limits long double");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10, " more limits long");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10, " more limits long long");
}

template<typename T>
void LogStream::formatInteger(T v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    size_t len = convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream& LogStream::operator<<(short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<unsigned int>(v);
  return *this;
}

LogStream& LogStream::operator<<(int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  if (buffer_.avail() >= kMaxNumericSize)
  {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = convertHex(buf+2, v);
    buffer_.add(len+2);
  }
  return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
		if (v <= INT32_MAX && v >= INT32_MIN)
		{
			size_t len = convertDouble(buffer_.current(), v);
			buffer_.add(len);
		}
		else
		{
#ifdef __WINDOWS__
        int len = SNPRINTF(buffer_.current(), kMaxNumericSize, kMaxLen, "%.6f", v);
#else
        int len = SNPRINTF(buffer_.current(), kMaxNumericSize, "%.6f", v);
#endif
				if (len >= 0)
					buffer_.add(len);
		}
  }
  return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
  static_assert(std::is_arithmetic<T>::value == true,"not arithmetix");

	length_ = SNPRINTF(buf_, sizeof buf_, fmt, val);

  assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations

template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int );
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);
