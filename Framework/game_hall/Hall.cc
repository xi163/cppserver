
#include "Hall.h"
#include "public/mgoOperation.h"
#include "public/redisKeys.h"
#include "rpc/client/RoomNodes.h"

HallServ::HallServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrRpc)
	: server_(loop, listenAddr, "tcpServer")
	, rpcserver_(loop, listenAddrRpc, "rpcServer")
	, gameRpcClients_(loop)
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
	, ipFinder_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&HallServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&HallServ::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	rpcClients_[rpc::containTy::kRpcGameTy].clients_ = &gameRpcClients_;
	rpcClients_[rpc::containTy::kRpcGameTy].ty_ = rpc::containTy::kRpcGameTy;
	threadTimer_->startLoop();
}

HallServ::~HallServ() {
	Quit();
}

void HallServ::Quit() {
	gameRpcClients_.closeAll();
	threadTimer_->getLoop()->quit();
	for (size_t i = 0; i < threadPool_.size(); ++i) {
		threadPool_[i]->stop();
	}
	if (zkclient_) {
		zkclient_->closeServer();
	}
	if (redisClient_) {
		redisClient_->unsubscribe();
	}
	muduo::net::ReactorSingleton::stop();
	server_.getLoop()->quit();
	google::protobuf::ShutdownProtobufLibrary();
}

void HallServ::registerHandlers() {
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_REQ)]
		= std::bind(&HallServ::cmd_keep_alive_ping, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_on_user_login, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL,
		::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE)]
		= std::bind(&HallServ::cmd_on_user_offline, this,
			std::placeholders::_1, std::placeholders::_2);
	/// <summary>
	/// 查询游戏房间列表
	/// </summary>
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_REQ)]
		= std::bind(&HallServ::cmd_get_game_info, this,
			std::placeholders::_1, std::placeholders::_2);
	/// <summary>
	/// 查询正在玩的游戏
	/// </summary>
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_get_playing_game_info, this,
			std::placeholders::_1, std::placeholders::_2);
	/// <summary>
	/// 查询指定游戏节点
	/// </summary>
	handlers_[packet::enword(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_get_game_server_message, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_REQ)]
		= std::bind(&HallServ::cmd_get_room_player_nums, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_set_headid, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL << 8,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_set_nickname, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_get_userscore, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_REQ)]
		= std::bind(&HallServ::cmd_get_play_record, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_RECORD_DETAIL_REQ)]
		= std::bind(&HallServ::cmd_get_play_record_detail, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER,
		::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER)]
		= std::bind(&HallServ::cmd_repair_hallserver, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_TASK_LIST_REQ)]
		= std::bind(&HallServ::cmd_get_task_list, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_AWARDS_REQ)]
		= std::bind(&HallServ::cmd_get_task_award, this,
			std::placeholders::_1, std::placeholders::_2);
}

bool HallServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&HallServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		_LOG_FATAL("error");
		abort();
		return false;
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&HallServ::registerZookeeper, this));
	return true;
}

void HallServ::onZookeeperConnected() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/ProxyServers"))
		zkclient_->createNode("/GAME/ProxyServers", "ProxyServers"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/HallServers"))
		zkclient_->createNode("/GAME/HallServers", "HallServers"/*, true*/);
	//if (ZNONODE == zkclient_->existsNode("/GAME/HallServersInvalid"))
	//	zkclient_->createNode("/GAME/HallServersInvalid", "HallServersInvalid", true);
	if (ZNONODE == zkclient_->existsNode("/GAME/GameServers"))
		zkclient_->createNode("/GAME/GameServers", "GameServers"/*, true*/);
	//if (ZNONODE == zkclient_->existsNode("/GAME/GameServersInvalid"))
	//	zkclient_->createNode("/GAME/GameServersInvalid", "GameServersInvalid", true);
	{
		//tcp
		std::vector<std::string> vec;
		boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
		nodeValue_ = vec[0] + ":" + vec[1];
		//rpc
		boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1];
		//http
		//boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));
		//nodeValue_ += ":" + vec[1];
		nodePath_ = "/GAME/HallServers/" + nodeValue_;
		zkclient_->createNode(nodePath_, nodeValue_, true);
		invalidNodePath_ = "/GAME/HallServersInvalid/" + nodeValue_;
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/ProxyServers",
			names,
			std::bind(
				&HallServ::onGateWatcher, this,
				placeholders::_1, std::placeholders::_2,
				placeholders::_3, std::placeholders::_4,
				placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用网关服列表%s", s.c_str());
		}
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/GameServers",
			names,
			std::bind(
				&HallServ::onGameWatcher, this,
				placeholders::_1, std::placeholders::_2,
				placeholders::_3, std::placeholders::_4,
				placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用游戏服列表%s", s.c_str());
			rpcClients_[rpc::containTy::kRpcGameTy].add(names);
		}
	}
}

