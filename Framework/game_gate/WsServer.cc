#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"


#include "public/codec/aes.h"
#include "public/codec/mymd5.h"
#include "public/codec/base64.h"
#include "public/codec/htmlcodec.h"
#include "public/codec/urlcodec.h"

#include "Gateway.h"

bool GateServ::onCondition(const muduo::net::InetAddress& peerAddr) {
	return true;
}

void GateServ::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
		numTotalReq_.incrementAndGet();
		if (num > kMaxConnections_) {
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
			std::bind(&GateServ::onConnected, this,
				std::placeholders::_1, std::placeholders::_2),
			std::bind(&GateServ::onMessage, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4),
			conn);

		EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
		assert(context);
		
		EntryPtr entry(new Entry(Entry::TypeE::TcpTy, muduo::net::WeakTcpConnectionPtr(conn), "客户端", "网关服"));

		ContextPtr entryContext(new Context(WeakEntryPtr(entry)));
		conn->setContext(entryContext);
		{
			//给新conn绑定一个worker线程，与之相关所有逻辑业务都在该worker线程中处理
			//int index = context->allocWorkerIndex();
			//assert(index >= 0 && index < threadPool_.size());
			//entryContext->setWorkerIndex(index);
		}
		{
			int index = context->getBucketIndex();
			assert(index >= 0 && index < bucketsPool_.size());
			RunInLoop(conn->getLoop(),
				std::bind(&ConnBucket::pushBucket, bucketsPool_[index].get(), entry));
		}
		{
			conn->setTcpNoDelay(true);
		}
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
		assert(!conn->getContext().empty());
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::dtor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::reset(conn);
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
#if !defined(MAP_USERID_SESSION) && 0
		//userid
		int64_t userid = entryContext->getUserID();
		if (userid > 0) {		
			//check before remove
			sessions_.remove(userid, conn);
		}
#endif
		int index = entryContext->getWorkerIndex();
		assert(index >= 0 && index < threadPool_.size());
		threadPool_[index]->run(
			std::bind(
				&GateServ::asyncOfflineHandler,
				this, entryContext));
	}
}

