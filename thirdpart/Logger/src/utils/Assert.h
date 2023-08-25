#ifndef INCLUDE_ASSERT_H_
#define INCLUDE_ASSERT_H_

#include "Logger/src/log/Logger.h"

// _ASSERT
#ifdef NDEBUG
	#define _ASSERT(expr) __ASSERT_VOID_CAST (0)
#else
	#define _ASSERT(expr) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : _LOG_S_FATAL("ASSERT: " + std::string(#expr))
#endif

// _ASSERT_S
#ifdef NDEBUG
	#define _ASSERT_S(expr, s) __ASSERT_VOID_CAST (0)
#else
	#define _S(s) (strlen(s) == 0 ? "" : (std::string(": ") + s).c_str())
	#define _ASSERT_S(expr, s) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : _LOG_S_FATAL("ASSERT: " + std::string(#expr) + _S(s))
#endif

// _ASSERT_V
#ifdef _windows_

	#ifdef NDEBUG
		#define _ASSERT_V(expr, fmt,...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, fmt,...) \
		  (expr) \
		   ? __ASSERT_VOID_CAST (0) \
		   : LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT(); std::abort()
	#endif

#else

	#ifdef NDEBUG
		#define _ASSERT_V(expr, args...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, args...) \
		  (expr) \
		   ? __ASSERT_VOID_CAST (0) \
		   : LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args); LOG_WAIT(); std::abort()
	#endif

#endif

#endif