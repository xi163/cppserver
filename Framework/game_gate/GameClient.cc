#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "Gate.h"

void GateServ::onGameConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		_LOG_INFO("网关服[%s] -> 游戏服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		_LOG_INFO("网关服[%s] -> 游戏服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		RunInLoop(threadTimer_->getLoop(),
			std::bind(
				&GateServ::asyncGameOfflineHandler,
				this, conn->peerAddress().toIpPort()));
	}
}

void GateServ::onGameMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime) {
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
			std::string session((char const*)pre_header->session, sizeof(pre_header->session));
			assert(!session.empty() && session.size() == packet::kSessionSZ);
			//session -> conn -> entryContext -> index
			muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
			if (peer) {
				Context& entryContext = boost::any_cast<Context&>(peer->getContext());
				entryContext.getWorker()->run(
					std::bind(
						&GateServ::asyncGameHandler,
						this,
						conn, peer, buffer, receiveTime));
			}
		}
		//数据包不足够解析，等待下次接收再解析
		else {
			_LOG_ERROR("error");
			break;
		}
	}
}

void GateServ::asyncGameHandler(
	muduo::net::WeakTcpConnectionPtr const& weakGameConn,
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	muduo::net::TcpConnectionPtr conn(weakGameConn.lock());
	if (!conn) {
		_LOG_ERROR("error");
		return;
	}
	packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
	packet::header_t const* header = packet::get_header(buf);
	std::string session((char const*)pre_header->session, sizeof(pre_header->session));
	assert(!session.empty() && session.size() == packet::kSessionSZ);
	//session -> conn
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		Context& entryContext = boost::any_cast<Context&>(peer->getContext());
		int64_t userId = pre_header->userId;
		assert(userId > 0 && userId == entryContext.getUserId());
		assert(session == entryContext.getSession());
		TraceMessageID(header->mainId, header->subId);
		//////////////////////////////////////////////////////////////////////////
		//进房间失败/离开房间/游戏结束，清理用户游戏节点
		//////////////////////////////////////////////////////////////////////////
		if (header->mainId == Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER ||
			header->mainId == Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER) {
			if (header->subId == ::GameServer::SUB_S2C_ENTER_ROOM_RES) {
				if (pre_header->ok == -1) {
					entryContext.resetClientConn(servTyE::kGameTy);
					{
						WRITE_LOCK(mutexGameUsers_);
						std::map<std::string, std::set<int64_t>>::iterator it = mapGameUsers_.find(conn->peerAddress().toIpPort());
						if (it != mapGameUsers_.end()) {
							std::set<int64_t>::iterator ir = it->second.find(userId);
							if (ir != it->second.end()) {
								it->second.erase(ir);
								if (it->second.empty()) {
									mapGameUsers_.erase(it);
								}
							}
						}
					}
				}
				else {
					{
						WRITE_LOCK(mutexGameUsers_);
						std::set<int64_t>& ref = mapGameUsers_[conn->peerAddress().toIpPort()];
						ref.insert(userId);
					}
				}
			}
			else if (header->subId == ::GameServer::SUB_S2C_USER_LEFT_RES && pre_header->ok == 0) {
				entryContext.resetClientConn(servTyE::kGameTy);
				{
					WRITE_LOCK(mutexGameUsers_);
					std::map<std::string, std::set<int64_t>>::iterator it = mapGameUsers_.find(conn->peerAddress().toIpPort());
					if (it != mapGameUsers_.end()) {
						std::set<int64_t>::iterator ir = it->second.find(userId);
						if (ir != it->second.end()) {
							it->second.erase(ir);
							if (it->second.empty()) {
								mapGameUsers_.erase(it);
							}
						}
					}
				}
			}
		}
		muduo::net::websocket::send(peer, (uint8_t const*)header, header->len);
	}
	else {
		_LOG_ERROR("error");
	}
}

void GateServ::asyncGameOfflineHandler(std::string const& ipPort) {
	_LOG_ERROR("%s", ipPort.c_str());
	{
		BufferPtr buffer;
		READ_LOCK(mutexGameUsers_);
		std::map<std::string, std::set<int64_t>>::iterator it = mapGameUsers_.find(ipPort);
		if (it != mapGameUsers_.end()) {
			for (std::set<int64_t>::iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
				muduo::net::TcpConnectionPtr peer(sessions_.get(*ir).lock());
				if (peer) {
					if (!buffer) {
						buffer = GateServ::packKickGameUserMsg();
					}
					muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
				}
			}
		}
	}
	{
		WRITE_LOCK(mutexGameUsers_);
		std::map<std::string, std::set<int64_t>>::iterator it = mapGameUsers_.find(ipPort);
		if (it != mapGameUsers_.end()) {
			mapGameUsers_.erase(it);
		}
	}
}

