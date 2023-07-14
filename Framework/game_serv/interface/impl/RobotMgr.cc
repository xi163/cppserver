
#include "RobotMgr.h"
#include "TableMgr.h"
#include "Table.h"

static RobotDelegateCreator LoadLibrary(std::string const& serviceName) {
	std::string dllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
	dllPath.append("/");
	//./libGame_s13s_robot.so
	std::string dllName = clearDllPrefix(serviceName);//(gameInfo_->gameServiceName);
	dllName.append("_robot");
	dllName.insert(0, "./lib");
	dllName.append(".so");
	dllName.insert(0, dllPath);
	LOG_WARN << ">>> Loading " << dllName;
	//getchar();
	void* handle = dlopen(dllName.c_str(), RTLD_LAZY);
	if (!handle) {
		char buf[BUFSIZ] = { 0 };
		snprintf(buf, BUFSIZ, " Can't Open %s, %s", dllName.c_str(), dlerror());
		LOG_ERROR << __FUNCTION__ << buf;
		exit(0);
	}
	RobotDelegateCreator creator = (RobotDelegateCreator)dlsym(handle, NameCreateRobotDelegate);
	if (!creator) {
		dlclose(handle);
		char buf[BUFSIZ] = { 0 };
		snprintf(buf, BUFSIZ, " Can't Find %s, %s", NameCreateRobotDelegate, dlerror());
		LOG_ERROR << __FUNCTION__ << buf;
		exit(0);
	}
	return creator;
}

CRobotMgr::CRobotMgr() :
    gameInfo_(NULL)
    , roomInfo_(NULL)
    , tableContext_(NULL)
    //, logicThread_(NULL)
    , percentage_(0) {
}

CRobotMgr::~CRobotMgr() {
    freeItems_.clear();
    items_.clear();
    gameInfo_ = NULL;
    roomInfo_ = NULL;
    tableContext_ = NULL;
    //logicThread_ = NULL;
}

void CRobotMgr::Init(tagGameInfo* gameInfo, tagGameRoomInfo* roomInfo, std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext) {
	if (!gameInfo || !roomInfo) {
		return;
	}
    RobotDelegateCreator creator = LoadLibrary(gameInfo->gameServiceName);
	if (!creator) {
		exit(0);
	}
    gameInfo_  = gameInfo;
    roomInfo_  = roomInfo;
    //logicThread_ = logicThread;
    tableContext_ = tableContext;
    creator_ = creator;
}

void CRobotMgr::Load() {
	try {
		mongocxx::collection androidStrategy = MONGODBCLIENT["gameconfig"]["android_strategy"];
		bsoncxx::document::value query_value = document{} << "gameid" << (int32_t)roomInfo_->gameId << "roomid" << (int32_t)roomInfo_->roomId << finalize;
		bsoncxx::stdx::optional<bsoncxx::document::value> result = androidStrategy.find_one(query_value.view());
		if (result) {
			bsoncxx::document::view view = result->view();
			//LOG_DEBUG << "Query Android Strategy:" << bsoncxx::to_json(view);
			robotStrategy_.gameId = view["gameid"].get_int32();
			robotStrategy_.roomId = view["roomid"].get_int32();
			robotStrategy_.exitLowScore = view["exitLowScore"].get_int64();
			robotStrategy_.exitHighScore = view["exitHighScore"].get_int64();
			robotStrategy_.minScore = view["minScore"].get_int64();
			robotStrategy_.maxScore = view["maxScore"].get_int64();
			auto arr = view["areas"].get_array();
			for (auto& elem : arr.value) {
				AndroidStrategyArea area;
				area.weight = elem["weight"].get_int32();
				area.lowTimes = elem["lowTimes"].get_int32();
				area.highTimes = elem["highTimes"].get_int32();
				robotStrategy_.areas.emplace_back(area);
			}
		}
		uint32_t robotCount = 0;
		//房间桌子数 * 每张桌子最大机器人数
		uint32_t needRobotCount = roomInfo_->tableCount * roomInfo_->maxAndroidCount;
		//机器人账号
		mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["android_user"];
		bsoncxx::document::value query_value2 = document{} << "gameId" << (int32_t)roomInfo_->gameId << "roomId" << (int32_t)roomInfo_->roomId << finalize;
		mongocxx::cursor cursor = coll.find({query_value2});
		for (auto& doc : cursor) {
			//LOG_DEBUG << "QueryResult:" << bsoncxx::to_json(doc);
			UserBaseInfo userInfo;
			userInfo.userId = doc["userId"].get_int64();
			userInfo.account = doc["account"].get_utf8().value.to_string();
			userInfo.nickName = doc["nickName"].get_utf8().value.to_string();
			userInfo.headId = doc["headId"].get_int32();
			//doc["enterTime"].get_utf8().value.to_string();
			//doc["leaveTime"].get_utf8().value.to_string();
			//userInfo.takeMinScore = doc["minScore"].get_int64();
			//userInfo.takeMaxScore = doc["maxScore"].get_int64();
			userInfo.takeMinScore = robotStrategy_.minScore;
			userInfo.takeMaxScore = robotStrategy_.maxScore;
			userInfo.userScore = 0;
			userInfo.agentId = 0;
			userInfo.status = 0;
			userInfo.location = doc["location"].get_utf8().value.to_string();
			//创建子游戏机器人代理
			std::shared_ptr<IRobotDelegate> robotDelegate = creator_();
			//创建机器人
			std::shared_ptr<CRobot> robot(new CRobot());
			if (!robot || !robotDelegate) {
				LOG_ERROR << __FUNCTION__ << " create robot userid = " << userInfo.userId << " Failed!";
				break;
			}
            robot->SetUserBaseInfo(userInfo);
			robotDelegate->SetStrategy(&robotStrategy_);
			robot->Init(robotDelegate);
            freeItems_.emplace_back(robot);
			if (++robotCount >= needRobotCount) {
				break;
			}
			LOG_DEBUG << "Robot userId:" << userInfo.userId << " score:" << userInfo.userScore;
		}
		LOG_INFO << ">>> PreLoad Robot count:" << robotCount << " need:" << needRobotCount;
	}
	catch (std::exception& e) {
		LOG_ERROR << __FUNCTION__ << " exception:" << e.what();
	}
#if 0
    logicThread_->getLoop()->runEvery(3.0, boost::bind(&CRobotMgr::OnTimerCheckIn, this));
#endif
}

