#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "../Gate.h"

bool GateServ::onCondition(const muduo::net::InetAddress& peerAddr, muduo::net::InetRegion& peerRegion) {
 	std::string ipaddr = peerAddr.toIp();
	//dead loop bug???
 	ipLocator_.GetAddressByIp(ipaddr.c_str(), peerRegion.location, peerRegion.country);
	//country
	for (std::vector<std::string>::const_iterator it = country_list_.begin();
		it != country_list_.end(); ++it) {
		if (peerRegion.country.find(*it) != std::string::npos) {
			//location
			for (std::vector<std::string>::const_iterator ir = location_list_.begin();
				ir != location_list_.end(); ++ir) {
				if (peerRegion.location.find(*ir) != std::string::npos) {
					Infof("%s %s %s [√]通过", ipaddr.c_str(), peerRegion.country.c_str(), peerRegion.location.c_str());
					return true;
				}
			}
			break;
		}
	}
	Infof("%s %s %s [×]禁止访问", ipaddr.c_str(), peerRegion.country.c_str(), peerRegion.location.c_str());
	return false;
}

void GateServ::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_[KWebsocketTy].incrementAndGet();
		Infof("网关服[%s] <- 客户端[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		numTotalReq_.incrementAndGet();
		if (num > maxConnections_) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 1
			conn->forceClose();
#else
			conn->forceCloseWithDelay(0.2f);
#endif
			//会调用onMessage函数
			ASSERT(conn->getContext().empty());

			numTotalBadReq_.incrementAndGet();
			return;
		}
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::ctor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::hook(
			std::bind(&GateServ::onVerify, this,
				std::placeholders::_1),
			std::bind(&GateServ::onConnected, this,
				std::placeholders::_1, std::placeholders::_2),
			std::bind(&GateServ::onMessage, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4), conn, path_handshake_);
		EntryPtr entry(new Entry(Entry::TypeE::TcpTy, conn, "客户端", "网关服"));
		RunInLoop(conn->getLoop(),
			std::bind(&Buckets::push, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
		conn->setContext(Context(entry));
		conn->setTcpNoDelay(true);
	}
	else {
		int32_t num = numConnected_[KWebsocketTy].decrementAndGet();
		Infof("网关服[%s] <- 客户端[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		ASSERT(!conn->getContext().empty());
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::dtor
		//////////////////////////////////////////////////////////////////////////
		//muduo::net::websocket::reset(conn);
		Context& entryContext = boost::any_cast<Context&>(conn->getContext());
		if (entryContext.getWorker()) {
			entryContext.getWorker()->run(
				std::bind(
					&GateServ::asyncOfflineHandler,
					this, entryContext));
		}
	}
}

bool GateServ::onVerify(muduo::net::http::IRequest const* request) {
	switch (verify_) {
	case true: {
		std::string token = request->getHeader("Sec-WebSocket-Verify", true);
		Debugf("%s", token.c_str());
		if (token.empty()) {
			return false;
		}
		break;
	}
	}
	return true;
}

void GateServ::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {
	
	conn->getLoop()->assertInLoopThread();
	
	ASSERT(!conn->getContext().empty());
	Context& entryContext = boost::any_cast<Context&>(conn->getContext());
	EntryPtr entry(entryContext.getWeakEntryPtr().lock());
	if (entry) {
		muduo::net::InetAddress address(ipaddr, 0);
		entryContext.setFromIp(address.ipv4NetEndian());
		std::string uuid = utils::uuid::createUUID();
		std::string session = utils::buffer2HexStr((unsigned char const*)uuid.data(), uuid.length());
		entryContext.setSession(session);
		//session -> hash(session) -> index
		entryContext.setWorker(session, hash_session_, threadPool_);
		//map[session] = weakConn
		entities_.add(session, conn);
		Infof("%s %s %s session[%s]", ipaddr.c_str(), conn->peerRegion().country.c_str(), conn->peerRegion().location.c_str(), session.c_str());
	}
	else {
		Errorf("%s %s %s", ipaddr.c_str(), conn->peerRegion().country.c_str(), conn->peerRegion().location.c_str());
	}
}

void GateServ::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, int msgType,
	muduo::Timestamp receiveTime) {
	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		return;
	}
	conn->getLoop()->assertInLoopThread();
	switch (msgType) {
	case muduo::net::websocket::MessageT::TyTextMessage: {
		Debugf("%.*s", buf->readableBytes(), buf->peek());
		std::string ipaddr = conn->peerAddress().toIp();
		if (conn->peerRegion().location.find("相同") != std::string::npos ||
			conn->peerRegion().location.find("同一") != std::string::npos ||
			conn->peerRegion().location.find("局域") != std::string::npos ||
			conn->peerRegion().location.find("湖南") != std::string::npos) {
			std::string s = utils::sprintf("您的IP %s %s %s, websocket TextMessage:%.*s",
				ipaddr.c_str(), conn->peerRegion().country.c_str(), conn->peerRegion().location.c_str(), buf->readableBytes(), buf->peek());
			muduo::net::websocket::send(conn, s.data(), s.length(), muduo::net::websocket::MessageT::TyTextMessage);
		}
		else {
			std::string s = utils::sprintf("来自 %s %s %s 的老6, 干啥呢, 想踩缝纫机了?!",
				ipaddr.c_str(), conn->peerRegion().country.c_str(), conn->peerRegion().location.c_str());
			muduo::net::websocket::send(conn, s.data(), s.length(), muduo::net::websocket::MessageT::TyTextMessage);
		}
		break;
	}
	case muduo::net::websocket::MessageT::TyBinaryMessage: {
		const uint16_t len = buf->peekInt16(true);
		if (unlikely(len > packet::kMaxPacketSZ ||
					 len < packet::kHeaderLen)) {
			//ASSERT_V(false, "len:%d bufsize:%d", len, buf->readableBytes());
			Errorf("len:%d bufsize:%d", len, buf->readableBytes());
			if (conn) {
#if 0
				//不再发送数据
				conn->shutdown();
#else
				conn->forceClose();
#endif
			}
			numTotalBadReq_.incrementAndGet();
		}
		else if (likely(len <= buf->readableBytes() &&
						len >= packet::kHeaderLen)) {
			Context& entryContext = boost::any_cast<Context&>(conn->getContext());
			EntryPtr entry(entryContext.getWeakEntryPtr().lock());
			if (entry) {
				RunInLoop(conn->getLoop(),
					std::bind(&Buckets::update, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
#if 0
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->swap(*buf);
#else
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->append(buf->peek(), static_cast<size_t>(buf->readableBytes()));
				buf->retrieve(buf->readableBytes());
#endif
				//std::string s = utils::buffer2HexStr((unsigned char const*)buffer->peek(), buffer->readableBytes());
				//Tracef("len:%d bufsize:%d\n%s\n\n", len, buffer->readableBytes(), s.c_str());
				entryContext.getWorker()->run(
					std::bind(
						&GateServ::asyncClientHandler,
						this, conn, buffer, receiveTime));
			}
			else {
				numTotalBadReq_.incrementAndGet();
				Errorf("entry invalid");
			}
		}
		//数据包不足够解析，等待下次接收再解析
		else /*if (unlikely(len > buf->readableBytes()))*/ {
			Errorf("len:%d bufsize:%d", len, buf->readableBytes());
			if (conn) {
#if 0
				//不再发送数据
				conn->shutdown();
#else
				conn->forceClose();
#endif
			}
			numTotalBadReq_.incrementAndGet();
		}
		return;
	}
	case muduo::net::websocket::MessageT::TyCloseMessage: {
		Debugf("%.*s", buf->readableBytes(), buf->peek());
		break;
	}
	case muduo::net::websocket::MessageT::TyPingMessage: {
		Debugf("%.*s", buf->readableBytes(), buf->peek());
		break;
	}
	}
	if (conn) {
#if 0
		//不再发送数据
		conn->shutdown();
#else
		conn->forceClose();
#endif
	}
}

void GateServ::asyncClientHandler(
	const muduo::net::WeakTcpConnectionPtr& weakConn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
	//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
	//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
	//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
	//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
	//
	//锁定同步业务操作，先锁超时对象entry，再锁conn，避免超时和业务同时处理的情况
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (conn) {
		if (buf->readableBytes() < packet::kHeaderLen) {
			numTotalBadReq_.incrementAndGet();
			return;
		}
		packet::header_t* header = (packet::header_t*)buf->peek();
		uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		//ASSERT(header->crc == crc);
		switch (header->crc == crc) {
		case true:
			break;
		default:
			Errorf("getCheckSum error");
#if 0
			//不再发送数据
			conn->shutdown();
#else
			conn->forceClose();
#endif
			return;
		}
		size_t len = buf->readableBytes();
		if (header->len == len &&
			header->ver == 1 &&
			header->sign == HEADER_SIGN) {
			switch (header->mainId) {
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY: {
				switch (header->enctype) {
				case packet::PUBENC_PROTOBUF_NONE: {
					TraceMessageId(header->mainId, header->subId);
					int cmd = packet::enword(header->mainId, header->subId);
					CmdCallbacks::const_iterator it = handlers_.find(cmd);
					if (it != handlers_.end()) {
						CmdCallback const& handler = it->second;
						handler(conn, buf);
					}
					else {
						Errorf("unregister handler %d:%d", header->mainId, header->subId);
					}
					break;
				}
				default: {
					numTotalBadReq_.incrementAndGet();
					break;
				}
				}
				break;
			}
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_FRIEND:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL: {
				TraceMessageId(header->mainId, header->subId);
				Context& entryContext = boost::any_cast<Context&>(conn->getContext());
				int64_t userId = entryContext.getUserId();
				uint32_t clientIp = entryContext.getFromIp();
				std::string const& session = entryContext.getSession();
				std::string const& aesKey = entryContext.getAesKey();
				ClientConn const& clientConn = entryContext.getClientConn(containTy::kHallTy);
				muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
				ASSERT(header->len == len);
				ASSERT(header->len >= packet::kHeaderLen);
				//非登录消息 userid > 0
				if (header->subId != ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ &&
					userId == 0) {
					Errorf("user Must Login Hall Server First!");
					break;
				}
				BufferPtr buffer = packet::packMessage(
					userId,
					session,
					aesKey,
					clientIp,
					0,
					nodeValue_,
					buf->peek(),
					header->len);
				if (buffer) {
					sendHallMessage(entryContext, buffer, userId);
				}
				break;
			}
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER_CLUB:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER_FRIEND:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC_FRIEND:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
				TraceMessageId(header->mainId, header->subId);
				Context& entryContext = boost::any_cast<Context&>(conn->getContext());
				int64_t userId = entryContext.getUserId();
				uint32_t clientIp = entryContext.getFromIp();
				std::string const& session = entryContext.getSession();
				std::string const& aesKey = entryContext.getAesKey();
				ClientConn const& clientConn = entryContext.getClientConn(containTy::kGameTy);
				muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
				ASSERT(header->len == len);
				ASSERT(header->len >= packet::kHeaderLen);
				if (userId == 0) {
					Errorf("user Must Login Hall Server First!");
					break;
				}
				BufferPtr buffer = packet::packMessage(
					userId,
					session,
					aesKey,
					clientIp,
					0,
					nodeValue_,
					buf->peek(),
					header->len);
				if (buffer) {
					sendGameMessage(entryContext, buffer, userId);
				}
				break;
			}
			default: {
				numTotalBadReq_.incrementAndGet();
				break;
			}
		}
		}
		else {
			numTotalBadReq_.incrementAndGet();
		}
	}
	else {
		numTotalBadReq_.incrementAndGet();
		Errorf("TcpConnectionPtr.conn invalid");
	}
}

