#include "Game.h"

#include "RobotMgr.h"
#include "PlayerMgr.h"
#include "TableMgr.h"
#include "TableThread.h"

#include "proto/Game.Common.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "public/mgoOperation.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"

//#include "TaskService.h"

GameServ::GameServ(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrRpc,
	uint32_t gameId, uint32_t roomId)
	: server_(loop, listenAddr, "tcpServer")
	, rpcserver_(loop, listenAddrRpc, "rpcServer")
	, thisThread_(new muduo::net::EventLoopThread(std::bind(&GameServ::threadInit, this), "MainEventLoopThread"))
	, thisTimer_(new muduo::net::EventLoopThread(std::bind(&GameServ::threadInit, this), "EventLoopThreadTimer"))
	, ipFinder_("qqwry.dat")
	, gameId_(gameId)
	, roomId_(roomId) {
	CTableThreadMgr::get_mutable_instance().Init(loop, "TableThreadPool");
	registerHandlers();
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");
	rpcserver_.registerService(&rpcservice_);
	server_.setConnectionCallback(
		std::bind(&GameServ::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&GameServ::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

GameServ::~GameServ() {
	Quit();
}

void GameServ::Quit() {
	CTableMgr::get_mutable_instance().KickAll();
	thisThread_->getLoop()->quit();
	thisTimer_->getLoop()->quit();
	CTableThreadMgr::get_mutable_instance().quit();
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

void GameServ::registerHandlers() {
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
		::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_REQ)]
		= std::bind(&GameServ::cmd_keep_alive_ping, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
		::GameServer::SUBID::SUB_C2S_ENTER_ROOM_REQ)]
		= std::bind(&GameServ::cmd_on_user_enter_room, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
		::GameServer::SUBID::SUB_C2S_USER_READY_REQ)]
		= std::bind(&GameServ::cmd_on_user_ready, this,
			std::placeholders::_1, std::placeholders::_2);
	//点击离开按钮
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
		::GameServer::SUBID::SUB_C2S_USER_LEFT_REQ)]
		= std::bind(&GameServ::cmd_on_user_left_room, this,
			std::placeholders::_1, std::placeholders::_2);
	//关闭页面
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
		::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE)]
		= std::bind(&GameServ::cmd_on_user_offline, this,
			std::placeholders::_1, std::placeholders::_2);
	handlers_[packet::enword(
		::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC, 0)]
		= std::bind(&GameServ::cmd_on_game_message, this,
			std::placeholders::_1, std::placeholders::_2);
// 	handlers_[packet::enword(
// 		::Game::Common::MAIN_MESSAGE_HTTP_TO_SERVER,
// 		::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER)]
// 		= std::bind(&GameServ::cmd_notifyRepairServerResp, this,
// 			std::placeholders::_1, std::placeholders::_2);
}

bool GameServ::InitZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&GameServ::onZookeeperConnected, this));
	if (!zkclient_->connectServer()) {
		_LOG_FATAL("error");
		abort();
		return false;
	}
	return true;
}

void GameServ::onZookeeperConnected() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_serv"))
		zkclient_->createNode("/GAME/game_serv", "game_serv"/*, true*/);
	//if (ZNONODE == zkclient_->existsNode("/GAME/game_servInvalid"))
	//	zkclient_->createNode("/GAME/game_servInvalid", "game_servInvalid", true);
	{
		nodeValue_ += utils::random::charStr(16, rTy::UpperCharNumber);
		nodeValue_ += ":" + std::to_string(gameId_);
		nodeValue_ += ":" + std::to_string(roomId_);
		nodeValue_ += ":" + std::to_string(kGoldCoin);
		//tcp
		std::vector<std::string> vec;
		boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[0] + ":" + vec[1];
		//rpc
		boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1];
		//http
		//boost::algorithm::split(vec, httpserver_.ipPort(), boost::is_any_of(":"));
		//nodeValue_ += ":" + vec[1];
		nodePath_ = "/GAME/game_serv/" + nodeValue_;
		zkclient_->createNode(nodePath_, nodeValue_, true);
		invalidNodePath_ = "/GAME/game_servInvalid/" + nodeValue_;
	}
	{
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/game_gate",
			names,
			std::bind(
				&GameServ::onGateWatcher, this,
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
			"/GAME/game_hall",
			names,
			std::bind(
				&GameServ::onHallWatcher, this,
				placeholders::_1, std::placeholders::_2,
				placeholders::_3, std::placeholders::_4,
				placeholders::_5), this)) {
			std::string s;
			for (std::string const& name : names) {
				s += "\n" + name;
			}
			_LOG_WARN("可用大厅服列表%s", s.c_str());
		}
	}
}

