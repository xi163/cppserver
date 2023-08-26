
#include "Hall.h"
#include "public/mgoOperation.h"
#include "public/redisKeys.h"
#include "public/mgoKeys.h"
#include "rpc/client/RoomNodes.h"

HallServ::HallServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrRpc)
	: server_(loop, listenAddr, "tcpServer")
	, rpcserver_(loop, listenAddrRpc, "rpcServer")
	, gameRpcClients_(loop)
	, thisTimer_(new muduo::net::EventLoopThread(std::bind(&HallServ::threadInit, this), "EventLoopThreadTimer"))
	, ipLocator_("qqwry.dat") {
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	server_.setConnectionCallback(
		std::bind(&HallServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&HallServ::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	rpcClients_[rpc::containTy::kRpcGameTy].clients_ = &gameRpcClients_;
	rpcClients_[rpc::containTy::kRpcGameTy].ty_ = rpc::containTy::kRpcGameTy;
}

HallServ::~HallServ() {
	Quit();
}

void HallServ::Quit() {
	gameRpcClients_.closeAll();
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
	/// 获取所有游戏列表
	/// </summary>
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_REQ)]
		= std::bind(&HallServ::cmd_get_game_info, this,
			std::placeholders::_1, std::placeholders::_2);
	/// <summary>
	/// 查询正在玩的游戏/游戏服务器IP
	/// </summary>
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_REQ)]
		= std::bind(&HallServ::cmd_get_playing_game_info, this,
			std::placeholders::_1, std::placeholders::_2);
	/// <summary>
	/// 查询指定游戏节点/游戏服务器IP
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
// 	handlers_[packet::enword(
// 		::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER,
// 		::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER)]
// 		= std::bind(&HallServ::cmd_repair_hallserver, this,
// 			std::placeholders::_1, std::placeholders::_2);
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


	//===================俱乐部==================
	
	// 获取俱乐部房间信息
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_ROOM_INFO_MESSAGE_REQ)]
		= std::bind(&HallServ::GetRoomInfoMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取我的俱乐部
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_CLUB_HALL_MESSAGE_REQ)]
		= std::bind(&HallServ::GetMyClubHallMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 创建俱乐部
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_CREATE_CLUB_MESSAGE_REQ)]
		= std::bind(&HallServ::CreateClubMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 加入俱乐部
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_JOIN_THE_CLUB_MESSAGE_REQ)]
		= std::bind(&HallServ::JoinTheClubMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 退出俱乐部 
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_EXIT_THE_CLUB_MESSAGE_REQ)]
		= std::bind(&HallServ::ExitTheClubMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 设置是否开启自动成为合伙人
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_SET_AUTO_BECOME_PARTNER_MESSAGE_REQ)]
		= std::bind(&HallServ::SetAutoBecomePartnerMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 成为合伙人
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_BECOME_PARTNER_MESSAGE_REQ)]
		= std::bind(&HallServ::BecomePartnerMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 佣金兑换金币
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_EXCHANGE_MY_REVENUE_REQ)]
		= std::bind(&HallServ::ExchangeRevenueMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取佣金提取记录 
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_EXCHANGE_MY_REVENUE_RECORD_REQ)]
		= std::bind(&HallServ::GetExchangeRevenueRecordMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取我的业绩
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_REQ)]
		= std::bind(&HallServ::GetMyAchievementMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取会员业绩详情
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_MEMBER_REQ)]
		= std::bind(&HallServ::GetAchievementDetailMemberMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取合伙人业绩详情
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_PARTNER_REQ)]
		= std::bind(&HallServ::GetAchievementDetailPartnerMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取俱乐部内我的团队我的俱乐部
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_CLUB_REQ)]
		= std::bind(&HallServ::GetMyClubMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取我的团队成员
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_TEAM_REQ)]
		= std::bind(&HallServ::GetMyTeamMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 设置下一级合伙人提成比例
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_SET_SUBORDINATE_RATE_REQ)]
		= std::bind(&HallServ::SetSubordinateRateMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取游戏明细记录
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_PLAY_RECORD_REQ)]
		= std::bind(&HallServ::GetPlayRecordMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取玩家俱乐部所有游戏记录列表
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_ALL_PLAY_RECORD_REQ)]
		= std::bind(&HallServ::GetAllPlayRecordMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取玩家账户明细列表
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_USER_SCORE_CHANGE_RECORD_REQ)]
		= std::bind(&HallServ::GetUserScoreChangeRecordMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 我的上级信息
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_CLUB_PROMOTER_REQ)]
		= std::bind(&HallServ::GetClubPromoterInfoMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 开除此用户
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_FIRE_MEMBER_REQ)]
		= std::bind(&HallServ::FireMemberMessage_club, this,
			std::placeholders::_1, std::placeholders::_2);
	// 获取俱乐部申请QQ
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB,
		::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_APPLY_CLUB_QQ_REQ)]
		= std::bind(&HallServ::GetApplyClubQQMessage_club, this,
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
	return true;
}

