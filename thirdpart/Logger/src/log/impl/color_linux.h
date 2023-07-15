#ifndef INCLUDE_COLOR_LINUX_H
#define INCLUDE_COLOR_LINUX_H

#include "../../Macro.h"

#ifdef _linux_

#define COLOR_Reset		"\033[0m"

#define COLOR_Black		"\033[1;32;30m"
#define COLOR_Red		"\033[1;32;31m"
#define COLOR_Green		"\033[1;32;32m"
#define COLOR_Yellow	"\033[1;32;33m"
#define COLOR_Blue		"\033[1;32;34m"
#define COLOR_Purple	"\033[1;32;35m"
#define COLOR_Cyan		"\033[1;32;36m"
#define COLOR_Gray		"\033[1;32;37m"
#define COLOR_White		"\033[1;32;97m"

#define FOREGROUND_Red		0
#define FOREGROUND_Green	1
#define FOREGROUND_Yellow	2
#define FOREGROUND_Blue		3
#define FOREGROUND_Purple	4
#define FOREGROUND_Cyan		5
#define FOREGROUND_Gray		6
#define FOREGROUND_White	7

static const char* COLOR[] = {
	COLOR_Red,
	COLOR_Green,
	COLOR_Yellow,
	COLOR_Blue,
	COLOR_Purple,
	COLOR_Cyan,
	COLOR_Gray,
	COLOR_White,
};

static int Printf(int color, char const* fmt, ...) {
	static size_t const MSGSZ = 512;
	char format[MSGSZ] = { 0 };
	snprintf(format, MSGSZ, "%s%s%s", COLOR[color], fmt, COLOR_Reset);
	va_list ap;
	va_start(ap, format);
	size_t n = printf(fmt, ap);
	va_end(ap);
	return n;
}

#endif

#endif