void GameServ::onGateWatcher(
	int type, int state,
	const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_gate",
		names,
		std::bind(
			&GameServ::onGateWatcher, this,
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

void GameServ::onHallWatcher(int type, int state,
	const shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context) {
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/game_hall",
		names,
		std::bind(
			&GameServ::onHallWatcher, this,
			placeholders::_1, std::placeholders::_2,
			placeholders::_3, std::placeholders::_4,
			placeholders::_5), this)) {
		std::string s;
		for (std::string const& name : names) {
			s += "\n" + name;
		}
		_LOG_WARN("可用大厅服列表%s", s.c_str());
	}
}

void GameServ::registerZookeeper() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "GAME"/*, true*/);
	if (ZNONODE == zkclient_->existsNode("/GAME/game_serv"))
		zkclient_->createNode("/GAME/game_serv", "game_serv"/*, true*/);
	if (ZNONODE == zkclient_->existsNode(nodePath_)) {
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&GameServ::registerZookeeper, this));
}

bool GameServ::InitRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		_LOG_FATAL("error");
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	redisClient_->subscribeRefreshConfigMessage(
		std::bind(&GameServ::on_refresh_game_config, this, std::placeholders::_1));
	redisClient_->startSubThread();
	return true;
}

bool GameServ::InitMongoDB(std::string const& url) {
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

void GameServ::threadInit() {
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

bool GameServ::InitServer() {
	switch (tracemsg_) {
	case true:
		initTraceMessageID();
		break;
	}
	if (mgo::LoadGameRoomInfo(gameId_, roomId_, gameInfo_, roomInfo_)) {
		return true;
	}
	_LOG_ERROR("error");
	return false;
}

void GameServ::Start(int numThreads, int numWorkerThreads, int maxSize) {
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	muduo::net::ReactorSingleton::start();
	
	thisThread_->startLoop();
	thisTimer_->startLoop();
	
	numWorkerThreads = std::min<int>(numWorkerThreads, roomInfo_.tableCount);
	CTableThreadMgr::get_mutable_instance().setThreadNum(numWorkerThreads);
	CTableThreadMgr::get_mutable_instance().start(std::bind(&GameServ::threadInit, this), this);
	CTableMgr::get_mutable_instance().Init(this);
	CRobotMgr::get_mutable_instance().Init(this);
	
	std::vector<std::string> vec;
	boost::algorithm::split(vec, rpcserver_.ipPort(), boost::is_any_of(":"));

	_LOG_WARN("GameServ = %s rpc:%s 房间号[%d] %s numThreads: I/O = %d worker = %d", server_.ipPort().c_str(), vec[1].c_str(), roomId_, getModeStr(kGoldCoin).c_str(), numThreads, numWorkerThreads);

	server_.start(true);
	rpcserver_.start(true);

	thisTimer_->getLoop()->runAfter(5.0f, std::bind(&GameServ::registerZookeeper, this));
	//thisTimer_->getLoop()->runAfter(3.0f, std::bind(&GameServ::db_refresh_game_room_info, this));
	//thisTimer_->getLoop()->runAfter(30, std::bind(&GameServ::redis_refresh_room_player_nums, this));

	CTableThreadMgr::get_mutable_instance().startCheckUserIn();
}

void GameServ::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		{
			WRITE_LOCK(mutexGateConns_);
			mapGateConns_[conn->peerAddress().toIpPort()] = conn;
		}
		int32_t num = numConnected_.incrementAndGet();
		_LOG_INFO("游戏服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
	else {
		{
			WRITE_LOCK(mutexGateConns_);
			mapGateConns_.erase(conn->peerAddress().toIpPort());
		}
		int32_t num = numConnected_.decrementAndGet();
		_LOG_INFO("游戏服[%s] <- 网关服[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		RunInLoop(thisThread_->getLoop(),
			std::bind(
				&GameServ::asyncOfflineHandler,
				this, conn->peerAddress().toIpPort()));
	}
}

void GameServ::onMessage(
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
#if 1
			std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header->userId);
			if (player) {
				std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
				if (table) {
					RunInLoop(table->GetLoop(),
						std::bind(
							&GameServ::asyncLogicHandler,
							this,
							muduo::net::WeakTcpConnectionPtr(conn), buffer, receiveTime));
					continue;
				}
			}
#endif
			RunInLoop(thisThread_->getLoop(),
				std::bind(
					&GameServ::asyncLogicHandler,
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

void GameServ::asyncLogicHandler(
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
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER_CLUB:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER_FRIEND:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC_FRIEND:
		case Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_GAME_SERVER:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
		case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
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
		default:
			break;
		}
	}
	else {
		_LOG_ERROR("error");
	}
}

void GameServ::asyncOfflineHandler(std::string const& ipPort) {
	_LOG_ERROR("%s", ipPort.c_str());
	//READ_LOCK(mutexGateUsers_);
	std::map<std::string, std::set<int64_t>>::iterator it = mapGateUsers_.find(ipPort);
	if (it != mapGateUsers_.end()) {
		if (!it->second.empty()) {
			std::vector<int64_t> v;
			std::copy(it->second.begin(), it->second.end(), std::back_inserter(v));
			for (std::vector<int64_t>::iterator ir = v.begin(); ir != v.end(); ++ir) {
				std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(*ir);
				if (player) {
					std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
					if (table) {
						RunInLoop(table->GetLoop(), CALLBACK_0([=](std::shared_ptr<CTable>& table, std::shared_ptr<CPlayer>& player) {
							table->assertThisThread();
							//tableDelegate_->OnUserLeft -> ClearTableUser -> DelContext -> erase(it)
							table->OnUserOffline(player);
						}, table, player));
					}
				}
			}
		}
	}
}

void GameServ::send(
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

void GameServ::send(
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

void GameServ::send(
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

void GameServ::send(
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

void GameServ::cmd_keep_alive_ping(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::Game::Common::KeepAliveMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::Game::Common::KeepAliveMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		rspdata.set_retcode(1);
		rspdata.set_errormsg("[Game]User LoginInfo TimeOut, Restart Login.");
		//用户登陆token
		std::string const& token = reqdata.session();
		int64_t userid = 0;
		uint32_t agentid = 0;
		std::string account;
		if (REDISCLIENT.GetTokenInfo(token, userid, account, agentid)) {
			//std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
			//if (player && player->isOffline()) {
			//	rspdata.set_retcode(3);
			//	rspdata.set_errormsg("[Game]KEEP ALIVE PING Error User Offline!");
			//}
			/*else */if (REDISCLIENT.ResetExpiredOnlineInfo(userid)) {
				rspdata.set_retcode(0);
				rspdata.set_errormsg("[Game]KEEP ALIVE PING OK.");
			}
			else {
				rspdata.set_retcode(2);
				rspdata.set_errormsg("[Game]KEEP ALIVE PING Error UserId Not Find!");
			}
		}
		else {
			rspdata.set_retcode(1);
			rspdata.set_errormsg("[Game]KEEP ALIVE PING Error Session Not Find!");
		}
		send(conn, &rspdata,
			::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES,
			pre_header_, header_);
	}
}

void GameServ::cmd_on_user_enter_room(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::GameServer::MSG_C2S_UserEnterMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::GameServer::MSG_S2C_UserEnterMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());
		if (reqdata.gameid() != gameInfo_.gameId ||
			reqdata.roomid() != roomInfo_.roomId) {
			return;
		}
		
		//判断玩家是否在上分
		//Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER
		//::GameServer::SUB_S2C_ENTER_ROOM_RES
		//ERROR_ENTERROOM_USER_ORDER_SCORE
		//需要先加锁禁止玩家上分操作，然后继续

		if (REDISCLIENT.ExistOnlineInfo(pre_header_->userId)) {
			std::string redisPasswd;
			if (REDISCLIENT.GetUserLoginInfo(pre_header_->userId, "dynamicPassword", redisPasswd)) {
				std::string passwd = utils::buffer2HexStr(
					(unsigned char const*)
					reqdata.dynamicpassword().c_str(),
					reqdata.dynamicpassword().size());
				if (passwd == redisPasswd) {
					AddContext(conn, pre_header_, header_);
				}
				else {
					const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
					//DelContext(pre_header_->userId);
					//REDISCLIENT.DelOnlineInfo(pre_header_->userId);
					//桌子密码错误
					SendGameErrorCode(conn,
						::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
						::GameServer::SUB_S2C_ENTER_ROOM_RES,
						ERROR_ENTERROOM_PASSWORD_ERROR, "ERROR_ENTERROOM_PASSWORD_ERROR", pre_header_, header_);
					return;
				}
			}
			else {
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
				//DelContext(pre_header_->userId);
				//REDISCLIENT.DelOnlineInfo(pre_header_->userId);
				//会话不存在
				SendGameErrorCode(conn,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_ENTER_ROOM_RES,
					ERROR_ENTERROOM_NOSESSION, "ERROR_ENTERROOM_NOSESSION", pre_header_, header_);
				return;
			}
		}
		else {
			const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
			//DelContext(pre_header_->userId);
			//REDISCLIENT.DelOnlineInfo(pre_header_->userId);
			//游戏已结束
			SendGameErrorCode(conn,
				::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
				::GameServer::SUB_S2C_ENTER_ROOM_RES,
				ERROR_ENTERROOM_GAME_IS_END, "ERROR_ENTERROOM_GAME_IS_END", pre_header_, header_);
			return;
		}
		std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
		if (player && player->Valid()) {
			player->setTrustee(false);
			std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
			if (table) {
				RunInLoop(table->GetLoop(), CALLBACK_0([=](
					const muduo::net::WeakTcpConnectionPtr& weakConn,
					BufferPtr const& buf, std::shared_ptr<CTable>& table) {
					packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
					packet::header_t const* header_ = packet::get_header(buf);
					if (player->isOffline()) {
						_LOG_WARN("[%s][%d][%s] %d %d 断线重连进房间",
							table->GetRoundId().c_str(), table->GetTableId(), table->StrGameStatus().c_str(),
							player->GetChairId(), player->GetUserId());
					}
					else {
						_LOG_WARN("[%s][%d][%s] %d %d 异地登陆进房间",
							table->GetRoundId().c_str(), table->GetTableId(), table->StrGameStatus().c_str(),
							player->GetChairId(), player->GetUserId());
					}
					table->assertThisThread();
					if (table->CanJoinTable(player)) {
						table->OnUserEnterAction(player, pre_header_, header_);
					}
					else {
						const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
						DelContext(pre_header_->userId);
						REDISCLIENT.DelOnlineInfo(pre_header_->userId);
						muduo::net::TcpConnectionPtr conn(weakConn.lock());
						if (conn) {
							SendGameErrorCode(conn,
								::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
								::GameServer::SUB_S2C_ENTER_ROOM_RES,
								ERROR_ENTERROOM_GAME_IS_END, "ERROR_ENTERROOM_GAME_IS_END", pre_header_, header_);
						}
					}
				}, conn, buf, table));
			}
			else {
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
				DelContext(pre_header_->userId);
				REDISCLIENT.DelOnlineInfo(pre_header_->userId);
				SendGameErrorCode(conn,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_ENTER_ROOM_RES,
					ERROR_ENTERROOM_GAME_IS_END, "ERROR_ENTERROOM_GAME_IS_END", pre_header_, header_);
			}
			return;
		}
		//判断在其他游戏中
		uint32_t gameid = 0, roomid = 0;
		if (REDISCLIENT.GetOnlineInfo(pre_header_->userId, gameid, roomid)) {
			if (gameid != 0 && roomid != 0 &&
				gameInfo_.gameId != gameid || roomInfo_.roomId != roomid) {
				::GameServer::MSG_S2C_PlayInOtherRoom rspdata;
				rspdata.mutable_header()->CopyFrom(reqdata.header());
				rspdata.set_gameid(gameid);
				rspdata.set_roomid(roomid);
				send(conn,
					&rspdata,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_PLAY_IN_OTHERROOM,
					pre_header_, header_);
				SendGameErrorCode(conn,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_PLAY_IN_OTHERROOM,
					ERROR_ENTERROOM_USERINGAME, "ERROR_ENTERROOM_USERINGAME", pre_header_, header_);
				return;
			}
		}
		UserBaseInfo userInfo;
		if (conn) {
			if (!mgo::GetUserBaseInfo(pre_header_->userId, userInfo)) {
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
				DelContext(pre_header_->userId);
				REDISCLIENT.DelOnlineInfo(pre_header_->userId);
				SendGameErrorCode(conn,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_USERNOTEXIST,
					"ERROR_ENTERROOM_USERNOTEXIST", pre_header_, header_);
				return;
			}
		}
		else {
			const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
			DelContext(pre_header_->userId);
			REDISCLIENT.DelOnlineInfo(pre_header_->userId);
			SendGameErrorCode(conn,
				::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
				::GameServer::SUB_S2C_ENTER_ROOM_RES, ERROR_ENTERROOM_NOSESSION,
				"ERROR_ENTERROOM_NOSESSION", pre_header_, header_);
			return;
		}
		_LOG_WARN("roomid:%d enterMinScore:%lld enterMaxScore:%lld %lld.Score:%lld", roomInfo_.roomId,
			roomInfo_.enterMinScore, roomInfo_.enterMaxScore, pre_header_->userId, userInfo.userScore);
		//最小进入条件
		if (roomInfo_.enterMinScore > 0 &&
			userInfo.userScore < roomInfo_.enterMinScore) {
			const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
			DelContext(pre_header_->userId);
			REDISCLIENT.DelOnlineInfo(pre_header_->userId);
			SendGameErrorCode(conn,
				::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
				::GameServer::SUB_S2C_ENTER_ROOM_RES,
				ERROR_ENTERROOM_SCORENOENOUGH,
				"ERROR_ENTERROOM_SCORENOENOUGH", pre_header_, header_);
			return;
		}
		//最大进入条件
		if (roomInfo_.enterMaxScore > 0 &&
			userInfo.userScore > roomInfo_.enterMaxScore) {
			const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
			DelContext(pre_header_->userId);
			REDISCLIENT.DelOnlineInfo(pre_header_->userId);
			SendGameErrorCode(conn,
				::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
				::GameServer::SUB_S2C_ENTER_ROOM_RES,
				ERROR_ENTERROOM_SCORELIMIT,
				"ERROR_ENTERROOM_SCORELIMIT", pre_header_, header_);
			return;
		}
		{
			std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().New(pre_header_->userId);
			if (!player) {
				REDISCLIENT.DelOnlineInfo(pre_header_->userId);
				return;
			}
			userInfo.ip = pre_header_->clientIp;
			player->SetUserBaseInfo(userInfo);
			std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().FindSuit(player, INVALID_TABLE);
			if (table) {
				RunInLoop(table->GetLoop(), CALLBACK_0([=](
					const muduo::net::WeakTcpConnectionPtr& weakConn,
					BufferPtr const& buf, std::shared_ptr<CTable>& table) {
					packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
					packet::header_t const* header_ = packet::get_header(buf);
					table->assertThisThread();
					if (table->RoomSitChair(player, pre_header_, header_)) {
					}
					else {
						const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
						DelContext(pre_header_->userId);
						REDISCLIENT.DelOnlineInfo(pre_header_->userId);
						if (table->GetPlayerCount() == 0) {
							CTableMgr::get_mutable_instance().Delete(table->GetTableId());
						}
						muduo::net::TcpConnectionPtr conn(weakConn.lock());
						if (conn) {
							SendGameErrorCode(conn,
								::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
								::GameServer::SUB_S2C_ENTER_ROOM_RES,
								ERROR_ENTERROOM_TABLE_FULL,
								"ERROR_ENTERROOM_TABLE_FULL", pre_header_, header_);
						}
					}
				}, conn, buf, table));
			}
			else {
				const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
				DelContext(pre_header_->userId);
				REDISCLIENT.DelOnlineInfo(pre_header_->userId);
				SendGameErrorCode(conn,
					::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER,
					::GameServer::SUB_S2C_ENTER_ROOM_RES,
					ERROR_ENTERROOM_TABLE_FULL,
					"ERROR_ENTERROOM_TABLE_FULL", pre_header_, header_);
			}
		}
	}
}

void GameServ::cmd_on_user_ready(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::GameServer::MSG_C2S_UserReadyMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::GameServer::MSG_S2C_UserReadyMessageResponse rspdata;
		rspdata.mutable_header()->CopyFrom(reqdata.header());

		std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
		if (player) {
			std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
			if (table) {
				RunInLoop(table->GetLoop(), CALLBACK_0([=](std::shared_ptr<CTable>& table, std::shared_ptr<CPlayer>& player) {
					table->assertThisThread();
					table->SetUserReady(player->GetChairId());
				}, table, player));
				rspdata.set_retcode(0);
				rspdata.set_errormsg("OK");
			}
			else {
				rspdata.set_retcode(1);
				rspdata.set_errormsg("SetUserReady find table failed");
			}
		}
		else {
			rspdata.set_retcode(2);
			rspdata.set_errormsg("SetUserReady find user failed");
		}
		send(conn, &rspdata, ::GameServer::SUB_S2C_USER_READY_RES, pre_header_, header_);
	}
}

//点击离开按钮
void GameServ::cmd_on_user_left_room(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::GameServer::MSG_C2S_UserLeftMessage reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		::GameServer::MSG_C2S_UserLeftMessageResponse rspdata;
		std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
		if (player) {
			std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
			if (table) {
				RunInLoop(table->GetLoop(), CALLBACK_0([=](
					uint32_t gameId, uint32_t roomId, int Type,
					const muduo::net::WeakTcpConnectionPtr& weakConn,
					BufferPtr const& buf, std::shared_ptr<CPlayer>& player) {
					packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
					packet::header_t const* header_ = packet::get_header(buf);
					::GameServer::MSG_C2S_UserLeftMessageResponse rspdata;
					rspdata.mutable_header()->set_sign(PROTOBUF_SIGN);
					rspdata.set_gameid(gameId);
					rspdata.set_roomid(roomId);
					rspdata.set_type(Type);
					table->assertThisThread();
					//KickUser(pre_header_->userId, KICK_GS | KICK_CLOSEONLY);
					if (table->CanLeftTable(pre_header_->userId)) {
						if (table->OnUserLeft(player, true)) {
							rspdata.set_retcode(0);
							rspdata.set_errormsg("OK");
						}
						else {
							rspdata.set_retcode(1);
							rspdata.set_errormsg("OnUserLeft failed");
						}
					}
					else {
						const_cast<packet::internal_prev_header_t*>(pre_header_)->ok = -1;
						rspdata.set_retcode(2);
						rspdata.set_errormsg("正在游戏中，不能离开");
					}
					muduo::net::TcpConnectionPtr conn(weakConn.lock());
					if (conn) {
						send(conn, &rspdata, ::GameServer::SUB_S2C_USER_LEFT_RES, pre_header_, header_);
					}
				}, reqdata.gameid(), reqdata.roomid(), reqdata.type(), conn, buf, player));
				return;
			}
			rspdata.mutable_header()->CopyFrom(reqdata.header());
			rspdata.set_gameid(reqdata.gameid());
			rspdata.set_roomid(reqdata.roomid());
			rspdata.set_type(reqdata.type());
			rspdata.set_retcode(3);
			rspdata.set_errormsg("OnUserLeft find table failed");
		}
		else {
			rspdata.mutable_header()->CopyFrom(reqdata.header());
			rspdata.set_gameid(reqdata.gameid());
			rspdata.set_roomid(reqdata.roomid());
			rspdata.set_type(reqdata.type());
			rspdata.set_retcode(4);
			rspdata.set_errormsg("OnUserLeft find user failed");
		}
		send(conn, &rspdata, ::GameServer::SUB_S2C_USER_LEFT_RES, pre_header_, header_);
	}
}

//关闭页面
void GameServ::cmd_on_user_offline(
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
		_LOG_ERROR("KICK_LEAVEGS %d", pre_header_->userId);
		//KickUser(pre_header_->userId, KICK_GS | KICK_CLOSEONLY);
		std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
		if (!player) {
			return;
		}
		std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
		if (table) {
			RunInLoop(table->GetLoop(), CALLBACK_0([=](std::shared_ptr<CTable>& table, std::shared_ptr<CPlayer>& player) {
				table->assertThisThread();
				table->OnUserOffline(player);
			}, table, player));
		}
		else {
		}
	}
}

