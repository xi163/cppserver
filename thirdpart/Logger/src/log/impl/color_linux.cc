#include "color_linux.h"

#ifdef _linux_

#define COLOR_Reset		"\033[0m"

#define COLOR_Black		"\033[0;32;30m"
#define COLOR_Red		"\033[0;32;31m"
#define COLOR_Green		"\033[0;32;32m"
#define COLOR_Yellow	"\033[0;32;33m"
#define COLOR_Blue		"\033[0;32;34m"
#define COLOR_Purple	"\033[0;32;35m"
#define COLOR_Cyan		"\033[0;32;36m"
#define COLOR_Gray		"\033[0;32;37m"
#define COLOR_White		"\033[0;32;97m"

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

int Printf(int color, char const* fmt, ...) {
	static size_t const MSGSZ = 512;
	char format[MSGSZ] = { 0 };
	snprintf(format, MSGSZ, "%s%s%s", COLOR[color], fmt, COLOR_Reset);
	va_list ap;
	va_start(ap, format);
	size_t n = vprintf(format, ap);
	va_end(ap);
	return n;
}

#endif