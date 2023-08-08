#include "Logger/src/generic/generic.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"


namespace STD {

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
}