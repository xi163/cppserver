#ifndef INCLUDE_STD_GENERIC_H
#define INCLUDE_STD_GENERIC_H

#include "Logger/src/generic/variant.h"

namespace STD {

	//////////////////////////////////////
	//STD::generic_map[string]any
	class generic_map : public std::map<std::string, any> {
	public:
		any& operator[](std::string const& key);
	};
}

#endif