
#include "Api.h"

ApiServ::ApiServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
	: server_(loop, listenAddr, "ApiServ")
	, httpServer_(loop, listenAddrHttp, "httpServer")
	, gateRpcClients_(loop)
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
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
	, ipFinder_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&ApiServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpServer_.setConnectionCallback(
		std::bind(&ApiServ::onHttpConnection, this, std::placeholders::_1));
	httpServer_.setMessageCallback(
		std::bind(&ApiServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpServer_.setWriteCompleteCallback(
		std::bind(&ApiServ::onHttpWriteComplete, this, std::placeholders::_1));
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

ApiServ::~ApiServ() {
	Quit();
}

void ApiServ::Quit() {
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
		_LOG_FATAL("error");
		abort();
		return false;
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&ApiServ::registerZookeeper, this));
	return true;
}

void ApiServ::onZookeeperConnected() {
	std::vector<std::string> vec;
	boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
	path_handshake_ = "/ws_" + vec[1];
	//网关服RPC ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/RPCProxyServers",
		names,
		std::bind(
			&ApiServ::onGateRpcWatcher, this,
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

void ApiServ::onGateRpcWatcher(int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	//网关服RPC ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/RPCProxyServers",
		names,
		std::bind(
			&ApiServ::onGateRpcWatcher, this,
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

void ApiServ::registerZookeeper() {

}

bool ApiServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
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

bool ApiServ::InitMongoDB(std::string const& url) {
	//http://mongocxx.org/mongocxx-v3/tutorial/
	_LOG_INFO(url.c_str());
	mongocxx::instance instance{};
	//mongoDBUrl_ = url;
	//http://mongocxx.org/mongocxx-v3/tutorial/
	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	MongoDBClient::ThreadLocalSingleton::setUri(url);
	return true;
}

static __thread mongocxx::database* dbgamemain_;

void ApiServ::threadInit() {
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
	_LOG_WARN("redisLock%s", s.c_str());
}

bool ApiServ::InitServer() {
	initTraceMessageID();
	return true;
}

void ApiServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();

	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&ApiServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	_LOG_INFO("ApiServ = %s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), numThreads, numWorkerThreads);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&ApiServ::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpServer_.setConditionCallback(std::bind(&ApiServ::onHttpCondition, this, std::placeholders::_1));
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
	threadTimer_->getLoop()->runAfter(3, std::bind(&ApiServ::refreshAgentInfo, this));
	threadTimer_->getLoop()->runAfter(3, std::bind(&ApiServ::refreshWhiteList, this));
}

void ApiServ::cmd_on_user_login(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {

}