void GateServ::sendGameMessage(
	Context& entryContext,
	BufferPtr const& buf, int64_t userId) {
	//_LOG_INFO("...");
	ClientConn const& clientConn = entryContext.getClientConn(servTyE::kGameTy);
	muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
	if (gameConn) {
		assert(gameConn->connected());
#if !defined(NDEBUG)
#if 0
		assert(
			std::find(
				std::begin(clients_[servTyE::kGameTy].names_),
				std::end(clients_[servTyE::kGameTy].names_),
				clientConn.first) != clients_[servTyE::kGameTy].names_.end());
#endif
		clients_[servTyE::kGameTy].clients_->check(clientConn.first, true);
#endif
		if (buf) {
			//_LOG_DEBUG("len = %d", buf->readableBytes());
			gameConn->send(buf.get());
		}
	}
	else {
		{
			//用户当前游戏节点不存在/不可用，需要指定
			if (clientConn.first.empty()) {
				_LOG_WARN("%d 游戏节点不存在，需要指定", userId);
			}
			else {
				_LOG_ERROR("%d 游戏节点[%s]不可用，需要指定", userId, clientConn.first.c_str());
			}
			std::string serverIp;//roomid:ip:port:mode
			if (REDISCLIENT.GetUserOnlineInfoIP(userId, serverIp)) {
				//获取目标游戏节点
				ClientConn clientConn;
				clients_[servTyE::kGameTy].clients_->get(serverIp, clientConn);
				muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
				if (gameConn) {
					//指定用户游戏节点
					entryContext.setClientConn(servTyE::kGameTy, clientConn);
					_LOG_INFO("%d 游戏节点[%s]，指定成功!", userId, serverIp.c_str());
				}
				else {
					//目标游戏节点不可用，要求zk实时监控
					_LOG_INFO("%d 游戏节点[%s]不可用，指定失败!", userId, serverIp.c_str());
				}
			}
			else {
				_LOG_ERROR("%d 游戏节点IP不存在!", userId);
			}
		}
		ClientConn const& clientConn = entryContext.getClientConn(servTyE::kGameTy);
		muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
		if (gameConn) {
			assert(gameConn->connected());
#if !defined(NDEBUG)
#if 0
			assert(
				std::find(
					std::begin(clients_[servTyE::kGameTy].names_),
					std::end(clients_[servTyE::kGameTy].names_),
					clientConn.first) != clients_[servTyE::kGameTy].names_.end());
#endif
			clients_[servTyE::kGameTy].clients_->check(clientConn.first, true);
#endif
			if (buf) {
				gameConn->send(buf.get());
			}
		}
		else {
			packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
			packet::header_t const* header = packet::get_header(buf);
			_LOG_ERROR("%s error", fmtMessageID(header->mainId, header->subId).c_str());
			EntryPtr entry(entryContext.getWeakEntryPtr().lock());
			if (entry) {
				muduo::net::TcpConnectionPtr peer(entry->getWeakConnPtr().lock());
				if (peer) {
					BufferPtr buffer = GateServ::packNotifyFailedMsg(header->mainId, header->subId);
					muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
				}
			}
		}
	}
}

void GateServ::onUserOfflineGame(
	Context& entryContext, bool leave) {
	//MY_TRY()
	int64_t userId = entryContext.getUserId();
	uint32_t clientIp = entryContext.getFromIp();
	std::string const& session = entryContext.getSession();
	std::string const& aeskey = entryContext.getAesKey();
	if (userId > 0 && !session.empty()) {
		BufferPtr buffer = packet::packMessage(
			userId,
			session,
			aeskey,
			clientIp,
			KICK_LEAVEGS,
			::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
			::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE,
			NULL);
		if (buffer) {
			TraceMessageID(
				::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
				::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE);
			assert(buffer->readableBytes() < packet::kMaxPacketSZ);
			sendGameMessage(entryContext, buffer, userId);
		}
	}
	//MY_CATCH()
}