void HallServ::onGateWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/ProxyServers",
		names,
		std::bind(
			&HallServ::onGateWatcher, this,
			placeholders::_1, std::placeholders::_2,
			placeholders::_3, std::placeholders::_4,
			placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用网关服列表%s", s.c_str());
	}
}

void HallServ::onGameWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/GameServers",
		names,
		std::bind(
			&HallServ::onGameWatcher, this,
			placeholders::_1, std::placeholders::_2,
			placeholders::_3, std::placeholders::_4,
			placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用游戏服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcGameTy].process(names);
	}
}

void HallServ::registerZookeeper() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/HallServers"))
		zkclient_->createNode("/GAME/HallServers", "HallServers"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		_LOG_INFO(nodePath_.c_str());
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&HallServ::registerZookeeper, this));
}

bool HallServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		_LOG_FATAL("error");
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	redisClient_->subscribeRefreshConfigMessage(
		std::bind(&HallServ::on_refresh_game_config, this, std::placeholders::_1));
	redisClient_->startSubThread();
	return true;
}

bool HallServ::InitMongoDB(std::string const& url) {
	//http://mongocxx.org/mongocxx-v3/tutorial/
	_LOG_INFO("%s", url.c_str());
	mongocxx::instance instance{};
	//mongoDBUrl_ = url;
	//http://mongocxx.org/mongocxx-v3/tutorial/
	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	MongoDBClient::ThreadLocalSingleton::setUri(url);
	return true;
}

static __thread mongocxx::database* dbgamemain_;

void HallServ::threadInit() {
	if (!REDISCLIENT.initRedisCluster(redisIpaddr_, redisPasswd_)) {
		_LOG_FATAL("initRedisCluster error");
	}
	static __thread mongocxx::database db = MONGODBCLIENT["gamemain"];
	dbgamemain_ = &db;
	std::string s;
	for (std::vector<std::string>::const_iterator it = redlockVec_.begin();
		it != redlockVec_.end(); ++it) {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, *it, boost::is_any_of(":"));
		REDISLOCK.AddServerUrl(vec[0].c_str(), atol(vec[1].c_str()), redisPasswd_);
		s += "\n" + vec[0];
		s += ":" + vec[1];
	}
	//_LOG_WARN("redisLock%s", s.c_str());
}

bool HallServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessageID();
		break;
	}
	return true;
}

void HallServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();

	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&HallServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	std::vector<std::string> vec;
	boost::algorithm::split(vec, rpcserver_, boost::is_any_of(":"));

	_LOG_WARN("HallServ = %s rpc:%s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), vec[1].c_str(), numThreads, numWorkerThreads);

	server_.start(true);

	threadTimer_->getLoop()->runAfter(3.0f, std::bind(&HallServ::db_refresh_game_room_info, this));
	threadTimer_->getLoop()->runAfter(30, std::bind(&HallServ::redis_refresh_room_player_nums, this));
}

void HallServ::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		_LOG_INFO("大厅服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		_LOG_INFO("大厅服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void HallServ::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
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
			std::string session((char const*)pre_header->session, packet::kSessionSZ);
			assert(!session.empty() && session.size() == packet::kSessionSZ);
			//session -> hash(session) -> index
			int index = hash_session_(session) % threadPool_.size();
			threadPool_[index]->run(
				std::bind(
					&HallServ::asyncLogicHandler,
					this,
					muduo::net::WeakTcpConnectionPtr(conn), buffer, receiveTime));
		}
		//数据包不足够解析，等待下次接收再解析
		else {
			_LOG_ERROR("error");
			break;
		}
	}
}