void GateServ::asyncOfflineHandler(Context& entryContext) {
	std::string const& session = entryContext.getSession();
	if (!session.empty()) {
		entities_.remove(session);
	}
	int64_t userid = entryContext.getUserId();
	if (userid > 0) {
		sessions_.remove(userid, session);
	}
	onUserOfflineHall(entryContext);
	onUserOfflineGame(entryContext);
}

BufferPtr GateServ::packKickGameUserMsg() {
	::GameServer::MSG_S2C_UserEnterMessageResponse msg;
	msg.mutable_header()->set_sign(HEADER_SIGN);
	msg.set_retcode(0);
	msg.set_errormsg("游戏服务异常中止");

	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::GameServer::SUB_S2C_ENTER_ROOM_RES, &msg);

	TraceMessageId(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::GameServer::SUB_S2C_ENTER_ROOM_RES);

	return buffer;
}

BufferPtr GateServ::packNotifyFailedMsg(uint8_t mainId, uint8_t subId) {
	::Game::Common::ProxyNotifyFailedMessage msg;
	msg.mutable_header()->set_sign(PROTOBUF_SIGN);
	msg.set_mainid(mainId);
	msg.set_subid(subId);
	msg.set_msg("请求服务不可用");

	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_FAILED_MESSAGE, &msg);

	TraceMessageId(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_FAILED_MESSAGE);

	return buffer;
}

