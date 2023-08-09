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
			v_ldouble,
			v_string,
		};
	public:
		variant();
		explicit variant(bool val);
		explicit variant(char val);
		explicit variant(unsigned char val);
		explicit variant(short val);
		explicit variant(unsigned short val);
		explicit variant(int val);
		explicit variant(unsigned int val);
		explicit variant(long val);
		explicit variant(unsigned long val);
		explicit variant(long long val);
		explicit variant(unsigned long long val);
		explicit variant(float val);
		explicit variant(double val);
		explicit variant(long double val);
		explicit variant(char const* val);
		explicit variant(std::string const& val);
	public:
		variant& operator=(bool val);
		variant& operator=(char val);
		variant& operator=(unsigned char val);
		variant& operator=(short val);
		variant& operator=(unsigned short val);
		variant& operator=(int val);
		variant& operator=(unsigned int val);
		variant& operator=(long val);
		variant& operator=(unsigned long val);
		variant& operator=(long long val);
		variant& operator=(unsigned long long val);
		variant& operator=(float val);
		variant& operator=(double val);
		variant& operator=(long double val);
		variant& operator=(char const* val);
		variant& operator=(std::string const& val);
	public:
		bool as_bool();
		char as_char();
		unsigned char as_uchar();
		short as_short();
		unsigned short as_ushort();
		int as_int();
		unsigned int as_uint();
		long as_long();
		unsigned long as_ulong();
		long long as_int64();
		unsigned long long as_uint64();
		float as_float();
		double as_double();
		long double as_ldouble();
		std::string as_string();
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
			long double lf64;
			long long i64;
			unsigned long long u64;
		}u;
		std::string s;
	};

	typedef variant any;
}

#endif