void HallServ::onZookeeperConnected() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_hall"))
		zkclient_->createNode("/GAME/game_hall", "game_hall"/*, true*/);
	//if (ZNONODE == zkclient_->existsNode("/GAME/game_hallInvalid"))
	//	zkclient_->createNode("/GAME/game_hallInvalid", "game_hallInvalid", true);
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
		nodePath_ = "/GAME/game_hall/" + nodeValue_;
		zkclient_->createNode(nodePath_, nodeValue_, true);
		invalidNodePath_ = "/GAME/game_hallInvalid/" + nodeValue_;
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/game_gate",
			names,
			std::bind(
				&HallServ::onGateWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
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
			"/GAME/game_serv",
			names,
			std::bind(
				&HallServ::onGameWatcher, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5), this)) {
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
		"/GAME/game_gate",
		names,
		std::bind(
			&HallServ::onGateWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
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
		"/GAME/game_serv",
		names,
		std::bind(
			&HallServ::onGameWatcher, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5), this)) {
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
	if (ZNONODE == zkclient_->existsNode("/GAME/game_hall"))
		zkclient_->createNode("/GAME/game_hall", "game_hall"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&HallServ::registerZookeeper, this));
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
		CALLBACK_1([&](std::string msg) {
			loadGameRoomInfos();
		}));
	redisClient_->startSubThread();
	return true;
}

bool HallServ::InitMongoDB(std::string const& url) {
	MongoDBClient::initialize(url);
	return true;
}