void GameServ::cmd_on_game_message(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
	::GameServer::MSG_CSC_Passageway reqdata;
	if (reqdata.ParseFromArray(msg, msgLen)) {
		uint8_t const* data = (uint8_t const*)reqdata.passdata().data();
		uint32_t len = reqdata.passdata().size();
		std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(pre_header_->userId);
		if (player) {
			std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
			if (table) {
				RunInLoop(table->GetLoop(), CALLBACK_0([=](BufferPtr const& buf, std::shared_ptr<CTable>& table, std::shared_ptr<CPlayer>& player) {
					packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
					packet::header_t const* header_ = packet::get_header(buf);
					//uint8_t const* msg = packet::get_msg(buf);
					//size_t msgLen = packet::get_msglen(buf);
					//::GameServer::MSG_CSC_Passageway reqdata;
					/*if (reqdata.ParseFromArray(msg, msgLen))*/ {
						//uint8_t const* data = (uint8_t const*)reqdata.passdata().data();
						//uint32_t len = reqdata.passdata().size();
						table->assertThisThread();
						table->OnGameEvent(player->GetChairId(), header_->subId, data, len);
					}
				}, buf, table, player));
			}
		}
	}
}

void GameServ::cmd_notifyRepairServerResp(
	const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf) {
	packet::internal_prev_header_t const* pre_header_ = packet::get_pre_header(buf);
	packet::header_t const* header_ = packet::get_header(buf);
	uint8_t const* msg = packet::get_msg(buf);
	size_t msgLen = packet::get_msglen(buf);
}

