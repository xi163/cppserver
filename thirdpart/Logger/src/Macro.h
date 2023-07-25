﻿#ifndef INCLUDE_MACRO_H
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

typedef int pid_t;
typedef int tid_t;

#ifdef _windows_
#pragma execution_character_set("utf-8")
#endif

#endif