void HallServ::threadInit() {
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
	
	thisTimer_->startLoop();
	
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&HallServ::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	std::vector<std::string> vec;
	boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));

	_LOG_WARN("HallServ = %s rpc:%s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), vec[1].c_str(), numThreads, numWorkerThreads);

	server_.start(true);

	thisTimer_->getLoop()->runAfter(5.0f, OBJ_CALLBACK_0(this, &HallServ::registerZookeeper));
	thisTimer_->getLoop()->runEvery(3.0f, CALLBACK_0([&]() {
		loadGameRoomInfos();
	}));
	thisTimer_->getLoop()->runEvery(3.0f, OBJ_CALLBACK_0(this, &HallServ::redis_refresh_room_player_nums));
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
		//case Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_CLUB:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL_FRIEND:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL:
		case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL: {
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
				ipLocator_.GetAddressByIp(ntohl(pre_header_->clientIp), location, country);
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
							_LOG_ERROR("uuid:%s passwd:%s", uuid.c_str(), passwd.c_str());
							
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
							_LOG_DEBUG("%d:%s:%s LOGIN SERVER OK! ip: %s %s gateIp: %s session: %s",
								userId,
								info.account.c_str(),
								info.nickName.c_str(),
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
/// 获取所有游戏列表
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
		switch (reqdata.type()) {
		case GameMode::kGoldCoin:
		case GameMode::kFriendRoom: {
			READ_LOCK(gameinfo_mutex_[reqdata.type()]);
			rspdata.CopyFrom(gameinfo_[reqdata.type()]);
			rspdata.mutable_header()->set_sign(PROTOBUF_SIGN);
			rspdata.set_type(reqdata.type());
			rspdata.set_retcode(0);
			rspdata.set_errormsg("Get Game Message OK!");
			break;
		}
		case GameMode::kClub: {
			std::set<uint32_t> vec;
			{
				READ_LOCK(mutexGamevisibility_);
				std::map<int64_t, std::set<uint32_t>>::const_iterator it = mapClubvisibility_.find(reqdata.clubid());
				if (it != mapClubvisibility_.end()) {
					//std::copy(it->second.begin(), it->second.end(), vec.begin());
					for (std::set<uint32_t>::const_iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
						vec.insert(*ir);
					}
				}
			}
			for (std::set<uint32_t>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
				//_LOG_INFO("%d 对俱乐部 %d 可见", *it, reqdata.clubid());
			}
			if (vec.empty()) {
				//_LOG_ERROR("无private游戏对俱乐部 %d 可见", reqdata.clubid());
			}
			READ_LOCK(gameinfo_mutex_[reqdata.type()]);
			rspdata.mutable_header()->set_sign(PROTOBUF_SIGN);
			rspdata.set_type(reqdata.type());
			rspdata.set_retcode(0);
			rspdata.set_errormsg("Get Game Message OK!");
			if (gameinfo_[reqdata.type()].gamemessage_size() == 0) {
				_LOG_ERROR("%s 游戏列表为空", getModeStr(reqdata.type()).c_str());
			}
			for (int i = 0; i < gameinfo_[reqdata.type()].gamemessage_size(); ++i) {
				switch (gameinfo_[reqdata.type()].gamemessage(i).gameprivate()) {
					//0-全部可见 1-私有 针对指定俱乐部可见
				case 0:
					rspdata.add_gamemessage()->CopyFrom(gameinfo_[reqdata.type()].gamemessage(i));
					break;
				default:
				case 1: {
					
					std::set<uint32_t>::const_iterator it = vec.find(gameinfo_[reqdata.type()].gamemessage(i).gameid());
					if (it != vec.end()) {
						//_LOG_WARN("%d 对俱乐部 %d 可见", gameinfo_[reqdata.type()].gamemessage(i).gameid(), reqdata.clubid());
						rspdata.add_gamemessage()->CopyFrom(gameinfo_[reqdata.type()].gamemessage(i));
					}
					else {
						//_LOG_ERROR("%d 对俱乐部 %d 不可见", gameinfo_[reqdata.type()].gamemessage(i).gameid(), reqdata.clubid());
					}
					break;
				}
				}
			}
			break;
		}
		default:
			rspdata.mutable_header()->CopyFrom(reqdata.header());
			rspdata.set_type(reqdata.type());
			rspdata.set_retcode(-1);
			rspdata.set_errormsg("Get Game type error!");
			break;
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_RES,
			pre_header_, header_);
	}
}

/// <summary>
/// 查询正在玩的游戏/游戏服务器IP
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
/// 查询指定游戏节点/游戏服务器IP
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
		rspdata.set_type(reqdata.type());
		rspdata.set_gameid(reqdata.gameid());
		rspdata.set_roomid(reqdata.roomid());
		rspdata.set_clubid(reqdata.clubid());
		rspdata.set_tableid(reqdata.tableid());
		rspdata.set_servid(reqdata.servid());
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
			std::string ipport;
			switch (reqdata.type())
			{
			case kGoldCoin:
				room::nodes::balance_server(kGoldCoin, gameid, roomid, ipport);
				if (ipport.empty()) {
					rspdata.set_retcode(ERROR_ENTERROOM_GAMENOTEXIST);
					rspdata.set_errormsg("Game Server Not found!!!");
				}
				break;
			case kFriendRoom:
				rspdata.set_retcode(ERROR_ENTERROOM_GAMENOTEXIST);
				rspdata.set_errormsg("Game Server Not found!!!");
				break;
			case kClub: {
				if (reqdata.clubid() > 0) {
					if (mgo::opt::Count(
						mgoKeys::db::GAMEMAIN,
						mgoKeys::tbl::GAME_CLUB_MEMBER,
						document{} << "userid" << int64_t{ pre_header_->userId } << "clubid" << b_int64{ reqdata.clubid() } << finalize, 1) > 0) {
						if (!reqdata.servid().empty() && reqdata.tableid() >= 0) {
							if (room::nodes::validate_server(kClub, reqdata.gameid(), reqdata.roomid(), ipport, reqdata.servid(), reqdata.tableid(), reqdata.clubid())) {
							}
							else {
								rspdata.set_retcode(1);
								rspdata.set_errormsg("Get Game Server IP Err: validate");
							}
						}
						else {
							bool is_private = false;
							bool is_visible = false;
							{
								READ_LOCK(gameinfo_mutex_[reqdata.type()]);
								for (int i = 0; i < gameinfo_[reqdata.type()].gamemessage_size(); ++i) {
									if (gameinfo_[reqdata.type()].gamemessage(i).gameid()) {
										is_private = gameinfo_[reqdata.type()].gamemessage(i).gameprivate();
										break;
									}
								}
							}
							if (is_private) {
								READ_LOCK(mutexGamevisibility_);
								std::map<int64_t, std::set<uint32_t>>::const_iterator it = mapClubvisibility_.find(reqdata.clubid());
								if (it != mapClubvisibility_.end()) {
									std::set<uint32_t>::const_iterator ir = it->second.find(reqdata.gameid());
									is_visible = ir != it->second.end();
								}
							}
							if (is_private && !is_visible) {
								rspdata.set_retcode(5);
								rspdata.set_errormsg("Game Server Not found!!!");
							}
							else {
								room::nodes::balance_server(kClub, gameid, roomid, ipport);
								if (ipport.empty()) {
									rspdata.set_retcode(5);
									rspdata.set_errormsg("Game Server Not found!!!");
								}
							}
						}
					}
					else {
						rspdata.set_retcode(2);
						rspdata.set_errormsg("Get Game Server IP Err: isNot club member or invalid clubid");
					}
				}
				else {
					rspdata.set_retcode(3);
					rspdata.set_errormsg("Get Game Server IP Err: clubId is 0");
				}
				break;
			}
			default:
				rspdata.set_retcode(4);
				rspdata.set_errormsg("Get Game Server IP Err: type invalid");
			}
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

void HallServ::loadGameRoomInfos() {
	MY_TRY()
		int index = RANDOM().betweenInt(0, threadPool_.size() - 1).randInt_re();
	if (index >= 0 && index < threadPool_.size()) {
		threadPool_[index]->run(CALLBACK_0([&]() {
			{
				WRITE_LOCK(gameinfo_mutex_[kGoldCoin]);
				mgo::LoadGameRoomInfos(gameinfo_[kGoldCoin]);
			}
			{
				WRITE_LOCK(mutexGamevisibility_);
				mgo::LoadClubGamevisibility(mapGamevisibility_, mapClubvisibility_);
			}
			{
				WRITE_LOCK(gameinfo_mutex_[kClub]);
				mgo::LoadGameClubRoomInfos(gameinfo_[kClub]);
			}
			
		}));
	}
	MY_CATCH()
}


void HallServ::redis_refresh_room_player_nums() {
	static STD::Random r(0, threadPool_.size() - 1);
	int index = r.randInt_re();
	assert(index >= 0 && index < threadPool_.size());
	threadPool_[index]->run(std::bind(&HallServ::redis_update_room_player_nums, this));
	thisTimer_->getLoop()->runAfter(30, std::bind(&HallServ::redis_refresh_room_player_nums, this));
}

void HallServ::redis_update_room_player_nums() {
	WRITE_LOCK(room_playernums_mutex_);
// 	room_playernums_.clear_header();
// 	room_playernums_.clear_gameplayernummessage();
// 	auto& gameMessage = gameinfo_.gamemessage();
// 	try {
// 		//各个子游戏
// 		for (auto& gameinfo : gameMessage) {
// 			::HallServer::SimpleGameMessage* simpleMessage = room_playernums_.add_gameplayernummessage();
// 			uint32_t gameid = gameinfo.gameid();
// 			simpleMessage->set_gameid(gameid);
// 			auto& gameroommsg = gameinfo.gameroommsg();
// 			//各个子游戏房间
// 			for (auto& roominfo : gameroommsg) {
// 				::HallServer::RoomPlayerNum* roomPlayerNum = simpleMessage->add_roomplayernum();
// 				uint32_t roomid = roominfo.roomid();
// 				//redis获取房间人数
// 				uint64_t playerNums = 0;
// 				//if (room_servers_.find(roomid) != room_servers_.end()) {
// 				//	REDISCLIENT.GetGameServerplayerNum(room_servers_[roomid], playerNums);
// 				//}
// 				//更新房间游戏人数
// 				roomPlayerNum->set_roomid(roomid);
// 				roomPlayerNum->set_playernum(playerNums);
// 				const_cast<::HallServer::GameRoomMessage&>(roominfo).set_playernum(playerNums);
// 				//_LOG_DEBUG("roomId=%d playerCount=%d", roomid, playerNums);
// 			}
// 		}
// 	}
// 	catch (std::exception& e) {
// 		_LOG_ERROR(e.what());
// 	}
}

//===================俱乐部==================

// 获取俱乐部房间信息
void HallServ::GetRoomInfoMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetRoomInfoMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetRoomInfoMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		if (reqdata.clubid() > 0) {
			if (reqdata.gameid() > 0) {
				if (reqdata.roomid() > 0) {
					::club::game::room::info info;
					room::nodes::get_club_room_info(kClub, reqdata.clubid(), reqdata.gameid(), reqdata.roomid(), info);
					::club::info* clubinfo = rspdata.mutable_info();
					clubinfo->set_clubid(reqdata.clubid());
					::club::game::info* gameinfo = clubinfo->add_games();
					gameinfo->set_gameid(reqdata.gameid());
					::club::game::room::info* roominfo = gameinfo->add_rooms();
					if (info.roomid() > 0) {
						roominfo->CopyFrom(info);
					}
					else {
						roominfo->set_roomid(reqdata.roomid());
					}
				}
				else {
					::club::game::info info;
					room::nodes::get_club_room_info(kClub, reqdata.clubid(), reqdata.gameid(), info);
					::club::info* clubinfo = rspdata.mutable_info();
					clubinfo->set_clubid(reqdata.clubid());
					::club::game::info* gameinfo = clubinfo->add_games();
					if (info.gameid() > 0) {
						gameinfo->CopyFrom(info);
					}
					else {
						gameinfo->set_gameid(reqdata.gameid());
					}
				}
			}
			else {
				::club::info info;
				room::nodes::get_club_room_info(kClub, reqdata.clubid(), info);
				if (info.clubid() > 0) {
					rspdata.mutable_info()->CopyFrom(info);
				}
				else {
					rspdata.mutable_info()->set_clubid(reqdata.clubid());
				}
			}
		}
		send(conn, &rspdata,
			::Game::Common::CLIENT_TO_HALL_CLUB_GET_ROOM_INFO_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 获取我的俱乐部
void HallServ::GetMyClubHallMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetMyClubHallMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetMyClubHallMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		std::vector<UserClubInfo> infos;
		if (!mgo::LoadUserClubs(pre_header_->userId, infos)) {
			rspdata.set_retcode(-1);
			rspdata.set_errormsg("获取我的俱乐部失败");
		}
		else {
			for (std::vector<UserClubInfo>::iterator it = infos.begin();
				it != infos.end(); ++it) {
				rspdata.set_userid(pre_header_->userId);
				::ClubHallServer::ClubHallInfo* info = rspdata.add_clubinfos();
				info->set_clubid(it->clubId);
				info->set_clubname(it->clubName);
				info->set_clubiconid(it->iconId);
				info->set_promoterid(it->promoterId);
				info->set_status(it->status);
				info->set_invitationcode(it->invitationCode);
				info->set_clubplayernum(it->playerNum);
				info->set_rate(it->ratio);
				info->set_autobcpartnerrate(it->autoPartnerRatio);
				info->set_url(it->url);
				info->set_createtime(it->joinTime.format());
			}
			rspdata.set_retcode(0);
			rspdata.set_errormsg("获取我的俱乐部成功");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_CLUB_HALL_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 创建俱乐部
void HallServ::CreateClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::CreateClubMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::CreateClubMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
 		UserClubInfo info;
		Msg const& errmsg = mgo::CreateClub(pre_header_->userId, reqdata.clubname(), reqdata.rate(), reqdata.autobcpartnerrate(), info);
		if (errmsg.code == Ok.code) {
			rspdata.set_userid(pre_header_->userId);
			rspdata.set_retcode(Ok.code);
			rspdata.set_errormsg("创建俱乐部成功");
		}
		else {
			rspdata.set_retcode(errmsg.code);
			rspdata.set_errormsg(errmsg.errmsg());
		}
		if (info.clubId > 0) {
			rspdata.mutable_clubinfo()->set_clubid(info.clubId);
			rspdata.mutable_clubinfo()->set_clubname(info.clubName);
			rspdata.mutable_clubinfo()->set_clubiconid(info.iconId);
			rspdata.mutable_clubinfo()->set_promoterid(info.promoterId);
			rspdata.mutable_clubinfo()->set_status(info.status);
			rspdata.mutable_clubinfo()->set_invitationcode(info.invitationCode);
			rspdata.mutable_clubinfo()->set_clubplayernum(info.playerNum);
			rspdata.mutable_clubinfo()->set_rate(info.ratio);
			rspdata.mutable_clubinfo()->set_autobcpartnerrate(info.autoPartnerRatio);
			rspdata.mutable_clubinfo()->set_url(info.url);
			rspdata.mutable_clubinfo()->set_createtime(info.joinTime.format());
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_CREATE_CLUB_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 加入俱乐部
void HallServ::JoinTheClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::JoinTheClubMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::JoinTheClubMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		UserClubInfo info;
		if (reqdata.invitationcode() > 0) {
			//用户通过邀请码加入
			int64_t clubId = 0;
			Msg const& errmsg = mgo::JoinClub(clubId, reqdata.invitationcode(), pre_header_->userId);
			if (errmsg.code == Ok.code) {
				mgo::LoadUserClub(pre_header_->userId, clubId, info);
				rspdata.set_userid(pre_header_->userId);
				rspdata.set_retcode(Ok.code);
				rspdata.set_errormsg("欢迎您成为俱乐部一员");
			}
			else if (errmsg.code == ERR_JoinClub_UserAlreadyInClub.code) {
				mgo::LoadUserClub(pre_header_->userId, clubId, info);
				rspdata.set_userid(pre_header_->userId);
				rspdata.set_retcode(Ok.code);
				rspdata.set_errormsg(errmsg.errmsg());
			}
			else {
				rspdata.set_retcode(errmsg.code);
				rspdata.set_errormsg(errmsg.errmsg());
			}
		}
		else if (reqdata.userid() > 0 && reqdata.clubid() > 0) {
			//代理发起人邀请加入
			Msg const& errmsg = mgo::InviteJoinClub(reqdata.clubid(), pre_header_->userId, reqdata.userid(), 1);
			if (errmsg.code == Ok.code) {
				mgo::LoadUserClub(reqdata.userid(), reqdata.clubid(), info);
				rspdata.set_userid(reqdata.userid());
				rspdata.set_retcode(Ok.code);
				rspdata.set_errormsg("邀请加入俱乐部成功");
			}
			else if (errmsg.code == ERR_JoinClub_UserAlreadyInClub.code) {
				mgo::LoadUserClub(pre_header_->userId, reqdata.clubid(), info);
				rspdata.set_userid(pre_header_->userId);
				rspdata.set_retcode(Ok.code);
				rspdata.set_errormsg(errmsg.errmsg());
			}
			else {
				rspdata.set_retcode(errmsg.code);
				rspdata.set_errormsg(errmsg.errmsg());
			}
		}
		else {
			rspdata.set_retcode(-1);
			rspdata.set_errormsg("加入俱乐部参数错误");
		}
		if (info.clubId > 0) {
			rspdata.mutable_clubinfo()->set_clubid(info.clubId);
			rspdata.mutable_clubinfo()->set_clubname(info.clubName);
			rspdata.mutable_clubinfo()->set_clubiconid(info.iconId);
			rspdata.mutable_clubinfo()->set_promoterid(info.promoterId);
			rspdata.mutable_clubinfo()->set_status(info.status);
			rspdata.mutable_clubinfo()->set_invitationcode(info.invitationCode);
			rspdata.mutable_clubinfo()->set_clubplayernum(info.playerNum);
			rspdata.mutable_clubinfo()->set_rate(info.ratio);
			rspdata.mutable_clubinfo()->set_autobcpartnerrate(info.autoPartnerRatio);
			rspdata.mutable_clubinfo()->set_url(info.url);
			rspdata.mutable_clubinfo()->set_createtime(info.joinTime.format());
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_JOIN_THE_CLUB_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 退出俱乐部
void HallServ::ExitTheClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::ExitTheClubMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::ExitTheClubMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		Msg const& errmsg = mgo::ExitClub(pre_header_->userId, reqdata.clubid());
		if (errmsg.code == Ok.code) {
			rspdata.set_retcode(Ok.code);
			rspdata.set_errormsg("您退出俱乐部成功");
		}
		else if (errmsg.code == ERR_ExitClub_UserNotInClub.code){
			rspdata.set_retcode(Ok.code);
			rspdata.set_errormsg(errmsg.errmsg());
		}
		else {
			rspdata.set_retcode(errmsg.code);
			rspdata.set_errormsg(errmsg.errmsg());
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_EXIT_THE_CLUB_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 设置是否开启自动成为合伙人
void HallServ::SetAutoBecomePartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::SetAutoBecomePartnerMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::SetAutoBecomePartnerMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_SET_AUTO_BECOME_PARTNER_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 成为合伙人
void HallServ::BecomePartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::BecomePartnerMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::BecomePartnerMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_BECOME_PARTNER_MESSAGE_RES,
			pre_header_, header_);
	}
}

// 佣金兑换金币
void HallServ::ExchangeRevenueMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::ExchangeRevenueMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::ExchangeRevenueMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_EXCHANGE_MY_REVENUE_RES,
			pre_header_, header_);
	}
}

