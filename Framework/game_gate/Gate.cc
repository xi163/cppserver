#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "Gate.h"

GateServ::GateServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrTcp,
	const muduo::net::InetAddress& listenAddrRpc,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
	: server_(loop, listenAddr, "wsServer")
	, tcpserver_(loop, listenAddrTcp, "tcpServer")
	, rpcserver_(loop, listenAddrRpc, "rpcServer")
	, httpserver_(loop, listenAddrHttp, "httpServer")
	, gateRpcClients_(loop)
	, gateClients_(loop)
	, hallClients_(loop)
	, gameClients_(loop)
	, idleTimeout_(3)
	, maxConnections_(15000)
	, server_state_(kRunning)
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
	, ipFinder_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	rpcserver_.registerService(&rpcservice_);
	server_.setConnectionCallback(
		std::bind(&GateServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	tcpserver_.setConnectionCallback(
		std::bind(&GateServ::onTcpConnection, this, std::placeholders::_1));
	tcpserver_.setMessageCallback(
		std::bind(&GateServ::onTcpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setConnectionCallback(
		std::bind(&GateServ::onHttpConnection, this, std::placeholders::_1));
	httpserver_.setMessageCallback(
		std::bind(&GateServ::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	httpserver_.setWriteCompleteCallback(
		std::bind(&GateServ::onHttpWriteComplete, this, std::placeholders::_1));
	gateClients_.setConnectionCallback(
		std::bind(&GateServ::onGateConnection, this, std::placeholders::_1));
	gateClients_.setMessageCallback(
		std::bind(&GateServ::onGateMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	clients_[containTy::kGateTy].clients_ = &gateClients_;
	clients_[containTy::kGateTy].ty_ = containTy::kGateTy;
	hallClients_.setConnectionCallback(
		std::bind(&GateServ::onHallConnection, this, std::placeholders::_1));
	hallClients_.setMessageCallback(
		std::bind(&GateServ::onHallMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	clients_[containTy::kHallTy].clients_ = &hallClients_;
	clients_[containTy::kHallTy].ty_ = containTy::kHallTy;
	gameClients_.setConnectionCallback(
		std::bind(&GateServ::onGameConnection, this, std::placeholders::_1));
	gameClients_.setMessageCallback(
		std::bind(&GateServ::onGameMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	clients_[containTy::kGameTy].clients_ = &gameClients_;
	clients_[containTy::kGameTy].ty_ = containTy::kGameTy;
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
	threadTimer_->startLoop();
}

GateServ::~GateServ() {
	Quit();
}

void GateServ::Quit() {
	hallClients_.closeAll();
	gameClients_.closeAll();
	gateRpcClients_.closeAll();
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

void GateServ::registerHandlers() {
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_REQ)] =
		std::bind(&GateServ::cmd_getAesKey, this, std::placeholders::_1, std::placeholders::_2);
}

bool GateServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&GateServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		_LOG_FATAL("error");
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&GateServ::registerZookeeper, this));
	return true;
}

void GateServ::onZookeeperConnected() {
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
		//websocket
		std::vector<std::string> vec;
		boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
		nodeValue_ = vec[0] + ":" + vec[1];
		path_handshake_ = "/ws_" + vec[1];
		//tcp
		boost::algorithm::split(vec, tcpserver_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1];
		//rpc
		boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1];
		//http
		boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1];
		nodePath_ = "/GAME/ProxyServers/" + nodeValue_;
		zkclient_->createNode(nodePath_, nodeValue_, true);
		invalidNodePath_ = "/GAME/ProxyServersInvalid/" + nodeValue_;
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/ProxyServers",
			names,
			std::bind(
				&GateServ::onGateWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用网关服列表%s", s.c_str());
			clients_[containTy::kGateTy].add(names, nodeValue_);
			rpcClients_[rpc::containTy::kRpcGateTy].add(names, nodeValue_);
		}
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/HallServers",
			names,
			std::bind(
				&GateServ::onHallWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用大厅服列表%s", s.c_str());
			clients_[containTy::kHallTy].add(names);
		}
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/GameServers",
			names,
			std::bind(
				&GateServ::onGameWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用游戏服列表%s", s.c_str());
			clients_[containTy::kGameTy].add(names);
		}
	}
}

void GateServ::onGateWatcher(
	int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/ProxyServers",
		names,
		std::bind(
			&GateServ::onHallWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用网关服列表%s", s.c_str());
		clients_[containTy::kGateTy].process(names, nodeValue_);
		rpcClients_[rpc::containTy::kRpcGateTy].process(names, nodeValue_);
	}
}

void GateServ::onHallWatcher(
	int type, int state, const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/HallServers",
		names,
		std::bind(
			&GateServ::onHallWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用大厅服列表%s", s.c_str());
		clients_[containTy::kHallTy].process(names);
	}
}

void GateServ::onGameWatcher(
	int type, int state, const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/GameServers",
		names,
		std::bind(
			&GateServ::onGameWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用游戏服列表%s", s.c_str());
		clients_[containTy::kGameTy].process(names);
	}
}

void GateServ::registerZookeeper() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/ProxyServers"))
		zkclient_->createNode("/GAME/ProxyServers", "ProxyServers"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		_LOG_INFO(nodePath_.c_str());
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	threadTimer_->getLoop()->runAfter(5.0f, std::bind(&GateServ::registerZookeeper, this));
}

bool GateServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		_LOG_FATAL("error");
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	//跨网关顶号处理(异地登陆)
	redisClient_->subscribeUserLoginMessage(std::bind(&GateServ::onUserLoginNotify, this, std::placeholders::_1));
	//跑马灯通告消息
	redisClient_->subscribePublishMsg(1, CALLBACK_1(GateServ::onMarqueeNotify, this));
	//redisClient_->subscribePublishMsg(0, [&](std::string const& msg) {
	//	threadTimer_->getLoop()->runAfter(10, CALLBACK_1(GateServ::onLuckPushNotify, this, msg));
	//	});
	redisClient_->startSubThread();
	return true;
}

bool GateServ::InitMongoDB(std::string const& url) {
#if 0
	//http://mongocxx.org/mongocxx-v3/tutorial/
	_LOG_INFO("%s", url.c_str());
	mongocxx::instance instance{};
	//mongoDBUrl_ = url;
	//http://mongocxx.org/mongocxx-v3/tutorial/
	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	MongoDBClient::ThreadLocalSingleton::setUri(url);
#endif
	return true;
}

#if 0
static __thread mongocxx::database* dbgamemain_;
#endif

void GateServ::threadInit() {
	if (!REDISCLIENT.initRedisCluster(redisIpaddr_, redisPasswd_)) {
		_LOG_FATAL("initRedisCluster error");
	}
#if 0
	static __thread mongocxx::database db = MONGODBCLIENT["gamemain"];
	dbgamemain_ = &db;
#endif
#if 0
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
#endif
}

bool GateServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessageID();
		break;
	}
	return true;
}

void GateServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();

	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&GateServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	std::vector<std::string> vec;
	boost::algorithm::split(vec, nodeValue_, boost::is_any_of(":"));

	_LOG_TRACE("GateSrv = %s tcp:%s rpc:%s http:%s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), vec[2].c_str(), vec[3].c_str(), vec[4].c_str(), numThreads, numWorkerThreads);

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == eApiCtrl::kOpenAccept) {
		server_.setConditionCallback(std::bind(&GateServ::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		httpserver_.setConditionCallback(std::bind(&GateServ::onHttpCondition, this, std::placeholders::_1));
	}

	server_.start(true);
	tcpserver_.start(true);
	rpcserver_.start(true);
	httpserver_.start(true);

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