void GameServ::KickUser(int64_t userId, int32_t kickType) {
	std::shared_ptr<CPlayer> player = CPlayerMgr::get_mutable_instance().Get(userId);
	if (!player) {
		return;
	}
#if 0
	packet::internal_prev_header_t pre_header;
	memset(&pre_header, 0, packet::kPrevHeaderLen);
	TableContext ctx = GetContext(userId);
	std::shared_ptr<packet::internal_prev_header_t> pre_header_ = ctx.get<1>();
	if (!pre_header_) {
		return;
	}
	memcpy(&pre_header, pre_header_.get(), packet::kPrevHeaderLen);
	pre_header.len = packet::kPrevHeaderLen;
	pre_header.kicking = 1;//KICK_LEAVEGS
	pre_header.userId = userId;
	packet::setCheckSum(&pre_header);
	muduo::net::WeakTcpConnectionPtr weakConn = ctx.get<0>();
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (likely(conn)) {
		conn->send((char const*)&pre_header, pre_header.len);
	}
#else
	std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(player->GetTableId());
	if (table) {
		RunInLoop(table->GetLoop(), CALLBACK_0([=](std::shared_ptr<CTable>& table, std::shared_ptr<CPlayer>& player) {
			table->assertThisThread();
			//tableDelegate_->OnUserLeft -> ClearTableUser -> DelContext -> erase(it)
			table->OnUserOffline(player);
		}, table, player));
	}
#endif
}

