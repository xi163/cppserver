#include "gettimeofday.h"

#ifdef _windows_

//frequency
LARGE_INTEGER frequency() {
	//CPU ticks per sec
	LARGE_INTEGER f;
	::QueryPerformanceFrequency(&f);
	return f;
}

//counter
LARGE_INTEGER counter() {
	//CPU time
	LARGE_INTEGER c;
	::QueryPerformanceCounter(&c);
	return c;
}

//gettickcount
double gettickcount() {
	//microseconds
	static LARGE_INTEGER f = frequency();
	LARGE_INTEGER c = counter();//delta = (c2-c1)/f microseconds
	return (double)c.QuadPart / (double)f.QuadPart;
}

//gettimeofday
void gettimeofday(struct timeval* tv, struct timezone* tz) {
#if 1
	uint64_t  intervals;
	FILETIME  filetime;

	GetSystemTimeAsFileTime(&filetime);

	/*
	* A file time is a 64-bit value that represents the number
	* of 100-nanosecond intervals that have elapsed since
	* January 1, 1601 12:00 A.M. UTC.
	*
	* Between January 1, 1970 (Epoch) and January 1, 1601 there were
	* 134744 days,
	* 11644473600 seconds or
	* 11644473600,000,000,0 100-nanosecond intervals.
	*
	* See also MSKB Q167296.
	*/

	intervals = ((uint64_t)filetime.dwHighDateTime << 32) | filetime.dwLowDateTime;
	intervals -= 116444736000000000;

	tv->tv_sec = (long)(intervals / 10000000);
	tv->tv_usec = (long)((intervals % 10000000) / 10);
#elif 0
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  systemtime;
	FILETIME    filetime;
	uint64_t    time;

	GetSystemTime(&systemtime);
	SystemTimeToFileTime(&systemtime, &filetime);
	time = ((uint64_t)filetime.dwLowDateTime);
	time += ((uint64_t)filetime.dwHighDateTime) << 32;

	tv->tv_sec = (long)((time - EPOCH) / 10000000L);
	tv->tv_usec = (long)(systemtime.wMilliseconds * 1000);
#else
// 	auto now = std::chrono::system_clock::now();
// 	auto sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
// 	auto usec = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
// 	tv->tv_sec = sec.count();
// 	tv->tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(usec - std::chrono::duration_cast<std::chrono::microseconds>(sec)).count();
#endif
}

//https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows#comment80964594_38212960
//clock_gettime
int clock_gettime(int, struct timespec* tv) {
	static LARGE_INTEGER freq = frequency();
	static LARGE_INTEGER startCount = counter();
	static struct _init_t {
		_init_t() {
			timespec_get(&x, TIME_UTC);
		}
		struct timespec x;
	}tv_start;
	time_t sec_part;
	long nsec_part;

	LARGE_INTEGER curCount = counter();

	curCount.QuadPart -= startCount.QuadPart;
	sec_part = curCount.QuadPart / freq.QuadPart;
	nsec_part = (long)((curCount.QuadPart - (sec_part * freq.QuadPart))
		* 1000000000UL / freq.QuadPart);

	tv->tv_sec = tv_start.x.tv_sec + sec_part;
	tv->tv_nsec = tv_start.x.tv_nsec + nsec_part;
	if (tv->tv_nsec >= 1000000000UL) {
		tv->tv_sec += 1;
		tv->tv_nsec -= 1000000000UL;
	}
	return 0;
}

#endif

//gettime
double gettime() {
#ifdef _windows_
	return gettickcount();
#else

	// CLOCK_REALTIME
	// CLOCK_MONOTONIC
	// CLOCK_MONOTONIC_RAW
	// CLOCK_PROCESS_CPUTIME_ID
	// CLOCK_THREAD_CPUTIME_ID
 	//timespec ts;
 	//clock_gettime(CLOCK_MONOTONIC, &ts);
 	//return (double)ts.tv_sec + ts.tv_nsec * kSecsPerNanosecond;

	//ts.tv_sec * 1000 + ts.tv_nsec * 0.000001;//纳秒级

	timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + tv.tv_usec * kSecsPerMicrosecond;

	//tv.tv_sec * 1000 + tv.tv_usec * 0.001;//微秒级
#endif
}