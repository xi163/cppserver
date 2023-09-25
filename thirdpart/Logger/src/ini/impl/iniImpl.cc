#include "../ini.h"
#include "iniImpl.h"
#include "../../utils/impl/utilsImpl.h"

#include "../../log/impl/LoggerImpl.h"
#include "../../excp/impl/excpImpl.h"

#include "../../op/file/impl/FileImpl.h"

namespace utils {

	static std::string _trimLineCRLF(char* buf, size_t len) {
		if (buf && buf[0] && len > 0) {
			if (len > 1 && (buf[len - 2] == '\r' && buf[len - 1] == '\n')) {
				//remove tail \r\n
				buf[len - 2] = '\0';
				buf[len - 1] = '\0';
			}
			else if (/*buf[len - 1] == '\r' || */buf[len - 1] == '\n') {
				//remove tail \r or \n
				buf[len - 1] = '\0';
			}
			return buf;
		}
		return "";
	}

	static std::string _trimLineCRLF(std::string const& str) {
		std::string s(str);
		if (!s.empty()) {
#if 1
			size_t len = s.length();
			if (len > 1 && (s[len - 2] == '\r' && s[len - 1] == '\n')) {
				//remove tail \r\n
				s[len - 2] = '\0';
				s[len - 1] = '\0';
			}
			else if (/*s[len - 1] == '\r' || */s[len - 1] == '\n') {
				//remove tail \r or \n
				s[len - 1] = '\0';
			}
			return s;

#else
			utils::_replaceAll(s, "\r\n", "");
			if (str == s) {
				//utils::_replaceAll(s, "\r", "");
				utils::_replaceAll(s, "\n", "");
			}
#endif
		}
		return s;
	}

	namespace INI {
		void _readBuffer(char const* buf, Sections& sections) {
			sections.clear();
			std::string st(buf);
			std::string field;
			while (!st.empty()) {
				//读取一行数据
				std::string::size_type pos = st.find_first_of("\r\n");
				if (pos == std::string::npos) {
					if (!st.empty()) {
						std::string s = st.substr(0, pos);
						if (!s.empty()) {
							if (s.length() > 2 && s[0] == '[' && s[s.length() - 1] == ']') {
								field = s.substr(1, s.length() - 2);
								Section& ref = sections[field];
							}
							else if (!field.empty()) {
								std::string::size_type pos = s.find_first_of('=');
								if (pos != std::string::npos) {
									Section& ref = sections[field];
									std::string key = s.substr(0, pos);
									std::string value = s.substr(pos + 1, -1);
#ifdef USEKVMAP
									ref[key] = value;
#else
									Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
										return item.first == key;
										});
									if (ir != ref.end()) {
										ir->second = value;
									}
									else {
										ref.emplace_back(std::make_pair(key, value));
									}
#endif
								}
							}
						}
					}
					break;
				}
				std::string s = st.substr(0, pos);
				if (!s.empty()) {
					if (s.length() > 2 && s[0] == '[' && s[s.length() - 1] == ']') {
						field = s.substr(1, s.length() - 2);
						Section& ref = sections[field];
					}
					else if (!field.empty()) {
						std::string::size_type pos = s.find_first_of('=');
						if (pos != std::string::npos) {
							Section& ref = sections[field];
							std::string key = s.substr(0, pos);
							std::string value = s.substr(pos + 1, -1);
#ifdef USEKVMAP
							ref[key] = value;
#else
							Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
								return item.first == key;
								});
							if (ir != ref.end()) {
								ir->second = value;
							}
							else {
								ref.emplace_back(std::make_pair(key, value));
							}
#endif
						}
					}
				}
				else {
					break;
				}
				st = st.substr(pos + 2, -1);
			};
		}
		
		void _readFile(char const* filename, Sections& sections) {
			std::string field;
			std::string s;
#if 0
			FILE* stream = fopen(filename, "r");
			if (stream) {
				char chr[1024];
				while (!feof(stream)) {
					//读取一行数据
					if (NULL != fgets(chr, sizeof chr, stream)) {
						s = utils::_trimLineCRLF(chr, strlen(chr));
						if (!s.empty()) {
							if (s.length() > 2 && s[0] == '[' && s[s.length() - 1] == ']') {
								field = s.substr(1, s.length() - 2);
								Section& ref = sections[field];
							}
							else if (!field.empty()) {
								std::string::size_type pos = s.find_first_of('=');
								if (pos != std::string::npos) {
									Section& ref = sections[field];
									std::string key = s.substr(0, pos);
									std::string value = s.substr(pos + 1, -1);
#ifdef USEKVMAP
									ref[key] = value;
#else
									Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
										return item.first == key;
										});
									if (ir != ref.end()) {
										ir->second = value;
									}
									else {
										ref.emplace_back(std::make_pair(key, value));
									}
#endif
								}
							}
						}
					}
				}
				fclose(stream);
			}
