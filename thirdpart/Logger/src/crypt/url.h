#ifndef INCLUDE_URL_H
#define INCLUDE_URL_H

#include "Logger/src/Macro.h"

namespace utils {
	namespace URL {

		std::string Encode(const std::string& str);
		std::string Decode(const std::string& str);

		std::string MultipleEncode(const std::string& str);
		std::string MultipleDecode(const std::string& str);
	}
}

#endif