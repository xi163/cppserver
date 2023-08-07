#ifndef INCLUDE_ORDERHANDLER_H
#define INCLUDE_ORDERHANDLER_H

#include "public/gameStruct.h"

struct OrderReq {
	int	Type;
	int64_t userId;
	int64_t scoreI64;
	double Score;
	int64_t	Timestamp;
	std::string Account;
	std::string orderId;
	agent_info_t* p_agent_info;
	//boost::property_tree::ptree& latest;
	int* testTPS;
};

int addScore(OrderReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime);

int subScore(OrderReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime);

int Order(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime);

#endif