void GateServ::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {

	conn->getLoop()->assertInLoopThread();

	LOG_INFO << __FUNCTION__ << " --- *** " << "客户端真实IP[" << ipaddr << "]";

	assert(!conn->getContext().empty());
	ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
	assert(entryContext);
	{
		muduo::net::InetAddress address(ipaddr, 0);
		entryContext->setFromIp(address.ipNetEndian());
	}
	std::string uuid = createUUID();
	std::string session = buffer2HexStr((unsigned char const*)uuid.data(), uuid.length());
	{
		//优化前，conn->name()断线重连->session变更->重新登陆->异地登陆通知
		//优化后，conn->name()断线重连->session过期检查->登陆校验->异地登陆判断
		entryContext->setSession(session);
	}
	{
		//////////////////////////////////////////////////////////////////////////
		//session -> hash(session) -> index
		//////////////////////////////////////////////////////////////////////////
		int index = hash_session_(session) % threadPool_.size();
		entryContext->setWorkerIndex(index);
	}
	{
		//////////////////////////////////////////////////////////////////////////
		//map[session] = weakConn
		//////////////////////////////////////////////////////////////////////////
		entities_.add(session, muduo::net::WeakTcpConnectionPtr(conn));
		LOG_WARN << __FUNCTION__ << " session[ " << session << " ]";
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
	const uint16_t len = buf->peekInt16();
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
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		EntryPtr entry(entryContext->getWeakEntryPtr().lock());
		if (entry) {
			{
				EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
				assert(context);
				
				int index = context->getBucketIndex();
				assert(index >= 0 && index < bucketsPool_.size());

				RunInLoop(conn->getLoop(),
					std::bind(&ConnBucket::updateBucket, bucketsPool_[index].get(), entry));
			}
			{
#if 0
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->swap(*buf);
#else
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->append(buf->peek(), static_cast<size_t>(buf->readableBytes()));
				buf->retrieve(buf->readableBytes());
#endif
				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());
				threadPool_[index]->run(
					std::bind(
						&GateServ::asyncClientHandler,
						this, entryContext->getWeakEntryPtr(), buffer, receiveTime));
			}
		}
		else {
			numTotalBadReq_.incrementAndGet();
			LOG_ERROR << __FUNCTION__ << " --- *** " << "entry invalid";
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

void GateServ::asyncClientHandler(
	WeakEntryPtr const& weakEntry,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
	//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
	//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
	//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
	//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
	//
	//锁定同步业务操作，先锁超时对象entry，再锁conn，避免超时和业务同时处理的情况
	EntryPtr entry(weakEntry.lock());
	if (entry) {
		//entry->setLocked();
		muduo::net::TcpConnectionPtr conn(entry->getWeakConnPtr().lock());
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
						TraceMessageID(header->mainId, header->subId);
						int cmd = packet::enword(header->mainId, header->subId);
						CmdCallbacks::const_iterator it = handlers_.find(cmd);
						if (it != handlers_.end()) {
							CmdCallback const& handler = it->second;
							handler(conn, buf);
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
					TraceMessageID(header->mainId, header->subId);
					{
						ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
						assert(entryContext);
						int64_t userId = entryContext->getUserID();
						uint32_t clientIp = entryContext->getFromIp();
						std::string const& session = entryContext->getSession();
						std::string const& aesKey = entryContext->getAesKey();
						ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
						muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
						assert(header->len == len);
						assert(header->len >= packet::kHeaderLen);
#if 0
						//////////////////////////////////////////////////////////////////////////
						//玩家登陆网关服信息
						//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
						//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
						//网关服ID格式：session|ip:port:port:pid
						//第一个ip:port是网关服监听客户端的标识
						//第二个ip:port是网关服监听订单服的标识
						//pid标识网关服进程id
						//////////////////////////////////////////////////////////////////////////
						//网关服servid session|ip:port:port:pid
						std::string const& servId = nodeValue_;
#endif
						//非登录消息 userid > 0
						if (header->subId != ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ &&
							userId == 0) {
							LOG_ERROR << __FUNCTION__ << " user Must Login Hall Server First!";
							break;
						}
						BufferPtr buffer = packet::packMessage(
							userId,
							session,
							aesKey,
							clientIp,
							0,
#if 0
							servId,
#endif
							buf->peek(),
							header->len);
						if (buffer) {
							sendHallMessage(*entryContext.get(), buffer, userId);
						}
					}
					break;
				}
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
					TraceMessageID(header->mainId, header->subId);
					{
						ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
						assert(entryContext);
						int64_t userId = entryContext->getUserID();
						uint32_t clientIp = entryContext->getFromIp();
						std::string const& session = entryContext->getSession();
						std::string const& aesKey = entryContext->getAesKey();
						ClientConn const& clientConn = entryContext->getClientConn(servTyE::kGameTy);
						muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
						assert(header->len == len);
						assert(header->len >= packet::kHeaderLen);
#if 0
						//////////////////////////////////////////////////////////////////////////
						//玩家登陆网关服信息
						//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
						//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
						//网关服ID格式：session|ip:port:port:pid
						//第一个ip:port是网关服监听客户端的标识
						//第二个ip:port是网关服监听订单服的标识
						//pid标识网关服进程id
						//////////////////////////////////////////////////////////////////////////
						//网关服servid session|ip:port:port:pid
						std::string const& servId = nodeValue_;
#endif
						if (userId == 0) {
							LOG_ERROR << __FUNCTION__ << " user Must Login Hall Server First!";
							break;
						}
						BufferPtr buffer = packet::packMessage(
							userId,
							session,
							aesKey,
							clientIp,
							0,
#if 0
							servId,
#endif
							buf->peek(),
							header->len);
						if (buffer) {
							sendGameMessage(*entryContext.get(), buffer, userId);
						}
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
			LOG_ERROR << __FUNCTION__ << " --- *** " << "TcpConnectionPtr.conn invalid";
		}
		//entry->setLocked(false);
	}
	else {
		numTotalBadReq_.incrementAndGet();
		LOG_ERROR << __FUNCTION__ << " --- *** " << "entry invalid";
	}
}

void GateServ::asyncOfflineHandler(ContextPtr /*const*/& entryContext) {
	if (entryContext) {
		LOG_ERROR << __FUNCTION__;
		std::string const& session = entryContext->getSession();
		if (!session.empty()) {
			entities_.remove(session);
		}
		int64_t userid = entryContext->getUserID();
		if (userid > 0) {
			sessions_.remove(userid, session);
		}
		onUserOfflineHall(*entryContext.get());
		onUserOfflineGame(*entryContext.get());
	}
}

BufferPtr GateServ::packClientShutdownMsg(int64_t userid, int status) {
	::Game::Common::ProxyNotifyShutDownUserClientMessage msg;
	msg.mutable_header()->set_sign(PROTO_BUF_SIGN);
	msg.set_userid(userid);
	msg.set_status(status);
	
	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY, &msg);
	
	TraceMessageID(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY);
	
	return buffer;
}

BufferPtr GateServ::packNoticeMsg(
	int32_t agentid, std::string const& title,
	std::string const& content, int msgtype) {
	::ProxyServer::Message::NotifyNoticeMessageFromProxyServerMessage msg;
	msg.mutable_header()->set_sign(PROTO_BUF_SIGN);
	msg.add_agentid(agentid);
	msg.set_title(title.c_str());
	msg.set_message(content);
	msg.set_msgtype(msgtype);
	
	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY, &msg);
	
	TraceMessageID(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
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
		TraceMessageID(mainId, subId);
		entities_.broadcast(buffer);
	}
}

void GateServ::refreshBlackList() {
	if (blackListControl_ == IpVisitCtrlE::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		RunInLoop(server_.getLoop(), std::bind(&GateServ::refreshBlackListInLoop, this));
	}
	else if (blackListControl_ == IpVisitCtrlE::kOpen) {
		//同步刷新IP访问黑名单
		refreshBlackListSync();
	}
}

bool GateServ::refreshBlackListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == IpVisitCtrlE::kOpen);
	{
		WRITE_LOCK(blackList_mutex_);
		blackList_.clear();
	}
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = blackList_.begin();
		it != blackList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问黑名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
	return false;
}

bool GateServ::refreshBlackListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == IpVisitCtrlE::kOpenAccept);
	server_.getLoop()->assertInLoopThread();
	blackList_.clear();
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = blackList_.begin();
		it != blackList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问黑名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
	return false;
}

void GateServ::cmd_getAesKey(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::header_t const* header_ = (packet::header_t const*)buf->peek();
}