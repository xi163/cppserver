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
		explicit time_point(::time_point const& t);
		explicit time_point(time_t t);
		static ::time_point now();
		static time_point std_now();
		time_t to_time_t();
		::time_point const& get() const;
		::time_point& get();
		time_point duration(int64_t millsec);
		time_point& add(int64_t millsec);
		int64_t to_sec();
		int64_t to_millisec();
		int64_t to_microsec();
		int64_t to_nanosec();
	private:
		::time_point t_;
	};
}

#endif