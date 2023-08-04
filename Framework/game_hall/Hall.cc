
#include "Hall.h"

HallServ::HallServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr) :
	server_(loop, listenAddr, "HallServ")
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
	, ipFinder_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&HallServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&HallServ::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	threadTimer_->startLoop();
}

HallServ::~HallServ() {
	Quit();
}

void HallServ::Quit() {
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
		std::vector<std::string> vec;
		boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
		//ip:port
		nodeValue_ = vec[0] + ":" + vec[1];
		nodePath_ = "/GAME/HallServers/" + nodeValue_;
		zkclient_->createNode(nodePath_, nodeValue_, true);
		invalidNodePath_ = "/GAME/HallServersInvalid/" + nodeValue_;
	}
	{
		//网关服 ip:port:port:pid
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
		//游戏服 roomid:ip:port
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
			WRITE_LOCK(room_servers_mutex_);
			for (std::string const& name : names) {
				s += "\n" + name;
				std::vector<string> vec;
				boost::algorithm::split(vec, name, boost::is_any_of(":"));
				std::vector<std::string>& room = room_servers_[stoi(vec[0])];
				room.emplace_back(name);
			}
			_LOG_WARN("可用游戏服列表%s", s.c_str());
		}
	}
}

void HallServ::onGateWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	//网关服 ip:port:port:pid
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
	//游戏服 roomid:ip:port
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
		WRITE_LOCK(room_servers_mutex_);
		for (std::string const& name : names) {
			s += "\n" + name;
			std::vector<string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			std::vector<std::string>& room = room_servers_[stoi(vec[0])];
			room.emplace_back(name);
		}
		_LOG_WARN("可用游戏服列表%s", s.c_str());
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
	initTraceMessageID();
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

	_LOG_INFO("HallServ = %s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), numThreads, numWorkerThreads);

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
			std::string session((char const*)pre_header->session, sizeof(pre_header->session));
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
		rspdata.set_errormsg("KEEP ALIVE PING OK.");
		//用户登陆token
		std::string const& token = reqdata.session();
		REDISCLIENT.resetExpired("k.token." + token);
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
		int64_t userid = 0;
		uint32_t agentid = 0;
		std::string account;
		//用户登陆token
		std::string const& token = reqdata.session();
		try {
			//登陆IP
			uint32_t ipaddr = pre_header_->clientIp;
			//查询IP所在国家/地区
			std::string country, location;
			ipFinder_.GetAddressByIp(ntohl(ipaddr), location, country);
			std::string loginIp = utils::inetToIp(ipaddr);
			//不能频繁登陆操作(间隔5s)
			std::string key = REDIS_LOGIN_3S_CHECK + token;
			if (REDISCLIENT.exists(key)) {
				_LOG_ERROR("%s IP:%s 频繁登陆", location.c_str(), loginIp.c_str());
				return;
			}
			else {
				REDISCLIENT.set(key, key, 5);
			}
			//系统当前时间
			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			//redis查询token，判断是否过期
			if (redis_get_token_info(token, userid, account, agentid)) {
				mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
				bsoncxx::document::value query_value = document{} << "userid" << userid << finalize;
				bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(query_value.view());
				if (result) {
					//查询用户数据
					//_LOG_DEBUG("Query result:\n%s", bsoncxx::to_json(*result).c_str());
					bsoncxx::document::view view = result->view();
					std::string account_ = view["account"].get_utf8().value.to_string();
					int32_t agentid_ = view["agentid"].get_int32();
					int32_t headid = view["headindex"].get_int32();
					std::string nickname = view["nickname"].get_utf8().value.to_string();
					int64_t score = view["score"].get_int64();
					int32_t status_ = view["status"].get_int32();
					std::string lastLoginIp = view["lastloginip"].get_utf8().value.to_string();//lastLoginIp
					std::chrono::system_clock::time_point lastLoginTime = view["lastlogintime"].get_date();//lastLoginTime
					//userid(account,agentid)必须一致
					//if (account == account_ && agentid == agentid_) {
					if (account == account_) {
						//////////////////////////////////////////////////////////////////////////
						//玩家登陆网关服信息
						//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
						//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
						//网关服ID格式：session|ip:port:port:pid
						//第一个ip:port是网关服监听客户端的标识
						//第二个ip:port是网关服监听订单服的标识
						//pid标识网关服进程id
						//std::string proxyinfo = string(internal_header->session) + "|" + string(internal_header->servid);
						//REDISCLIENT.set("s.uid:" + to_string(userId) + ":proxy", proxyinfo);
						if (status_ == 2) {
							rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_SEAL_ACCOUNTS);
							rspdata.set_errormsg("对不起，您的账号已冻结，请联系客服。");
							db_add_login_logger(userid, loginIp, location, now, 1, agentid);
							goto end;
						}
						//全服通知到各网关服顶号处理
						std::string session((char const*)pre_header_->session, sizeof(pre_header_->session));
						boost::property_tree::ptree root;
						std::stringstream s;
						root.put("userid", userid);
						root.put("session", session);
						boost::property_tree::write_json(s, root, false);
						std::string msg = s.str();
						REDISCLIENT.publishUserLoginMessage(msg);

						std::string uuid = utils::uuid::createUUID();
						std::string passwd = utils::buffer2HexStr((unsigned char const*)uuid.c_str(), uuid.size());
						REDISCLIENT.SetUserLoginInfo(userid, "dynamicPassword", passwd);

						rspdata.set_userid(userid);
						rspdata.set_account(account);
						rspdata.set_agentid(agentid);
						rspdata.set_nickname(nickname);
						rspdata.set_headid(headid);
						rspdata.set_score(score);
						rspdata.set_gamepass(uuid);
						rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_OK);
						rspdata.set_errormsg("User Login OK.");

						//通知网关服登陆成功
						const_cast<packet::internal_prev_header_t*>(pre_header_)->userId = userid;
						const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;

						//db更新用户登陆信息
						db_update_login_info(userid, loginIp, lastLoginTime, now);
						//db添加用户登陆日志
						db_add_login_logger(userid, loginIp, location, now, 0, agentid);
						//redis更新登陆时间
						REDISCLIENT.SetUserLoginInfo(userid, "lastlogintime", std::to_string(chrono::system_clock::to_time_t(now)));
						//redis更新token过期时间
						REDISCLIENT.resetExpired("k.token." + token);
						_LOG_DEBUG("%d LOGIN SERVER OK!", userid);
					}
					else {
						//账号不一致
						rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
						rspdata.set_errormsg("account/agentid Not Exist Error.");
						db_add_login_logger(userid, loginIp, location, now, 2, agentid);
					}
				}
				else {
					//账号不存在
					_LOG_ERROR("%d Not Exist Error", userid);
					rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
					rspdata.set_errormsg("userid Not Exist Error.");
					db_add_login_logger(userid, loginIp, location, now, 3, agentid);
				}
			}
			else {
				//token不存在或已过期
				_LOG_ERROR("%s Not Exist Error", token.c_str());
				rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_ACCOUNTS_NOT_EXIST);
				rspdata.set_errormsg("Session Not Exist Error.");
				db_add_login_logger(userid, loginIp, location, now, 4, agentid);
			}
		}
		catch (std::exception& e) {
			_LOG_ERROR(e.what());
			rspdata.set_retcode(::HallServer::LoginMessageResponse::LOGIN_NETBREAK);
			rspdata.set_errormsg("Database/redis Update Error.");
		}
	end:
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
	int64_t userid = pre_header_->userId;
	db_update_online_status(userid, 0);
	std::string lastLoginTime;
	if (REDISCLIENT.GetUserLoginInfo(userid, "lastlogintime", lastLoginTime)) {
		std::chrono::system_clock::time_point loginTime = std::chrono::system_clock::from_time_t(stoull(lastLoginTime));
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		db_add_logout_logger(userid, loginTime, now);
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
		int64_t userid = pre_header_->userId;
		uint32_t gameid = 0, roomid = 0;
		if (REDISCLIENT.GetUserOnlineInfo(userid, gameid, roomid)) {
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
		int64_t userid = pre_header_->userId;
		uint32_t gameid_ = 0, roomid_ = 0;
		if (REDISCLIENT.GetUserOnlineInfo(userid, gameid_, roomid_)) {
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
			if ((roomid - gameid * 10) < 20) {
				random_game_server_ipport(roomid, ipport);
			}
			else {
			}
			//可能ipport节点不可用，要求zk实时监控
			if (!ipport.empty()) {
				//redis更新玩家游戏节点
				REDISCLIENT.SetUserOnlineInfoIP(userid, ipport);
				//redis更新玩家游戏中
				REDISCLIENT.SetUserOnlineInfo(userid, gameid, roomid);
				rspdata.set_retcode(0);
				rspdata.set_errormsg("Get Game Server IP OK.");
				//通知网关服成功
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = 1;
			}
			else {
				//分配失败，清除游戏中状态
				REDISCLIENT.DelUserOnlineInfo(userid);
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
		int64_t userid = pre_header_->userId;
		int32_t headid = reqdata.headid();
		try {
			mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
			userCollection.update_one(document{} << "userid" << userid << finalize,
				document{} << "$set" << open_document <<
				"headindex" << headid << close_document << finalize);
			rspdata.set_userid(userid);
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
		int64_t userid = pre_header_->userId;
		int64_t score = 0;
		rspdata.set_userid(userid);
		try {
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "score" << 1 << finalize);
			mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
			bsoncxx::stdx::optional<bsoncxx::document::value> result =
				userCollection.find_one(document{} << "userid" << userid << finalize, opts);
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
		int64_t userid = pre_header_->userId;
		int32_t gameid = reqdata.gameid();
		rspdata.set_gameid(gameid);
		try {
			std::string roundid;//牌局编号
			int32_t roomid = 0;//房间号
			int64_t winscore = 0;//输赢分
			std::chrono::system_clock::time_point endTime;//结束时间
			int64_t endTimestamp = 0;//结束时间戳
			//(userid, gameid)
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(document{} << "gameinfoid" << 1 << "roomid" << 1 << "winscore" << 1 << "gameendtime" << 1 << finalize);
			opts.sort(document{} << "_id" << -1 << finalize);//"_id"字段排序
			opts.limit(10); //显示最新10条
			mongocxx::collection playCollection = MONGODBCLIENT["gamemain"]["play_record"];
			mongocxx::cursor cursor = playCollection.find(document{} << "userid" << userid << "gameid" << gameid << finalize, opts);
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
		//userid
		int64_t userid = pre_header_->userId;
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
	static STD::Random r(0, threadPool_.size() - 1);
	int index = r.randInt_re();
	assert(index >= 0 && index < threadPool_.size());
	threadPool_[index]->run(std::bind(&HallServ::db_update_game_room_info, this));
}

void HallServ::db_update_game_room_info() {
	WRITE_LOCK(gameinfo_mutex_);
	gameinfo_.clear_header();
	gameinfo_.clear_gamemessage();
	try {
		//游戏类型，抽水率，排序id，游戏类型，热度，维护状态，游戏名称
		uint32_t gameid = 0, revenueratio = 0, gamesortid = 0, gametype = 0, gameishot = 0;
		std::string gamename;
		int32_t gamestatus = 0;
		//房间号，桌子数量，最小/最多游戏人数，房间状态，房间名称
		uint32_t roomid = 0, tableCount = 0, minPlayerNum = 0, maxPlayerNum = 0, roomstatus = 0;
		std::string roomname;
		//房间底注，房间顶注，最小/最大准入分，区域限红
		int64_t floorscore, ceilscore, enterMinScore, enterMaxScore, maxJettonScore;

		mongocxx::collection kindCollection = MONGODBCLIENT["gameconfig"]["game_kind"];
		mongocxx::cursor cursor = kindCollection.find({});
		for (auto& doc : cursor) {
			//_LOG_DEBUG("%s", bsoncxx::to_json(doc).c_str());
			gameid = doc["gameid"].get_int32();						//游戏ID
			gamename = doc["gamename"].get_utf8().value.to_string();//游戏名称
			gamesortid = doc["sort"].get_int32();					//游戏排序0 1 2 3 4
			gametype = doc["type"].get_int32();						//0-百人场  1-对战类
			gameishot = doc["ishot"].get_int32();					//0-正常 1-火爆 2-新
			gamestatus = doc["status"].get_int32();					//-1:关停 0:暂未开放 1：正常开启  2：敬请期待
			::HallServer::GameMessage* gameMsg = gameinfo_.add_gamemessage();
			gameMsg->set_gameid(gameid);
			gameMsg->set_gamename(gamename);
			gameMsg->set_gamesortid(gamesortid);
			gameMsg->set_gametype(gametype);
			gameMsg->set_gameishot(gameishot);
			gameMsg->set_gamestatus(gamestatus);
			//各游戏房间
			auto rooms = doc["rooms"].get_array();
			for (auto& doc_ : rooms.value)
			{
				roomid = doc_["roomid"].get_int32();						//房间类型 初 中 高 房间
				roomname = doc_["roomname"].get_utf8().value.to_string();//类型名称 初 中 高
				tableCount = doc_["tablecount"].get_int32();				//桌子数量 有几桌游戏开启中
				floorscore = doc_["floorscore"].get_int64();				//房间底注
				ceilscore = doc_["ceilscore"].get_int64();				//房间顶注
				enterMinScore = doc_["enterminscore"].get_int64();		//进游戏最低分(最小准入分)
				enterMaxScore = doc_["entermaxscore"].get_int64();		//进游戏最大分(最大准入分)
				minPlayerNum = doc_["minplayernum"].get_int32();			//最少游戏人数(至少几人局)
				maxPlayerNum = doc_["maxplayernum"].get_int32();			//最多游戏人数(最大几人局)
				maxJettonScore = doc_["maxjettonscore"].get_int64();		//各区域最大下注(区域限红)
				roomstatus = doc_["status"].get_int32();					//-1:关停 0:暂未开放 1：正常开启  2：敬请期待

				::HallServer::GameRoomMessage* roomMsg = gameMsg->add_gameroommsg();
				roomMsg->set_roomid(roomid);
				roomMsg->set_roomname(roomname);
				roomMsg->set_tablecount(tableCount);
				roomMsg->set_floorscore(floorscore);
				roomMsg->set_ceilscore(ceilscore);
				roomMsg->set_enterminscore(enterMinScore);
				roomMsg->set_entermaxscore(enterMaxScore);
				roomMsg->set_minplayernum(minPlayerNum);
				roomMsg->set_maxplayernum(maxPlayerNum);
				roomMsg->set_maxjettonscore(maxJettonScore);
				roomMsg->set_status(roomstatus);
				//配置可下注筹码数组
				bsoncxx::types::b_array jettons;
				bsoncxx::document::element elem = doc_["jettons"];
				if (elem.type() == bsoncxx::type::k_array)
					jettons = elem.get_array();
				else
					jettons = doc_["jettons"]["_v"].get_array();
				for (auto& jetton : jettons.value) {
					roomMsg->add_jettons(jetton.get_int64());
				}
				//房间在游戏中人数
				roomMsg->set_playernum(0);
			}
		}
		//redis_update_room_player_nums();
	}
	catch (exception& e) {
		_LOG_ERROR(e.what());
	}
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
				if (room_servers_.find(roomid) != room_servers_.end()) {
					REDISCLIENT.GetGameServerplayerNum(room_servers_[roomid], playerNums);
				}
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

/// <summary>
/// 随机一个指定类型游戏节点
/// </summary>
/// <param name="roomid"></param>
/// <param name="ipport"></param>
void HallServ::random_game_server_ipport(uint32_t roomid, std::string& ipport) {
	ipport.clear();
	static STD::Random r;
	READ_LOCK(room_servers_mutex_);
	std::map<int, std::vector<std::string>>::iterator it = room_servers_.find(roomid);
	if (it != room_servers_.end()) {
		std::vector<std::string>& rooms = it->second;
		if (rooms.size() > 0) {
			int index = r.betweenInt(0, rooms.size() - 1).randInt_mt();
			ipport = rooms[index];
		}
	}
}

//redis查询token，判断是否过期
bool HallServ::redis_get_token_info(
	std::string const& token,
	int64_t& userid, std::string& account, uint32_t& agentid) {
	try {
		std::string value;
		if (REDISCLIENT.get("k.token." + token, value)) {
			boost::property_tree::ptree root;
			std::stringstream s(value);
			boost::property_tree::read_json(s, root);
			userid = root.get<int64_t>("uid");
			account = root.get<std::string>("account");
			//agentid = root.get<uint32_t>("agentid");
			return userid > 0;
		}
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
	return false;
}

//db更新用户登陆信息(登陆IP，时间)
bool HallServ::db_update_login_info(
	int64_t userid,
	std::string const& loginIp,
	std::chrono::system_clock::time_point& lastLoginTime,
	std::chrono::system_clock::time_point& now) {
	bool bok = false;
	int64_t days = (std::chrono::duration_cast<std::chrono::seconds>(lastLoginTime.time_since_epoch())).count() / 3600 / 24;
	int64_t nowDays = (std::chrono::duration_cast<chrono::seconds>(now.time_since_epoch())).count() / 3600 / 24;
	try {
		bsoncxx::document::value query_value = document{} << "userid" << userid << finalize;
		mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
		if (days + 1 == nowDays) {
			bsoncxx::document::value update_value = document{}
				<< "$set" << open_document
				<< "lastlogintime" << bsoncxx::types::b_date(now)
				<< "lastloginip" << loginIp
				<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
				<< close_document
				<< "$inc" << open_document
				<< "activedays" << 1								//活跃天数+1
				<< "keeplogindays" << 1 << close_document			//连续登陆天数+1
				<< finalize;
			userCollection.update_one(query_value.view(), update_value.view());
		}
		else if (days == nowDays) {
			bsoncxx::document::value update_value = document{}
				<< "$set" << open_document
				<< "lastlogintime" << bsoncxx::types::b_date(now)
				<< "lastloginip" << loginIp
				<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
				<< close_document
				<< finalize;
			userCollection.update_one(query_value.view(), update_value.view());
		}
		else {
			bsoncxx::document::value update_value = document{}
				<< "$set" << open_document
				<< "lastlogintime" << bsoncxx::types::b_date(now)
				<< "lastloginip" << loginIp
				<< "keeplogindays" << bsoncxx::types::b_int32{ 1 }	//连续登陆天数1
				<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
				<< close_document
				<< "$inc" << open_document
				<< "activedays" << 1 << close_document				//活跃天数+1
				<< finalize;
			userCollection.update_one(query_value.view(), update_value.view());
		}
		bok = true;
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
	return bok;
}

//db更新用户在线状态
bool HallServ::db_update_online_status(int64_t userid, int32_t status) {
	bool bok = false;
	try {
		//////////////////////////////////////////////////////////////////////////
		//玩家登陆网关服信息
		//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
		//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
		//网关服ID格式：session|ip:port:port:pid
		//第一个ip:port是网关服监听客户端的标识
		//第二个ip:port是网关服监听订单服的标识
		//pid标识网关服进程id
		//////////////////////////////////////////////////////////////////////////
		REDISCLIENT.del("s.uid:" + to_string(userid) + ":proxy");
		bsoncxx::document::value query_value = document{} << "userid" << userid << finalize;
		mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
		bsoncxx::document::value update_value = document{}
			<< "$set" << open_document
			<< "onlinestatus" << bsoncxx::types::b_int32{ status }
			<< close_document
			<< finalize;
		userCollection.update_one(query_value.view(), update_value.view());
		bok = true;
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
	return bok;
}

//db添加用户登陆日志
bool HallServ::db_add_login_logger(
	int64_t userid,
	std::string const& loginIp,
	std::string const& location,
	std::chrono::system_clock::time_point& now,
	uint32_t status, uint32_t agentid) {
	bool bok = false;
	try {
		mongocxx::collection loginLogCollection = MONGODBCLIENT["gamemain"]["login_log"];
		bsoncxx::document::value insert_value = bsoncxx::builder::stream::document{}
			<< "userid" << userid
			<< "loginip" << loginIp
			<< "address" << location
			<< "status" << (int32_t)status //错误码
			<< "agentid" << (int32_t)agentid
			<< "logintime" << bsoncxx::types::b_date(now)
			<< bsoncxx::builder::stream::finalize;
		//_LOG_DEBUG("Insert Document: %s", bsoncxx::to_json(insert_value).c_str());
		bsoncxx::stdx::optional<mongocxx::result::insert_one> result = loginLogCollection.insert_one(insert_value.view());
		bok = true;
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
	return bok;
}

bool HallServ::db_add_logout_logger(
	int64_t userid,
	std::chrono::system_clock::time_point& loginTime,
	std::chrono::system_clock::time_point& now) {
	bool bok = false;
	try {

		int32_t agentid = 0;
		mongocxx::options::find opts = mongocxx::options::find{};
		opts.projection(document{} << "agentid" << 1 << finalize);
		bsoncxx::document::value query_value = document{} << "userid" << userid << finalize;
		mongocxx::collection userCollection = MONGODBCLIENT["gamemain"]["game_user"];
		bsoncxx::stdx::optional<bsoncxx::document::value> result = userCollection.find_one(query_value.view(), opts);
		bsoncxx::document::view view = result->view();
		agentid = view["agentid"].get_int32();
		//在线时长
		chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(now - loginTime);
		mongocxx::collection logoutLogCollection = MONGODBCLIENT["gamemain"]["logout_log"];
		bsoncxx::document::value insert_value = document{}
			<< "userid" << userid
			<< "logintime" << bsoncxx::types::b_date(loginTime)					//登陆时间
			<< "logouttime" << bsoncxx::types::b_date(now)						//离线时间
			<< "agentid" << agentid
			<< "playseconds" << bsoncxx::types::b_int64{ durationTime.count() } //在线时长
		<< finalize;
		/*bsoncxx::stdx::optional<mongocxx::result::insert_one> result = */logoutLogCollection.insert_one(insert_value.view());
		bok = true;
	}
	catch (std::exception& e) {
		_LOG_ERROR(e.what());
	}
	return bok;
}