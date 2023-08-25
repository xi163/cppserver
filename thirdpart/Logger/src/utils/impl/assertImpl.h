#ifndef INCLUDE_ASSERTIMPL_H_
#define INCLUDE_ASSERTIMPL_H_

#include "Logger/src/log/impl/LoggerImpl.h"

// __ASSERT
#ifdef NDEBUG
	#define __ASSERT(expr) __ASSERT_VOID_CAST (0)
#else
	#define __ASSERT(expr) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : __LOG_S_FATAL("ASSERT: " + std::string(#expr))
#endif

// __ASSERT_S
#ifdef NDEBUG
	#define __ASSERT_S(expr, s) __ASSERT_VOID_CAST (0)
#else
	#define __S(s) (strlen(s) == 0 ? "" : (std::string(": ") + s).c_str())
	#define __ASSERT_S(expr, s) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : __LOG_S_FATAL("ASSERT: " + std::string(#expr) + __S(s))
#endif

// __ASSERT_V
#ifdef _windows_

	#ifdef NDEBUG
		#define __ASSERT_V(expr, fmt,...) __ASSERT_VOID_CAST (0)
	#else
		#define __ASSERT_V(expr, fmt,...) __ASSERT_S(expr, utils::_sprintf(fmt, ##__VA_ARGS__).c_str())
	#endif

#else

	#ifdef NDEBUG
		#define __ASSERT_V(expr, args...) __ASSERT_VOID_CAST (0)
	#else
		#define __ASSERT_V(expr, args...) __ASSERT_S(expr, utils::_sprintf(NULL, ##args).c_str())
	#endif

#endif

#endif