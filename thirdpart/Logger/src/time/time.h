#ifndef INCLUDE_STD_TIME_H
#define INCLUDE_STD_TIME_H

#include "Logger/src/Macro.h"

#define NOW STD::time_point::now
#define STD_NOW STD::time_point::std_now

namespace STD {

	//////////////////////////////////////
	//STD::time_point
	class time_point {
	public:
		explicit time_point(::time_point const& t) :t_(t) {
		}
		explicit time_point(time_t t)
			:t_(std::chrono::system_clock::from_time_t(t)) {
		}
		static ::time_point now() {
			return std::chrono::system_clock::now();
		}
		static time_point std_now() {
			return time_point(now());
		}
		time_t to_time_t() {
			return std::chrono::system_clock::to_time_t(t_);
		}
		::time_point const& get() const {
			return t_;
		}
		::time_point& get() {
			return t_;
		}
		time_point duration(int64_t millsec) {
			return time_point(t_ + std::chrono::milliseconds(millsec));
		}
		time_point& add(int64_t millsec) {
			t_ += std::chrono::milliseconds(millsec);
			return *this;
		}
		int64_t to_sec() {
			return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch()).count();
		}
		int64_t to_millisec() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch()).count();
		}
		int64_t to_microsec() {
			return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch()).count();
		}
		int64_t to_nanosec() {
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
	private:
		::time_point t_;
	};
}

#endif