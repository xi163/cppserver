#ifndef INCLUDE_MGO_KEYS_H
#define INCLUDE_MGO_KEYS_H

#include "Logger/src/Macro.h"

namespace mgoKeys {

	namespace db {
		static char const* GAMELOG = "gamelog";
		static char const* GAMEMAIN = "gamemain";
		static char const* GAMECONFIG = "gameconfig";
	}

	namespace tbl {
		static char const* GAME_REPLAY = "game_replay";

		static char const* ROBOT_USER = "android_user";
		static char const* ROBOT_STRATEGY = "android_strategy";
		static char const* GAME_KIND = "game_kind";
		static char const* AUTO_INCREMENT = "auto_increment";

		static char const* GAMEUSER = "game_user";
		static char const* LOG_LOGIN = "login_log";
		static char const* LOG_LOGOUT = "logout_log";
		static char const* LOG_GAME = "game_log";
		static char const* PLAY_RECORD = "play_record";
	}
}

#endif