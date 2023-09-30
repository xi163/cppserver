#ifndef INCLUDEASSERT_H_
#define INCLUDEASSERT_H_

#include "Logger/src/log/Logger.h"

namespace LOGGER {
	template <typename T>
	T* FatalNotNull(int level, char const* file, int line, char const* func, char const* stack, char const* names, T* ptr) {
		if (ptr == NULL) {
			::LOGGER::Logger::instance()->write_s_fatal(level, file, line, func, stack, F_DETAIL | F_SYNC, names);
		}
		return ptr;
	}
}

#ifdef NDEBUG
	#define ASSERT_NOTNULL(ptr) (ptr)
#else
	#define ASSERT_NOTNULL(ptr) \
		LOGGER::FatalNotNull(PARAM_FATAL, "'" #ptr "' Must be non NULL", (ptr))
#endif

// ASSERT
#ifdef NDEBUG
	#define ASSERT(expr) __ASSERT_VOID_CAST (0)
#else
	#define ASSERT(expr) \
	  (expr) \
	   ? __ASSERT_VOID_CAST (0) \
	   : Fatal("ASSERT: " + std::string(#expr))
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

#endif