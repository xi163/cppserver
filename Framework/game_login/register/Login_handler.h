#ifndef INCLUDE_LOGINHANDLER_H
#define INCLUDE_LOGINHANDLER_H

void Login(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime);

#endif