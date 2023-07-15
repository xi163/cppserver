#ifndef INCLUDE_COLOR_H
#define INCLUDE_COLOR_H

#ifdef _windows_
#include "color_win.h"

#elif defined(_linux_)
#include "color_linux.h"

static const int color[][2] = {
		{FOREGROUND_Red, FOREGROUND_Cyan},     //LVL_FATAL
		{FOREGROUND_Red, FOREGROUND_Cyan},     //LVL_ERROR
		{FOREGROUND_Cyan, FOREGROUND_Purple},  //LVL_WARN
		{FOREGROUND_Purple, FOREGROUND_White}, //LVL_INFO
		{FOREGROUND_Yellow, FOREGROUND_Green}, //LVL_TRACE
		{FOREGROUND_Green, FOREGROUND_Yellow}, //LVL_DEBUG
};

#endif

#endif