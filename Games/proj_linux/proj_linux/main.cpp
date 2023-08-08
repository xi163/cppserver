#include "public/Inc.h"

// void testcircular() {
// 	boost::circular_buffer<int> cb(3);
// 	_LOG_INFO("cap:%d size:%d", cb.capacity(), cb.size());
// 	cb.resize(3);
// 	_LOG_INFO("cap:%d size:%d", cb.capacity(), cb.size());
// 	cb.push_back(1);
// 	cb.push_back(2);
// 	cb.push_back(3);
// 	for (int i = 0; i < cb.size(); ++i) {
// 		_LOG_INFO("%d", cb[i]);
// 	}
// 	std::cout << std::endl;
// 	//此时容量已满，下面新的push_back操作将在头部覆盖一个元素
// 	cb.push_back(4);
// 	for (int i = 0; i < cb.size(); ++i) {
// 		_LOG_INFO("%d", cb[i]);
// 	}
// 	std::cout << std::endl;
// 	//下面的push_front操作将在尾部覆盖一个元素
// 	cb.push_front(5);
// 	for (int i = 0; i < cb.size(); ++i)
// 	{
// 		_LOG_INFO("%d", cb[i]);
// 	}
// }

// class Base {
// public: 
// 	virtual void Foo() {
// 	}
// };
// 
// class Derive : public Base {
// public:
// 	void Foo(int a) {
// 
// 	}
// 	virtual void Foo(int a, int b) {
// 	}
// 	virtual void Foo(int a, int b, int c) {
// 	}
// };
// void FFF(Base const& b) {
// 
// }
// 
// enum TTTY {
// 
// 	Hello = 1,
// };

//////////////////////////////////////
	//STD::variant


int main() {
	
	STD::any v;
	v = false;
	_LOG_INFO("bool:%d", v.as_bool());
	v = (short)10;
	_LOG_INFO("short:%d", v.as_short());
	v = 10;
	_LOG_INFO("int:%d", v.as_int());
	v = (int64_t)10;
	_LOG_INFO("int:%lld", v.as_int64());
	v = 0.5f;
	_LOG_INFO("float:%f", v.as_float());
	v = 0.00005;
	_LOG_INFO("float:%f", v.as_double());
	v = "hello,world";
	_LOG_INFO("float:%s", v.as_string().c_str());
	v = std::string("hello,world");
	_LOG_INFO("float:%s", v.as_string().c_str());

	STD::generic_map m;
	m["k1"] = false;
	m["k2"] = (short)10;
	m["k3"] = 11;
	m["k4"] = (int64_t)150000;
	m["k5"] = 0.5f;
	m["k6"] = 0.00005;
	m["k7"] = "hello,world";
	m["k8"] = std::string("str");
	
	_LOG_INFO("k1:%d", m["k1"].as_bool());
	_LOG_INFO("k2:%d", m["k2"].as_short());
	_LOG_INFO("k3:%d", m["k3"].as_int());
	_LOG_INFO("k4:%lld", m["k4"].as_int64());
	_LOG_INFO("k5:%f", m["k5"].as_float());
	_LOG_INFO("k6:%f", m["k6"].as_double());
	_LOG_INFO("k7:%s", m["k7"].as_string().c_str());
	_LOG_INFO("k8:%s", m["k8"].as_string().c_str());
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