std::shared_ptr<CRobot> CRobotMgr::Pick() {
	std::shared_ptr<CRobot> robot;
    {
        //WRITE_LOCK(mutex_);
		if (!freeItems_.empty()) {
			robot = freeItems_.front();
			freeItems_.pop_front();
		}
    }
	if (robot) {
		robot->Reset();
		{
			//WRITE_LOCK(mutex_);
			items_[robot->GetUserId()] = robot;
		}
	}
	return robot;
}

void CRobotMgr::Delete(int64_t userId) {
	{
		//WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CRobot>>::iterator it = items_.find(userId);
		if (it != items_.end()) {
			std::shared_ptr<CRobot> robot = it->second;
			items_.erase(it);
			robot->Reset();
			freeItems_.emplace_back(robot);
		}
	}
    LOG_ERROR << __FUNCTION__ << " " << userId << " used = " << items_.size() << " free = " << freeItems_.size();
}

int64_t CRobotMgr::randScore(int64_t minScore, int64_t maxScore) {
	int i = 0;
#if 0
	int r = weight_.rand_.betweenInt(0, 1000).randInt_mt();
	for (; i < robotStrategy_.areas.size(); ++i) {
		if (r < robotStrategy_.areas[i].weight) {
			break;
		}
	}
#else
	int weight[robotStrategy_.areas.size()];
	for (i = 0; i < robotStrategy_.areas.size(); ++i) {
		weight[i] = robotStrategy_.areas[i].weight;
	}
	weight_.init(weight, robotStrategy_.areas.size());
	weight_.shuffle();
	i = weight_.getResult();
#endif
	int64_t lowLineScore = robotStrategy_.areas[i].lowTimes * minScore / 100;
	int64_t highLineScore = robotStrategy_.areas[i].highTimes * maxScore / 100;
	return weight_.rand_.betweenInt64(lowLineScore, highLineScore).randInt64_mt();
}

//一次进n个机器人
int CRobotMgr::randomOnce(int32_t need, int N) {
	if (N > 1) {
		return (need > 1) ?
			weight_.rand_.betweenInt(1, (need > N) ? N : need).randInt_mt() :
			need;
	}
	return (need > 1) ? 1 : need;
}