BufferPtr GateServ::packOrderScoreMsg(int16_t userid, int64_t score) {
	::Game::Common::ProxyNotifyOrderScoreMessage msg;
	msg.mutable_header()->set_sign(PROTOBUF_SIGN);
	msg.set_userid(userid);
	msg.set_score(score);

	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE, &msg);

	TraceMessageId(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE);
	
	return buffer;
}

BufferPtr GateServ::packClientShutdownMsg(int64_t userid, int status) {
	::Game::Common::ProxyNotifyShutDownUserClientMessage msg;
	msg.mutable_header()->set_sign(PROTOBUF_SIGN);
	msg.set_userid(userid);
	msg.set_status(status);
	msg.set_msg("异地登陆被强制踢下线");
	
	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY, &msg);

	TraceMessageId(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY);

	return buffer;
}

BufferPtr GateServ::packNoticeMsg(
	int32_t agentid, std::string const& title,
	std::string const& content, int msgtype) {
	::ProxyServer::Message::NotifyNoticeMessageFromProxyServerMessage msg;
	msg.mutable_header()->set_sign(PROTOBUF_SIGN);
	msg.add_agentid(agentid);
	msg.set_title(title.c_str());
	msg.set_message(content);
	msg.set_msgtype(msgtype);

	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY, &msg);

	TraceMessageId(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY);

	return buffer;
}

