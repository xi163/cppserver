#include "Logger/src/time/time.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"

namespace STD {

	//////////////////////////////////////
	//STD::time_point
	time_point::time_point(::time_point const& t) :t_(t) {
	}
	time_point::time_point(time_t t)
		:t_(std::chrono::system_clock::from_time_t(t)) {
	}
	::time_point time_point::now() {
		return std::chrono::system_clock::now();
	}
	time_point time_point::std_now() {
		return time_point(now());
	}
	time_t time_point::to_time_t() {
		return std::chrono::system_clock::to_time_t(t_);
	}
	::time_point const& time_point::get() const {
		return t_;
	}
	::time_point& time_point::get() {
		return t_;
	}
	time_point time_point::duration(int64_t millsec) {
		return time_point(t_ + std::chrono::milliseconds(millsec));
	}
	time_point& time_point::add(int64_t millsec) {
		t_ += std::chrono::milliseconds(millsec);
		return *this;
	}
	int64_t time_point::to_sec() {
		return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_millisec() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_microsec() {
		return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_nanosec() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(t_.time_since_epoch()).count();//t.time_since_epoch().count()
	}
	/*
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
	*/
}