#else
			std::fstream stream(filename);
			if (stream.is_open()) {
				//读取一行数据
				while (getline(stream, s)) {
					s = utils::_trimLineCRLF(s);
					if (!s.empty()) {
						if (s.length() > 2 && s[0] == '[' && s[s.length() - 1] == ']') {
							field = s.substr(1, s.length() - 2);
							Section& ref = sections[field];
						}
						else if (!field.empty()) {
							std::string::size_type pos = s.find_first_of('=');
							if (pos != std::string::npos) {
								Section& ref = sections[field];
								std::string key = s.substr(0, pos);
								std::string value = s.substr(pos + 1, -1);
#ifdef USEKVMAP
								ref[key] = value;
#else
								Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
									return item.first == key;
									});
								if (ir != ref.end()) {
									ir->second = value;
								}
								else {
									ref.emplace_back(std::make_pair(key, value));
								}
#endif
							}
						}
					}
				}
				stream.close();
			}
#endif
		}
		
		ReaderImpl::ReaderImpl():m_(New<Sections>()) {
			//placement new
		}
		
		ReaderImpl::~ReaderImpl() {
			Delete<Sections>(m_);
		}
		
		bool ReaderImpl::parse(char const* filename) {
			m_->clear();
			if (filename && filename[0]) {
				utils::INI::_readFile(filename, *m_);
			}
			return m_->size() > 0;
		}

		bool ReaderImpl::parse(char const* buf, size_t len) {
			m_->clear();
			if (buf && buf[0]) {
				utils::INI::_readBuffer(buf, *m_);
			}
			return m_->size() > 0;
		}
		
		Sections const& ReaderImpl::get() {
			return *m_;
		}

		Section* ReaderImpl::get(char const* section) {
			if (section && section[0]) {
				Sections::iterator it = m_->find(section);
				if (it != m_->end()) {
					return &it->second;
				}
			}
			return NULL;
		}
		
		std::string ReaderImpl::get(char const* section, char const* key) {
			if (section && section[0]) {
				Sections::iterator it = m_->find(section);
				if (it != m_->end()) {
					Section& ref = it->second;
					if (key && key[0]) {
#ifdef USEKVMAP
						Section::iterator ir = ref.find(key);
#else
						Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
							return item.first == key;
							});
#endif
						if (ir != ref.end()) {
							return ir->second;
						}
					}
				}
			}
			return "";
		}

		std::string ReaderImpl::get(char const* section, char const* key, bool& hasKey) {
			if (section && section[0]) {
				Sections::iterator it = m_->find(section);
				if (it != m_->end()) {
					Section& ref = it->second;
					if (key && key[0]) {
#ifdef USEKVMAP
						Section::iterator ir = ref.find(key);
#else
						Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
							return item.first == key;
							});
#endif
						if (ir != ref.end()) {
							hasKey = true;
							return ir->second;
						}
					}
				}
			}
			return "";
		}

		void ReaderImpl::set(char const* section, char const* key, char const* value, char const* filename) {
			if (m_->empty()) {
				parse(filename);
			}
			if (section && section[0]) {
				Sections::iterator it = m_->find(section);
				if (it != m_->end()) {
					if (key && key[0]) {
						Section& ref = it->second;
#ifdef USEKVMAP
						ref[key] = value;
#else
						Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
							return item.first == key;
							});
						if (ir != ref.end()) {
							ir->second = value;
						}
						else {
							ref.emplace_back(std::make_pair(key, value));
						}
#endif
						write(filename);
					}
				}
				else {
					if (key && key[0]) {
						Section& ref = (*m_)[section];
#ifdef USEKVMAP
						ref[key] = value;
#else
						Section::iterator ir = std::find_if(std::begin(ref), std::end(ref), [&](Item& item) -> bool {
							return item.first == key;
							});
						if (ir != ref.end()) {
							ir->second = value;
						}
						else {
							ref.emplace_back(std::make_pair(key, value));
						}
#endif
						write(filename);
					}
				}
			}
		}

		void ReaderImpl::write(char const* filename) {
			if (filename && filename[0]) {
				Operation::FileImpl f(filename);
				if (f.Valid() && f.Open(Operation::Mode::M_WRITE)) {
					for (Sections::const_iterator it = m_->begin();
						it != m_->end(); ++it) {
						//先写[section]
						if (it == m_->begin()) {
							std::string s = "[" + it->first + "]" + "\n";
							f.Write(&s.front(), 1, s.length());
						}
						else {
							std::string s = "\n[" + it->first + "]" + "\n";
							f.Write(&s.front(), 1, s.length());
						}
						Section const& ref = it->second;
						for (Section::const_iterator ir = ref.begin();
							ir != ref.end(); ++ir) {
							//再写key=value
							std::string s = ir->first + "=" + ir->second + "\n";
							f.Write(&s.front(), 1, s.length());
						}
					}
					f.Flush();
					f.Close();
				}
			}
		}
	}
}