// 获取佣金提取记录 
void HallServ::GetExchangeRevenueRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetExchangeRevenueRecordMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetExchangeRevenueRecordMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_EXCHANGE_MY_REVENUE_RECORD_RES,
			pre_header_, header_);
	}
}

// 获取我的业绩
void HallServ::GetMyAchievementMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetMyAchievementMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetMyAchievementMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_RES,
			pre_header_, header_);
	}
}

// 获取会员业绩详情
void HallServ::GetAchievementDetailMemberMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetAchievementDetailMemberMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetAchievementDetailMemberMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_MEMBER_RES,
			pre_header_, header_);
	}
}

// 获取合伙人业绩详情
void HallServ::GetAchievementDetailPartnerMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetAchievementDetailPartnerMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetAchievementDetailPartnerMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_PARTNER_RES,
			pre_header_, header_);
	}
}

// 获取俱乐部内我的团队我的俱乐部
void HallServ::GetMyClubMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetMyClubMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetMyClubMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_CLUB_RES,
			pre_header_, header_);
	}
}

// 获取我的团队成员
void HallServ::GetMyTeamMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetMyTeamMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetMyTeamMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_MY_TEAM_RES,
			pre_header_, header_);
	}
}

// 设置下一级合伙人提成比例
void HallServ::SetSubordinateRateMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::SetSubordinateRateMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::SetSubordinateRateMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_SET_SUBORDINATE_RATE_RES,
			pre_header_, header_);
	}
}