TableContext GameServ::GetContext(int64_t userId) {
	_LOG_ERROR("%d", userId);
	TableContext ctx;
	bool ok = false;
	RunInLoop(thisThread_->getLoop(), OBJ_CALLBACK_0(this, &GameServ::GetContextInLoop, userId, std::ref(ctx), std::ref(ok)));
	while (!ok);
	return ctx;
}

void GameServ::GetContextInLoop(int64_t userId, TableContext& context, bool& ok) {
	thisThread_->getLoop()->assertInLoopThread();
	std::string ipPort;
	std::shared_ptr<gate_t> gate;
	std::shared_ptr<packet::internal_prev_header_t> pre_header;
	std::shared_ptr<packet::header_t> header;
	muduo::net::WeakTcpConnectionPtr weakConn;
	if (userId > 0) {
		{
			//READ_LOCK(mutexUserGates_);
			std::map<int64_t, std::shared_ptr<gate_t>>::iterator it = mapUserGates_.find(userId);
			if (it != mapUserGates_.end()) {
				gate = it->second;
				ipPort = gate->IpPort;
				pre_header = gate->pre_header;
				header = gate->header;
			}
		}
		if (!ipPort.empty()) {
			READ_LOCK(mutexGateConns_);
			auto it = mapGateConns_.find(ipPort);
			if (it != mapGateConns_.end()) {
				weakConn = it->second;
			}
		}
	}
	//context = TableContext(weakConn, pre_header, header);
	context = boost::make_tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>>(weakConn, pre_header, header);
	ok = true;
}

