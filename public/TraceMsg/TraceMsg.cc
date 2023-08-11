#include "Logger/src/Macro.h"

#include <string.h>
#include <stdint.h>
#include <string>
#include <sstream>

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MY_CMD_STR(n, s, l) { n, ""#n, s, l },

#define MY_TAB_MAP(var, MY_CMD_MAP_) \
	static struct { \
		int id_; \
		char const *name_; \
		char const* desc_; \
		short const level_; \
	}var[] = { \
		MY_CMD_MAP_(MY_CMD_STR) \
	}

#define MY_CMD_DESC(id, var, name, desc, lvl) \
	int const n = ARRAYSIZE(var); \
	for (int i = 0; i < n; ++i) { \
		if (var[i].id_ == id) { \
			name = var[i].name_; \
			desc = var[i].desc_; \
			lvl = var[i].level_; \
			break; \
		} \
	}

#define MY_MAINID_MAP(XX) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_ID_BEGIN, "", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY, "网关服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL, "大厅服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, "游戏服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC, "逻辑服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL, "大厅服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_HALL_TO_PROXY, "网关服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_GAME_SERVER, "游戏服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_GAME_SERVER_TO_PROXY, "网关服", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER, "服务器", LVL_DEBUG) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, "比赛服", LVL_DEBUG)
MY_TAB_MAP(mainid_, MY_MAINID_MAP);

#define MY_SUBID_CLIENT_TO_SERVER_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::MESSAGE_CLIENT_TO_SERVER_SUBID_BEGIN, "", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_REQ, "心跳 SYN", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES, "心跳 ACK", LVL_DEBUG)
MY_TAB_MAP(subid_client_to_server_, MY_SUBID_CLIENT_TO_SERVER_MAP);

#define MY_SUBID_CLIENT_TO_PROXY_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::MESSAGE_CLIENT_TO_PROXY_SUBID_BEGIN, "", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_REQ, "查询AESKey REQ", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_RES, "查询AESKey RSP", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_USER_CONNECTION_MESSAGE_REQ, "绑定用户连接REQ", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_USER_CONNECTION_MESSAGE_RES, "绑定用户连接RSP", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_GAME_SERVER_MESSAGE_REQ, "绑定游戏服REQ", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_GAME_SERVER_MESSAGE_RES, "绑定游戏服RSP", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY, "关闭客户端通知", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY, "订阅通知", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE, "用户上下分通知!!!", LVL_DEBUG)
MY_TAB_MAP(subid_client_to_proxy_, MY_SUBID_CLIENT_TO_PROXY_MAP);

//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_REQ, "修改头像REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_RES, "修改头像RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_REQ, "修改昵称REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_RES, "修改头像RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_REQ, "查询积分REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_RES, "查询积分RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_REQ, "查询游戏记录REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_RES, "查询游戏记录RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_REQ, "查询房间玩家数REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_RES, "查询房间玩家数RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_REQ, "查询比赛房间信息REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_RES, "查询比赛房间信息RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_REQ, "查询比赛记录REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_RES, "查询比赛记录RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_REQ, "查询比赛最佳记录REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_RES, "查询比赛最佳记录RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_REQ, "查询转盘积分REQ", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_RES, "查询转盘积分RSP", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_REQ, "", LVL_DEBUG) \
//XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_RES, "", LVL_DEBUG)
#define MY_SUBID_CLIENT_TO_HALL_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::MESSAGE_CLIENT_TO_HALL_SUBID_BEGIN, "", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ, "登陆大厅REQ", LVL_INFO) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES, "登陆大厅RSP", LVL_INFO) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_REQ, "查询游戏房间列表REQ", LVL_WARN) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_RES, "查询游戏房间列表RSP", LVL_WARN) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_REQ, "查询指定游戏节点REQ", LVL_WARN) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES, "查询指定游戏节点RSP", LVL_WARN) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_REQ, "查询正在玩的游戏REQ", LVL_WARN) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_RES, "查询正在玩的游戏RSP", LVL_WARN)
MY_TAB_MAP(subid_client_to_hall_, MY_SUBID_CLIENT_TO_HALL_MAP);

//XX(::GameServer::SUBID::SUB_S2C_USER_ENTER_NOTIFY, "进入房间通知", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_USER_SCORE_NOTIFY, "用户积分通知", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_USER_STATUS_NOTIFY, "用户状态通知", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_C2S_CHANGE_TABLE_REQ, "用户换桌REQ", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_CHANGE_TABLE_RES, "用户换桌RSP", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_C2S_USER_RECHARGE_OK_REQ, "用户充值成功 REQ", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_USER_RECHARGE_OK_RES, "用户充值成功 RSP", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_C2S_USER_EXCHANGE_FAIL_REQ, "用户充值失败 REQ", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_USER_EXCHANGE_FAIL_RES, "用户充值失败 RSP", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_C2S_CHANGE_TABLE_SIT_CHAIR_REQ, "用户入座REQ", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_CHANGE_TABLE_SIT_CHAIR_RES, "用户入座RSP", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_S2C_PLAY_IN_OTHERROOM, "用户游戏中", LVL_DEBUG) \
//XX(::GameServer::SUBID::SUB_GF_SYSTEM_MESSAGE, "系统消息", LVL_DEBUG)
#define MY_SUBID_CLIENT_TO_GAME_SERVER_MAP(XX) \
	XX(::GameServer::SUBID::SUBID_BEGIN, "", LVL_DEBUG) \
	XX(::GameServer::SUBID::SUB_C2S_ENTER_ROOM_REQ, "进入房间REQ", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_S2C_ENTER_ROOM_RES, "进入房间RSP", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_C2S_USER_READY_REQ, "用户准备REQ", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_S2C_USER_READY_RES, "用户准备RSP", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_C2S_USER_LEFT_REQ, "用户离开REQ", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_S2C_USER_LEFT_RES, "用户离开RSP", LVL_INFO)
MY_TAB_MAP(subid_client_to_game_server_, MY_SUBID_CLIENT_TO_GAME_SERVER_MAP);

#define MY_SUBID_PROXY_TO_HALL_MAP(XX) \
	XX(::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE, "用户离线", LVL_DEBUG)
MY_TAB_MAP(subid_proxy_to_hall_, MY_SUBID_PROXY_TO_HALL_MAP);

#define MY_SUBID_PROXY_TO_GAME_SERVER_MAP(XX) \
	XX(::GameServer::SUBID::SUB_S2C_ENTER_ROOM_RES, "进入房间RSP", LVL_INFO) \
	XX(::GameServer::SUBID::SUB_S2C_USER_LEFT_RES, "用户离开RSP", LVL_INFO) \
	XX(::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE, "用户离线", LVL_DEBUG)
MY_TAB_MAP(subid_proxy_to_game_server_, MY_SUBID_PROXY_TO_GAME_SERVER_MAP);

#define MY_SUBID_GAME_SERVER_TO_PROXY_MAP(XX) \
	XX(::Game::Common::MESSAGE_GAME_SERVER_TO_PROXY_SUBID::MESSAGE_GAME_SERVER_TO_PROXY_SUBID_BEGIN, "", LVL_DEBUG) \
	XX(::Game::Common::MESSAGE_GAME_SERVER_TO_PROXY_SUBID::PROXY_NOTIFY_KILL_BOSS_MESSAGE_REQ, "踢出玩家", LVL_DEBUG)
MY_TAB_MAP(subid_game_server_to_proxy_, MY_SUBID_GAME_SERVER_TO_PROXY_MAP);

#define MY_SUBID_HTTP_TO_SERVER_MAP(XX) \
	XX(::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER, "Http修复", LVL_DEBUG)
MY_TAB_MAP(subid_http_to_server_, MY_SUBID_HTTP_TO_SERVER_MAP);

#if 0

#define MY_SUBID_CLIENT_TO_MATCH_SERVER_MAP(XX) \
	XX(::MatchServer::SUBID::SUBID_BEGIN, "") \
	XX(::MatchServer::SUBID::SUB_C2S_ENTER_MATCH_REQ, "报名 REQ", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_C2S_LEFT_MATCH_REQ, "退赛 REQ", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_C2S_USER_RANK_REQ, "查询比赛排行REQ", LVL_DEBUG)	\
	XX(::MatchServer::SUBID::SUB_S2C_ENTER_MATCH_RESP, "报名 RSP", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_LEFT_MATCH_RESP, "退赛 RSP", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_USER_ENTER_NOTIFY, "报名 notify", LVL_DEBUG)	\
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_SCENE, "比赛场景", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_PLAYER_UPDATE, "更新比赛人数", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_RANK_UPDATE, "更新比赛排行", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_FINISH, "比赛结束", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_USER_RANK_RESP, "查询比赛排行RSP", LVL_DEBUG) \
	XX(::MatchServer::SUBID::SUB_S2C_UPGRADE, "比赛升级", LVL_DEBUG)
