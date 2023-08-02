#ifndef INCLUDE_ERRORCODE_H
#define INCLUDE_ERRORCODE_H

#include "Logger/src/Macro.h"
#include "IncMuduo.h"


#ifndef NotScore
#define NotScore(a) ((a)<0.01f)
#endif

enum eApiType {
	OpUnknown = -1,
	OpAddScore = 2,
	OpSubScore = 3,
};

enum ServiceStateE {
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

#define ERROR_MAP(XX, YY) \
	XX(Ok, "OK") \
	\
	YY(CreateGameUser, 10001, "创建游戏账号失败") \
	XX(GameGateNotExist, "没有可用的游戏网关") \
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
	XX(SubScoreHandleInsertOrderIDExists, "玩家下分订单已存在") \
	XX(SubScoreHandleInsertDataUserInGaming, "玩家正在游戏中，不能下分") \
	XX(SubScoreHandleInsertDataOverScore, "玩家下分超出玩家现有总分值") \
	XX(SubScoreHandleInsertDataOutTime, "玩家下分等待超时") \
	XX(OrderScoreCheckOrderNotExistsError, "订单不存在") \
	XX(OrderScoreCheckDataError, "订单查询错误") \
	XX(OrderScoreCheckUserNotExistsError, "玩家不存在") \
	XX(AddScoreHandleInsertDataOutTime, "玩家上分等待超时") \
	XX(AddScoreHandleInsertDataOverScore, "玩家上分超出代理现有总分值") \
	XX(AddScoreHandleInsertDataOrderIDExists, "玩家上分订单已存在") \
	\
	YY(KickOutGameUserOffLineUserAccountError, 93, "玩家账号已被封停") \
	XX(GameRecordGetNoDataError, "无注单数据") \

#define ERROR_ENUM_X(n, s) k##n,
#define ERROR_ENUM_Y(n, i, s) k##n = i,
enum ErrorCode {
	ERROR_MAP(ERROR_ENUM_X, ERROR_ENUM_Y)
};
#undef ERROR_ENUM_X
#undef ERROR_ENUM_Y

struct Msg {
	std::string const msg() const {
		return desc.empty() ?
			errmsg : "[" + desc + "]" + errmsg;
	}
	int code;
	std::string desc;
	std::string errmsg;
};

namespace ERR {
#define ERROR_DEF_X(n, s) \
	static const Msg Err##n = Msg{ k##n, "k"#n, s };
#define ERROR_DEF_Y(n, i, s) \
	static const Msg Err##n = Msg{ k##n, "k"#n, s };
	ERROR_MAP(ERROR_DEF_X, ERROR_DEF_Y)
#undef ERROR_DEF_X
#undef ERROR_DEF_Y
}

namespace response {
	namespace json {
		namespace err {
			void Result(Msg const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		}
	}
}

// 代理信息
struct agent_info_t {
	int64_t		score;              //代理分数 
	int32_t		status;             //是否被禁用 0正常 1停用
	int32_t		agentId;            //agentId
	int32_t		cooperationtype;    //合作模式  1 买分 2 信用
	std::string descode;            //descode 
	std::string md5code;            //MD5 
};

//static std::map<int32_t, agent_info_t> agent_info_;
//static mutable boost::shared_mutex agent_info_mutex_;

#endif
