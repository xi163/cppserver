#include "Logger/src/log/Logger.h"
//#include "public/Inc.h"
#include "Logger/src/Macro.h"

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

void hmset(std::string key, STD::generic_map& m) {
	std::string cmd = "HMSET " + key;
	for (STD::generic_map::iterator it = m.begin(); it != m.end(); ++it)
	{
		std::string field = it->first;
		std::string value = it->second.as_string();
		if (value.length())
		{
			_LOG_ERROR("[%s]=%s", field.c_str(), value.c_str());
			//value = string_replace(value, " ", "_");
			_LOG_ERROR("[%s]=%s", field.c_str(), value.c_str());
			cmd += " ";
			cmd += field;
			cmd += " ";
			cmd += value;
		}
	}
	_LOG_ERROR("cmd = %s", cmd.c_str());
}
int main() {
	

	STD::generic_map m;
	m["gameid"] = 630;
	m["roomid"] = 6301;

	for (STD::generic_map::iterator it = m.begin(); it != m.end(); ++it) {
		_LOG_ERROR("[%s] = %s length:%d", it->first.c_str(), it->second.as_string().c_str(), it->second.as_string().length());
	}


	m["gateip"] = "192.168.0.113:10003:9049:9013";
	m["session"] = "3CE12F81CA064718AF35B42A64DB6381";
	std::string key = std::string("h.token.")+ "Bm6DR59tdca-RjGtdRBjwQs2vT1xkn2NoiyVc1cEKkQ1OPxe6VfyJ9q5qovmtbkrO4vLL8XNJOWS_p-VuF38mpQIvkW5G8XfMEDom1F13WLsCcwKbk2Nk_Nx8ydnSJec8je6tUBmbkXAYsqEu__HvMwzWpXC-PmSBWR2kWU5NDJ91jdYa4OnuOP5vvnvQYy2j6BK1xh0XIzHbPXh8MOmgcWiUNHMWRB96UXArw57lLJTyG7rdCTKdCFByqLXyDHx0HuHfhYEDksWGloT-wl2cRSw7cAfb5EWkGxpYCnNlpGaZtOiQqT4S471LZ2Uqph2whlLt8Anik5hmZg0NztznT2WTzk2imxxwEq1fbu7E9lIte2x_7oN5h-jyHoHLNglmYufEfS7uXzolrdFag8ItXqUGrIrS960OPYXvZgECO5jaJ8hVcwPSROQl_Nqr39NV6jimeLmPEUD2-D4s7H96Q==";
	hmset(key, m);
	

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