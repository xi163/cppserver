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
int main() {
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