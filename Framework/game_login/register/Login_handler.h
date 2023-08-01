#ifndef INCLUDE_LOGINHANDLER_H
#define INCLUDE_LOGINHANDLER_H

void Login(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime);

#endif