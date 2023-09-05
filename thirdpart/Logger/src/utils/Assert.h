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
	#define _S(s) (strlen(s) == 0 ? "" : std::string(": ").append(s).c_str())
	#define _ASSERT_S(expr, s) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : _LOG_S_FATAL(std::string("ASSERT: ").append( #expr ).append(_S(s))))
#endif

// _ASSERT_V
#ifdef _windows_

	#ifdef NDEBUG
		#define _ASSERT_V(expr, fmt,...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, fmt,...) _ASSERT_S(expr, utils::sprintf(fmt, ##__VA_ARGS__).c_str())
	#endif

#else

	#ifdef NDEBUG
		#define _ASSERT_V(expr, args...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, args...) _ASSERT_S(expr, utils::sprintf(NULL, ##args).c_str())
	#endif

#endif

#endif