void HallServ::asyncLogicHandler(
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (!conn) {
		_LOG_ERROR("error");
		return;
	}
	packet::internal_prev_header_t const* pre_header = packet::get_pre_header(buf);
	packet::header_t const* header = packet::get_header(buf);
	size_t len = buf->readableBytes();
	if (header->len == len - packet::kPrevHeaderLen &&
		header->ver == 1 &&
		header->sign == HEADER_SIGN) {
		switch (header->mainId) {
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
		case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL:
		case Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER: {
			switch (header->enctype) {
			case packet::PUBENC_PROTOBUF_NONE: {
				TraceMessageID(header->mainId, header->subId);
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
			case packet::PUBENC_PROTOBUF_RSA: {
				TraceMessageID(header->mainId, header->subId);
				break;
			}
			case packet::PUBENC_PROTOBUF_AES: {
				break;
			}
			default:
				break;
			}
			break;
		}
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC:
			break;
		default:
			break;
		}
	}
	else {
		_LOG_ERROR("error");
	}
}

void HallServ::send(
	const muduo::net::TcpConnectionPtr& conn,
	uint8_t const* msg, size_t len,
	uint8_t mainId,
	uint8_t subId,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	std::vector<uint8_t> data;
	packet::packMessage(data, msg, len, mainId, subId, pre_header_, header_);
	TraceMessageID(mainId, subId);
	conn->send(&data[0], data.size());
}

void HallServ::send(
	const muduo::net::TcpConnectionPtr& conn,
	uint8_t const* msg, size_t len,
	uint8_t subId,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	std::vector<uint8_t> data;
	packet::packMessage(data, msg, len, subId, pre_header_, header_);
	TraceMessageID(header_->mainId, subId);
	conn->send(&data[0], data.size());
}

void HallServ::send(
	const muduo::net::TcpConnectionPtr& conn,
	::google::protobuf::Message* msg,
	uint8_t mainId,
	uint8_t subId,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	std::vector<uint8_t> data;
	packet::packMessage(data, msg, mainId, subId, pre_header_, header_);
	TraceMessageID(mainId, subId);
	conn->send(&data[0], data.size());
}

void HallServ::send(
	const muduo::net::TcpConnectionPtr& conn,
	::google::protobuf::Message* msg,
	uint8_t subId,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	std::vector<uint8_t> data;
	packet::packMessage(data, msg, subId, pre_header_, header_);
	TraceMessageID(header_->mainId, subId);
	conn->send(&data[0], data.size());
}

void HallServ::cmd_keep_alive_ping(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::Game::Common::KeepAliveMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::Game::Common::KeepAliveMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(0);
		rspdata.set_errormsg("[Hall]KEEP ALIVE PING OK.");
		//用户登陆token
		std::string const& token = reqdata.session();
		REDISCLIENT.ResetExpiredToken(token);
		int64_t userId = 0;
		uint32_t agentId = 0;
		std::string account;
		if (REDISCLIENT.GetTokenInfo(token, userId, account, agentId)) {
			REDISCLIENT.ResetExpiredUserToken(userId);
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_on_user_login(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::LoginMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::LoginMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(-1);
		rspdata.set_errormsg("Unkwown Error");
		int64_t userId = 0;
		uint32_t agentId = 0;
		std::string account;
		try {
			do {
				std::string country, location;
				ipFinder_.GetAddressByIp(ntohl(pre_header_->clientIp), location, country);
				std::string loginIp = utils::inetToIp(pre_header_->clientIp);
				std::string key = redisKeys::prefix_token_limit + reqdata.session();
				if (REDISCLIENT.exists(key)) {
					rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
					rspdata.set_errormsg("频繁登陆操作");
					_LOG_ERROR("%s ip:%s %s 频繁登陆操作",
						reqdata.session().c_str(),
						loginIp.c_str(), location.c_str());
					break;
				}
				else {
					REDISCLIENT.set(key, key, redisKeys::Expire_TokenLimit);
				}
				STD::time_point now = STD_NOW();
				if (REDISCLIENT.GetTokenInfo(reqdata.session(), userId, account, agentId)) {
					UserBaseInfo info;
					if (mgo::GetUserBaseInfo(
						document{}
						<< "account" << 1
						<< "agentid" << 1
						<< "nickname" << 1
						<< "headindex" << 1
						<< "lastlogintime" << 1
						<< "lastloginip" << 1
						<< "score" << 1
						<< "status" << 1 << finalize,
						document{} << "userid" << b_int64{ userId } << finalize,
						info)) {
						if (account == info.account/* && agentId == info.agentId*/) {
							if (info.status <= 0) {
								rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_SEAL_ACCOUNTS);
								rspdata.set_errormsg("对不起，您的账号已冻结，请联系客服。");
								mgo::AddLoginLog(userId, loginIp, location, now, 1);
								_LOG_ERROR("账号 %lld %s 已冻结!", userId, info.account.c_str());
								break;
							}
							//全服通知到各网关服顶号处理
							std::string gateIp((char const*)pre_header_->servId);
							assert(!gateIp.empty());
							std::string session((char const*)pre_header_->session);
							assert(!session.empty());
							boost::property_tree::ptree root;
							std::stringstream s;
							root.put("userid", userId);
							root.put("gateip", gateIp);
							root.put("session", session);
							boost::property_tree::write_json(s, root, false);
							std::string msg = s.str();
							REDISCLIENT.publishUserLoginMessage(msg);

							std::string uuid = utils::uuid::createUUID();
							std::string passwd = utils::buffer2HexStr((unsigned char const*)uuid.c_str(), uuid.size());
							REDISCLIENT.SetUserLoginInfo(userId, "dynamicPassword", passwd);

							rspdata.set_userid(userId);
							rspdata.set_account(account);
							rspdata.set_agentid(agentId);
							rspdata.set_nickname(info.nickName);
							rspdata.set_headid(info.headId);
							rspdata.set_score(info.userScore);
							rspdata.set_gamepass(uuid);
							rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_OK);
							rspdata.set_errormsg("User Login OK.");

							const_cast<packet::internal_prev_header_t*>(pre_header_)->userId = userId;
							const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;

							//db更新用户登陆信息
							mgo::UpdateLogin(userId, loginIp, info.lastlogintime, now);
							//db添加用户登陆日志
							mgo::AddLoginLog(userId, loginIp, location, now, 0);
							//redis更新登陆时间
							REDISCLIENT.SetUserLoginInfo(userId, "lastlogintime", std::to_string(now.to_time_t()));
							//redis更新token过期时间
#if 0
							REDISCLIENT.ResetExpiredToken(reqdata.session());
#else
							REDISCLIENT.SetTokenInfoIP(reqdata.session(), gateIp, session);
#endif
							REDISCLIENT.ResetExpiredUserToken(userId);
							_LOG_DEBUG("%d LOGIN SERVER OK! ip: %s %s gateIp: %s session: %s",
								userId,
								loginIp.c_str(),
								location.c_str(),
								gateIp.c_str(), session.c_str());
						}
						else {
							//账号不一致
							rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
							rspdata.set_errormsg("account/agentId Not Exist Error.");
							mgo::AddLoginLog(userId, loginIp, location, now, 2);
						}
					}
					else {
						//账号不存在
						_LOG_ERROR("%d Not Exist Error", userId);
						rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
						rspdata.set_errormsg("userId Not Exist Error.");
						mgo::AddLoginLog(userId, loginIp, location, now, 3);
					}
				}
				else {
					//token不存在或已过期
					_LOG_ERROR("%s Not Exist Error", reqdata.session().c_str());
					rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
					rspdata.set_errormsg("Session Not Exist Error.");
					mgo::AddLoginLog(userId, loginIp, location, now, 4);
				}
			}while (0);
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_NETBREAK);
			rspdata.set_errormsg("Database/redis Update Error.");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_on_user_offline(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	switch (pre_header_->kicking) {
	case KICK_REPLACE:
		_LOG_ERROR("KICK_REPLACE %d", pre_header_->userId);
		break;
	default:
		mgo::UpdateLogout(pre_header_->userId);
		std::string lastLoginTime;
		if (REDISCLIENT.GetUserLoginInfo(pre_header_->userId, "lastlogintime", lastLoginTime)) {
			mgo::AddLogoutLog(
				pre_header_->userId,
				STD::time_point(STD::variant(lastLoginTime).as_uint64()), STD_NOW());
		}
	}
}

/// <summary>
/// 查询游戏房间列表
/// </summary>
void HallServ::cmd_get_game_info(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetGameMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetGameMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(0);
		rspdata.set_errormsg("Get Game Message OK!");
		{
			READ_LOCK(gameinfo_mutex_);
			rspdata.CopyFrom(gameinfo_);
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_RES,
			pre_header_, header_);
	}
}

/// <summary>
/// 查询正在玩的游戏
/// </summary>
void HallServ::cmd_get_playing_game_info(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetPlayingGameInfoMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetPlayingGameInfoMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		int64_t userId = pre_header_->userId;
		uint32_t gameid = 0, roomid = 0;
		if (REDISCLIENT.GetOnlineInfo(userId, gameid, roomid)) {
			rspdata.set_gameid(gameid);
			rspdata.set_roomid(roomid);
			rspdata.set_retcode(0);
			rspdata.set_errormsg("Get Playing Game Info OK.");
			const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;
		}
		else {
			rspdata.set_retcode(1);
			rspdata.set_errormsg("This User Not In Playing Game!");
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_RES,
			pre_header_, header_);
	}
}

