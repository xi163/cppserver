#ifndef INCLUDE_BASE64_H
#define INCLUDE_BASE64_H

#include "Logger/src/Macro.h"

namespace utils {
	namespace base64 {
		std::string Encode(unsigned char const*, unsigned int len);
		std::string Decode(std::string const& s);
	}
}

#endif