#include "ErrorCode.h"
#include "Response.h"

namespace response {
	namespace json {
		namespace err {

			int Result(Msg const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp) {
				return response::json::Result(msg.code, msg.msg(), data, rsp);
			}
		}
	}
}