#include "public/Inc.h"

void testcircular() {
	boost::circular_buffer<int> cb(3);
	_LOG_INFO("cap:%d size:%d", cb.capacity(), cb.size());
	cb.resize(3);
	_LOG_INFO("cap:%d size:%d", cb.capacity(), cb.size());
	cb.push_back(1);
	cb.push_back(2);
	cb.push_back(3);
	for (int i = 0; i < cb.size(); ++i) {
		_LOG_INFO("%d", cb[i]);
	}
	std::cout << std::endl;
	//此时容量已满，下面新的push_back操作将在头部覆盖一个元素
	cb.push_back(4);
	for (int i = 0; i < cb.size(); ++i) {
		_LOG_INFO("%d", cb[i]);
	}
	std::cout << std::endl;
	//下面的push_front操作将在尾部覆盖一个元素
	cb.push_front(5);
	for (int i = 0; i < cb.size(); ++i)
	{
		_LOG_INFO("%d", cb[i]);
	}
}

class Base {
public: 
	virtual void Foo() {
	}
};

class Derive : public Base {
public:
	void Foo(int a) {

	}
	virtual void Foo(int a, int b) {
	}
	virtual void Foo(int a, int b, int c) {
	}
};
void FFF(Base const& b) {

}
#define ERROR_MAP(XX, YY) \
	XX(Ok, "OK") \
	\
	YY(CreateGameUser, 10001, "创建游戏账号失败") \
	XX(GameGateNotExist, "没有可用的游戏网关") \
	\
	YY(InsideErrorOrNonExcutive, -10, "内部异常或未执行任务") \
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
	XX(GameRecordGetNoDataError, "无注单数据")


#define ERROR_ENUM_X(n, s) k##n,
#define ERROR_ENUM_Y(n, i, s) k##n = i,
enum ErrorCode {
	ERROR_MAP(ERROR_ENUM_X, ERROR_ENUM_Y)
};
#undef ERROR_ENUM_X
#undef ERROR_ENUM_Y

struct Msg {
	std::string msg() {
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

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ERROR_DESC_X(n, s) { k##n, "k"#n, s },
#define ERROR_DESC_Y(n, i, s){ k##n, "k"#n, s },
#define TABLE_DECLARE(var, MY_MAP_) \
	static struct { \
		int id_; \
		char const* name_; \
		char const* desc_; \
	}var[] = { \
		MY_MAP_(ERROR_DESC_X, ERROR_DESC_Y) \
	}
//#undef ERROR_DESC_X
//#undef ERROR_DESC_Y

#define ERROR_FETCH(id, var, str) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			std::string name = var[i].name_; \
			std::string desc = var[i].desc_; \
			str = name.empty() ? \
				desc : "[" + name + "]" + desc;\
			break; \
		}\
	}

#define ERROR_DECLARE(varname) \
	std::string Get##varname##Str(int varname);

ERROR_DECLARE(Error)

#define ERROR_IMPLEMENT(varname) \
		std::string get##varname##Str(int varname) { \
			TABLE_DECLARE(table_##varname##s_, ERROR_MAP); \
			std::string str##varname; \
			ERROR_FETCH(varname, table_##varname##s_, str##varname); \
			return str##varname; \
		}

ERROR_IMPLEMENT(Error);

int main() {
	_LOG_DEBUG("错误信息：%s[%d]%s",
		ERR::ErrGameRecordGetNoDataError.desc.c_str(),
		ERR::ErrGameRecordGetNoDataError.code,
		ERR::ErrGameRecordGetNoDataError.errmsg.c_str());
	_LOG_DEBUG(getErrorStr(kAddScoreHandleInsertDataError).c_str());
	//kNoError;
	//kCreateGameUser;
	//std::vector<Derive&> l;
	std::string s = utils::random::charStr(128);
	_LOG_DEBUG(s.c_str());
	Derive d;
	d.Foo(1,1);
	FFF(d);
	//l.emplace_back(d);
	//STD::Random r2;
	//r2.betweenInt64(0, 5).randInt64_mt();
	STD::Weight weight;
	weight.rand().betweenInt64(0, 5).randInt64_mt();

	getchar();
	return 0;
}