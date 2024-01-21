#ifndef SPOT_UTILITY_COMPATIABLE_H
#define SPOT_UTILITY_COMPATIABLE_H
#include <stdio.h>
#include <string.h>
#include<algorithm>
#include <math.h>
#include <float.h>
#pragma warning(disable:4996)

#ifdef __WINDOWS__
#define SNPRINTF _snprintf_s
#define Sprintf sprintf_s
#define __builtin_expect(EXP, C)  (EXP) 
#define Thread_local __declspec(thread)
#define fileno _fileno
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#include <time.h>
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <process.h>
#define DIRDELIMIER '\\'
#define strcpy strcpy_s

#endif

#define SPOT_SHUT_WR  1
#ifdef __LINUX__
#include <sys/time.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <dirent.h>
#include <pwd.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include<typeinfo>
#include<arpa/inet.h>
#include<fcntl.h>
#include<netdb.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>
#include <sys/eventfd.h>
#define SNPRINTF snprintf
#define DIRDELIMIER '/'
#define ADDRESS_FAMILY sa_family_t
#define WSAGetLastError() errno
//#define DBL_MAX          1.7976931348623158e+308 // max value
#endif


#define PID_T uint64_t

#ifdef __WINDOWS__   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__  
#else          //*NIX
#define __FUNCTION_NAME__   __func__ 
#endif


#ifdef __WINDOWS__
#define STRTOK	strtok_s
#define GETPID  _getpid
#else
#define STRTOK	strtok_r
#define GETPID  getpid
#endif

#ifdef __WINDOWS__
#define SLEEP(ms) Sleep(ms)
#define SLEEPUS(us) Sleep(us*1000)
#else
#define SLEEP(ms) usleep((ms)*1000)
#define SLEEPUS(us) usleep(us)
#endif

#ifdef __WINDOWS__
#define __memory_barrier() MemoryBarrier()
#else
#define __memory_barrier() __asm__ __volatile__("" ::: "memory")
#endif

#endif