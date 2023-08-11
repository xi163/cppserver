#include "Logger/src/generic/variant.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"

namespace STD {

	//////////////////////////////////////
	//STD::variant

	variant::variant()
#ifdef _union
		: type(v_null)
#endif
	{
#ifdef _union
		memset(&u, 0, sizeof u);
#endif
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
// 	variant::variant(variant const& ref) {
// 		copy(ref);
// 	}
// 	variant& variant::operator=(variant const& ref) {
// 		return copy(ref);
// 	}
// 	variant& variant::copy(variant const& ref) {
// #ifdef _union
// 		memcpy(&u, &ref.u, sizeof u);
// #endif
// 		s = ref.s;
// 		return *this;
// 	}
	variant& variant::operator=(bool val) {
#ifdef _union
		u.b8 = val;
		type = v_bool;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(char val) {
#ifdef _union
		u.i8 = val;
		type = v_char;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(unsigned char val) {
#ifdef _union
		u.u8 = val;
		type = v_uchar;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(short val) {
#ifdef _union
		u.i16 = val;
		type = v_short;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(unsigned short val) {
#ifdef _union
		u.u16 = val;
		type = v_ushort;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(int val) {
#ifdef _union
		u.i32 = val;
		type = v_int;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(unsigned int val) {
#ifdef _union
		u.u32 = val;
		type = v_uint;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(long val) {
#ifdef _union
		u.l32 = val;
		type = v_long;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(unsigned long val) {
#ifdef _union
		u.ul32 = val;
		type = v_ulong;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(long long val) {
#ifdef _union
		u.i64 = val;
		type = v_int64;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(unsigned long long val) {
#ifdef _union
		u.u64 = val;
		type = v_uint64;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(float val) {
#ifdef _union
		u.f32 = val;
		type = v_float;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(double val) {
#ifdef _union
		u.f64 = val;
		type = v_double;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(long double val) {
#ifdef _union
		u.lf64 = val;
		type = v_ldouble;
#else
		s = std::to_string(val);
#endif
		return *this;
	}
	variant& variant::operator=(char const* val) {
#ifdef _union
		s = val;
		type = v_string;
#else
		s = val;
#endif
		return *this;
	}
	variant& variant::operator=(std::string const& val) {
#ifdef _union
		s = val;
		type = v_string;
#else
		s = val;
#endif
		return *this;
	}
	bool variant::variant::as_bool() {
#ifdef _union
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
#else
		return std::atoi(s.c_str());
#endif
	}
	char variant::as_char() {
#ifdef _union
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
#else
		return std::atoi(s.c_str());
#endif
	}
	unsigned char variant::as_uchar() {
#ifdef _union
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
#else
		return std::atoi(s.c_str());
#endif
	}
	short variant::as_short() {
#ifdef _union
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
#else
		return std::atoi(s.c_str());
#endif
	}
	unsigned short variant::as_ushort() {
#ifdef _union
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
#else
		return std::atoi(s.c_str());
#endif
	}
	int variant::as_int() {
#ifdef _union
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
#else
		return std::stoi(s.c_str());
#endif
	}
	unsigned int variant::as_uint() {
#ifdef _union
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
#else
		return std::stoul(s.c_str());
#endif
	}
	long variant::as_long() {
#ifdef _union
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
#else
		return std::stol(s.c_str());
#endif
	}
	unsigned long variant::as_ulong() {
#ifdef _union
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
#else
		return std::stoul(s.c_str());
#endif
	}
	long long variant::as_int64() {
#ifdef _union
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
#else
		return std::stoll(s.c_str());
#endif
	}
	unsigned long long variant::as_uint64() {
#ifdef _union
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
#else
		return std::stoull(s.c_str());
#endif
	}
	float variant::as_float() {
#ifdef _union
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
#else
		return std::stof(s.c_str());
#endif
	}
	double variant::as_double() {
#ifdef _union
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
#else
		return std::stod(s.c_str());
#endif
	}
	long double variant::as_ldouble() {
#ifdef _union
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
#else
		return std::stold(s.c_str());
#endif
	}
	std::string variant::as_string() {
#ifdef _union
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
#else
		return s;
#endif
	}
}