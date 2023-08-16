#ifndef INCLUDE_MACRO_H
#define INCLUDE_MACRO_H

#if defined(WIN32) || defined(_WIN32)|| defined(WIN64) || defined(_WIN64)
#define _windows_
#elif defined(__linux__) || defined(linux) || defined(__linux) || defined(__gnu_linux__)
#define _linux_
#if defined(__ANDROID__)
#define _android_
#endif
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define _apple_
#endif

#if 0

#define MY_PST (-8)
#define MY_MST (-7)
#define MY_EST (-5)
#define MY_BST (+1)
//UTC/GMT
#define MY_UTC (+0)
//(UTC+04:00) Asia/Dubai
#define MY_GST (+4)
//(UTC+08:00) Asia/shanghai, Beijing(China)
#define MY_CST (+8)
#define MY_JST (+9)

#else

#define TIMEZONE_MAP(XX, YY) \
	YY(MY_PST, -8, "PST") \
	YY(MY_MST, -7, "MST") \
	YY(MY_EST, -5, "EST") \
	YY(MY_BST, +1, "BST") \
	YY(MY_UTC, +0, "UTC") \
	YY(MY_GST, +4, "GST") \
	YY(MY_CST, +8, "CST") \
	YY(MY_JST, +9, "JST") \

#endif

#define LVL_FATAL       0
#define LVL_ERROR       1
#define LVL_WARN        2
#define LVL_INFO        3
#define LVL_TRACE       4
#define LVL_DEBUG       5

#define F_PURE          0x00
#define F_SYNC          0x01
#define F_DETAIL        0x02
#define F_TMSTMP        0x04

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
//#include <dirent.h>
#include <string.h> //memset
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
//#include <curl/curl.h>

//被 *.c 文件包含会有问题
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <random>
#include <chrono>
#include <atomic> //atomic_llong
#include <memory>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <functional>
#include <ios>
#include <thread>
#include <mutex>
//#include <shared_mutex>
#include <condition_variable>

typedef std::chrono::system_clock::time_point time_point;

#ifdef _windows_

#include <iconv.h>
#include <direct.h>
#include <io.h>
#include <tchar.h>
#include <conio.h>
//#include <WinSock2.h> //timeval
#include <windows.h>
//#define __FUNC__ __FUNCSIG__//__FUNCTION__
#define __FUNC__ __FUNCTION__//__FUNCSIG__
#define INVALID_HANDLE_VALUE ((HANDLE)(-1))
//#define snprintf     _snprintf //_snprintf_s
#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp
#define strtoull     _strtoui64
#define xsleep(t) Sleep(t) //milliseconds
#define clscr() system("cls")

#include <shared_mutex>

#define read_lock(mutex) std::shared_lock<std::shared_mutex> guard(mutex)
#define write_lock(mutex) std::unique_lock<std::shared_mutex> guard(mutex)

#elif defined(_linux_)

#include <unistd.h> //ssize_t
#include <signal.h>
//#include <sigaction.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
//#include <net/if.h>

#define __FUNC__ __func__
#define INVALID_HANDLE_VALUE (-1)
#define xsleep(t) usleep((t) * 1000) //microseconds
#define clscr() system("reset")

#include "Logger/src/IncBoost.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#endif

#define _PARAM_ __FILE__,__LINE__,__FUNC__

typedef int pid_t, tid_t;

#include "Logger/src/const.h"

enum {
	TIMEZONE_MAP(ENUM_X, ENUM_Y)
};

STATIC_FUNCTION_IMPLEMENT(TIMEZONE_MAP, DETAIL_X, DETAIL_Y, DESC, TimeZoneDesc)

enum rTy {
	Number = 0,
	LowerChar = 1,
	LowerCharNumber = 2,
	UpperChar = 3,
	UpperCharNumber = 4,
	Char = 5,
	CharNumber = 6,
	rMax
};

#ifdef _windows_
#pragma execution_character_set("utf-8")
#endif

#define ContentLength			"Content-Length"
#define ContentType				"Content-Type"

#define ContentType_Text		"text/plain"
#define ContentType_Json		"application/json"
#define ContentType_Xml			"application/xml"
#define ContentType_Html		"text/html"

#define ContentType_Text_utf8	"text/plain;charset=utf-8"
#define ContentType_Json_utf8	"application/json;charset=utf-8"
#define ContentType_Xml_utf8	"application/xml;charset=utf-8"
#define ContentType_Html_utf8	"text/html;charset=utf-8"

typedef std::map<std::string, std::string> HttpParams;

#endif