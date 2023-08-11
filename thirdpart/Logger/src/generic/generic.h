#ifndef INCLUDE_STD_GENERIC_H
#define INCLUDE_STD_GENERIC_H

#include "Logger/src/generic/variant.h"

namespace STD {

	//////////////////////////////////////
	//STD::generic_map[string]any
#if 0
	class generic_map : public std::map<std::string, any> {
	public:
		bool has(std::string const& key);
		any& operator[](std::string const& key);
	};
#else
	class generic_map {
	public:
		typedef std::map<std::string, any>::iterator iterator;
	public:
		~generic_map();
		iterator begin();
		iterator end();
		void clear();
		bool empty();
		size_t size();
		bool has(std::string const& key);
		any& operator[](std::string const& key);
		std::map<std::string, any>& operator*();
	private:
		std::map<std::string, any> m;
	};
#endif
}

#endif