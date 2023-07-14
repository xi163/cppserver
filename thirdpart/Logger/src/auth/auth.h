#ifndef INCLUDE_AUTH_H
#define INCLUDE_AUTH_H

#include "../Macro.h"

#define AUTHORIZATION_SUPPORT

namespace utils {

	void regAuthCallback(char const* expired, int64_t timezone);
}

//防反汇编识别/防HOOK破解
typedef bool (*__sysCallback)();
extern __sysCallback __sysCall;

#ifdef AUTHORIZATION_SUPPORT
#define AUTHORIZATION_CHECK \
	if (__sysCall && __sysCall()) { \
		return; \
	}

#define AUTHORIZATION_CHECK_B \
	if (__sysCall && __sysCall()) { \
		return false; \
	}

#define AUTHORIZATION_CHECK_P \
	if (__sysCall && __sysCall()) { \
		return NULL; \
	}

#define AUTHORIZATION_CHECK_R \
	if (__sysCall && __sysCall()) { \
		return -1; \
	}

#define AUTHORIZATION_CHECK_I \
	if (__sysCall && __sysCall()) { \
		return 0; \
	}

#define AUTHORIZATION_CHECK_S \
	if (__sysCall && __sysCall()) { \
		return ""; \
	}
#define RegAuthCallback utils::regAuthCallback
#else
#define AUTHORIZATION_CHECK
#define AUTHORIZATION_CHECK_B
#define AUTHORIZATION_CHECK_P
#define AUTHORIZATION_CHECK_R
#define AUTHORIZATION_CHECK_I
#define AUTHORIZATION_CHECK_S
#define RegAuthCallback
#endif

#endif