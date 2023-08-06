#ifndef INCLUDE_ERRORCODE_H
#define INCLUDE_ERRORCODE_H

#include "Logger/src/Macro.h"

#define ERRORCODE_MAP(XX, YY) \
	YY(CreateGameUser, 10001, "创建游戏账号失败") \
	XX(GameGateNotExist, "没有可用的游戏网关") \
	XX(Decrypt, "请求token参数解密失败") \
	XX(CheckMd5, "请求token参数MD5校验失败") \
	\
	YY(InsideErrorOrNonExcutive, -10, "内部异常或未执行任务") \
	XX(GameHandleProxyIDError, "代理ID不存在或代理已停用") \
	XX(GameHandleProxyMD5CodeError, "代理MD5校验码错误") \
	XX(GameHandleProxyDESCodeError, "参数转码或代理解密校验码错误") \
	XX(GameHandleParamsError, "传输参数错误") \
	XX(GameHandleOperationTypeError, "操作类型参数错误") \
	XX(GameHandleUserNotExistsError, "玩家账号不存在") \
	XX(GameHandleUserLineCodeNull, "站点编码为空") \
	XX(GameHandleUserLineCodeNotExists, "站点编码不存在") \
	XX(GameHandleRequestInvalidation, "请求已失效") \
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