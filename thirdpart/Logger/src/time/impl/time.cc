#include "Logger/src/time/time.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"

namespace STD {

	//////////////////////////////////////
	//STD::time_point
	time_point::time_point(::time_point const& t) :t_(t) {
	}
	time_point::time_point(std::chrono::seconds const& t) :t_(t) {
	}
	time_point::time_point(std::chrono::milliseconds const& t) :t_(t) {
	}
	time_point::time_point(std::chrono::microseconds const& t) :t_(t) {
	}
	time_point::time_point(std::chrono::nanoseconds const& t) :t_(t) {
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
	time_t time_point::to_time_t() const {
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
	time_point const time_point::duration(int64_t millsec) const {
		return time_point(t_ + std::chrono::milliseconds(millsec));
	}
	time_point& time_point::add(int64_t millsec) {
		t_ += std::chrono::milliseconds(millsec);
		return *this;
	}
	int64_t time_point::to_sec() {
		return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_sec() const {
		return std::chrono::duration_cast<std::chrono::seconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_millisec() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_millisec() const {
		return std::chrono::duration_cast<std::chrono::milliseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_microsec() {
		return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_microsec() const {
		return std::chrono::duration_cast<std::chrono::microseconds>(t_.time_since_epoch()).count();
	}
	int64_t time_point::to_nanosec() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(t_.time_since_epoch()).count();//t.time_since_epoch().count()
	}
	int64_t time_point::to_nanosec() const {
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
	::time_point& time_point::operator*() {
		return t_;
	}
	time_point time_point::operator+(std::chrono::seconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator+(std::chrono::seconds const& t) const {
		return time_point(t_ + t);
	}
	time_point time_point::operator-(std::chrono::seconds const& t) {
		return time_point(t_ - t);
	}
	time_point const time_point::operator-(std::chrono::seconds const& t) const {
		return time_point(t_ - t);
	}
	time_point& time_point::operator+=(std::chrono::seconds const& t) {
		t_ += t;
		return *this;
	}
	time_point& time_point::operator-=(std::chrono::seconds const& t) {
		t_ -= t;
		return *this;
	}
	time_point time_point::operator+(std::chrono::milliseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator+(std::chrono::milliseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point time_point::operator-(std::chrono::milliseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator-(std::chrono::milliseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point& time_point::operator+=(std::chrono::milliseconds const& t) {
		t_ += t;
		return *this;
	}
	time_point& time_point::operator-=(std::chrono::milliseconds const& t) {
		t_ -= t;
		return *this;
	}
	time_point time_point::operator+(std::chrono::microseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator+(std::chrono::microseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point time_point::operator-(std::chrono::microseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator-(std::chrono::microseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point& time_point::operator+=(std::chrono::microseconds const& t) {
		t_ += t;
		return *this;
	}
	time_point& time_point::operator-=(std::chrono::microseconds const& t) {
		t_ -= t;
		return *this;
	}
	time_point time_point::operator+(std::chrono::nanoseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator+(std::chrono::nanoseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point time_point::operator-(std::chrono::nanoseconds const& t) {
		return time_point(t_ + t);
	}
	time_point const time_point::operator-(std::chrono::nanoseconds const& t) const {
		return time_point(t_ + t);
	}
	time_point& time_point::operator+=(std::chrono::nanoseconds const& t) {
		t_ += t;
		return *this;
	}
	time_point& time_point::operator-=(std::chrono::nanoseconds const& t) {
		t_ -= t;
		return *this;
	}
	time_point time_point::operator+(int64_t millsec) {
		return duration(millsec);
	}
	time_point const time_point::operator+(int64_t millsec) const {
		return duration(millsec);
	}
	time_point time_point::operator-(int64_t millsec) {
		return duration(-millsec);
	}
	time_point const time_point::operator-(int64_t millsec) const {
		return duration(-millsec);
	}
	time_point& time_point::operator+=(int64_t millsec) {
		return add(millsec);
	}
	time_point& time_point::operator-=(int64_t millsec) {
		return add(-millsec);
	}
	time_point time_point::operator-(::time_point const& t) {
		//time_point(std::chrono::nanoseconds)
		return time_point(t_ - t);
	}
	time_point const time_point::operator-(::time_point const& t) const {
		//time_point(std::chrono::nanoseconds)
		return time_point(t_ - t);
	}
	time_point time_point::operator-(time_point const& t) {
		//time_point(std::chrono::nanoseconds)
		return time_point(t_ - t.t_);
	}
	time_point const time_point::operator-(time_point const& t) const {
		//time_point(std::chrono::nanoseconds)
		return time_point(t_ - t.t_);
	}
}