#include "ErrorCode.h"
#include "Response.h"

namespace ERR {

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ERROR_DESC_X(n, s) { k##n, "k"#n, s },
#define ERROR_DESC_Y(n, i, s){ k##n, "k"#n, s },
#define TABLE_DECLARE(var, MY_MAP_) \
	static struct { \
		int id_; \
		char const* name_; \
		char const* desc_; \
	}var[] = { \
		MY_MAP_(ERROR_DESC_X, ERROR_DESC_Y) \
	}

#define ERROR_FETCH(id, var, str) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			std::string name = var[i].name_; \
			std::string desc = var[i].desc_; \
			str = name.empty() ? \
				desc : "[" + name + "]" + desc;\
			break; \
		}\
	}

#define ERROR_IMPLEMENT(varname) \
		std::string get##varname##Str(int varname) { \
			TABLE_DECLARE(table_##varname##s_, ERROR_MAP); \
			std::string str##varname; \
			ERROR_FETCH(varname, table_##varname##s_, str##varname); \
			return str##varname; \
		}

	ERROR_IMPLEMENT(Error)
#undef ERROR_DESC_X
#undef ERROR_DESC_Y
}

namespace response {
	namespace json {
		namespace err {

			int Result(Msg const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp) {
				return response::json::Result(msg.code, msg.errmsg(), data, rsp);
			}
			int Result(Msg const& msg, std::string const& extra, BOOST::Any const& data, muduo::net::HttpResponse& rsp) {
				switch (extra.empty()) {
				case true:
					return response::json::Result(msg.code, msg.errmsg(), data, rsp);
				default:
					return response::json::Result(msg.code, msg.errmsg() + " " + std::move(extra), data, rsp);
				}
			}
		}
	}
}