void GateServ::broadcastNoticeMsg(
	std::string const& title,
	std::string const& msg,
	int32_t agentid, int msgType) {
	BufferPtr buffer = packNoticeMsg(
		agentid,
		title,
		msg,
		msgType);
	if (buffer) {
		sessions_.broadcast(buffer);
	}
}

void GateServ::broadcastMessage(int mainId, int subId, ::google::protobuf::Message* msg) {
	BufferPtr buffer = packet::packMessage(mainId, subId, msg);
	if (buffer) {
		TraceMessageId(mainId, subId);
		entities_.broadcast(buffer);
	}
}

void GateServ::refreshBlackList() {
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		RunInLoop(server_.getLoop(), std::bind(&GateServ::refreshBlackListInLoop, this));
	}
	else if (blackListControl_ == eApiCtrl::kOpen) {
		//同步刷新IP访问黑名单
		refreshBlackListSync();
	}
}

bool GateServ::refreshBlackListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	ASSERT(blackListControl_ == eApiCtrl::kOpen);
	{
		WRITE_LOCK(black_list_mutex_);
		black_list_.clear();
	}
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = black_list_.begin();
		it != black_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	Debugf("IP访问黑名单\n%s", s.c_str());
	return false;
}

bool GateServ::refreshBlackListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	ASSERT(blackListControl_ == eApiCtrl::kOpenAccept);
	server_.getLoop()->assertInLoopThread();
	black_list_.clear();
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = black_list_.begin();
		it != black_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	Debugf("IP访问黑名单\n%s", s.c_str());
	return false;
}

void GateServ::cmd_getAesKey(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::header_t const* header_ = (packet::header_t const*)buf->peek();
}