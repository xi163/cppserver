#ifndef INCLUDE_GAMECONST_H
#define INCLUDE_GAMECONST_H

#include "Logger/src/Macro.h"

#ifndef NotScore
#define NotScore(a) ((a)<0.01f)
#endif

enum eCooType {
	buyScore = 1, //代理买分
	credit = 2,   //代理信用
};

enum eApiType {
	OpUnknown = -1,
	OpAddScore = 2,//上分
	OpSubScore = 3,//下分
};

enum {
	kRunning = 0,//服务中
	kRepairing = 1,//维护中
};

enum eApiCtrl {
	kClose = 0,
	kOpen = 1,//应用层IP截断
	kOpenAccept = 2,//网络底层IP截断
};

enum eApiVisit {
	kEnable = 0,//IP允许访问
	kDisable = 1,//IP禁止访问
};

//1-金币场 2-好友房 3-俱乐部
#define GAMEMODE_MAP(XX, YY) \
	YY(GoldCoin, 1, "金币场") \
	XX(FriendRoom, "好友房") \
	XX(Club, "俱乐部") \

//0-百人 1-对战
#define GAMETYPE_MAP(XX, YY) \
	XX(GameType_BaiRen, "百人") \
	XX(GameType_Confrontation, "对战") \

//游戏状态 - 各子游戏需要进行补充
#define GAMESTATUS_MAP(XX, YY) \
	XX(GAME_STATUS_INIT, "初始") \
	XX(GAME_STATUS_FREE, "空闲准备") \
	\
	YY(GAME_STATUS_START, 100, "游戏进行") \
	\
	YY(GAME_STATUS_END, 200, "游戏结束") \

enum GameType {
	GAMETYPE_MAP(ENUM_X, ENUM_Y)
};

enum GameStatus {
	GAMESTATUS_MAP(ENUM_X, ENUM_Y)
};

enum GameMode {
	GAMEMODE_MAP(K_ENUM_X, K_ENUM_Y)
};

STATIC_FUNCTION_IMPLEMENT(GAMETYPE_MAP, DETAIL_X, DETAIL_Y, NAME, TypeName)
STATIC_FUNCTION_IMPLEMENT(GAMETYPE_MAP, DETAIL_X, DETAIL_Y, DESC, TypeDesc)
STATIC_FUNCTION_IMPLEMENT(GAMETYPE_MAP, DETAIL_X, DETAIL_Y, STR, TypeStr)

STATIC_FUNCTION_IMPLEMENT(GAMESTATUS_MAP, DETAIL_X, DETAIL_Y, NAME, StatusName)
STATIC_FUNCTION_IMPLEMENT(GAMESTATUS_MAP, DETAIL_X, DETAIL_Y, DESC, StatusDesc)
STATIC_FUNCTION_IMPLEMENT(GAMESTATUS_MAP, DETAIL_X, DETAIL_Y, STR, StatusStr)

STATIC_FUNCTION_IMPLEMENT(GAMEMODE_MAP, K_DETAIL_X, K_DETAIL_Y, NAME, ModeName)
STATIC_FUNCTION_IMPLEMENT(GAMEMODE_MAP, K_DETAIL_X, K_DETAIL_Y, DESC, ModeDesc)
STATIC_FUNCTION_IMPLEMENT(GAMEMODE_MAP, K_DETAIL_X, K_DETAIL_Y, STR, ModeStr)

#endif