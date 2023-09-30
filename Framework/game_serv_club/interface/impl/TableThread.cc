#include "Logger/src/log/Logger.h"
#include "Logger/src/log/Assert.h"
#include "public/gameConst.h"

#include "TableThread.h"
#include "TableMgr.h"
#include "Table.h"
#include "RobotMgr.h"

CTableThread::CTableThread(muduo::net::EventLoop* loop, ITableContext* tableContext)
	: loop_(ASSERT_NOTNULL(loop))
	, tableContext_(ASSERT_NOTNULL(tableContext)) {
	ASSERT(tableContext_->GetGameInfo());
	ASSERT(tableContext_->GetRoomInfo());
}

CTableThread::~CTableThread() {
}

void CTableThread::append(uint16_t tableId) {
	tableId_.emplace_back(tableId);
}

void CTableThread::startCheckUserIn() {
	if (enable()) {
		switch (tableContext_->GetGameInfo()->gameType) {
		case GameType_BaiRen: {
			loop_->runEvery(3.0, std::bind(&CTableThread::checkUserIn, this));
			break;
		}
		case GameType_Confrontation:
			loop_->runEvery(0.1f, std::bind(&CTableThread::checkUserIn, this));
			break;
		}
	}
}

bool CTableThread::enable() {
	return tableContext_->GetRoomInfo()->bEnableAndroid;
}

int64_t CTableThread::randScore(tagGameRoomInfo* roomInfo, int64_t minScore, int64_t maxScore) {
	int i = 0;
#if 0
	int r = weight_.rand().betweenInt(0, 1000).randInt_mt();
	for (; i < roomInfo->robotStrategy_.areas.size(); ++i) {
		if (r < roomInfo->robotStrategy_.areas[i].weight) {
			break;
		}
	}
#else
	int weight[roomInfo->robotStrategy_.areas.size()];
	for (i = 0; i < roomInfo->robotStrategy_.areas.size(); ++i) {
		weight[i] = roomInfo->robotStrategy_.areas[i].weight;
	}
	weight_.init(weight, roomInfo->robotStrategy_.areas.size());
	weight_.shuffle();
	i = weight_.getResult();
#endif
	int64_t lowLineScore = roomInfo->robotStrategy_.areas[i].lowTimes * minScore / 100;
	int64_t highLineScore = roomInfo->robotStrategy_.areas[i].highTimes * maxScore / 100;
	return weight_.rand().betweenInt64(lowLineScore, highLineScore).randInt64_mt();
}

//一次进n个机器人
int CTableThread::randomOnce(int32_t need, int N) {
	if (N > 1) {
		return (need > 1) ?
			weight_.rand().betweenInt(1, (need > N) ? N : need).randInt_mt() :
			need;
	}
	return (need > 1) ? 1 : need;
}

