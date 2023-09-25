#ifndef INCLUDE_GETTIMEOFDAY_H
#define INCLUDE_GETTIMEOFDAY_H

#include "../../Macro.h"

#define kSecsPerMillisecond 1e-3
#define kSecsPerMicrosecond 1e-6
#define kSecsPerNanosecond  1e-9
#define kMillisecsPerSecond 1000/*1e3*/
#define kMicrosecsPerSecond 1000000/*1e6*/
#define kNanosecsPerSecond  1000000000/*1e9*/

#ifdef _windows_

#include <chrono>

// struct timeval {
// 	time_t tv_sec;
// 	long tv_usec;
// };

//frequency
LARGE_INTEGER frequency();

//counter
LARGE_INTEGER counter();

//gettickcount
double gettickcount();

//clock_gettime
int clock_gettime(int, struct timespec* tv);

//gettimeofday
void gettimeofday(struct timeval* tv, struct timezone* tz);

#endif

//gettime
double gettime();

#endif