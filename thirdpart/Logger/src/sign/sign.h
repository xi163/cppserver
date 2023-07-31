#ifndef INCLUDE_SIGN_H
#define INCLUDE_SIGN_H

#include "Logger/src/Macro.h"

namespace utils {
	namespace sign {

		std::string Encode(BOOST::Any const& data, int64_t expired, std::string const& secret);

	}
}
#endif