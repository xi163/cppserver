#ifndef INCLUDE_ROOMNODES_H
#define INCLUDE_ROOMNODES_H

#include "Logger/src/Macro.h"
#include "public/gameConst.h"

#include "club.pb.h"
#include "game.pb.h"
#include "room.pb.h"
#include "table.pb.h"
#include "user.pb.h"

namespace room {
	namespace nodes {
		void add(GameMode mode, std::string const& name);
		void remove(GameMode mode, std::string const& name);
		void random_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport);
		void balance_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport);
		void get_club_room_info(GameMode mode, int64_t clubId, ::club::info& info);
		void get_club_room_info(GameMode mode, int64_t clubId, uint32_t gameId, ::club::game::info& info);
		void get_club_room_info(GameMode mode, int64_t clubId, uint32_t gameId, uint32_t roomId, ::club::game::room::info& info);
	}
}

#endif