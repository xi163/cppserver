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

enum TTTY {

	Hello = 1,
};
// struct Msg {
// 	std::string const errmsg() const {
// 		return name.empty() ?
// 			desc : "[" + name + "]" + desc;
// 	}
// 	int code;
// 	std::string name;
// 	std::string desc;
// };
// 
// #define MSG_X(n, s, T) \
// 	static const Msg n = Msg{ T::n, #n, s };
// #define MSG_Y(n, i, s, T) \
// 	static const Msg n = Msg{ T::n, #n, s };
// 
// namespace tip {
// 	//static const Msg Hello = Msg{ TTTY::Hello ,"Hello","Hello" };
// 	MSG_X(Hello, "你好", TTTY)
// }

#include "Logger/src/Macro.h"

namespace STD {

	class time_point_t {
	public:
		explicit time_point_t(time_point const& t) :t_(t) {
		}
		explicit time_point_t(time_t t)
			:t_(std::chrono::system_clock::from_time_t(t)) {
		}
		static time_point now() {
			return std::chrono::system_clock::now();
		}
		time_t to_time_t() {
			return std::chrono::system_clock::to_time_t(t_);
		}
		time_point const& get() const {
			return t_;
		}
		time_point& get() {
			return t_;
		}
		time_point_t duration(int64_t millsec) {
			return time_point_t(t_ + std::chrono::milliseconds(millsec));
		}
		time_point_t& add(int64_t millsec) {
			t_ += std::chrono::milliseconds(millsec);
			return *this;
		}
		int64_t since_sec() {
			return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch()).count();
		}
		int64_t since_sec(int64_t millsec) {
			return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch() + std::chrono::milliseconds(millsec)).count();
		}
		int64_t since_millisec() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch()).count();
		}
		int64_t since_millisec(int64_t millsec) {
			return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch() + std::chrono::milliseconds(millsec)).count();
		}
		int64_t since_microsec() {
			return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch()).count();
		}
		int64_t since_microsec(int64_t millsec) {
			return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch() + std::chrono::milliseconds(millsec)).count();
		}
		int64_t since_nanosec() {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(t_.time_since_epoch()).count();//t.time_since_epoch().count()
		}
		int64_t since_nanosec(int64_t millsec) {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(t_.time_since_epoch() + std::chrono::milliseconds(millsec)).count();
		}
	private:
		time_point t_;
	};
}


int main() {
	time_t sec1 = time(NULL);
	time_point now = std::chrono::system_clock::now();
	time_t sec2 = std::chrono::system_clock::to_time_t(now);
	int64_t sec3 = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	int64_t millisec = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	int64_t microsec = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
	int64_t nanosec = now.time_since_epoch().count();
	int64_t nanosec2 = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	_LOG_INFO("\nsec1=%lld\nsec2=%lld\nsec3=%lld\nmillisec=%lld\nmicrosec=%lld\nnanosec=%lld\nnnanosec2=%lld",
		sec1, sec2, sec3, millisec, microsec, nanosec, nanosec2);
	//tip::Hello;
	//std::vector<Derive&> l;
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