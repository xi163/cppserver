#ifndef INCLUDE_ROOMNODES_H
#define INCLUDE_ROOMNODES_H

#include "Logger/src/Macro.h"
#include "public/gameConst.h"

namespace room {
	namespace nodes {
		void add(GameMode mode, std::string const& name);
		void remove(GameMode mode, std::string const& name);
		void random_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport);
		void balance_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport);
		void get_room_info(GameMode mode, int64_t clubId);
		void get_room_info(GameMode mode, int64_t clubId, uint32_t gameId);
		void get_room_info(GameMode mode, int64_t clubId, uint32_t gameId, uint32_t roomId);
	}
}

#endif