#ifndef INCLUDEASSERTIMPL_H_
#define INCLUDEASSERTIMPL_H_

#include "Logger/src/log/impl/LoggerImpl.h"

#ifdef NDEBUG
	#define _ASSERT_NOTNULL(ptr) (ptr)
	#define _ASSERT_NOTNULL_REF(ptr) (ptr)
#else
	#define _ASSERT_NOTNULL(ptr) \
		LOGGER::_FatalNotNull(_PARAM_FATAL, (ptr), "'" #ptr "' Must be non NULL")
	#define _ASSERT_NOTNULL_REF(ptr) \
		LOGGER::_FatalNotNullRef(_PARAM_FATAL, (ptr), "'" #ptr "' Must be non NULL")
#endif

// _ASSERT
#ifdef NDEBUG
	#define _ASSERT(expr) __ASSERT_VOID_CAST (0)
#else
	#define _ASSERT(expr) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : _Fatal(std::string("ASSERT: ").append(#expr))
#endif

// _ASSERT_S
#ifdef NDEBUG
	#define _ASSERT_S(expr, s) __ASSERT_VOID_CAST (0)
#else
	#define __S(s) (strlen(s) == 0 ? "" : std::string(": ").append(s).c_str())
	#define _ASSERT_S(expr, s) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : _Fatal(std::string("ASSERT: ").append( #expr ).append(__S(s)))
#endif

// _ASSERT_V
#ifdef _windows_

	#ifdef NDEBUG
		#define _ASSERT_V(expr, fmt,...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, fmt,...) _ASSERT_S(expr, utils::_sprintf(fmt, ##__VA_ARGS__).c_str())
	#endif

#else

	#ifdef NDEBUG
		#define _ASSERT_V(expr, args...) __ASSERT_VOID_CAST (0)
	#else
		#define _ASSERT_V(expr, args...) _ASSERT_S(expr, utils::_sprintf(NULL, ##args).c_str())
	#endif

#endif

// _ASSERT_IF
#define _ASSERT_IF(cond, expr) \
	(cond) \
	? _ASSERT(expr) \
	: __ASSERT_VOID_CAST (0)

// _ASSERT_S_IF
#define _ASSERT_S_IF(cond, expr, s) \
	(cond) \
	? _ASSERT_S(expr, s) \
	: __ASSERT_VOID_CAST (0)

// _ASSERT_V_IF
#ifdef _windows_

#define _ASSERT_V_IF(cond, expr, fmt,...) \
	(cond) \
	? _ASSERT_V(expr, fmt, ##__VA_ARGS__) \
	: __ASSERT_VOID_CAST (0)

#else

#define _ASSERT_V_IF(cond, expr, args...) \
	(cond) \
	? _ASSERT_V(expr, ##args) \
	: __ASSERT_VOID_CAST (0)

#endif

#endif