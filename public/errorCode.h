#ifndef INCLUDE_ERRORCODE_H
#define INCLUDE_ERRORCODE_H

#include "Logger/src/Macro.h"

#define ERRORENTERROOM_MAP(XX, YY) \
	YY(ERROR_ENTERROOM_NOSESSION, 1, "对不起,连接会话丢失,请稍后重试.") \
	XX(ERROR_ENTERROOM_GAMENOTEXIST, "对不起,当前游戏服务不存在,请稍后重试.") \
	XX(ERROR_ENTERROOM_TABLE_FULL, "对不起,当前房间已满,请稍后重试.") \
	XX(ERROR_ENTERROOM_SEAT_FULL, "对不起,当前桌台已满,请稍后重试.") \
	XX(ERROR_ENTERROOM_USERNOTEXIST, "对不起,查询玩家信息失败,请稍后重试.") \
	XX(ERROR_ENTERROOM_SCORENOENOUGH, "对不起,您的金币不足,请充值后重试.") \
	XX(ERROR_ENTERROOM_ALREAY_START, "对不起,当前游戏已经开始,请耐心等待下一局.") \
	XX(ERROR_ENTERROOM_SCORELIMIT, "对不起,您的金币过多,无法进入当前房间.") \
	XX(ERROR_ENTERROOM_USERINGAME, "对不起,您当前正在别的游戏中,无法进入当前房间.") \
	XX(ERROR_ENTERROOM_SERVERSTOP, "对不起,当前游戏服务器正在维护,请稍后重试.") \
	XX(ERROR_ENTERROOM_LONGTIME_NOOP, "对不起,您长时间没有操作,已被请出当前房间.") \
	XX(ERROR_ENTERROOM_SWITCHTABLEFAIL, "对不起,当前游戏已经开始,请在游戏结束后换桌.") \
	XX(ERROR_ENTERROOM_GAME_IS_END, "对不起,断线重连，游戏已结束") \
	XX(ERROR_ENTERROOM_PASSWORD_ERROR, "对不起，密码错误，进房间失败") \
	XX(ERROR_ENTERROOM_STOP_CUR_USER, "对不起，当前用户账号已经冻结") \
	XX(ERROR_ENTERROOM_USER_ORDER_SCORE, "您正在下分，请稍后进入房间") \

enum EnterRoomErrCode {
	ERRORENTERROOM_MAP(ENUM_X, ENUM_Y)
};

#define ERRORCODE_MAP(XX, YY) \
	YY(CreateGameUser, 10001, "创建游戏账号失败") \
	XX(GameGateNotExist, "没有可用的游戏网关") \
	XX(GameLoginNotExist, "没有可用的登陆节点") \
	XX(GameApiNotExist, "没有可用的API节点") \
	XX(Decrypt, "请求token参数解密失败") \
	XX(CheckMd5, "请求token参数MD5校验失败") \
	\
	YY(InsideErrorOrNonExcutive, -100, "内部异常或未执行任务") \
	XX(GameHandleProxyIDError, "代理ID不存在或代理已停用") \
	XX(GameHandleProxyMD5CodeError, "代理MD5校验码错误") \
	XX(GameHandleProxyDESCodeError, "参数转码或代理解密校验码错误") \
	XX(GameHandleParamsError, "传输参数错误") \
	XX(GameHandleOperationTypeError, "操作类型参数错误") \
	XX(GameHandleUserNotExistsError, "玩家账号不存在") \
	XX(GameHandleUserLineCodeNull, "站点编码为空") \
	XX(GameHandleUserLineCodeNotExists, "站点编码不存在") \
	XX(GameHandleRequestInvalidation, "请求已失效") \
	XX(InvitedUserNotExist, "被邀请人不存在") \
	XX(UserAlreadyInClub, "被邀请人已加入俱乐部") \
	XX(InvitorNotInClubOrClubNotExist, "邀请人不是俱乐部成员或俱乐部不存在,无法操作") \
	XX(InvalidInvitationcode, "无效邀请码或已失效") \
	\
	YY(AddScoreHandleInsertDataError, 32, "玩家上分失败") \
	XX(SubScoreHandleInsertDataError, "玩家下分失败") \
	XX(SubScoreHandleUserNotExistsError, "玩家不存在") \	
	XX(SubScoreHandleInsertOrderIDExists, "玩家下分订单已存在") \
	XX(SubScoreHandleInsertDataUserInGaming, "玩家正在游戏中，不能下分") \
	XX(SubScoreHandleInsertDataOverScore, "玩家下分超出玩家现有总分值") \
	XX(SubScoreHandleInsertDataOutTime, "玩家下分等待超时") \
	XX(OrderScoreCheckOrderNotExistsError, "订单不存在") \
	XX(OrderScoreCheckDataError, "订单查询错误") \
	XX(OrderScoreCheckUserNotExistsError, "玩家不存在") \
	XX(AddScoreHandleUserNotExistsError, "玩家不存在") \
	XX(AddScoreHandleInsertDataOutTime, "玩家上分等待超时") \
	XX(AddScoreHandleInsertDataOverScore, "玩家上分超出代理现有总分值") \
	XX(AddScoreHandleInsertDataOrderIDExists, "玩家上分订单已存在") \
	\
	YY(KickOutGameUserOffLineUserAccountError, 93, "玩家账号已被封停") \
	XX(GameRecordGetNoDataError, "无注单数据") \

enum ErrorCode {
	ERRORCODE_MAP(K_ENUM_X, K_ENUM_Y)
};

ERRORCODE_MAP(E_K_MSG_X, E_K_MSG_Y)

STATIC_FUNCTION_IMPLEMENT(ERRORCODE_MAP, K_DETAIL_X, K_DETAIL_Y, NAME, ErrorName)
STATIC_FUNCTION_IMPLEMENT(ERRORCODE_MAP, K_DETAIL_X, K_DETAIL_Y, DESC, ErrorDesc)
STATIC_FUNCTION_IMPLEMENT(ERRORCODE_MAP, K_DETAIL_X, K_DETAIL_Y, STR, ErrorStr)

#endif