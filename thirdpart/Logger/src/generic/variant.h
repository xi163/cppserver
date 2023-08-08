#ifndef INCLUDE_STD_VARIANT_H
#define INCLUDE_STD_VARIANT_H

#include "Logger/src/Macro.h"

namespace STD {

	//////////////////////////////////////
	//STD::variant
	class variant {
		enum v_type {
			v_null,
			v_bool,
			v_char,
			v_uchar,
			v_short,
			v_ushort,
			v_int,
			v_uint,
			v_long,
			v_ulong,
			v_int64,
			v_uint64,
			v_float,
			v_double,
			v_string,
		};
	public:
		variant()
			: type(v_null) {
			memset(&u, 0, sizeof u);
		}
		explicit variant(bool val) {
			*this = val;
		}
		explicit variant(char val) {
			*this = val;
		}
		explicit variant(unsigned char val) {
			*this = val;
		}
		explicit variant(short val) {
			*this = val;
		}
		explicit variant(unsigned short val) {
			*this = val;
		}
		explicit variant(int val) {
			*this = val;
		}
		explicit variant(unsigned int val) {
			*this = val;
		}
		explicit variant(long val) {
			*this = val;
		}
		explicit variant(unsigned long val) {
			*this = val;
		}
		explicit variant(long long val) {
			*this = val;
		}
		explicit variant(unsigned long long val) {
			*this = val;
		}
		explicit variant(float val) {
			*this = val;
		}
		explicit variant(double val) {
			*this = val;
		}
		explicit variant(char const* val) {
			*this = val;
		}
		explicit variant(std::string const& val) {
			*this = val;
		}
	public:
		variant& operator=(bool val) {
			u.b8 = val;
			type = v_bool;
		}
		variant& operator=(char val) {
			u.i8 = val;
			type = v_char;
		}
		variant& operator=(unsigned char val) {
			u.u8 = val;
			type = v_uchar;
		}
		variant& operator=(short val) {
			u.i16 = val;
			type = v_short;
		}
		variant& operator=(unsigned short val) {
			u.u16 = val;
			type = v_ushort;
		}
		variant& operator=(int val) {
			u.i32 = val;
			type = v_int;
		}
		variant& operator=(unsigned int val) {
			u.u32 = val;
			type = v_uint;
		}
		variant& operator=(long val) {
			u.l32 = val;
			type = v_long;
		}
		variant& operator=(unsigned long val) {
			u.ul32 = val;
			type = v_ulong;
		}
		variant& operator=(long long val) {
			u.i64 = val;
			type = v_int64;
		}
		variant& operator=(unsigned long long val) {
			u.u64 = val;
			type = v_uint64;
		}
		variant& operator=(float val) {
			u.f32 = val;
			type = v_float;
		}
		variant& operator=(double val) {
			u.f64 = val;
			type = v_double;
		}
		variant& operator=(char const* val) {
			s = val;
			type = v_string;
		}
		variant& operator=(std::string const& val) {
			s = val;
			type = v_string;
		}
	public:
		inline bool as_bool() {
			return u.b8;
		}
		inline short as_short() {
			return u.i16;
		}
		inline unsigned short as_ushort() {
			return u.u16;
		}
		inline int as_int() {
			return u.i32;
		}
		inline unsigned int as_uint() {
			return u.u32;
		}
		inline int as_long() {
			return u.l32;
		}
		inline unsigned int as_ulong() {
			return u.ul32;
		}
		inline long long as_int64() {
			return u.i64;
		}
		inline unsigned long long as_uint64() {
			return u.u64;
		}
		inline float as_float() {
			return u.f32;
		}
		inline double as_double() {
			return u.f64;
		}
		inline std::string as_string() {
			return s;
		}
	public:
		inline std::string to_string() {
			switch (type) {
			case v_bool: return std::to_string(u.b8);
			case v_char: return std::to_string(u.i8);
			case v_uchar: return std::to_string(u.u8);
			case v_short: return std::to_string(u.i16);
			case v_ushort: return std::to_string(u.u16);
			case v_int: return std::to_string(u.i32);
			case v_uint: return std::to_string(u.u32);
			case v_long: return std::to_string(u.l32);
			case v_ulong: return std::to_string(u.ul32);
			case v_int64: return std::to_string(u.i64);
			case v_uint64: return std::to_string(u.u64);
			case v_float: return std::to_string(u.f32);
			case v_double: return std::to_string(u.f64);
			case v_string: return s;
			}
			return "";
		}
	private:
		v_type type;
		union {
			bool b8;
			char i8;
			unsigned char u8;
			short i16;
			unsigned short u16;
			int i32;
			unsigned int u32;
			long l32;
			unsigned long ul32;
			float f32;
			double f64;
			long long i64;
			unsigned long long u64;
		}u;
		std::string s;
	};

	typedef variant any;
}

#endif