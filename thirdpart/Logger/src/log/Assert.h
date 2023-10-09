#ifndef INCLUDEASSERT_H_
#define INCLUDEASSERT_H_

#include "Logger/src/log/Logger.h"

#ifdef NDEBUG
	#define ASSERT_NOTNULL(ptr) (ptr)
	#define ASSERT_NOTNULL_REF(ptr) (ptr)
#else
	#define ASSERT_NOTNULL(ptr) \
		LOGGER::FatalNotNull(PARAM_FATAL, (ptr), "'" #ptr "' Must be non NULL")
	#define ASSERT_NOTNULL_REF(ptr) \
		LOGGER::FatalNotNullRef(PARAM_FATAL, (ptr), "'" #ptr "' Must be non NULL")
#endif

// ASSERT
#ifdef NDEBUG
	#define ASSERT(expr) __ASSERT_VOID_CAST (0)
#else
	#define ASSERT(expr) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : Fatal(std::string("ASSERT: ").append(#expr))
#endif

// ASSERT_S
#ifdef NDEBUG
	#define ASSERT_S(expr, s) __ASSERT_VOID_CAST (0)
#else
	#define _S(s) (strlen(s) == 0 ? "" : std::string(": ").append(s).c_str())
	#define ASSERT_S(expr, s) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : Fatal(std::string("ASSERT: ").append( #expr ).append(_S(s)))
#endif

// ASSERT_V
#ifdef _windows_

	#ifdef NDEBUG
		#define ASSERT_V(expr, fmt,...) __ASSERT_VOID_CAST (0)
	#else
		#define ASSERT_V(expr, fmt,...) ASSERT_S(expr, utils::sprintf(fmt, ##__VA_ARGS__).c_str())
	#endif

#else

	#ifdef NDEBUG
		#define ASSERT_V(expr, args...) __ASSERT_VOID_CAST (0)
	#else
		#define ASSERT_V(expr, args...) ASSERT_S(expr, utils::sprintf(NULL, ##args).c_str())
	#endif

#endif

// ASSERT_IF
#define ASSERT_IF(cond, expr) \
	(cond) \
	? ASSERT(expr) \
	: __ASSERT_VOID_CAST (0)

// ASSERT_S_IF
#define ASSERT_S_IF(cond, expr, s) \
	(cond) \
	? ASSERT_S(expr, s) \
	: __ASSERT_VOID_CAST (0)

// ASSERT_V_IF
#ifdef _windows_

#define ASSERT_V_IF(cond, expr, fmt,...) \
	(cond) \
	? ASSERT_V(expr, fmt, ##__VA_ARGS__) \
	: __ASSERT_VOID_CAST (0)

#else

#define ASSERT_V_IF(cond, expr, args...) \
	(cond) \
	? ASSERT_V(expr, ##args) \
	: __ASSERT_VOID_CAST (0)

#endif

#endif