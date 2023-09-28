#include "../ini.h"
#include "iniImpl.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../auth/auth.h"

namespace utils {

	namespace INI {
#ifndef USEKVMAP
		std::string& Section::operator[](std::string const& key) {
			Section::iterator ir = std::find_if(begin(), end(), [&](Item& kv) -> bool {
				return kv.first == key;
				});
			if (ir != end()) {
				return ir->second;
			}
			else {
				emplace_back(std::make_pair(key, ""));
				return back().second;
			}
		}
#endif
		Reader::Reader() :impl_(New<ReaderImpl>()) {
			//placement new
		}
		Reader::~Reader() {
			Delete<ReaderImpl>(impl_);
		}
		bool Reader::parse(char const* filename) {
			AUTHORIZATION_CHECK_B;
			return impl_->parse(filename);
		}
		bool Reader::parse(char const* buf, size_t len) {
			AUTHORIZATION_CHECK_B;
			return impl_->parse(buf, len);
		}
		Sections const& Reader::get() {
			return impl_->get();
		}
		Section* Reader::get(char const* section) {
			return impl_->get(section);
		}
		std::string Reader::get(char const* section, char const* key) {
			return impl_->get(section, key);
		}
		std::string Reader::get(char const* section, char const* key, bool& hasKey) {
			return impl_->get(section, key, hasKey);
		}
		void Reader::set(char const* section, char const* key, char const* value, char const* filename) {
			AUTHORIZATION_CHECK;
			impl_->set(section, key, value, filename);
		}
	}
}