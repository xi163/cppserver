#ifndef INCLUDE_REDIS_KEYS_H
#define INCLUDE_REDIS_KEYS_H

#include "Logger/src/Macro.h"

namespace redisKeys {
	// "k."  - string
	// "h."  - hash
	// "l."  - list
	// "s."  - set
	// "zs." - zset
	static char const* prefix_uid = "h.uid.";
	static char const* prefix_token = "k.token.";
	static char const* prefix_account_uid = "k.account.uid.";
	static char const* prefix_phone_uid = "k.phone.uid.";
	static char const* prefix_email_uid = "k.email.uid.";
	static char const* prefix_online_uid_gameinfo = "k.online.uid.gameinfo.";
	//static char const* prefix_uid_account = "k.uid.account.";
	//static char const* prefix_uid_phone   = "k.uid.phone.";
	//static char const* prefix_uid_email   = "k.uid.email.";


	static uint32_t const DB_INDEX_DEFAULT = 0;
	static uint32_t const DB_INDEX_USER = 0;

	static uint32_t const Expire_HalfDays = 60 * 60 * 12 * 1;
	static uint32_t const Expire_1Days = 60 * 60 * 24 * 1;
	static uint32_t const Expire_2Days = 60 * 60 * 24 * 2;
	static uint32_t const Expire_1Week = 60 * 60 * 24 * 7;
	static uint32_t const Expire_30Days = 60 * 60 * 24 * 30;

	static uint32_t const Expire_Uid = Expire_30Days;
	static uint32_t const Expire_Token = 5 * 60;
	static uint32_t const Expire_AccountUid = Expire_1Week;
	static uint32_t const Expire_PhoneUid = Expire_1Week;
	static uint32_t const Expire_EmailUid = Expire_1Week;
	static uint32_t const Expire_UidGameInfo = 3 * 60;
	//static uint32_t const Expire_Ecs_UserCoin   = 60 * 60 * 24 * 30; //用户金币
	//static uint32_t const Expire_Ecs_Lock       = 60 * 60 * 12;      //房间锁
	//static uint32_t const Expire_Ecs_Server     = 60 * 3;            //ecs服务
	static uint32_t const Expire_Session = 60 * 60 * 72;
	//static uint32_t const Expire_Admin_Total = 60 * 60 * 24 * 45; //统计
	static uint32_t const Expire_UserInfo = 3600 * 24;
	//static uint32_t const InfoTtlWeek           = 7 * 3600 * 24;
	//static uint32_t const DEFULT_REDIS_WEEk_TTL = 7 * 3600 * 24;
	//static uint32_t const MonthTtl              = 30 * 3600 * 24; //一个月缓存
	static uint32_t const HalfMonthTtl = 15 * 3600 * 24; //半个月缓存

	struct Keys {
		int	DB; //0-15
		std::string	Key;
	};
}

#endif