void GameServ::AddContext(
	const muduo::net::TcpConnectionPtr& conn,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	_LOG_ERROR("%d", pre_header_->userId);
	RunInLoop(thisThread_->getLoop(), OBJ_CALLBACK_0(this, &GameServ::AddContextInLoop, conn, pre_header_, header_));
}

void GameServ::AddContextInLoop(
	const muduo::net::TcpConnectionPtr& conn,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	thisThread_->getLoop()->assertInLoopThread();
	{
		//WRITE_LOCK(mutexUserGates_);
		std::map<int64_t, std::shared_ptr<gate_t>>::iterator it = mapUserGates_.find(pre_header_->userId);
		//已存在 map[userid]gate
		if (it != mapUserGates_.end()) {
			//和之前是同一个gate
			if (it->second->IpPort == conn->peerAddress().toIpPort()) {
				memcpy(it->second->pre_header.get(), pre_header_, packet::kPrevHeaderLen);
				memcpy(it->second->header.get(), header_, packet::kHeaderLen);
				return;
			}
			//换了gate
			std::string ipPort = it->second->IpPort;
			it->second->IpPort = conn->peerAddress().toIpPort();
			memcpy(it->second->pre_header.get(), pre_header_, packet::kPrevHeaderLen);
			memcpy(it->second->header.get(), header_, packet::kHeaderLen);
			//删除旧的记录 map[gate]users
			{
				//WRITE_LOCK(mutexGateUsers_);
				std::map<std::string, std::set<int64_t>>::iterator it = mapGateUsers_.find(ipPort);
				if (it != mapGateUsers_.end()) {
					std::set<int64_t>::iterator ir = it->second.find(pre_header_->userId);
					if (ir != it->second.end()) {
						it->second.erase(ir);
						numUsers_.decrement();
						if (it->second.empty()) {
							mapGateUsers_.erase(it);
						}
					}
				}
			}
		}
		//不存在 map[userid]gate
		else {
			std::shared_ptr<packet::internal_prev_header_t> pre_header(new packet::internal_prev_header_t());
			std::shared_ptr<packet::header_t> header(new packet::header_t());
			memcpy(pre_header.get(), pre_header_, packet::kPrevHeaderLen);
			memcpy(header.get(), header_, packet::kHeaderLen);
			std::shared_ptr<gate_t>  gate(new gate_t());
			gate->IpPort = conn->peerAddress().toIpPort();
			gate->pre_header = pre_header;
			gate->header = header;
			mapUserGates_[pre_header_->userId] = gate;
		}
	}
	{
		//WRITE_LOCK(mutexGateUsers_);
		std::set<int64_t>& ref = mapGateUsers_[conn->peerAddress().toIpPort()];
		ref.insert(pre_header_->userId);
		numUsers_.increment();
	}
}

