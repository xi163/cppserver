#ifndef INCLUDE_ROOMNODES_H
#define INCLUDE_ROOMNODES_H

#include "Logger/src/Macro.h"

namespace room {
	namespace nodes {
		void add(std::string const& name);
		void remove(std::string const& name);
		void random_server(uint32_t roomid, std::string& ipport);
		void balance_server(uint32_t roomid, std::string& ipport);
	}
}

#endif