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

#define BUFSZ 1024

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

#define LEVEL_MAP(XX, YY) \
	YY(LVL_FATAL, 0, "FATAL") \
	YY(LVL_ERROR, 1, "ERROR") \
	YY(LVL_WARN,  2, "WARN") \
	YY(LVL_INFO,  3, "INFO") \
	YY(LVL_TRACE, 4, "TRACE") \
	YY(LVL_DEBUG, 5, "DEBUG") \

#define MODE_MAP(XX, YY) \
	YY(M_STDOUT_ONLY, 0, "STDOUT_ONLY") \
	YY(M_FILE_ONLY,   1, "FILE_ONLY") \
	YY(M_STDOUT_FILE, 2, "STDOUT_FILE") \

#define STYLE_MAP(XX, YY) \
	YY(F_SYNC,              0x1000,                "SYNC") \
	YY(F_DETAIL,            0x0001,                "DETAIL") \
	YY(F_TMSTMP,            0x0002,                "TMSTMP") \
	YY(F_FN,                0x0004,                "FN") \
	YY(F_TMSTMP_FN,         0x0008,                "TMSTMP_FN") \
	YY(F_FL,                0x0010,                "FL") \
	YY(F_TMSTMP_FL,         0x0020,                "TMSTMP_FL") \
	YY(F_FL_FN,             0x0040,                "FL_FN") \
	YY(F_TMSTMP_FL_FN,      0x0080,                "TMSTMP_FL_FN") \
	YY(F_TEXT,              0x0100,                "TEXT") \
	YY(F_PURE,              0x0200,                "PURE") \
	YY(F_DETAIL_SYNC,       F_DETAIL|F_SYNC,       "DETAIL_SYNC") \
	YY(F_TMSTMP_SYNC,       F_TMSTMP|F_SYNC,       "TMSTMP_SYNC") \
	YY(F_FN_SYNC,           F_FN|F_SYNC,           "FN_SYNC") \
	YY(F_TMSTMP_FN_SYNC,    F_TMSTMP_FN|F_SYNC,    "TMSTMP_FN_SYNC") \
	YY(F_FL_SYNC,           F_FL|F_SYNC,           "FL_SYNC") \
	YY(F_TMSTMP_FL_SYNC,    F_TMSTMP_FL|F_SYNC,    "TMSTMP_FL_SYNC") \
	YY(F_FL_FN_SYNC,        F_FL_FN|F_SYNC,        "FL_FN_SYNC") \
	YY(F_TMSTMP_FL_FN_SYNC, F_TMSTMP_FL_FN|F_SYNC, "TMSTMP_FL_FN_SYNC") \
	YY(F_TEXT_SYNC,         F_TEXT|F_SYNC,         "TEXT_SYNC") \
	YY(F_PURE_SYNC,         F_PURE|F_SYNC,         "PURE_SYNC") \

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
#include <cassert>
#include <cstring>
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

#define CALLBACK_0(cb, ...) std::bind(cb, ##__VA_ARGS__)
#define CALLBACK_1(cb, ...) std::bind(cb, std::placeholders::_1, ##__VA_ARGS__)
#define CALLBACK_2(cb, ...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define CALLBACK_3(cb, ...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)
#define CALLBACK_4(cb, ...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)
#define CALLBACK_5(cb, ...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##__VA_ARGS__)

#define OBJ_CALLBACK_0(obj, cb, ...) std::bind(cb, obj, ##__VA_ARGS__)
#define OBJ_CALLBACK_1(obj, cb, ...) std::bind(cb, obj, std::placeholders::_1, ##__VA_ARGS__)
#define OBJ_CALLBACK_2(obj, cb, ...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define OBJ_CALLBACK_3(obj, cb, ...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)
#define OBJ_CALLBACK_4(obj, cb, ...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)
#define OBJ_CALLBACK_5(obj, cb, ...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##__VA_ARGS__)

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
#include <limits.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <pwd.h>

#define __FUNC__ __func__
#define INVALID_HANDLE_VALUE (-1)
#define xsleep(t) usleep((t) * 1000) //microseconds
#define clscr() system("reset")

#include "Logger/src/IncBoost.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CALLBACK_0(cb, args...) std::bind(cb, ##args)
#define CALLBACK_1(cb, args...) std::bind(cb, std::placeholders::_1, ##args)
#define CALLBACK_2(cb, args...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, ##args)
#define CALLBACK_3(cb, args...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##args)
#define CALLBACK_4(cb, args...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##args)
#define CALLBACK_5(cb, args...) std::bind(cb, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##args)

#define OBJ_CALLBACK_0(obj, cb, args...) std::bind(cb, obj, ##args)
#define OBJ_CALLBACK_1(obj, cb, args...) std::bind(cb, obj, std::placeholders::_1, ##args)
#define OBJ_CALLBACK_2(obj, cb, args...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, ##args)
#define OBJ_CALLBACK_3(obj, cb, args...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##args)
#define OBJ_CALLBACK_4(obj, cb, args...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##args)
#define OBJ_CALLBACK_5(obj, cb, args...) std::bind(cb, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##args)

#endif

#define _PARAM_ __FILE__,__LINE__,__FUNC__

typedef int pid_t, tid_t;

#include "Logger/src/const.h"

enum {
	TIMEZONE_MAP(ENUM_X, ENUM_Y)
};

enum {
	LEVEL_MAP(ENUM_X, ENUM_Y)
};

enum {
	MODE_MAP(ENUM_X, ENUM_Y)
};

enum {
	STYLE_MAP(ENUM_X, ENUM_Y)
};

STATIC_FUNCTION_IMPLEMENT(TIMEZONE_MAP, DETAIL_X, DETAIL_Y, NAME, TimeZoneName)
STATIC_FUNCTION_IMPLEMENT(TIMEZONE_MAP, DETAIL_X, DETAIL_Y, DESC, TimeZoneDesc)
STATIC_FUNCTION_IMPLEMENT(LEVEL_MAP, DETAIL_X, DETAIL_Y, NAME, LevelName)
STATIC_FUNCTION_IMPLEMENT(LEVEL_MAP, DETAIL_X, DETAIL_Y, DESC, LevelDesc)
STATIC_FUNCTION_IMPLEMENT(MODE_MAP, DETAIL_X, DETAIL_Y, NAME, ModeName)
STATIC_FUNCTION_IMPLEMENT(MODE_MAP, DETAIL_X, DETAIL_Y, DESC, ModeDesc)
STATIC_FUNCTION_IMPLEMENT(STYLE_MAP, DETAIL_X, DETAIL_Y, NAME, StyleName)
STATIC_FUNCTION_IMPLEMENT(STYLE_MAP, DETAIL_X, DETAIL_Y, DESC, StyleDesc)

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