/// <summary>
/// 查询指定游戏节点
/// </summary>
void HallServ::cmd_get_game_server_message(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetGameServerMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetGameServerMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(1);
		rspdata.set_errormsg("Unknown error.");
		rspdata.set_gameid(reqdata.gameid());
		rspdata.set_roomid(reqdata.roomid());
		uint32_t gameid = reqdata.gameid();
		uint32_t roomid = reqdata.roomid();
		int64_t userId = pre_header_->userId;
		uint32_t gameid_ = 0, roomid_ = 0;
		if (REDISCLIENT.GetOnlineInfo(userId, gameid_, roomid_)) {
			if (gameid != gameid_ || roomid != roomid_) {
				rspdata.set_retcode(ERROR_ENTERROOM_USERINGAME);
				rspdata.set_errormsg("user in other game.");
			}
			else {
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;
				rspdata.set_retcode(0);
				rspdata.set_errormsg("Get Game Server IP OK.");
			}
		}
		else {
			//随机一个指定类型游戏节点
			std::string ipport;
			room::nodes::balance_server(roomid, ipport);
			//可能ipport节点不可用，要求zk实时监控
			if (!ipport.empty()) {
				//redis更新玩家游戏节点
				REDISCLIENT.SetOnlineInfoIP(userId, ipport);
				//redis更新玩家游戏中
				REDISCLIENT.SetOnlineInfo(userId, gameid, roomid);
				rspdata.set_retcode(0);
				rspdata.set_errormsg("Get Game Server IP OK.");
				//通知网关服成功
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;
			}
			else {
				//分配失败，清除游戏中状态
				REDISCLIENT.DelOnlineInfo(userId);
				rspdata.set_retcode(ERROR_ENTERROOM_GAMENOTEXIST);
				rspdata.set_errormsg("Game Server Not found!!!");
			}
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_get_room_player_nums(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetServerPlayerNum reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetServerPlayerNumResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(0);
		rspdata.set_errormsg("Get ServerPlayerNum Message OK!");
		{
			READ_LOCK(room_playernums_mutex_);
			rspdata.CopyFrom(room_playernums_);
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_set_headid(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::SetHeadIdMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::SetHeadIdMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(0);
		rspdata.set_errormsg("OK.");
		int64_t userId = pre_header_->userId;
		int32_t headid = reqdata.headid();
		try {
			mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
			userCollection.update_one(document{} << "userid" << userId << finalize,
				document{} << "$set" << open_document <<
				"headindex" << headid << close_document << finalize);
			rspdata.set_userid(userId);
			rspdata.set_headid(headid);
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(1);
			rspdata.set_errormsg("Database  Error.");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_set_nickname(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
}

void HallServ::cmd_get_userscore(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetUserScoreMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetUserScoreMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		int64_t userId = pre_header_->userId;
		int64_t score = 0;
		rspdata.set_userid(userId);
		try {
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "score" << 1 << finalize);
			mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
			bsoncxx::stdx::optional<bsoncxx::document::value> result =
				userCollection.find_one(document{} << "userid" << userId << finalize, opts);
			if (result) {
				bsoncxx::document::view view = result->view();
				score = view["score"].get_int64();
				rspdata.set_score(score);
				rspdata.set_retcode(0);
				rspdata.set_errormsg("CMD GET USER SCORE OK.");
			}
			else {
				rspdata.set_retcode(1);
				rspdata.set_errormsg("CMD GET USER SCORE OK.");
			}
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(2);
			rspdata.set_errormsg("Database  Error.");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_get_play_record(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetPlayRecordMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetPlayRecordMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		int64_t userId = pre_header_->userId;
		int32_t gameid = reqdata.gameid();
		rspdata.set_gameid(gameid);
		try {
			std::string roundid;//牌局编号
			int32_t roomid = 0;//房间号
			int64_t winscore = 0;//输赢分
			std::chrono::system_clock::time_point endTime;//结束时间
			int64_t endTimestamp = 0;//结束时间戳
			//(userId, gameid)
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "gameinfoid" << 1 << "roomid" << 1 << "winscore" << 1 << "gameendtime" << 1 << finalize);
			opts.sort(document{} << "_id" << -1 << finalize);//"_id"字段排序
			opts.limit(10); //显示最新10条
			mongocxx::collection playCollection = MONGODBCLIENT["gamemain"]["play_record"];
			mongocxx::cursor cursor = playCollection.find(document{} << "userid" << userId << "gameid" << gameid << finalize, opts);
			for (auto& doc : cursor) {
				roundid = doc["gameinfoid"].get_utf8().value.to_string();	//牌局编号
				roomid = doc["roomid"].get_int32();							//房间号
				winscore = doc["winscore"].get_int64();						//输赢分
				endTime = doc["gameendtime"].get_date();					//结束时间
				endTimestamp = (std::chrono::duration_cast<std::chrono::seconds>(endTime.time_since_epoch())).count();

				::HallServer::GameRecordInfo* gameRecordInfo = rspdata.add_detailinfo();
				gameRecordInfo->set_gameroundno(roundid);
				gameRecordInfo->set_roomid(roomid);
				gameRecordInfo->set_winlosescore(winscore);
				gameRecordInfo->set_gameendtime(endTimestamp);
			}
			rspdata.set_retcode(0);
			rspdata.set_errormsg("CMD GET USER SCORE OK.");
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(1);
			rspdata.set_errormsg("MongoDB  Error.");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_get_play_record_detail(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::HallServer::GetRecordDetailMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::HallServer::GetRecordDetailResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		std::string const& roundid = reqdata.gameroundno();
		rspdata.set_gameroundno(roundid);
		rspdata.set_retcode(-1);
		rspdata.set_errormsg("Not Exist.");
		int64_t userId = pre_header_->userId;
		try {
			std::string jsondata;
			mongocxx::collection replayCollection = MONGODBCLIENT["gamelog"]["game_replay"];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "detail" << 1 << finalize);
			bsoncxx::stdx::optional<bsoncxx::document::value> result =
				replayCollection.find_one(document{} << "gameinfoid" << roundid << finalize, opts);
			if (result) {
				bsoncxx::document::view view = result->view();
				if (view["detail"]) {
					switch (view["detail"].type()) {
					case bsoncxx::type::k_null:
						break;
					case bsoncxx::type::k_utf8:
						jsondata = view["detail"].get_utf8().value.to_string();
						_LOG_ERROR(jsondata.c_str());
						rspdata.set_detailinfo(jsondata);
						break;
					case bsoncxx::type::k_binary:
						rspdata.set_detailinfo(view["detail"].get_binary().bytes, view["detail"].get_binary().size);
						break;
					}
				}
				rspdata.set_retcode(0);
				rspdata.set_errormsg("CMD GET GAME RECORD DETAIL OK.");
			}
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(1);
			rspdata.set_errormsg("MongoDB Error.");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_RECORD_DETAIL_RES,
			pre_header_, header_);
	}
}

void HallServ::cmd_repair_hallserver(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
}

void HallServ::cmd_get_task_list(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
}

void HallServ::cmd_get_task_award(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
}

void HallServ::db_refresh_game_room_info() {
	MY_TRY()
	int index = RANDOM().betweenInt(0, threadPool_.size() - 1).randInt_re();
	if (index >= 0 && index < threadPool_.size()) {
		threadPool_[index]->run(std::bind(&HallServ::db_update_game_room_info, this));
	}
	MY_CATCH()
}

void HallServ::db_update_game_room_info() {
	WRITE_LOCK(gameinfo_mutex_);
	gameinfo_.clear_header();
	gameinfo_.clear_gamemessage();
	mgo::LoadGameRoomInfos(gameinfo_);
}


void HallServ::redis_refresh_room_player_nums() {
	static STD::Random r(0, threadPool_.size() - 1);
	int index = r.randInt_re();
	assert(index >= 0 && index < threadPool_.size());
	threadPool_[index]->run(std::bind(&HallServ::redis_update_room_player_nums, this));
	threadTimer_->getLoop()->runAfter(30, std::bind(&HallServ::redis_refresh_room_player_nums, this));
}

void HallServ::redis_update_room_player_nums() {
	WRITE_LOCK(room_playernums_mutex_);
	room_playernums_.clear_header();
	room_playernums_.clear_gameplayernummessage();
	auto& gameMessage = gameinfo_.gamemessage();
	try {
		//各个子游戏
		for (auto& gameinfo : gameMessage) {
			::HallServer::SimpleGameMessage* simpleMessage = room_playernums_.add_gameplayernummessage();
			uint32_t gameid = gameinfo.gameid();
			simpleMessage->set_gameid(gameid);
			auto& gameroommsg = gameinfo.gameroommsg();
			//各个子游戏房间
			for (auto& roominfo : gameroommsg) {
				::HallServer::RoomPlayerNum* roomPlayerNum = simpleMessage->add_roomplayernum();
				uint32_t roomid = roominfo.roomid();
				//redis获取房间人数
				uint64_t playerNums = 0;
				//if (room_servers_.find(roomid) != room_servers_.end()) {
				//	REDISCLIENT.GetGameServerplayerNum(room_servers_[roomid], playerNums);
				//}
				//更新房间游戏人数
				roomPlayerNum->set_roomid(roomid);
				roomPlayerNum->set_playernum(playerNums);
				const_cast<::HallServer::GameRoomMessage&>(roominfo).set_playernum(playerNums);
				//_LOG_DEBUG("roomId=%d playerCount=%d", roomid, playerNums);
			}
		}
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
}

void HallServ::on_refresh_game_config(std::string msg) {
	db_refresh_game_room_info();
}

//===================俱乐部==================

// 获取游戏服务器IP
void HallServ::GetGameServerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取我的俱乐部
void HallServ::GetMyClubGameMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 加入俱乐部
void HallServ::JoinTheClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 退出俱乐部
void HallServ::ExitTheClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 设置是否开启自动成为合伙人
void HallServ::SetAutoBecomePartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 成为合伙人
void HallServ::BecomePartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 佣金兑换金币
void HallServ::ExchangeRevenueMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取佣金提取记录 
void HallServ::GetExchangeRevenueRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取我的业绩
void HallServ::GetMyAchievementMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取会员业绩详情
void HallServ::GetAchievementDetailMemberMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取合伙人业绩详情
void HallServ::GetAchievementDetailPartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取俱乐部内我的团队我的俱乐部
void HallServ::GetMyClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取我的团队成员
void HallServ::GetMyTeamMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 设置下一级合伙人提成比例
void HallServ::SetSubordinateRateMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取游戏明细记录
void HallServ::GetPlayRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取玩家俱乐部所有游戏记录列表
void HallServ::GetAllPlayRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取玩家账户明细列表
void HallServ::GetUserScoreChangeRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 我的上级信息
void HallServ::GetClubPromoterInfoMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 开除此用户
void HallServ::FireMemberMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}

// 获取俱乐部申请QQ
void HallServ::GetApplyClubQQMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
}