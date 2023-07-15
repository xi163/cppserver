#ifndef INCLUDE_COLOR_LINUX_H
#define INCLUDE_COLOR_LINUX_H

#include "../../Macro.h"

#ifdef _linux_

#define FOREGROUND_Red		0
#define FOREGROUND_Green	1
#define FOREGROUND_Yellow	2
#define FOREGROUND_Blue		3
#define FOREGROUND_Purple	4
#define FOREGROUND_Cyan		5
#define FOREGROUND_Gray		6
#define FOREGROUND_White	7

int Printf(int color, char const* fmt, ...);

#endif

#endif