void GameServ::DelContext(int64_t userId) {
	_LOG_ERROR("%d", userId);
	RunInLoop(thisThread_->getLoop(), OBJ_CALLBACK_0(this, &GameServ::DelContextInLoop, userId));
}

void GameServ::DelContextInLoop(int64_t userId) {
	thisThread_->getLoop()->assertInLoopThread();
	std::string ipPort;
	{
		//READ_LOCK(mutexUserGates_);
		std::map<int64_t, std::shared_ptr<gate_t>>::iterator it = mapUserGates_.find(userId);
		if (it != mapUserGates_.end()) {
			ipPort = it->second->IpPort;
		}
	}
	{
		//WRITE_LOCK(mutexGateUsers_);
		std::map<std::string, std::set<int64_t>>::iterator it = mapGateUsers_.find(ipPort);
		if (it != mapGateUsers_.end()) {
			std::set<int64_t>::iterator ir = it->second.find(userId);
			if (ir != it->second.end()) {
				it->second.erase(ir);
				numUsers_.decrement();
				if (it->second.empty()) {
					mapGateUsers_.erase(it);
				}
			}
		}
	}
	{
		//WRITE_LOCK(mutexUserGates_);
		mapUserGates_.erase(userId);
	}
}

bool GameServ::SendGameErrorCode(
	const muduo::net::TcpConnectionPtr& conn,
	uint8_t mainid, uint8_t subid,
	uint32_t errcode, std::string errmsg,
	packet::internal_prev_header_t const* pre_header_,
	packet::header_t const* header_) {
	::GameServer::MSG_S2C_UserEnterMessageResponse rspdata;
	rspdata.mutable_header()->set_sign(HEADER_SIGN);
	rspdata.set_retcode(errcode);
	rspdata.set_errormsg(errmsg);
	send(conn, &rspdata, mainid, subid, pre_header_, header_);
}

void GameServ::db_refresh_game_room_info() {

}


void GameServ::redis_refresh_room_player_nums() {

}

void GameServ::redis_update_room_player_nums() {

}

void GameServ::on_refresh_game_config(std::string msg) {
	db_refresh_game_room_info();
}