void CRobotMgr::OnTimerCheckIn() {
	if (!roomInfo_->bEnableAndroid) {
		return;
	}
	if (roomInfo_->serverStatus == SERVER_STOPPED) {
		return;
	}
	if (freeItems_.empty()) {
		LOG_ERROR << __FUNCTION__ << " 机器人没有库存了";
		return;
	}
	std::list<std::shared_ptr<CTable>> tables = CTableMgr::get_mutable_instance().GetUsedTables();
	for (auto it : tables) {
		std::shared_ptr<CTable>& table = it;
		if (table) {
			table->assertThisThread();
			uint32_t realCount = 0, robotCount = 0;
			table->GetPlayerCount(realCount, robotCount);
			uint32_t playerCount = realCount + robotCount;
			uint32_t MaxPlayer = roomInfo_->maxPlayerNum;
			uint32_t MinPlayer = roomInfo_->minPlayerNum;
			int32_t maxRobotCount = roomInfo_->maxAndroidCount;
			switch (gameInfo_->gameType) {
			case GameType_BaiRen: {
				//隔段时间计算机器人波动系数
				Hourtimer();
				maxRobotCount *= percentage_;
				//进入桌子的真人越多，允许进的机器人就越少
				if (roomInfo_->realChangeAndroid > 0) {
					maxRobotCount -= (int)realCount / roomInfo_->realChangeAndroid;
				}
				std::shared_ptr<CRobot> robot = std::make_shared<CRobot>();
				if (!table->CanJoinTable(robot)) {
					continue;
				}
				if (roomInfo_->bEnableAndroid && playerCount < MaxPlayer && robotCount < maxRobotCount) {
					if (roomInfo_->realChangeAndroid > 0) {
						maxRobotCount -= (int)realCount / roomInfo_->realChangeAndroid;
					}
					int n = randomOnce(maxRobotCount - robotCount);
					for (int i = 0; i < n; ++i) {
						std::shared_ptr<CRobot> robot = Pick();
						if (!robot) {
							LOG_ERROR << __FUNCTION__ << " 机器人没有库存了";
							continue;
						}
						int64_t score = randScore(
							roomInfo_->enterMinScore > 0 ? roomInfo_->enterMinScore : robot->GetTakeMinScore(),
							roomInfo_->enterMaxScore > 0 ? roomInfo_->enterMaxScore : robot->GetTakeMaxScore());
						robot->SetUserScore(score);
						std::shared_ptr<IRobotDelegate> robotDelegate = robot->GetDelegate();
						if (robotDelegate) {
#if 0
							robotDelegate->Init(table, robot);
#else
							robotDelegate->SetTable(it);
#endif
						}
						//更新DB机器人状态
						//UpdateAndroidStatus(robot->GetUserId(), 1);
						table->RoomSitChair(robot, NULL, NULL);
						//一次进一个机器人
						//break;
					}
				}
				break;
			}
			case GameType_Confrontation: {
				if (realCount == 0 || table->GetGameStatus() >= GAME_STATUS_START) {
					continue;
				}
				//对战游戏匹配前3.6秒都必须等待玩家加入桌子，禁入机器人，定时器到时空缺的机器人一次性填补
				//如果定时器触发前，真实玩家都齐了，秒开
				std::shared_ptr<CRobot> robot = std::make_shared<CRobot>();
				if (!table->CanJoinTable(robot)) {
					continue;
				}
				if (roomInfo_->bEnableAndroid && realCount > 0 && playerCount < MaxPlayer && robotCount < maxRobotCount) {
					int n = randomOnce(maxRobotCount - robotCount, 1);
					for (int i = 0; i < n; ++i) {
						std::shared_ptr<CRobot> robot = Pick();
						if (!robot) {
							LOG_ERROR << __FUNCTION__ << " 机器人没有库存了";
							continue;
						}
						int64_t score = randScore(
							roomInfo_->enterMinScore > 0 ? roomInfo_->enterMinScore : robot->GetTakeMinScore(),
							roomInfo_->enterMaxScore > 0 ? roomInfo_->enterMaxScore : robot->GetTakeMaxScore());
						robot->SetUserScore(score);
						std::shared_ptr<IRobotDelegate> robotDelegate = robot->GetDelegate();
						if (robotDelegate) {
#if 0
							robotDelegate->Init(table, robot);
#else
							robotDelegate->SetTable(it);
#endif
						}
						//更新DB机器人状态
						//UpdateAndroidStatus(robot->GetUserId(), 1);
						table->RoomSitChair(robot, NULL, NULL);
						//一次进一个机器人
						//break;
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

void CRobotMgr::Hourtimer() {
    time_t now = time(NULL);
    static time_t last = 0;
	if (percentage_ == 0 || (now - last) > 3600) {
		struct tm* local = localtime(&now);
		uint8_t hour = (int)local->tm_hour;
		float r = (weight_.rand_.betweenInt(0, 10).randInt_mt() + 95) * 0.01;//随机浮动一定比例0.9~1.1
        percentage_ = roomInfo_->enterAndroidPercentage[hour] ?
			roomInfo_->enterAndroidPercentage[hour] * (r) : 0.5 * (r);
		last = now;
	}
    //logicThread_->getLoop()->runAfter(60,std::bind(&CRobotMgr::Hourtimer,&CRobotMgr::get_mutable_instance()));
}