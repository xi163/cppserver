#include "Logger/src/utils/utils.h"
//#include "public/Inc.h"
#include "public/gameConst.h"
#include <google/protobuf/message.h>
#include "IncMuduo.h"

// static_assert
// //#include "RpcClients.h"
// assert()
// void assert_check(bool expr, char const* file, int line, char const* func, char const* format, ...) {
// //	LOG()
// }
// #define _LOG_ASSERT(expr, msg) \
// switch(expr){ \
// 	case false: \
// 	_LOG_FATAL("%s",msg)
// }\
// 




typedef boost::tuple<std::string,
	muduo::net::WeakTcpConnectionPtr,
	muduo::net::TcpConnectionPtr> ClientConn;

//typedef ::std::shared_ptr<Message> MessagePtr;
//typedef ::std::function<void(const ::google::protobuf::MessagePtr&)> ClientDoneCallback;
int main() {
	std::string s;
	s.append("dsff").c_str();
	int n = 1;
	//_ASSERT_S(n == 0, utils::sprintf("断言错误 n=%d", n).c_str());
	//_ASSERT_S(n == 0, "");
	//_ASSERT_S(n == 0);
	_ASSERT_V(n == 0, "断言错误 n=%d a=%d 操 ..............", n, 5);
// 	ClientConn conn;
// 	if (conn.get<0>().empty()) {
// 		_LOG_DEBUG(" ok");
// 	}
// 	else {
// 		_LOG_DEBUG(" 空1");
// 	}
// 	if (conn.get<1>().lock()) {
// 		_LOG_DEBUG(" ok");
// 	}
// 	else {
// 		_LOG_DEBUG(" 空1");
// 	}
// 	if (conn.get<2>()) {
// 		_LOG_DEBUG(" ok");
// 	}
// 	else {
// 		_LOG_DEBUG(" 空1");
// 	}
// 	std::shared_ptr<CPlayer> p = std::make_shared<CPlayer>();
// 	std::shared_ptr<CPlayer> p2 = std::shared_ptr<CPlayer>();
// 	_LOG_DEBUG("%s", p->name.c_str());
// 	_LOG_DEBUG("%s", p2->name.c_str());
// 	//muduo::net::RpcChannel::ClientDoneCallback done;
// 	
// 	if (muduo::net::RpcChannel::ClientDoneCallback()) {
// 		_LOG_DEBUG(" 有效");
// 	}
// 	else {
// 		_LOG_DEBUG(" 空1");
// 	}
// 	void (*fn)(int) = foo;
// 
// 	typedef std::function<void(int)> f;
// 	f fx;
// 	fx = CALLBACK_0(fn, 1);
	//fn1(8);
// 	std::vector<std::shared_ptr<CPlayer>> items_;
// 	std::shared_ptr<IPlayer>  target = items_[0];
// 	std::shared_ptr<CPlayer> d = std::dynamic_pointer_cast<CPlayer>(target);
// 	int arr[100] = { 0 };
// 	for (int i = 0; i < 10; ++i) {
// 		for (int k = 0; k < 10; ++k) {
// 			_LOG_DEBUG("%d", i * 10 + k);
// 		//	arr[(i * k + k] = i*k + k;//(k + 1) * i;
// 		}
// 	}
// 	for (int i = 0; i < 100; ++i) {
// 	//	_LOG_DEBUG("%d", arr[i]);
// 	}
// 	int* p = NULL;
// 	switch (p) {
// 	case NULL:
// 		break;
// 	default:
// 		break;
// 	}
// 	_LOG_INFO("%s", getTimeZoneDesc(MY_CST).c_str());
// 
// 	_LOG_WARN("tosec:%d to_time_t:%d", STD_NOW().to_sec(), STD_NOW().to_time_t());
// 	_LOG_ERROR(STD_NOW().format(STD::SECOND,MY_CST).c_str());
// 	_LOG_ERROR(STD_NOW().format(STD::precision::MILLISECOND, MY_CST).c_str());
// 	_LOG_ERROR(STD_NOW().format(STD::precision::MICROSECOND, MY_CST).c_str());
// 	boost::tuple<std::string,
// 		std::shared_ptr<int>,
// 		muduo::net::WeakTcpConnectionPtr> tup;
// 	std::string s = tup.get<0>();
// 	if (!s.empty()) {
// 		_LOG_INFO("tup.get<0>() ok");
// 	}
// 	else {
// 		_LOG_INFO("tup.get<0>() fail");
// 	}
// 	if (tup.get<1>()) {
// 		_LOG_INFO("tup.get<1>() ok");
// 	}
// 	else {
// 		_LOG_INFO("tup.get<1>() fail");
// 	}
// 	if (tup.get<2>()) {
// 		_LOG_INFO("tup.get<2>() ok");
// 	}
// 	else {
// 		_LOG_INFO("tup.get<2>() fail");
// 	}
// 	std::chrono::nanoseconds sec;
// 	std::chrono::system_clock::time_point t1, t2(sec);
// 	//t1 + t2;
// 	//t1 += t2;
// 	 auto x = t1 - t2;
// 	 //t2 = x;
// 	 t2 + sec;
// 	 t2 += sec;
// 	 t2 - sec;
// 	 t2 -= sec;
	//t1 -= t2;
	 //STD::time_point t3(t1 - t2);
// 	bsoncxx::document::view view;
// 	STD::time_point x(::time_point(view["registertime"].get_date()));
// 	 int64_t tt;
// 	 STD::time_point st(tt);
// 	STD::time_point now(NOW());
// 	
// 	STD::time_point loginTime = now - 5000;
// 	AddLogoutLog(loginTime, now);
// 	*STD_NOW();
// 	STD::generic_map m;
// 	m["gameid"] = 630;
// 	m["roomid"] = 6301;

// 	for (STD::generic_map::iterator it = m.begin(); it != m.end(); ++it) {
// 		_LOG_ERROR("[%s] = %s length:%d", it->first.c_str(), it->second.as_string().c_str(), it->second.as_string().length());
// 	}
// 
// 
// 	m["gateip"] = "192.168.0.113:10003:9049:9013";
// 	m["session"] = "3CE12F81CA064718AF35B42A64DB6381";
// 	std::string key = std::string("h.token.")+ "Bm6DR59tdca-RjGtdRBjwQs2vT1xkn2NoiyVc1cEKkQ1OPxe6VfyJ9q5qovmtbkrO4vLL8XNJOWS_p-VuF38mpQIvkW5G8XfMEDom1F13WLsCcwKbk2Nk_Nx8ydnSJec8je6tUBmbkXAYsqEu__HvMwzWpXC-PmSBWR2kWU5NDJ91jdYa4OnuOP5vvnvQYy2j6BK1xh0XIzHbPXh8MOmgcWiUNHMWRB96UXArw57lLJTyG7rdCTKdCFByqLXyDHx0HuHfhYEDksWGloT-wl2cRSw7cAfb5EWkGxpYCnNlpGaZtOiQqT4S471LZ2Uqph2whlLt8Anik5hmZg0NztznT2WTzk2imxxwEq1fbu7E9lIte2x_7oN5h-jyHoHLNglmYufEfS7uXzolrdFag8ItXqUGrIrS960OPYXvZgECO5jaJ8hVcwPSROQl_Nqr39NV6jimeLmPEUD2-D4s7H96Q==";
// 	hmset(key, m);
	

// 	_LOG_ERROR("shutdown1................................................");
// 	char* c = "shutdown2................................................";
// 	_LOG_ERROR(c);
// 	c = "shutdown3................................................";
// 	_LOG_S_ERROR(c);
// 	std::string s("mongodb://root:Lcw%4012345678#!@192.168.0.113:27017");
// 	_LOG_ERROR(s.c_str());
// 	_LOG_S_ERROR(s);
// 	_LOG_ERROR("%s", s.c_str());
// 	c = "mongodb://root:Lcw%4012345678#!@192.168.0.113:27017";
// 	_LOG_ERROR(c);
// 	STD::any v;
// 	v = false;
// 	_LOG_INFO("bool:%d", v.as_bool());
// 	v = (short)10;
// 	_LOG_INFO("short:%d", v.as_short());
// 	v = 10;
// 	_LOG_INFO("int:%d", v.as_int());
// 	v = (int64_t)10;
// 	_LOG_INFO("int:%lld", v.as_int64());
// 	v = 0.5f;
// 	_LOG_INFO("float:%f", v.as_float());
// 	v = 0.00005;
// 	_LOG_INFO("float:%f", v.as_double());
// 	v = "hello,world";
// 	_LOG_INFO("float:%s", v.as_string().c_str());
// 	v = std::string("hello,world");
// 	_LOG_INFO("float:%s", v.as_string().c_str());

// 	char* str = "new string";
// 	STD::generic_map m;
// 	m["k1"] = false;
// 	m["k2"] = (short)10;
// 	m["k3"] = 11;
// 	m["k4"] = (int64_t)150000;
// 	m["k5"] = 0.5f;
// 	m["k6"] = 0.00005;
// 	m["k7"] = "hello,world";
// 	m["k8"] = std::string("str");
// 	m["k9"] = str;
// 	_LOG_INFO("k1:%s", m["k1"].to_string().c_str());
// 	_LOG_INFO("k2:%s", m["k2"].to_string().c_str());
// 	_LOG_INFO("k3:%s", m["k3"].to_string().c_str());
// 	_LOG_INFO("k4:%s", m["k4"].to_string().c_str());
// 	_LOG_INFO("k5:%s", m["k5"].to_string().c_str());
// 	_LOG_INFO("k6:%s", m["k6"].to_string().c_str());
// 	_LOG_INFO("k7:%s", m["k7"].as_string().c_str());
// 	_LOG_INFO("k8:%s", m["k8"].to_string().c_str());
// 	_LOG_INFO("k9:%s", m["k9"].as_string().c_str());
	//tip::Hello;
	//std::vector<Derive&> l;
// 	Derive d;
// 	d.Foo(1,1);
// 	FFF(d);
	//l.emplace_back(d);
	//STD::Random r2;
	//r2.betweenInt64(0, 5).randInt64_mt();
	//STD::Weight weight;
	//weight.rand().betweenInt64(0, 5).randInt64_mt();

	getchar();
	return 0;
}