// 获取游戏明细记录
void HallServ::GetPlayRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetPlayRecordMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetPlayRecordMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_PLAY_RECORD_RES,
			pre_header_, header_);
	}
}

// 获取玩家俱乐部所有游戏记录列表
void HallServ::GetAllPlayRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetAllPlayRecordMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetAllPlayRecordMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_ALL_PLAY_RECORD_RES,
			pre_header_, header_);
	}
}

// 获取玩家账户明细列表
void HallServ::GetUserScoreChangeRecordMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetUserScoreChangeRecordMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetUserScoreChangeRecordMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_USER_SCORE_CHANGE_RECORD_RES,
			pre_header_, header_);
	}
}

// 我的上级信息
void HallServ::GetClubPromoterInfoMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetClubPromoterInfoMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetClubPromoterInfoMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_CLUB_PROMOTER_RES,
			pre_header_, header_);
	}
}

// 开除此用户
void HallServ::FireMemberMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::FireMemberMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::FireMemberMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		Msg const& errmsg = mgo::FireClubUser(reqdata.clubid(), pre_header_->userId, reqdata.userid());
		if (errmsg.code == Ok.code) {
			rspdata.set_retcode(Ok.code);
			rspdata.set_errormsg("开除俱乐部成员成功");
		}
		else if (errmsg.code == ERR_FireClub_UserNotInClub.code) {
			rspdata.set_retcode(Ok.code);
			rspdata.set_errormsg(errmsg.errmsg());
		}
		else {
			rspdata.set_retcode(errmsg.code);
			rspdata.set_errormsg(errmsg.errmsg());
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_FIRE_MEMBER_RES,
			pre_header_, header_);
	}
}

// 获取俱乐部申请QQ
void HallServ::GetApplyClubQQMessage_club(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::ClubHallServer::GetApplyClubQQMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::ClubHallServer::GetApplyClubQQMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_HALL_CLUB_SUBID::CLIENT_TO_HALL_CLUB_GET_APPLY_CLUB_QQ_RES,
			pre_header_, header_);
	}
}