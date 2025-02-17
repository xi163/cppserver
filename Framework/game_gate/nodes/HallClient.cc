#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "../Gate.h"

void GateServ::onHallConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_[kHallClientTy].incrementAndGet();
		Infof("网关服[%s] -> 大厅服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		int32_t num = numConnected_[kHallClientTy].decrementAndGet();
		Infof("网关服[%s] -> 大厅服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void GateServ::onHallMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime) {
	conn->getLoop()->assertInLoopThread();
	while (buf->readableBytes() >= packet::kMinPacketSZ) {
		const uint16_t len = buf->peekInt16(true);
		if (unlikely(len > packet::kMaxPacketSZ ||
					 len < packet::kPrevHeaderLen + packet::kHeaderLen)) {
			ASSERT_V(false, "len:%d bufsize:%d", len, buf->readableBytes());
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
		else if (likely(len <= buf->readableBytes() &&
						len >= packet::kPrevHeaderLen + packet::kHeaderLen)) {
			BufferPtr buffer(new muduo::net::Buffer(len));
			buffer->append(buf->peek(), static_cast<size_t>(len));
			buf->retrieve(len);
			packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buffer);
			ASSERT(packet::checkCheckSum(pre_header));
			packet::header_t const* header = packet::get_header(buffer);
			uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
			ASSERT(header->crc == crc);
			std::string session((char const*)pre_header->session, packet::kSessionSZ);
			ASSERT(!session.empty() && session.size() == packet::kSessionSZ);
			//session -> conn -> entryContext -> index
			muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
			if (peer) {
				Context& entryContext = boost::any_cast<Context&>(peer->getContext());
				entryContext.getWorker()->run(
					std::bind(
						&GateServ::asyncHallHandler,
						this,
						conn, peer, buffer, receiveTime));
			}
			else {
				//Errorf("error");
				//break;
			}
		}
		//数据包不足够解析，等待下次接收再解析
		else /*if (unlikely(len > buf->readableBytes()))*/ {
			Errorf("len:%d bufsize:%d", len, buf->readableBytes());
			break;
		}
	}
}

void GateServ::asyncHallHandler(
	muduo::net::WeakTcpConnectionPtr const& weakHallConn,
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	muduo::net::TcpConnectionPtr conn(weakHallConn.lock());
	if (!conn) {
		Errorf("error");
		return;
	}
	packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
	packet::header_t const* header = packet::get_header(buf);
	std::string session((char const*)pre_header->session, packet::kSessionSZ);
	ASSERT(!session.empty() && session.size() == packet::kSessionSZ);
	//session -> conn
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		Context& entryContext = boost::any_cast<Context&>(peer->getContext());
		int64_t userId = pre_header->userId;
		ASSERT(session == entryContext.getSession());
		TraceMessageId(header->mainId, header->subId);
		if (
			//////////////////////////////////////////////////////////////////////////
			//登陆成功，指定用户大厅节点
			//////////////////////////////////////////////////////////////////////////
			header->mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
			header->subId == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES &&
			pre_header->ok == 1) {
			ASSERT(userId > 0 && 0 == entryContext.getUserId());
			//指定userId
			entryContext.setUserId(userId);
			//指定大厅节点
			std::vector<std::string> vec;
			boost::algorithm::split(vec, conn->name(), boost::is_any_of(":"));
			std::string const& name = vec[0] + ":" + vec[1];
			ClientConn clientConn(name, conn);
			Warnf("%d 登陆成功，大厅节点 >>> %s", userId, name.c_str());
			entryContext.setClientConn(containTy::kHallTy, clientConn);
			//顶号处理 userid -> conn -> entryContext -> session
			muduo::net::TcpConnectionPtr old(sessions_.add(userId, peer).lock());
			if (old && old != peer && old.get() != peer.get()) {
				Context& entryContext_ = boost::any_cast<Context&>(old->getContext());
				std::string const& session_ = entryContext_.getSession();
				ASSERT(session_.size() == packet::kSessionSZ);
				if (session_ != session) {
					BufferPtr buffer = packClientShutdownMsg(userId, 0); ASSERT(buffer);
					muduo::net::websocket::send(old, buffer->peek(), buffer->readableBytes());
					entryContext_.setkicking(KICK_REPLACE);
#if 0
					old->getLoop()->runAfter(0.2f, [&]() {
						entry_.reset();
						});
#elif 1
					old->forceClose();
#else
					old->forceCloseWithDelay(0.2f);
#endif
					Errorf("KICK_REPLACE %s => %s", session_.c_str(), session.c_str());
				}
			}
		}
		else if (
			//////////////////////////////////////////////////////////////////////////
			//查询成功，指定用户游戏节点
			//////////////////////////////////////////////////////////////////////////
			header->mainId == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL && (
			header->subId == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES ||
			header->subId == ::Game::Common::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_RES) &&
			pre_header->ok == 1) {
			ASSERT(userId > 0 && userId == entryContext.getUserId());
			//判断用户当前游戏节点
			ClientConn const& clientConn = entryContext.getClientConn(containTy::kGameTy);
			muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
			if (gameConn) {
				//用户当前游戏节点正常，判断是否一致
				std::string serverIp;
				if (REDISCLIENT.GetOnlineInfoIP(userId, serverIp)) {
					//与目标游戏节点不一致，重新指定
					if (clientConn.first != serverIp) {
						Warnf("%d 游戏节点[%s] [%s]不一致，重新指定", userId, clientConn.first.c_str(), serverIp.c_str());
						//获取目标游戏节点
						ClientConn clientConn;
						clients_[containTy::kGameTy].clients_->get(serverIp, clientConn);
						muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
						if (gameConn) {
							//更新用户游戏节点
							entryContext.setClientConn(containTy::kGameTy, clientConn);
							Infof("%d 游戏节点[%s] 更新成功", userId, serverIp.c_str());
						}
						else {
							//目标游戏节点不可用，要求zk实时监控
							Errorf("%d 游戏节点[%s]不可用，更新失败!", userId, serverIp.c_str());
						}
					}
				}
				else {
					if (clientConn.first.empty()) {
						Errorf("%d GetOnlineInfoIP ERR 无法定位游戏节点!", userId);
					}
					else {
						Errorf("%d GetOnlineInfoIP ERR 无法定位游戏节点[%s]!", userId, clientConn.first.c_str());
					}
				}
			}
			else {
				//用户当前游戏节点不存在/不可用，需要指定
				if (clientConn.first.empty()) {
					//Warnf("%d 游戏节点不存在，需要指定", userId);
				}
				else {
					//Errorf("%d 游戏节点[%s]不可用，需要指定", userId, clientConn.first.c_str());
				}
				std::string serverIp;
				if (REDISCLIENT.GetOnlineInfoIP(userId, serverIp)) {
					//获取目标游戏节点
					ClientConn clientConn;
					clients_[containTy::kGameTy].clients_->get(serverIp, clientConn);
					muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
					if (gameConn) {
						//指定用户游戏节点
						entryContext.setClientConn(containTy::kGameTy, clientConn);
						Infof("%d 游戏节点[%s]，指定成功!", userId, serverIp.c_str());
					}
					else {
						//目标游戏节点不可用，要求zk实时监控
						Infof("%d 游戏节点[%s]不可用，指定失败!", userId, serverIp.c_str());
					}
				}
				else {
					if (clientConn.first.empty()) {
						Errorf("%d GetOnlineInfoIP ERR 无法定位游戏节点!", userId);
					}
					else {
						Errorf("%d GetOnlineInfoIP ERR 无法定位游戏节点[%s]!", userId, clientConn.first.c_str());
					}
				}
			}
		}
		muduo::net::websocket::send(peer, (uint8_t const*)header, header->len);
	}
	else {
		Errorf("error");
	}
}

