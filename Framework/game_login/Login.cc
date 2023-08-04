
#include "Login.h"

LoginServ::LoginServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
	: server_(loop, listenAddr, "LoginServ")
	, httpServer_(loop, listenAddrHttp, "httpServer")
	, gateRpcClients_(loop)
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
	, server_state_(kRunning)
	, ipFinder_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&LoginServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpServer_.setConnectionCallback(
		std::bind(&LoginServ::onHttpConnection, this, std::placeholders::_1));
	httpServer_.setMessageCallback(
		std::bind(&LoginServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpServer_.setWriteCompleteCallback(
		std::bind(&LoginServ::onHttpWriteComplete, this, std::placeholders::_1));
	rpcClients_[rpc::servTyE::kRpcGateTy].clients_ = &gateRpcClients_;
	rpcClients_[rpc::servTyE::kRpcGateTy].ty_ = rpc::servTyE::kRpcGateTy;
	//添加OpenSSL认证支持 httpServer_&server_ 共享证书
	muduo::net::ssl::SSL_CTX_Init(
		cert_path,
		private_key_path,
		client_ca_cert_file_path, client_ca_cert_dir_path);
	//指定SSL_CTX
	server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
	httpServer_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
	threadTimer_->startLoop();
}

LoginServ::~LoginServ() {
	Quit();
}

void LoginServ::Quit() {
	threadTimer_->getLoop()->quit();
	gateRpcClients_.closeAll();
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

void LoginServ::registerHandlers() {
// 	handlers_[packet::enword(
// 		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
// 		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)]
// 		= std::bind(&LoginServ::cmd_on_user_login, this,
// 			std::placeholders::_1, std::placeholders::_2);
}

bool LoginServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&LoginServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		_LOG_FATAL("error");
		abort();
		return false;
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&LoginServ::registerZookeeper, this));
	return true;
}

void LoginServ::onZookeeperConnected() {
	std::vector<std::string> vec;
	boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
	path_handshake_ = "/ws_" + vec[1];
	//网关服RPC ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/RPCProxyServers",
		names,
		std::bind(
			&LoginServ::onGateRpcWatcher, this,
			placeholders::_1, std::placeholders::_2,
			placeholders::_3, std::placeholders::_4,
			placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用网关服[RPC]列表%s", s.c_str());
		rpcClients_[rpc::servTyE::kRpcGateTy].add(names);
	}
}

void LoginServ::onGateRpcWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	//网关服RPC ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/RPCProxyServers",
		names,
		std::bind(
			&LoginServ::onGateRpcWatcher, this,
			placeholders::_1, std::placeholders::_2,
			placeholders::_3, std::placeholders::_4,
			placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用网关服[RPC]列表%s", s.c_str());
		rpcClients_[rpc::servTyE::kRpcGateTy].process(names);
	}
}

void LoginServ::registerZookeeper() {

}

bool LoginServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		_LOG_FATAL("error");
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	redisClient_->startSubThread();
	return true;
}

bool LoginServ::InitMongoDB(std::string const& url) {
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

void LoginServ::threadInit() {
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

bool LoginServ::InitServer() {
	initTraceMessageID();
	return true;
}

void LoginServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();

	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&LoginServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	_LOG_INFO("LoginServ = %s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), numThreads, numWorkerThreads);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&LoginServ::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpServer_.setConditionCallback(std::bind(&LoginServ::onHttpCondition, this, std::placeholders::_1));
	}

	server_.start(true);
	httpServer_.start(true);

	//sleep(2);

	std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool =
		muduo::net::ReactorSingleton::threadPool();
	std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();
	for (std::vector<muduo::net::EventLoop*>::const_iterator it = loops.begin();
		it != loops.end(); ++it) {
		(*it)->setContext(Buckets(*it, idleTimeout_, interval_));
		(*it)->runAfter(interval_, std::bind(&Buckets::pop, &boost::any_cast<Buckets&>((*it)->getContext())));
	}
}

void LoginServ::cmd_on_user_login(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {

}

void LoginServ::random_game_gate_ipport(uint32_t roomid, std::string& ipport) {
// 	ipport.clear();
// 	static STD::Random r;
// 	//READ_LOCK(room_servers_mutex_);
// 	std::map<int, std::vector<std::string>>::iterator it = room_servers_.find(roomid);
// 	if (it != room_servers_.end()) {
// 		std::vector<std::string>& rooms = it->second;
// 		if (rooms.size() > 0) {
// 			int index = r.betweenInt(0, rooms.size() - 1).randInt_mt();
// 			ipport = rooms[index];
// 		}
// 	}
}

//db更新用户登陆信息(登陆IP，时间)
bool LoginServ::db_update_login_info(
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
bool LoginServ::db_update_online_status(int64_t userid, int32_t status) {
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
bool LoginServ::db_add_login_logger(
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

bool LoginServ::db_add_logout_logger(
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