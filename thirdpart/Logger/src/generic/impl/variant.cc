#include "Logger/src/generic/variant.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"

namespace STD {

	//////////////////////////////////////
	//STD::variant

	variant::variant()
		: type(v_null) {
		memset(&u, 0, sizeof u);
	}
	variant::variant(bool val) {
		*this = val;
	}
	variant::variant(char val) {
		*this = val;
	}
	variant::variant(unsigned char val) {
		*this = val;
	}
	variant::variant(short val) {
		*this = val;
	}
	variant::variant(unsigned short val) {
		*this = val;
	}
	variant::variant(int val) {
		*this = val;
	}
	variant::variant(unsigned int val) {
		*this = val;
	}
	variant::variant(long val) {
		*this = val;
	}
	variant::variant(unsigned long val) {
		*this = val;
	}
	variant::variant(long long val) {
		*this = val;
	}
	variant::variant(unsigned long long val) {
		*this = val;
	}
	variant::variant(float val) {
		*this = val;
	}
	variant::variant(double val) {
		*this = val;
	}
	variant::variant(long double val) {
		*this = val;
	}
	variant::variant(char const* val) {
		*this = val;
	}
	variant::variant(std::string const& val) {
		*this = val;
	}
	inline variant& variant::operator=(bool val) {
		u.b8 = val;
		type = v_bool;
	}
	inline variant& variant::operator=(char val) {
		u.i8 = val;
		type = v_char;
	}
	inline variant& variant::operator=(unsigned char val) {
		u.u8 = val;
		type = v_uchar;
	}
	inline variant& variant::operator=(short val) {
		u.i16 = val;
		type = v_short;
	}
	inline variant& variant::operator=(unsigned short val) {
		u.u16 = val;
		type = v_ushort;
	}
	inline variant& variant::operator=(int val) {
		u.i32 = val;
		type = v_int;
	}
	inline variant& variant::operator=(unsigned int val) {
		u.u32 = val;
		type = v_uint;
	}
	inline variant& variant::operator=(long val) {
		u.l32 = val;
		type = v_long;
	}
	inline variant& variant::operator=(unsigned long val) {
		u.ul32 = val;
		type = v_ulong;
	}
	inline variant& variant::operator=(long long val) {
		u.i64 = val;
		type = v_int64;
	}
	inline variant& variant::operator=(unsigned long long val) {
		u.u64 = val;
		type = v_uint64;
	}
	inline variant& variant::operator=(float val) {
		u.f32 = val;
		type = v_float;
	}
	inline variant& variant::operator=(double val) {
		u.f64 = val;
		type = v_double;
	}
	inline variant& variant::operator=(long double val) {
		u.lf64 = val;
		type = v_ldouble;
	}
	inline variant& variant::operator=(char const* val) {
		s = val;
		type = v_string;
	}
	inline variant& variant::operator=(std::string const& val) {
		s = val;
		type = v_string;
	}
	inline bool variant::variant::as_bool() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::atoi(s.c_str());
		}
		return 0;
	}
	inline char variant::as_char() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::atoi(s.c_str());
		}
		return 0;
	}
	inline unsigned char variant::as_uchar() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::atoi(s.c_str());
		}
		return 0;
	}
	inline short variant::as_short() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::atoi(s.c_str());
		}
		return 0;
	}
	inline unsigned short variant::as_ushort() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::atoi(s.c_str());
		}
		return 0;
	}
	inline int variant::as_int() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stoi(s.c_str());
		}
		return 0;
	}
	inline unsigned int variant::as_uint() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stoul(s.c_str());
		}
		return 0;
	}
	inline long variant::as_long() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stol(s.c_str());
		}
		return 0;
	}
	inline unsigned long variant::as_ulong() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stoul(s.c_str());
		}
		return 0;
	}
	inline long long variant::as_int64() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stoll(s.c_str());
		}
		return 0;
	}
	inline unsigned long long variant::as_uint64() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stoull(s.c_str());
		}
		return 0;
	}
	inline float variant::as_float() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stof(s.c_str());
		}
		return 0;
	}
	inline double variant::as_double() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stod(s.c_str());
		}
		return 0;
	}
	inline long double variant::as_ldouble() {
		switch (type) {
		case v_bool: return u.b8;
		case v_char: return u.i8;
		case v_uchar: return  u.u8;
		case v_short: return u.i16;
		case v_ushort: return u.u16;
		case v_int: return u.i32;
		case v_uint: return u.u32;
		case v_long: return u.l32;
		case v_ulong: return u.ul32;
		case v_int64: return u.i64;
		case v_uint64: return u.u64;
		case v_float: return u.f32;
		case v_double: return u.f64;
		case v_ldouble: return u.lf64;
		case v_string: return std::stold(s.c_str());
		}
		return 0;
	}
	inline std::string variant::as_string() {
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
		case v_ldouble: return std::to_string(u.lf64);
		case v_string: return s;
		}
		return "";
	}
}