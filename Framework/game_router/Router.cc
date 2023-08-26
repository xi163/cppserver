
#include "Router.h"

RouterServ::RouterServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
	: server_(loop, listenAddr, "wsServer")
	, httpserver_(loop, listenAddrHttp, "httpServer")
	, loginRpcClients_(loop)
	, apiRpcClients_(loop)
	, gateRpcClients_(loop)
	, thisTimer_(new muduo::net::EventLoopThread(std::bind(&RouterServ::threadInit, this), "EventLoopThreadTimer"))
	, server_state_(kRunning)
	, ipLocator_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&RouterServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setConnectionCallback(
		std::bind(&RouterServ::onHttpConnection, this, std::placeholders::_1));
	httpserver_.setMessageCallback(
		std::bind(&RouterServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setWriteCompleteCallback(
		std::bind(&RouterServ::onHttpWriteComplete, this, std::placeholders::_1));
	rpcClients_[rpc::containTy::kRpcLoginTy].clients_ = &loginRpcClients_;
	rpcClients_[rpc::containTy::kRpcLoginTy].ty_ = rpc::containTy::kRpcLoginTy;
	rpcClients_[rpc::containTy::kRpcApiTy].clients_ = &apiRpcClients_;
	rpcClients_[rpc::containTy::kRpcApiTy].ty_ = rpc::containTy::kRpcApiTy;
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

RouterServ::~RouterServ() {
	Quit();
}

void RouterServ::Quit() {
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

void RouterServ::registerHandlers() {
// 	handlers_[packet::enword(
// 		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
// 		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ)]
// 		= std::bind(&RouterServ::cmd_on_user_login, this,
// 			std::placeholders::_1, std::placeholders::_2);
}

bool RouterServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&RouterServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		_LOG_FATAL("error");
		abort();
		return false;
	}
	return true;
}

void RouterServ::onZookeeperConnected() {
	//websocket
	std::vector<std::string> vec;
	boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
	nodeValue_ = vec[0] + ":" + vec[1];
	path_handshake_ = "/ws_" + vec[1];
	//http
	boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));
	nodeValue_ += ":" + vec[1];
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/game_login",
			names,
			std::bind(
				&RouterServ::onLoginWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用登陆服列表%s", s.c_str());
			rpcClients_[rpc::containTy::kRpcLoginTy].add(names);
		}
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/game_api",
			names,
			std::bind(
				&RouterServ::onApiWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用API服列表%s", s.c_str());
			rpcClients_[rpc::containTy::kRpcApiTy].add(names);
		}
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/game_gate",
			names,
			std::bind(
				&RouterServ::onGateWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用网关服列表%s", s.c_str());
			rpcClients_[rpc::containTy::kRpcGateTy].add(names);
		}
	}
}

void RouterServ::onLoginWatcher(
	int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_login",
		names,
		std::bind(
			&RouterServ::onLoginWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用登陆服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcLoginTy].process(names);
	}
}

void RouterServ::onApiWatcher(
	int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_api",
		names,
		std::bind(
			&RouterServ::onApiWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用API服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcApiTy].process(names);
	}
}

void RouterServ::onGateWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&RouterServ::onGateWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用网关服列表%s", s.c_str());
		rpcClients_[rpc::containTy::kRpcGateTy].process(names);
	}
}

void RouterServ::registerZookeeper() {

}

bool RouterServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
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

bool RouterServ::InitMongoDB(std::string const& url) {
	MongoDBClient::initialize(url);
	return true;
}

void RouterServ::threadInit() {
	if (!REDISCLIENT.initRedisCluster(redisIpaddr_, redisPasswd_)) {
		_LOG_FATAL("initRedisCluster error");
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
	//_LOG_WARN("redisLock%s", s.c_str());
}

bool RouterServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessageID();
		break;
	}
	return true;
}

void RouterServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();
	
	thisTimer_->startLoop();
	
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&RouterServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	std::vector<std::string> vec;
	boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));

	_LOG_WARN("RouterServ = %s http:%s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), vec[1].c_str(), numThreads, numWorkerThreads);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&RouterServ::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpserver_.setConditionCallback(std::bind(&RouterServ::onHttpCondition, this, std::placeholders::_1));
	}

	server_.start(true);
	httpserver_.start(true);

	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&RouterServ::registerZookeeper, this));

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

void RouterServ::cmd_on_user_login(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {

}