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

int main() {
	
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