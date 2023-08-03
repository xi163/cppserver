#ifndef INCLUDE_ORDERHANDLER_H
#define INCLUDE_ORDERHANDLER_H

int Order(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime);

#endif