//跨网关顶号处理(异地登陆)
void GateServ::onUserLoginNotify(std::string const& msg) {
	//Warnf(msg.c_str());
	try {
		std::stringstream ss(msg);
		boost::property_tree::ptree root;
		boost::property_tree::read_json(ss, root);
		int64_t userid = root.get<int64_t>("userid");
		std::string const gateip = root.get<std::string>("gateip");
		std::string const session = root.get<std::string>("session");
		if (gateip == nodeValue_) {
			return;
		}
		muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
		if (!peer) {
			//顶号处理 userid -> conn -> entryContext -> session
			muduo::net::TcpConnectionPtr old(sessions_.get(userid).lock());
			if (old) {
				Context& entryContext_ = boost::any_cast<Context&>(old->getContext());
				ASSERT(entryContext_.getUserId() == userid);
				std::string const& session_ = entryContext_.getSession();
				ASSERT(session_.size() == packet::kSessionSZ);
				//相同userid，不同session，非当前最新，则关闭之
				if (session_ != session) {
					BufferPtr buffer = packClientShutdownMsg(userid, 0); ASSERT(buffer);
					muduo::net::websocket::send(old, buffer->peek(), buffer->readableBytes());
					entryContext_.setkicking(KICK_REPLACE);
#if 0
					old->getLoop()->runAfter(0.2f, [&]() {
						entry_.reset();
						});
#elif 1
					old->forceClose();
#else
					old->forceCloseWithDelay(0.2f);
#endif
					Debugf("KICK_REPLACE %lld\ngateip %s => %s\nsession %s => %s",
						userid,
						nodeValue_.c_str(), gateip.c_str(),
						session_.c_str(), session.c_str());
				}
			}
		}
	}
	catch (boost::property_tree::ptree_error& e) {
		Errorf(e.what());
	}
}