void CTableThread::checkUserIn() {
	if (tableContext_->GetRoomInfo()->serverStatus == kStopped) {
		return;
	}
	if (CRobotMgr::get_mutable_instance().Empty()) {
		//Errorf("机器人没有库存了");
		return;
	}
	std::list<std::shared_ptr<CTable>> tables = CTableMgr::get_mutable_instance().UsedTables(tableId_);
	for (auto it : tables) {
		std::shared_ptr<CTable>& table = it;
		if (table) {
			if (table->roomInfo_->serverStatus == kStopped) {
				continue;
			}
			if (!table->roomInfo_->bEnableAndroid) {
				continue;
			}
			table->assertThisThread();
			size_t realCount = 0, robotCount = 0;
			table->GetPlayerCount(realCount, robotCount);
			size_t playerCount = realCount + robotCount;
			size_t MaxPlayer = table->roomInfo_->maxPlayerNum;
			size_t MinPlayer = table->roomInfo_->minPlayerNum;
			size_t maxRobotCount = table->roomInfo_->maxAndroidCount;
			switch (table->tableContext_->GetGameInfo()->gameType) {
			case GameType_BaiRen: {
				//隔段时间计算机器人波动系数
				hourtimer(table->roomInfo_);
				maxRobotCount *= percentage_;
				//进入桌子的真人越多，允许进的机器人就越少
				if (table->roomInfo_->realChangeAndroid > 0) {
					maxRobotCount -= (int)realCount / table->roomInfo_->realChangeAndroid;
				}
				std::shared_ptr<CRobot> robot = std::make_shared<CRobot>();//ctor
				//std::shared_ptr<CRobot> robot = std::shared_ptr<CRobot>();//null
				if (!table->CanJoinTable(robot)) {
					continue;
				}
				if (table->roomInfo_->bEnableAndroid && playerCount < MaxPlayer && robotCount < maxRobotCount) {
					if (table->roomInfo_->realChangeAndroid > 0) {
						maxRobotCount -= (int)realCount / table->roomInfo_->realChangeAndroid;
					}
					int n = randomOnce(maxRobotCount - robotCount);
					for (int i = 0; i < n; ++i) {
						std::shared_ptr<CRobot> robot = CRobotMgr::get_mutable_instance().Pick();
						if (!robot) {
							Errorf("机器人没有库存了");
							continue;
						}
						int64_t score = randScore(
							table->roomInfo_,
							table->roomInfo_->enterMinScore > 0 ? table->roomInfo_->enterMinScore : robot->GetTakeMinScore(),
							table->roomInfo_->enterMaxScore > 0 ? table->roomInfo_->enterMaxScore : robot->GetTakeMaxScore());
						robot->SetUserScore(score);
						std::shared_ptr<IRobotDelegate> robotDelegate = robot->GetDelegate();
						if (robotDelegate) {
#if 0
							robotDelegate->Init(table, robot);
#else
							robotDelegate->SetTable(table);
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
				std::shared_ptr<CRobot> robot = std::make_shared<CRobot>();//ctor
				//std::shared_ptr<CRobot> robot = std::shared_ptr<CRobot>();//null
				if (!table->CanJoinTable(robot)) {
					continue;
				}
				if (table->roomInfo_->bEnableAndroid && realCount > 0 && playerCount < MaxPlayer && robotCount < maxRobotCount) {
					int n = randomOnce(maxRobotCount - robotCount, 1);
					for (int i = 0; i < n; ++i) {
						std::shared_ptr<CRobot> robot = CRobotMgr::get_mutable_instance().Pick();
						if (!robot) {
							Errorf("机器人没有库存了");
							continue;
						}
						int64_t score = randScore(
							table->roomInfo_,
							table->roomInfo_->enterMinScore > 0 ? table->roomInfo_->enterMinScore : robot->GetTakeMinScore(),
							table->roomInfo_->enterMaxScore > 0 ? table->roomInfo_->enterMaxScore : robot->GetTakeMaxScore());
						robot->SetUserScore(score);
						std::shared_ptr<IRobotDelegate> robotDelegate = robot->GetDelegate();
						if (robotDelegate) {
#if 0
							robotDelegate->Init(table, robot);
#else
							robotDelegate->SetTable(table);
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

void CTableThread::hourtimer(tagGameRoomInfo* roomInfo) {
	time_t now = time(NULL);
	static time_t last = 0;
	if (percentage_ == 0 || (now - last) > 3600) {
		struct tm* local = localtime(&now);
		uint8_t hour = (int)local->tm_hour;
		float r = (weight_.rand().betweenInt(0, 10).randInt_mt() + 95) * 0.01;//随机浮动一定比例0.9~1.1
		percentage_ = roomInfo->enterAndroidPercentage[hour] ?
			roomInfo->enterAndroidPercentage[hour] * (r) : 0.5 * (r);
		last = now;
	}
}

CTableThreadMgr::CTableThreadMgr() {
}

CTableThreadMgr::~CTableThreadMgr() {
	//quit();
}

void CTableThreadMgr::Init(muduo::net::EventLoop* loop, std::string const& name) {
	if (!pool_) {
		pool_.reset(new muduo::net::EventLoopThreadPool(ASSERT_NOTNULL(loop), name));
	}
}

muduo::net::EventLoop* CTableThreadMgr::getNextLoop() {
	ASSERT_S(pool_, "pool is nil");
	return pool_->getNextLoop();
}

std::shared_ptr<muduo::net::EventLoopThreadPool> CTableThreadMgr::get() {
	ASSERT_S(pool_, "pool is nil");
	return pool_;
}

void CTableThreadMgr::setThreadNum(int numThreads) {
	ASSERT_S(pool_, "pool is nil");
	ASSERT_V(numThreads > 0, "numThreads:%d", numThreads);
	pool_->setThreadNum(numThreads);
}

void CTableThreadMgr::startInLoop(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb) {
	pool_->start(cb);
}

void CTableThreadMgr::start(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb, ITableContext* tableContext) {
	ASSERT_S(pool_, "pool is nil");
	if (!pool_->started() && started_.getAndSet(1) == 0) {
		RunInLoop(pool_->getBaseLoop(), std::bind(&CTableThreadMgr::startInLoop, this, cb));
		
		std::vector<muduo::net::EventLoop*> loops = pool_->getAllLoops();
		for (std::vector<muduo::net::EventLoop*>::const_iterator it = loops.begin(); it != loops.end(); ++it) {
			(*it)->setContext(LogicThreadPtr(new CTableThread(*it, tableContext)));
		}
	}
}

void CTableThreadMgr::startCheckUserIn(ITableContext* tableContext) {
	switch (tableContext->GetGameInfo()->gameType) {
	case GameType_BaiRen: {
		std::shared_ptr<CPlayer> player = std::make_shared<CPlayer>();
		CTableMgr::get_mutable_instance().FindSuit(player, INVALID_TABLE);
		break;
	}
	case GameType_Confrontation:
		break;
	}
	if (CRobotMgr::get_mutable_instance().Empty()) {
		return;
	}
	std::vector<muduo::net::EventLoop*> loops = pool_->getAllLoops();
	for (std::vector<muduo::net::EventLoop*>::const_iterator it = loops.begin(); it != loops.end(); ++it) {
		boost::any_cast<LogicThreadPtr>((*it)->getContext())->startCheckUserIn();
	}
}

void CTableThreadMgr::quit() {
	ASSERT_S(pool_, "pool is nil");
	if (pool_->started()) {
		RunInLoop(pool_->getBaseLoop(), std::bind(&CTableThreadMgr::quitInLoop, this));
	}
}

void CTableThreadMgr::quitInLoop() {
	std::vector<muduo::net::EventLoop*> loops = pool_->getAllLoops();
	for (std::vector<muduo::net::EventLoop*>::iterator it = loops.begin();
		it != loops.end(); ++it) {
		(*it)->quit();
	}
	pool_.reset();
}