#include "response.h"

namespace response {
	namespace text {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp) {
			rsp.setStatusCode(code);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Text_utf8);
			rsp.setBody(msg);
		}
	}
	namespace json {
		int Result(int code, std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			std::string s = BOOST::json::Result(code, msg, data);
			_LOG_ERROR("\n%s", s.c_str());
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Json_utf8);
			rsp.setBody(s);
			return code;
		}
		int Result(int code, std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			std::string s = BOOST::json::Result(code, msg, data);
			_LOG_ERROR("\n%s", s.c_str());
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Json_utf8);
			rsp.setBody(s);
			return code;
		}
		int Result(int code, std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			std::string s = BOOST::json::Result(code, msg, extra, data);
			_LOG_ERROR("\n%s", s.c_str());
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Json_utf8);
			rsp.setBody(s);
			return code;
		}
		int Result(int code, std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			std::string s = BOOST::json::Result(code, msg, extra, data);
			_LOG_ERROR("\n%s", s.c_str());
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Json_utf8);
			rsp.setBody(s);
			return code;
		}
		int Result(Msg const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(msg.code, msg.errmsg(), rsp, data);
		}
		int Result(Msg const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(msg.code, msg.errmsg(), rsp, data);
		}
		int Result(Msg const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			switch (extra.empty()) {
			case true:
				return Result(msg.code, msg.errmsg(), rsp, data);
			default:
				return Result(msg.code, msg.errmsg(), std::move(extra), rsp, data);
			}
		}
		int Result(Msg const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			switch (extra.empty()) {
			case true:
				return Result(msg.code, msg.errmsg(), rsp, data);
			default:
				return Result(msg.code, msg.errmsg(), std::move(extra), rsp, data);
			}
		}
		int Ok(muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Ok, rsp, data);
		}
		int Ok(muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Ok, rsp, data);
		}
		int OkMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Ok.code, msg, rsp, data);
		}
		int OkMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Ok.code, msg, rsp, data);
		}
		int OkMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Ok.code, msg, extra, rsp, data);
		}
		int OkMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Ok.code, msg, extra, rsp, data);
		}
		int Err(muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Error, rsp, data);
		}
		int Err(muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Error, rsp, data);
		}
		int ErrMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Error.code, msg, rsp, data);
		}
		int ErrMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Error.code, msg, rsp, data);
		}
		int ErrMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data) {
			return Result(::Error.code, msg, extra, rsp, data);
		}
		int ErrMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data) {
			return Result(::Error.code, msg, extra, rsp, data);
		}
		int BadRequest(muduo::net::HttpResponse& rsp) {
			std::string s = BOOST::json::Result(
				muduo::net::HttpResponse::k400BadRequest,
				"bad request",
				BOOST::Any());
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Json_utf8);
			rsp.setBody(s);
			return muduo::net::HttpResponse::k400BadRequest;
		}
	}
	namespace xml {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp) {
			std::stringstream ss;
			ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
				<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
				//<< "<xs:head>" << headers << "</xs:head>"
				<< "<xs:body>"
				//<< "<xs:method>" << req.methodString() << "</xs:method>"
				//<< "<xs:path>" << req.path() << "</xs:path>"
				//<< "<xs:query>" << utils::HTML::Encode(req.query()) << "</xs:query>"
				<< "<xs:response>" << msg << "</xs:response>"
				<< "</xs:body>"
				<< "</xs:root>";
			//rsp.setCloseConnection(true);
			//rsp.addHeader("Server", "MUDUO");
			rsp.setStatusCode(code);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Xml_utf8);
			rsp.setBody(ss.str());
		}
		void Test(muduo::net::HttpRequest const& req, muduo::net::HttpResponse& rsp) {
			std::string headers;
			for (std::map<std::string, std::string>::const_iterator it = req.headers().begin();
				it != req.headers().end(); ++it) {
				headers += it->first + ": " + it->second + "\n";
			}
			std::stringstream ss;
			ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
				<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
				<< "<xs:head>" << headers << "</xs:head>"
				<< "<xs:body>"
				<< "<xs:method>" << req.methodString() << "</xs:method>"
				<< "<xs:path>" << req.path() << "</xs:path>"
				<< "<xs:query>" << utils::HTML::Encode(req.query()) << "</xs:query>"
				<< "</xs:body>"
				<< "</xs:root>";
			rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Xml_utf8);
			rsp.setBody(ss.str());
		}
	}
	namespace html {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp) {
			rsp.setStatusCode(code);
			rsp.setStatusMessage("OK");
			rsp.setContentType(ContentType_Html_utf8);
			std::string now = muduo::Timestamp::now().toFormattedString();
			rsp.setBody("<html><head><h2>" + now + "<h2></head><body><h1>" + msg + "</h1></body></html>");
		}
		void NotFound(muduo::net::HttpResponse& rsp) {
			Result(muduo::net::HttpResponse::k404NotFound, "Not Found", rsp);
		}
	}
}