
#include "Api.h"

ApiServ::ApiServ(muduo::net::EventLoop* loop,
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
	, thisTimer_(new muduo::net::EventLoopThread(std::bind(&ApiServ::threadInit, this), "ThreadTimer"))
	, idleTimeout_(3)
	, isdecrypt_(false)
	, whiteListControl_(eApiCtrl::kClose)
	, maxConnections_(15000)
#ifdef _STAT_ORDER_QPS_
	, deltaTime_(10)
#endif
	, server_state_(kRunning)
	, ttlUserLock_(1000)
	, ttlAgentLock_(500)
	, ttlExpired_(30 * 60)
	, ipLocator_("qqwry.dat") {
	registerHandlers();
	muduo::net::EventLoopThreadPool::Singleton::init(loop, "IOThread");
	rpcserver_.setConditionCallback(
		std::bind(&ApiServ::onRpcCondition, this, std::placeholders::_1, std::placeholders::_2));
	rpcserver_.registerService(&rpcservice_);
	server_.setConditionCallback(
		std::bind(&ApiServ::onCondition, this, std::placeholders::_1, std::placeholders::_2));
	server_.setConnectionCallback(
		std::bind(&ApiServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setConditionCallback(
		std::bind(&ApiServ::onHttpCondition, this, std::placeholders::_1, std::placeholders::_2));
	httpserver_.setConnectionCallback(
		std::bind(&ApiServ::onHttpConnection, this, std::placeholders::_1));
	httpserver_.setMessageCallback(
		std::bind(&ApiServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setWriteCompleteCallback(
		std::bind(&ApiServ::onHttpWriteComplete, this, std::placeholders::_1));
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

ApiServ::~ApiServ() {
	Quit();
}

void ApiServ::Quit() {
	gateRpcClients_.closeAll();
	thisTimer_->getLoop()->quit();
	for (size_t i = 0; i < threadPool_.size(); ++i) {
		threadPool_[i]->stop();
	}
	if (zkclient_) {
		zkclient_->closeServer();
	}
	if (redisClient_) {
		redisClient_->unsubscribe();
	}
	muduo::net::EventLoopThreadPool::Singleton::quit();
	server_.getLoop()->quit();
	google::protobuf::ShutdownProtobufLibrary();
}

void ApiServ::registerHandlers() {
// 	handlers_[packet::enword(
// 		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
// 		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)]
// 		= std::bind(&ApiServ::cmd_on_user_login, this,
// 			std::placeholders::_1, std::placeholders::_2);
}

bool ApiServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&ApiServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		Fatalf("error");
		abort();
		return false;
	}
	return true;
}

void ApiServ::onZookeeperConnected() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_api"))
		zkclient_->createNode("/GAME/game_api", "game_api"/*, true*/);
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
	nodePath_ = "/GAME/game_api/" + nodeValue_;
	zkclient_->createNode(nodePath_, nodeValue_, true);
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&ApiServ::onGateWatcher, this,
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

void ApiServ::onGateWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&ApiServ::onGateWatcher, this,
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

void ApiServ::registerZookeeper() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_api"))
		zkclient_->createNode("/GAME/game_api", "game_api"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&ApiServ::registerZookeeper, this));
}

bool ApiServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
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

bool ApiServ::InitMongoDB(std::string const& url) {
	MongoDBClient::initialize(url);
	return true;
}

void ApiServ::threadInit() {
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

bool ApiServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessage();
		break;
	}
	return true;
}

void ApiServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::EventLoopThreadPool::Singleton::setThreadNum(numThreads);
	muduo::net::EventLoopThreadPool::Singleton::start();
	
	thisTimer_->startLoop();
	
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&ApiServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}
	
	Warnf("ApiServ = %s http:%s rpc:%s numThreads: I/O = %d worker = %d et[%d]", server_.ipPort().c_str(), httpserver_.ipPort().c_str(), rpcserver_.ipPort().c_str(), numThreads, numWorkerThreads, et_);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&ApiServ::onCondition, this, std::placeholders::_1, std::placeholders::_2));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpserver_.setConditionCallback(std::bind(&ApiServ::onHttpCondition, this, std::placeholders::_1, std::placeholders::_2));
	}

	server_.start(et_);
	rpcserver_.start(et_);
	httpserver_.start(et_);
	
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&ApiServ::registerZookeeper, this));
	
	//sleep(2);

	std::vector<muduo::net::EventLoop*> loops;
	muduo::net::EventLoopThreadPool::Singleton::getAllLoops(loops);
	for (std::vector<muduo::net::EventLoop*>::const_iterator it = loops.begin();
		it != loops.end(); ++it) {
		(*it)->setContext(Buckets(*it, idleTimeout_, interval_));
		(*it)->runAfter(interval_, std::bind(&Buckets::pop, &boost::any_cast<Buckets&>((*it)->getContext())));
	}
	thisTimer_->getLoop()->runAfter(3, std::bind(&ApiServ::refreshAgentInfo, this));
	thisTimer_->getLoop()->runAfter(3, std::bind(&ApiServ::refreshWhiteList, this));
}

void ApiServ::cmd_on_user_login(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {

}