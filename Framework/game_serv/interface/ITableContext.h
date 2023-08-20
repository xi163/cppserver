#ifndef INCLUDE_ITABLECONTEXT_H
#define INCLUDE_ITABLECONTEXT_H

#include "public/Inc.h"

namespace packet {
	struct internal_prev_header_t;
	struct header_t;
};

struct tagGameInfo;
struct tagGameRoomInfo;

class ITableContext {
public:
	virtual tagGameInfo* GetGameInfo() = 0;
	virtual tagGameRoomInfo* GetRoomInfo() = 0;
	virtual std::string const& ServId() = 0;
	virtual void KickUser(int64_t userId, int32_t kickType) = 0;
	virtual boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> GetContext(int64_t userId) = 0;
	virtual void DelContext(int64_t userId) = 0;
	virtual bool IsStopped() = 0;
	virtual void send(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t const* msg, size_t len,
		uint8_t mainId,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_) = 0;
	virtual void send(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t const* msg, size_t len,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_) = 0;
	virtual void send(
		const muduo::net::TcpConnectionPtr& conn,
		::google::protobuf::Message* msg,
		uint8_t mainId,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_) = 0;
	virtual void send(
		const muduo::net::TcpConnectionPtr& conn,
		::google::protobuf::Message* msg,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_) = 0;
};

#endif