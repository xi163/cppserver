#ifndef INCLUDE_ROOMNODES_H
#define INCLUDE_ROOMNODES_H

#include "Logger/src/Macro.h"
#include "public/gameConst.h"

namespace room {
	namespace nodes {
		void add(GameMode mode, std::string const& name);
		void remove(GameMode mode, std::string const& name);
		void random_server(GameMode mode, uint32_t roomid, std::string& ipport);
		void balance_server(GameMode mode, uint32_t roomid, std::string& ipport);
	}
}

#endif