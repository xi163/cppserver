#include "color_win.h"

#ifdef _windows_

int Printf(int color, char const* format, ...) {
	HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(h, color);
	va_list ap;
	va_start(ap, format);
	size_t n = vprintf(format, ap);
	va_end(ap);
	//::CloseHandle(h);
	return n;
}

#endif