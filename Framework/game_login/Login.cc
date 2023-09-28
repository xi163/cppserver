
#include "Login.h"

LoginServ::LoginServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrRpc,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
	: server_(loop, listenAddr, "wsServer")
	, rpcserver_(loop, listenAddrRpc, "rpcServer")
	, httpserver_(loop, listenAddrHttp, "httpServer")
	, gateRpcClients_(loop)
	, thisTimer_(new muduo::net::EventLoopThread(std::bind(&LoginServ::threadInit, this), "EventLoopThreadTimer"))
	, server_state_(kRunning)
	, ipLocator_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	rpcserver_.registerService(&rpcservice_);
	server_.setConnectionCallback(
		std::bind(&LoginServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setConnectionCallback(
		std::bind(&LoginServ::onHttpConnection, this, std::placeholders::_1));
	httpserver_.setMessageCallback(
		std::bind(&LoginServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setWriteCompleteCallback(
		std::bind(&LoginServ::onHttpWriteComplete, this, std::placeholders::_1));
	rpcClients_[rpc::containTy::kRpcGateTy].clients_ = &gateRpcClients_;
	rpcClients_[rpc::containTy::kRpcGateTy].ty_ = rpc::containTy::kRpcGateTy;
	//添加OpenSSL认证支持 httpserver_&server_ 共享证书
	muduo::net::ssl::SSL_CTX_Init(
		cert_path,
		private_key_path,
		client_ca_cert_file_path, client_ca_cert_dir_path);
	//指定SSL_CTX
	server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
	httpserver_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
}

LoginServ::~LoginServ() {
	Quit();
}

void LoginServ::Quit() {
	thisTimer_->getLoop()->quit();
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
		Fatalf("error");
		abort();
		return false;
	}
	return true;
}

void LoginServ::onZookeeperConnected() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_login"))
		zkclient_->createNode("/GAME/game_login", "game_login"/*, true*/);
	//websocket
	std::vector<std::string> vec;
	boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
	nodeValue_ = internetIp_ + ":" + vec[1];
	path_handshake_ = "/ws_" + vec[1];
	server_ipport_ = internetIp_ + ":" + vec[1];
	//http
	boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));
	nodeValue_ += ":" + vec[1];
	httpserver_ipport_ = internetIp_ + ":" + vec[1];
	//rpc
	boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));
	nodeValue_ += ":" + vec[0] + ":" + vec[1];
	nodePath_ = "/GAME/game_login/" + nodeValue_;
	zkclient_->createNode(nodePath_, nodeValue_, true);
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&LoginServ::onGateWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		Warnf("可用网关服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcGateTy].add(names);
	}
}

void LoginServ::onGateWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&LoginServ::onGateWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		Warnf("可用网关服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcGateTy].process(names);
	}
}

void LoginServ::registerZookeeper() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_login"))
		zkclient_->createNode("/GAME/game_login", "game_login"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&LoginServ::registerZookeeper, this));
}

bool LoginServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		Fatalf("error");
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	redisClient_->startSubThread();
	return true;
}

bool LoginServ::InitMongoDB(std::string const& url) {
	MongoDBClient::initialize(url);
	return true;
}

void LoginServ::threadInit() {
	if (!REDISCLIENT.initRedisCluster(redisIpaddr_, redisPasswd_)) {
		Fatalf("initRedisCluster error");
	}
	std::string s;
	for (std::vector<std::string>::const_iterator it = redlockVec_.begin();
		it != redlockVec_.end(); ++it) {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, *it, boost::is_any_of(":"));
		REDISLOCK.AddServerUrl(vec[0].c_str(), atol(vec[1].c_str()), redisPasswd_);
		s += "\n" + vec[0];
		s += ":" + vec[1];
	}
	//Warnf("redisLock%s", s.c_str());
}

bool LoginServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessage();
		break;
	}
	return true;
}

void LoginServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();
	
	thisTimer_->startLoop();
	
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&LoginServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}
	
	Warnf("LoginServ = %s http:%s rpc:%s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), httpserver_.ipPort().c_str(), rpcserver_.ipPort().c_str(), numThreads, numWorkerThreads);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&LoginServ::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpserver_.setConditionCallback(std::bind(&LoginServ::onHttpCondition, this, std::placeholders::_1));
	}

	server_.start(true);
	rpcserver_.start(true);
	httpserver_.start(true);
	
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&LoginServ::registerZookeeper, this));
	
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