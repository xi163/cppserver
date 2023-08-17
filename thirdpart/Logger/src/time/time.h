#ifndef INCLUDE_STD_TIME_H
#define INCLUDE_STD_TIME_H

#include "Logger/src/Macro.h"

#define NOW STD::time_point::now
#define STD_NOW STD::time_point::std_now

namespace STD {
	enum precision {
		SECOND,
		MILLISECOND,
		MICROSECOND,
	};
	//////////////////////////////////////
	//STD::time_point
	class time_point {
	public:
		explicit time_point();
		explicit time_point(::time_point const& t);
		explicit time_point(std::chrono::seconds const& t);
		explicit time_point(std::chrono::milliseconds const& t);
		explicit time_point(std::chrono::microseconds const& t);
		explicit time_point(std::chrono::nanoseconds const& t);
		explicit time_point(time_t t);
		static ::time_point now();
		static time_point std_now();
		time_t to_time_t();
		time_t to_time_t() const;
		::time_point const& get() const;
		::time_point& get();
		std::string const format(precision pre = SECOND, int64_t timzone = MY_CST) const;
		time_point duration(int64_t millsec);
		time_point const duration(int64_t millsec) const;
		time_point& add(int64_t millsec);
		time_point to_UTC(int64_t timzone);
		time_point const to_UTC(int64_t timzone) const;
		int64_t to_sec();
		int64_t to_sec() const;
		int64_t to_millisec();
		int64_t to_millisec() const;
		int64_t to_microsec();
		int64_t to_microsec() const;
		int64_t to_nanosec();
		int64_t to_nanosec() const;
	public:
		::time_point& operator*();
		::time_point const& operator*() const;
		time_point& operator=(::time_point const& t);
		time_point operator+(std::chrono::seconds const& t);
		time_point const operator+(std::chrono::seconds const& t) const;
		time_point operator-(std::chrono::seconds const& t);
		time_point const operator-(std::chrono::seconds const& t) const;
		time_point& operator+=(std::chrono::seconds const& t);
		time_point& operator-=(std::chrono::seconds const& t);

		time_point operator+(std::chrono::milliseconds const& t);
		time_point const operator+(std::chrono::milliseconds const& t) const;
		time_point operator-(std::chrono::milliseconds const& t);
		time_point const operator-(std::chrono::milliseconds const& t) const;
		time_point& operator+=(std::chrono::milliseconds const& t);
		time_point& operator-=(std::chrono::milliseconds const& t);

		time_point operator+(std::chrono::microseconds const& t);
		time_point const operator+(std::chrono::microseconds const& t) const;
		time_point operator-(std::chrono::microseconds const& t);
		time_point const operator-(std::chrono::microseconds const& t) const;
		time_point& operator+=(std::chrono::microseconds const& t);
		time_point& operator-=(std::chrono::microseconds const& t);

		time_point operator+(std::chrono::nanoseconds const& t);
		time_point const operator+(std::chrono::nanoseconds const& t) const;
		time_point operator-(std::chrono::nanoseconds const& t);
		time_point const operator-(std::chrono::nanoseconds const& t) const;
		time_point& operator+=(std::chrono::nanoseconds const& t);
		time_point& operator-=(std::chrono::nanoseconds const& t);

		time_point operator+(int64_t millsec);
		time_point const operator+(int64_t millsec) const;
		time_point operator-(int64_t millsec);
		time_point const operator-(int64_t millsec) const;
		time_point& operator+=(int64_t millsec);
		time_point& operator-=(int64_t millsec);

		time_point operator-(::time_point const& t);
		time_point const operator-(::time_point const& t) const;
		time_point operator-(time_point const& t);
		time_point const operator-(time_point const& t) const;
	private:
		::time_point t_;
	};
}

#endif