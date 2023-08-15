#ifndef INCLUDE_ROUTERHANDLER_H
#define INCLUDE_ROUTERHANDLER_H

int Router(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime);

#endif