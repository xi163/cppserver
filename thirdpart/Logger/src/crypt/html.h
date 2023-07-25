#ifndef INCLUDE_HTML_H
#define INCLUDE_HTML_H

#include "Logger/src/Macro.h"

namespace utils {
	namespace HTML {
		std::string Encode(const std::string& s);
		std::string Decode(const std::string& s);
	}
}

#endif