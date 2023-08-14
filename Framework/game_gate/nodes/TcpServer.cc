#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "../Gate.h"

void GateServ::onTcpConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_[KTcpTy].incrementAndGet();
		_LOG_INFO("网关服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		int32_t num = numConnected_[KTcpTy].decrementAndGet();
		_LOG_INFO("网关服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void GateServ::onTcpMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	conn->getLoop()->assertInLoopThread();
	while (buf->readableBytes() >= packet::kMinPacketSZ) {
		const uint16_t len = buf->peekInt16(true);
		if (likely(len > packet::kMaxPacketSZ ||
			len < packet::kPrevHeaderLen + packet::kHeaderLen ||
			buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen)) {
			if (conn) {
#if 0
				//不再发送数据
				conn->shutdown();
#else
				conn->forceClose();
#endif
			}
			break;
		}
		else if (likely(len <= buf->readableBytes())) {
			BufferPtr buffer(new muduo::net::Buffer(len));
			buffer->append(buf->peek(), static_cast<size_t>(len));
			buf->retrieve(len);
			packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buffer);
			assert(packet::checkCheckSum(pre_header));
			packet::header_t const* header = packet::get_header(buffer);
			uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
			assert(header->crc == crc);
			std::string session((char const*)pre_header->session, packet::kSessionSZ);
			assert(!session.empty() && session.size() == packet::kSessionSZ);
			//session -> conn -> entryContext -> index
			muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
			if (peer) {
				Context& entryContext = boost::any_cast<Context&>(peer->getContext());
				entryContext.getWorker()->run(
					std::bind(
						&GateServ::asyncTcpHandler,
						this,
						conn, peer, buffer, receiveTime));
			}
			else {
				//_LOG_ERROR("error");
				//break;
			}
		}
		//数据包不足够解析，等待下次接收再解析
		else {
			_LOG_ERROR("error");
			break;
		}
	}
}

void GateServ::asyncTcpHandler(
	muduo::net::WeakTcpConnectionPtr const& weakTcpConn,
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	muduo::net::TcpConnectionPtr conn(weakTcpConn.lock());
	if (!conn) {
		_LOG_ERROR("error");
		return;
	}
	packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
	packet::header_t const* header = packet::get_header(buf);
	std::string session((char const*)pre_header->session, packet::kSessionSZ);
	assert(!session.empty() && session.size() == packet::kSessionSZ);
	//session -> conn -> entryContext
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		Context& entryContext = boost::any_cast<Context&>(peer->getContext());
		int64_t userId = pre_header->userId;
		assert(userId == entryContext.getUserId());
		assert(session != entryContext.getSession());
		TraceMessageID(header->mainId, header->subId);
		muduo::net::websocket::send(peer, (uint8_t const*)header, header->len);
	}
	else {
		_LOG_ERROR("error");
	}
}

void GateServ::onMarqueeNotify(std::string const& msg) {
	_LOG_INFO("跑马灯消息\n%s", msg.c_str());
	broadcastNoticeMsg("跑马灯消息", msg, 0, 2);
}