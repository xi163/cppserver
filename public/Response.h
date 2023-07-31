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
		void Result(http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
	}
	namespace json {
		void Result(int code, std::string const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		void BadRequest(muduo::net::HttpResponse& rsp);
		void Ok(BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		void OkMsg(std::string const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		void Err(BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		void ErrMsg(std::string const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp);
	}
	namespace xml {
		void Result(http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
		void Test(muduo::net::HttpRequest const& req, muduo::net::HttpResponse& rsp);
	}
	namespace html {
		void Result(http::IResponse::StatusCode code, std::string const& msg, muduo::net::HttpResponse& rsp);
		void NotFound(muduo::net::HttpResponse& rsp);
	}
}

namespace response {
	namespace json {
		namespace err {

			struct Msg {
				int Code;
				std::string ErrMsg;
			};

			static const Msg ErrOk = Msg{ 0, "OK" };
			static const Msg ErrCreateGameUser = Msg{ 10001, "创建游戏账号失败" };
			static const Msg ErrGameGateNotExist = Msg{ 10002, "没有可用的游戏网关" };

			void Result(Msg const& msg, BOOST::Any const& data, muduo::net::HttpResponse& rsp);
		}
	}
}

#endif