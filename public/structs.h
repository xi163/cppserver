#ifndef INCLUDE_STRUCTS_H
#define INCLUDE_STRUCTS_H

#include "Logger/src/Macro.h"

//1-金币场 2-好友房 3-俱乐部
#define GAMEMODE_MAP(XX, YY) \
	YY(GoldCoin, 1, "金币场") \
	XX(FriendRoom, "好友房") \
	XX(Club, "俱乐部") \

#define GAMEMODE_ENUM_X(n, s) k##n,
#define GAMEMODE_ENUM_Y(n, i, s) k##n = i,
enum {
	GAMEMODE_MAP(GAMEMODE_ENUM_X, GAMEMODE_ENUM_Y)
};
#undef GAMEMODE_ENUM_X
#undef GAMEMODE_ENUM_Y

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define GAMEMODE_DESC_X(n, s) { k##n, "k"#n, s },
#define GAMEMODE_DESC_Y(n, i, s){ k##n, "k"#n, s },
#define TABLE_DECLARE(var, MY_MAP_) \
	static struct { \
		int id_; \
		char const* name_; \
		char const* desc_; \
	}var[] = { \
		MY_MAP_(GAMEMODE_DESC_X, GAMEMODE_DESC_Y) \
	}
#define GAMEMODE_FETCH_NAME(id, var, name) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			name = var[i].name_; \
			break; \
		}\
	}
#define GAMEMODE_FETCH_DESC(id, var, desc) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			desc = var[i].desc_; \
			break; \
		}\
	}
#define GAMEMODE_FETCH_STR(id, var, str) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			std::string name = var[i].name_; \
			std::string desc = var[i].desc_; \
			str = name.empty() ? \
				desc : "[" + name + "]" + desc;\
			break; \
		}\
	}
#define GAMEMODE_IMPLEMENT(NAME, varname) \
		static std::string get##varname(int varname) { \
			TABLE_DECLARE(table_##varname##s_, GAMEMODE_MAP); \
			std::string str##varname; \
			GAMEMODE_FETCH_##NAME(varname, table_##varname##s_, str##varname); \
			return str##varname; \
		}
GAMEMODE_IMPLEMENT(NAME, ModeName)
GAMEMODE_IMPLEMENT(DESC, ModeDesc)
GAMEMODE_IMPLEMENT(STR, ModeStr)
#undef GAMEMODE_DESC_X
#undef GAMEMODE_DESC_Y

struct agent_info_t {
	agent_info_t() {
		score = 0;
		status = 0;
		agentId = 0;
		cooperationtype = 0;
	}
	int64_t		score;              //代理分数 
	int32_t		status;             //是否被禁用 0正常 1停用
	int32_t		agentId;            //agentId
	int32_t		cooperationtype;    //合作模式  1 买分 2 信用
	std::string descode;            //descode 
	std::string md5code;            //MD5 
};

struct agent_user_t {
	agent_user_t() {
		userId = 0;
		score = 0;
		onlinestatus = 0;
	}
	int64_t userId;
	int64_t score;
	int32_t onlinestatus;
	std::string linecode;
};

struct UserBaseInfo {
	UserBaseInfo() {
		userId = -1;
		account = "";
		headId = 0;
		nickName = "";
		userScore = 0;
		agentId = 0;
		lineCode = "";
		status = 0;
		location = "";

		takeMaxScore = 0;
		takeMinScore = 0;
		ip = 0;

		alladdscore = 0;
		allsubscore = 0;
		winlostscore = 0;
		allbetscore = 0;
	}

	int64_t userId;
	std::string account;
	uint8_t headId;
	std::string nickName;
	int64_t userScore;
	uint32_t agentId;
	std::string lineCode;
	uint32_t status;
	uint32_t ip;
	std::string location;
	int64_t takeMaxScore;
	int64_t takeMinScore;
	int64_t alladdscore;
	int64_t allsubscore;
	int64_t winlostscore;
	int64_t allbetscore;
};

#endif