MY_TAB_MAP(subid_client_to_match_server_, MY_SUBID_CLIENT_TO_MATCH_SERVER_MAP);

#endif

extern "C" int strMessageID(
	std::string& strMainID,
	std::string& strMainDesc,
	std::string& strSubID,
	std::string& strSubDesc,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	int lvl;
	MY_CMD_DESC(mainId, mainid_, strMainID, strMainDesc, lvl);
	switch (mainId)
	{
	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY: {
		//MY_CMD_DESC(subId, subid_client_to_server_, strSubID, strSubDesc, lvl);
		MY_CMD_DESC(subId, subid_client_to_proxy_, strSubID, strSubDesc, lvl);
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL: {
		if (trace_hall_heartbeat) {
			MY_CMD_DESC(subId, subid_client_to_server_, strSubID, strSubDesc, lvl);
		}
		if (strSubID.empty()) {
			MY_CMD_DESC(subId, subid_client_to_hall_, strSubID, strSubDesc, lvl);
		}
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER: {
		if (trace_game_heartbeat) {
			MY_CMD_DESC(subId, subid_client_to_server_, strSubID, strSubDesc, lvl);
		}
		if (strSubID.empty()) {
			MY_CMD_DESC(subId, subid_client_to_game_server_, strSubID, strSubDesc, lvl);
		}
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
// 		break;
// 	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL: {
		MY_CMD_DESC(subId, subid_proxy_to_hall_, strSubID, strSubDesc, lvl);
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_HALL_TO_PROXY: {
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_GAME_SERVER: {
		MY_CMD_DESC(subId, subid_proxy_to_game_server_, strSubID, strSubDesc, lvl);
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_GAME_SERVER_TO_PROXY: {
// 		MY_CMD_DESC(subId, subid_game_server_to_proxy_, strSubID, strSubDesc, lvl);
// 		break;
// 	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER: {
		MY_CMD_DESC(subId, subid_http_to_server_, strSubID, strSubDesc, lvl);
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER: {
// 		MY_CMD_DESC(subId, subid_client_to_match_server_, strSubID, strSubDesc, lvl);
// 		break;
// 	}
	default:
		break;
	}
	if ((mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL ||
		mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER) &&
		strSubID.empty()) {
		return lvl;
	}
	std::stringstream sm, ss;
	sm << (int)mainId;
	strMainID += ":" + sm.str();
	ss << (int)subId;
	strSubID += ":" + ss.str();
	return lvl;
}

extern "C" int fmtMessageID(
	std::string & str,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	std::string strMainID, strMainDesc, strSubID, strSubDesc;
	int lvl = strMessageID(strMainID, strMainDesc, strSubID, strSubDesc, mainId, subId, trace_hall_heartbeat, trace_game_heartbeat);
	if ((mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL ||
		mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER) &&
		strSubID.empty()) {
		return lvl;
	}
	str =
		"\n" +
		(strMainDesc.empty() ? strMainID : (strMainID + " - " + strMainDesc)) +
		"\n" +
		(strSubDesc.empty() ? strSubID : (strSubID + " - " + strSubDesc));
	return lvl;
}