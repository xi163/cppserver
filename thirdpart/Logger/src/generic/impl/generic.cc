#include "Logger/src/generic/generic.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"


namespace STD {
#if 0
	bool generic_map::has(std::string const& key) {
		return find(key) != end();
	}

	any& generic_map::operator[](std::string const& key) {
		map::iterator it = find(key);
		if (it != end()) {
			return it->second;
		}
#if 0
		any& ref = (*this)[key];
		return ref;
#else
		//insert({ key, any() });
		std::pair<iterator, bool> result = insert(std::make_pair(key, any()));
		return result.first->second;
#endif
	}
#else
	generic_map::~generic_map() {
		m.clear();
	}
	
	generic_map::iterator generic_map::begin() {
		return m.begin();
	}

	generic_map::iterator generic_map::end() {
		return m.end();
	}

	void generic_map::clear() {
		m.clear();
	}
	
	bool generic_map::empty() {
		return m.empty();
	}
	
	size_t generic_map::size() {
		return m.size();
	}
	
	bool generic_map::has(std::string const& key) {
		return m.find(key) != m.end();
	}

	any& generic_map::operator[](std::string const& key) {
		std::map<std::string, any>::iterator it = m.find(key);
		if (it != m.end()) {
			return it->second;
		}
#if 1
		any& ref = m[key];
		return ref;
#else
		//insert({ key, any() });
		std::pair<std::map<std::string, any>::iterator, bool> result = m.insert(std::make_pair(key, any()));
		return result.first->second;
#endif
	}
	
	std::map<std::string, any>& generic_map::operator*() {
		return m;
	}
#endif
}