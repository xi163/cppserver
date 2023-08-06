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
		static char const* GAME_KIND_CLUB = "game_kind_club";
		static char const* AUTO_INCREMENT = "auto_increment";

		static char const* GAMEUSER = "game_user";
		static char const* AGENTINFO = "proxy_info";
		static char const* LOG_LOGIN = "login_log";
		static char const* LOG_LOGOUT = "logout_log";
		static char const* LOG_GAME = "game_log";
		static char const* PLAY_RECORD = "play_record";
		
		static char const* ADDSCORE_ORDER = "add_score_order";
		static char const* SUBSCORE_ORDER = "sub_score_order";
		static char const* USERSCORE_RECORD = "user_score_record";
		static char const* AGENT_SCORE_RECORD = "proxy_score_record";

		static char const* IP_WHITE_LIST = "ip_white_list";
	}
}

#endif