void GateServ::sendHallMessage(
	Context& entryContext,
	BufferPtr const& buf, int64_t userId) {
	//Infof("...");
	ClientConn const& clientConn = entryContext.getClientConn(containTy::kHallTy);
	muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
	if (hallConn) {
		ASSERT(hallConn->connected());
		ASSERT(entryContext.getUserId() > 0);
		//判断节点是否维护中
		if (!clients_[containTy::kHallTy].isRepairing(clientConn.first)) {
#if !defined(NDEBUG)
#if 0
			ASSERT(
				std::find(
					std::begin(clients_[containTy::kHallTy].clients_),
					std::end(clients_[containTy::kHallTy].clients_),
					clientConn.first) != clients_[containTy::kHallTy].clients_.end());
#endif
			clients_[containTy::kHallTy].clients_->check(clientConn.first, true);
#endif
			if (buf) {
				//Debugf("len = %d\n", buf->readableBytes());
				hallConn->send(buf.get());
			}
		}
		else {
			//Warnf("用户大厅服维护，重新分配");
			//用户大厅服维护，重新分配
			ClientConnList clients;
			//异步获取全部有效大厅连接
			clients_[containTy::kHallTy].clients_->getAll(clients);
			if (clients.size() > 0) {
				bool ok = false;
				std::map<std::string, bool> repairs;
				do {
					int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
					ASSERT(index >= 0 && index < clients.size());
					ClientConn const& clientConn = clients[index];
					muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
					if (hallConn) {
						//判断节点是否维护中
						if (ok = !clients_[containTy::kHallTy].isRepairing(clientConn.first)) {
							//账号已经登陆，但登陆大厅维护中，重新指定账号登陆大厅
							entryContext.setClientConn(containTy::kHallTy, clientConn);
							if (buf) {
								//Debugf("len = %d\n", buf->readableBytes());
								hallConn->send(buf.get());
							}
						}
						else {
							repairs[clientConn.first] = true;
						}
					}
					else {
						packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
						packet::header_t const* header = packet::get_header(buf);
						Errorf("%s error", fmtMessageId(header->mainId, header->subId).c_str());
						EntryPtr entry(entryContext.getWeakEntryPtr().lock());
						if (entry) {
							muduo::net::TcpConnectionPtr peer(entry->getWeakConnPtr().lock());
							if (peer) {
								BufferPtr buffer = GateServ::packNotifyFailedMsg(header->mainId, header->subId);
								muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
							}
						}
					}
				} while (!ok && repairs.size() != clients.size());
			}
			else {
				packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
				packet::header_t const* header = packet::get_header(buf);
				Errorf("%s error", fmtMessageId(header->mainId, header->subId).c_str());
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
	else {
		//Warnf("用户大厅服失效，重新分配");
		//用户大厅服失效，重新分配
		ClientConnList clients;
		//异步获取全部有效大厅连接
		clients_[containTy::kHallTy].clients_->getAll(clients);
		if (clients.size() > 0) {
			int retry_c = 0;
			bool ok = false;
			std::map<std::string, bool> repairs;
			do {
				int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
				ASSERT(index >= 0 && index < clients.size());
				ClientConn const& clientConn = clients[index];
				muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
				if (hallConn) {
					ASSERT(hallConn->connected());
					//判断节点是否维护中
					if (ok = !clients_[containTy::kHallTy].isRepairing(clientConn.first)) {
						if (entryContext.getUserId() > 0) {
							//账号已经登陆，但登陆大厅失效了，重新指定账号登陆大厅
							entryContext.setClientConn(containTy::kHallTy, clientConn);
						}
						if (buf) {
							//Debugf("len = %d\n", buf->readableBytes());
							hallConn->send(buf.get());
						}
					}
					else {
						repairs[clientConn.first] = true;
					}
				}
				else {
					packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
					packet::header_t const* header = packet::get_header(buf);
					Errorf("%s error", fmtMessageId(header->mainId, header->subId).c_str());
					EntryPtr entry(entryContext.getWeakEntryPtr().lock());
					if (entry) {
						muduo::net::TcpConnectionPtr peer(entry->getWeakConnPtr().lock());
						if (peer) {
							BufferPtr buffer = GateServ::packNotifyFailedMsg(header->mainId, header->subId);
							muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
						}
					}
				}
			} while (!ok && repairs.size() != clients.size() && ++retry_c < 3);
		}
		else {
			packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
			packet::header_t const* header = packet::get_header(buf);
			Errorf("%s error", fmtMessageId(header->mainId, header->subId).c_str());
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

void GateServ::onUserOfflineHall(Context& entryContext) {
	MY_TRY()
	int64_t userId = entryContext.getUserId();
	uint32_t clientip = entryContext.getFromIp();
	std::string const& session = entryContext.getSession();
	std::string const& aeskey = entryContext.getAesKey();
	if (userId > 0 && !session.empty()) {
		BufferPtr buffer = packet::packMessage(
			userId,
			session,
			aeskey,
			clientip,
			entryContext.getKicking(),
			nodeValue_,
			::Game::Common::MAIN_MESSAGE_PROXY_TO_HALL,
			::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE,
			NULL);
		if (buffer) {
			TraceMessageId(
				::Game::Common::MAIN_MESSAGE_PROXY_TO_HALL,
				::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE);
			ASSERT(buffer->readableBytes() < packet::kMaxPacketSZ);
			sendHallMessage(entryContext, buffer, userId);
		}
	}
	MY_CATCH()
}