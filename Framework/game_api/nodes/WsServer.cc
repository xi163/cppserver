#include "proto/Game.Common.pb.h"

#include "../Api.h"

bool ApiServ::onCondition(const muduo::net::InetAddress& peerAddr) {
	return true;
}

void ApiServ::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_[KWebsocketTy].incrementAndGet();
		_LOG_INFO("API服[%s] <- 客户端[%s] %s %d",
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
			assert(conn->getContext().empty());

			numTotalBadReq_.incrementAndGet();
			return;
		}
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::ctor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::hook(
			std::bind(&ApiServ::onConnected, this,
				std::placeholders::_1, std::placeholders::_2),
			std::bind(&ApiServ::onMessage, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4),
			conn, path_handshake_);
		EntryPtr entry(new Entry(Entry::TypeE::TcpTy, conn, "客户端", "登陆服"));
		RunInLoop(conn->getLoop(),
			std::bind(&Buckets::push, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
		conn->setContext(Context(entry));
		conn->setTcpNoDelay(true);
	}
	else {
		int32_t num = numConnected_[KWebsocketTy].decrementAndGet();
		_LOG_INFO("API服[%s] <- 客户端[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		assert(!conn->getContext().empty());
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::dtor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::reset(conn);
		Context& entryContext = boost::any_cast<Context&>(conn->getContext());
		if (entryContext.getWorker()) {
			entryContext.getWorker()->run(
				std::bind(
					&ApiServ::asyncOfflineHandler,
					this, entryContext));
		}
	}
}

void ApiServ::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {

	conn->getLoop()->assertInLoopThread();

	_LOG_INFO("客户端真实IP[%s]", ipaddr.c_str());

	assert(!conn->getContext().empty());
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
		_LOG_INFO("session[%s]", session.c_str());
	}
	else {
		_LOG_ERROR("error");
	}
}

void ApiServ::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, int msgType,
	muduo::Timestamp receiveTime) {
	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		return;
	}
	conn->getLoop()->assertInLoopThread();
	const uint16_t len = buf->peekInt16(true);
	if (likely(len > packet::kMaxPacketSZ ||
		len < packet::kHeaderLen)) {
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
	else if (likely(len <= buf->readableBytes())) {
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
			entryContext.getWorker()->run(
				std::bind(
					&ApiServ::asyncClientHandler,
					this, conn, buffer, receiveTime));
		}
		else {
			numTotalBadReq_.incrementAndGet();
			_LOG_ERROR("entry invalid");
		}
	}
	//数据包不足够解析，等待下次接收再解析
	else {
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
}

void ApiServ::asyncClientHandler(
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
			assert(header->crc == crc);
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
							_LOG_ERROR("unregister handler %d:%d", header->mainId, header->subId);
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
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL: {
					TraceMessageId(header->mainId, header->subId);
					Context& entryContext = boost::any_cast<Context&>(conn->getContext());
					int64_t userId = entryContext.getUserId();
					uint32_t clientIp = entryContext.getFromIp();
					std::string const& session = entryContext.getSession();
					std::string const& aesKey = entryContext.getAesKey();
					ClientConn const& clientConn = entryContext.getClientConn(containTy::kHallTy);
					muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
					assert(header->len == len);
					assert(header->len >= packet::kHeaderLen);
					//非登录消息 userid > 0
					if (header->subId != ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ &&
						userId == 0) {
						_LOG_ERROR("user Must Login Hall Server First!");
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
					}
					break;
				}
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
					assert(header->len == len);
					assert(header->len >= packet::kHeaderLen);
					if (userId == 0) {
						_LOG_ERROR("user Must Login Hall Server First!");
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
			_LOG_ERROR("TcpConnectionPtr.conn invalid");
		}
}

void ApiServ::asyncOfflineHandler(Context& entryContext) {
	std::string const& session = entryContext.getSession();
	if (!session.empty()) {
		entities_.remove(session);
	}
	int64_t userid = entryContext.getUserId();
	if (userid > 0) {
		sessions_.remove(userid, session);
	}
}

void ApiServ::refreshBlackList() {
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		RunInLoop(server_.getLoop(), std::bind(&ApiServ::refreshBlackListInLoop, this));
	}
	else if (blackListControl_ == eApiCtrl::kOpen) {
		//同步刷新IP访问黑名单
		refreshBlackListSync();
	}
}

bool ApiServ::refreshBlackListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == eApiCtrl::kOpen);
	{
		WRITE_LOCK(black_list_mutex_);
		black_list_.clear();
	}
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = black_list_.begin();
		it != black_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	_LOG_DEBUG("IP访问黑名单\n%s", s.c_str());
	return false;
}

bool ApiServ::refreshBlackListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == eApiCtrl::kOpenAccept);
	server_.getLoop()->assertInLoopThread();
	black_list_.clear();
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = black_list_.begin();
		it != black_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	_LOG_DEBUG("IP访问黑名单\n%s", s.c_str());
	return false;
}

// void ApiServ::cmd_getAesKey(
// 	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
// 	packet::header_t const* header_ = (packet::header_t const*)buf->peek();
// }