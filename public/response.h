#ifndef INCLUDE_RESPONSE_H
#define INCLUDE_RESPONSE_H

#include "public/Inc.h"

namespace response {
	/*
	HTTP/1.1 400 Bad Request\r\n\r\n
	HTTP/1.1 404 Not Found\r\n\r\n
	HTTP/1.1 405 服务维护中\r\n\r\n
	HTTP/1.1 500 IP访问限制\r\n\r\n
	HTTP/1.1 504 权限不够\r\n\r\n
	HTTP/1.1 505 timeout\r\n\r\n
	HTTP/1.1 600 访问量限制(1500)\r\n\r\n
	*/
	namespace text {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
	}
	namespace json {
		int Result(int code, std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Result(int code, std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int Result(int code, std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Result(int code, std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int Result(Msg const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Result(Msg const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int Result(Msg const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Result(Msg const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int Ok(muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Ok(muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int OkMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int OkMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int OkMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int OkMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int Err(muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int Err(muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int ErrMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int ErrMsg(std::string const& msg, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int ErrMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Json const& data);
		int ErrMsg(std::string const& msg, std::string const& extra, muduo::net::HttpResponse& rsp, BOOST::Any const& data = BOOST::Any());
		
		int BadRequest(muduo::net::HttpResponse& rsp);
	}
	namespace xml {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
		void Test(muduo::net::HttpRequest const& req, muduo::net::HttpResponse& rsp);
	}
	namespace html {
		void Result(muduo::net::http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
		void NotFound(muduo::net::HttpResponse& rsp);
	}
}

#endif