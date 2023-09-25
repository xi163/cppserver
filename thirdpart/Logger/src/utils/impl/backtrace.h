#ifndef INCLUDE_BACKTRACE_H
#define INCLUDE_BACKTRACE_H

#include "../../Macro.h"

namespace utils {

	std::string _stack_backtrace();

	void _crash_coredump(std::function<void()> cb);
}

#endif