#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "Gateway.h"

void GateServ::onInnConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		_LOG_INFO("网关服[%s] <- 推送服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		_LOG_INFO("网关服[%s] <- 推送服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void GateServ::onInnMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	conn->getLoop()->assertInLoopThread();
	while (buf->readableBytes() >= packet::kMinPacketSZ) {
		const uint16_t len = buf->peekInt16(true);
		if (likely(len > packet::kMaxPacketSZ ||
			len < packet::kPrevHeaderLen + packet::kHeaderLen)) {
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
			std::string session((char const*)pre_header->session, sizeof(pre_header->session));
			assert(!session.empty() && session.size() == packet::kSessionSZ);
#if 1
			//session -> hash(session) -> index
			int index = hash_session_(session) % threadPool_.size();
			threadPool_[index]->run(
				std::bind(
					&GateServ::asyncInnHandler,
					this,
					muduo::net::WeakTcpConnectionPtr(conn), buffer, receiveTime));
#else
			//session -> conn -> entryContext -> index
			muduo::net::WeakTcpConnectionPtr weakConn = entities_.get(session);
			muduo::net::TcpConnectionPtr peer(weakConn.lock());
			if (peer) {
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);
				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());
				threadPool_[index]->run(
					std::bind(
						&GateServ::asyncInnHandler,
						this,
						weakConn, buffer, receiveTime));
			}
#endif
		}
		//数据包不足够解析，等待下次接收再解析
		else {
			break;
		}
	}
}

void GateServ::asyncInnHandler(
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (!conn) {
		return;
	}
	if (buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen) {
		return;
	}
	packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
	packet::header_t const* header = packet::get_header(buf);
	std::string session((char const*)pre_header->session, sizeof(pre_header->session));
	assert(!session.empty() && session.size() == packet::kSessionSZ);
	//session -> conn
	muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
	if (peer) {
		ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
		assert(entryContext);
		int64_t userId = pre_header->userId;
		assert(userId == entryContext->getUserID());
		assert(session != entryContext->getSession());
		TraceMessageID(header->mainId, header->subId);
		muduo::net::websocket::send(peer, (uint8_t const*)header, header->len);
	}
}

void GateServ::onMarqueeNotify(std::string const& msg) {
	_LOG_INFO("跑马灯消息\n%s", msg.c_str());
	broadcastNoticeMsg("跑马灯消息", msg, 0, 2);
}