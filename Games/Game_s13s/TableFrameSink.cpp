#include "proto/s13s.Message.pb.h"

#include "s13s.h"
#include "funcC.h"
#include "cfg.h"

//#include "pb2Json.h"

#include "TableFrameSink.h"

#define TIME_GAME_START_DELAY 2

CGameTable::CGameTable(void) {
	maxAndroid_ = 0;
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//匹配真人超时时长(可配置)
    timeoutMatchSeconds_ = 3.5f;
	//补充机器人超时时长(可配置)
	timeoutAddAndroidSeconds_ = 0.4f;
	lastReadCfgTime_ = 0;
	readIntervalTime_ = 300;
	memset(bPlaying_, 0, sizeof(bPlaying_));
	for (int i = 0; i < GAME_PLAYER; ++i) {
		m_bPlayerOperated[i] = false;
        m_bRoundEndExit[i] = false;
    }
    m_lMarqueeMinScore = 10000;
	gameStatus_ = GAME_STATUS_INIT;
}

CGameTable::~CGameTable(void) {
}

bool CGameTable::SetTable(std::shared_ptr<ITable> const& table) {
	table_ = table; assert(table_);
	m_replay.cellscore = CellScore;//房间底注
	m_replay.roomname = ThisRoomName;
	m_replay.gameid = ThisGameId;//游戏类型
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	ReadConfigInformation();
    return true;
}

void CGameTable::ClearGameData() {
	//初始化玩家手牌
	memset(handCards_, 0, GAME_PLAYER * MAX_COUNT * sizeof(uint8_t));
	//确定牌型的玩家数
	selectcount = 0;
	//玩家比牌结果数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		compareCards_[i].Clear();
		m_bPlayerOperated[i] = false;
	}
	strRoundID_.clear();
}

std::string CGameTable::GetRoundId() {
	return strRoundID_;
}

void CGameTable::Reposition() {
}

void CGameTable::InitGameData() {
	assert(table_);
	memset(bPlaying_, 0, sizeof(bPlaying_));
	//初始化
	g.InitCards();
	//洗牌
	g.ShuffleCards();
	//初始化玩家手牌
	memset(handCards_, 0, GAME_PLAYER * MAX_COUNT * sizeof(uint8_t));
	//确定牌型的玩家数
	selectcount = 0;
	//玩家比牌结果数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		compareCards_[i].Clear();
		m_bPlayerOperated[i] = false;
	}
	m_replay.clear();
}

void CGameTable::ClearAllTimer() {
	ThisThreadTimer->cancel(timerIdGameReadyOver_);
	ThisThreadTimer->cancel(timerGroupID_);
	ThisThreadTimer->cancel(timerOpenCardID_);
	ThisThreadTimer->cancel(timerGameEndID_);
}

int CGameTable::randomMaxAndroidCount() {
	//return rand_.betweenInt(1, table_->GetRoomInfo()->maxAndroidCount).randInt_mt();
	return table_->GetRoomInfo()->maxAndroidCount;
}

bool CGameTable::OnUserEnter(int64_t userId, bool islookon) {
	assert(table_);
	std::shared_ptr<IPlayer> player = table_->GetPlayer(userId);
	assert(player);
	writeRealLog_ = (table_->GetRealPlayerCount() > 0);
	//if (!bPlaying_[player->GetChairId()]) {
		m_bPlayerOperated[player->GetChairId()] = true;
		m_bRoundEndExit[player->GetChairId()] = false;
	//}
	//准备时启动定时器
	if (table_->GetGameStatus() < GAME_STATUS_READY) {
		//必须真人玩家
		//assert(table_->GetPlayerCount() == 1);
		//assert(table_->GetRealPlayerCount() == 1);
		if (!player->IsRobot()) {
			_LOG_INFO("tableID[%d][%s][%s] %s %d %lld 首次入桌(real=%d AI=%d total=%d)",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(player->IsRobot() ? "AI" : "真人"), player->GetChairId(), userId,
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		else if (writeRealLog_) {
			_LOG_INFO("tableID[%d][%s][%s] %s %d %lld 首次入桌(real=%d AI=%d total=%d)",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(player->IsRobot() ? "AI" : "真人"), player->GetChairId(), userId,
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		table_->SetGameStatus(GAME_STATUS_READY);
		gameStatus_ = GAME_STATUS_READY;
		totalMatchSeconds_ = 0;
		if (maxAndroid_ == 0)
			maxAndroid_ = randomMaxAndroidCount();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CGameTable::OnTimerGameReadyOver, this));
		OnGameScene(player->GetChairId(), false);
	}
	else {
		if (!player->IsRobot()) {
			_LOG_INFO("tableID[%d][%s][%s] %s %d %lld %s入桌(real=%d AI=%d total=%d)",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(player->IsRobot() ? "AI" : "真人"), player->GetChairId(), userId, (bPlaying_[player->GetChairId()] ? "重连" : "新加"),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		else if (writeRealLog_) {
			_LOG_INFO("tableID[%d][%s][%s] %s %d %lld %s入桌(real=%d AI=%d total=%d)",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(player->IsRobot() ? "AI" : "真人"), player->GetChairId(), userId, (bPlaying_[player->GetChairId()] ? "重连" : "新加"),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		OnGameScene(player->GetChairId(), false);
	}
    return true;
}

bool CGameTable::CanJoinTable(std::shared_ptr<IPlayer> const& player) {
	//达到房间最大人数不能进了
	if (table_->GetPlayerCount() >= GAME_PLAYER) {
		std::shared_ptr<IPlayer> userItem = table_->GetPlayer(player->GetUserId());
		if (userItem && userItem->GetChairId() == player->GetChairId()) {//断线重连可以进
			//_LOG_ERROR("....%d", player->GetUserId());
			return true;
		}
		//_LOG_ERROR("....%d", player->GetUserId());
		return false;
	}
	if (player->GetUserId() == -1) { //new android enter
		//机器人先入桌的话要初始化
		if (maxAndroid_ == 0)
			maxAndroid_ = randomMaxAndroidCount();
		//游戏开始了机器人新玩家不准进入
		if (table_->GetGameStatus() >= GAME_STATUS_START) {
			//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " false 1";
			//_LOG_ERROR("....%d", player->GetUserId());
			return false;
		}
		//匹配真人时间或没有真人玩家，机器人不准进入
		if (totalMatchSeconds_ < timeoutMatchSeconds_ || table_->GetRealPlayerCount() < 1) {
			//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " false 2";
			//_LOG_ERROR("....%d", player->GetUserId());
			return false;
		}
		//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " " << table_->GetRobotPlayerCount() << "<" << maxAndroid_ << " true 1";
		//根据房间机器人配置来决定补充多少机器人
		//_LOG_ERROR("....%d  bool=%d", player->GetUserId(), table_->GetRobotPlayerCount() < maxAndroid_);
		return table_->GetRobotPlayerCount() < maxAndroid_;
	}
	else if (player->GetTableId() == INVALID_CHAIR) { //new real user enter
		//游戏开始了真人新玩家不准进入
		if (table_->GetGameStatus() >= GAME_STATUS_START) {
			//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " false 3";
			//_LOG_ERROR("....%d", player->GetUserId());
			return false;
		}
		if (table_->GetRealPlayerCount() > 0) {
			return false;
		}
		//_LOG_ERROR("....%d", player->GetUserId());
		//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " true 2";
		return true;
	}
	else {//断线重连
		std::shared_ptr<IPlayer> userItem = table_->GetPlayer(player->GetUserId());
		if (userItem) {
			//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " true 3";
			//_LOG_ERROR("....%d", player->GetUserId());
			return true;
		}
	}
	//_LOG_ERROR("....%d", player->GetUserId());
	//LOG_ERROR << __FUNCTION__ << " tableId = " << table_->GetTableId() << " false 4";
	return false;
}

bool CGameTable::CanLeftTable(int64_t userId) {
	std::shared_ptr<IPlayer> player = ByUserId(userId);
	if (!player) {
		return true;
	}
	uint16_t chairId = player->GetChairId();
	if (/*table_->GetGameStatus()*/gameStatus_ < GAME_STATUS_START ||
		/*table_->GetGameStatus()*/gameStatus_ >= GAME_STATUS_END ||
		!bPlaying_[chairId]) {
		return true;
	}
	return false;
}

bool CGameTable::OnUserReady(int64_t userId, bool islookon) {
    return true;
}

bool CGameTable::OnUserLeft(int64_t userId, bool lookon) {
	assert(table_);
	std::shared_ptr<IPlayer> player = ByUserId(userId); //assert(player);
	if (!player) {
		//assert(false);
		return false;
	}
	assert(player->GetChairId() >= 0);
	assert(player->GetChairId() < GAME_PLAYER);
	bool isAndroid = player->IsRobot();
	uint16_t chairId = player->GetChairId();
	//游戏未开始/已结束/非参与玩家
	if (/*table_->GetGameStatus()*/gameStatus_ < GAME_STATUS_START ||
		/*table_->GetGameStatus()*/gameStatus_ >= GAME_STATUS_END ||
		!bPlaying_[chairId]) {
		if (writeRealLog_) {
			_LOG_INFO("tableID[%d][%s] %s[%d][%d]准备离桌 real=%d AI=%d total=%d",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
        bPlaying_[chairId] = false;
        m_bRoundEndExit[chairId] = false;
        player->SetUserStatus(sOffline);
		table_->ClearTableUser(chairId, true, true);
		//匹配中没有真人玩家，清理腾出桌子
		if (/*table_->GetGameStatus()*/gameStatus_ == GAME_STATUS_READY &&
			table_->GetRealPlayerCount()/*table_->GetPlayerCount()*/ == 0) {
			if (writeRealLog_) {
				_LOG_INFO("tableID[%d][%s] %s[%d][%d]已经离桌(real=%d AI=%d total=%d) 重置桌子",
					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
					table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!table_->ExistUser(i)) {
					continue;
				}
				bPlaying_[chairId] = false;
				m_bRoundEndExit[chairId] = false;
				ByChairId(i)->SetUserStatus(sOffline);
				table_->ClearTableUser(i, true, true);
			}
			ClearAllTimer();
			table_->SetGameStatus(GAME_STATUS_INIT);
			gameStatus_ = GAME_STATUS_INIT;
			writeRealLog_ = false;
		}
		else {
			if (writeRealLog_) {
				_LOG_INFO("tableID[%d][%s] %s[%d][%d]已经离桌(real=%d AI=%d total=%d) 等待重置",
					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
					table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
		}
		return true;
	}
	else {
		_LOG_WARN("[%s][%d][%s] %s[%d][%d] 游戏中禁止离桌(real=%d AI=%d total=%d)",
			strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
			table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
	}
	return false;
}

//准备定时器
void CGameTable::OnTimerGameReadyOver() {
	writeRealLog_ = (table_->GetRealPlayerCount() > 0);
	//匹配中有真人玩家
	//assert(table_->GetRealPlayerCount() > 0);
	//匹配中准备状态
	assert((int)table_->GetGameStatus() == GAME_STATUS_READY);
	//累计匹配时间
	totalMatchSeconds_ += sliceMatchSeconds_;
	//匹配真人时间
	if (totalMatchSeconds_ <= timeoutMatchSeconds_) {
		if (table_->GetPlayerCount() < GAME_PLAYER) {
			if (writeRealLog_ > 0) {
				//不满游戏人数，继续等待
// 				_LOG_INFO("tableID[%d][%s]匹配真人时间(dt=%.1f|%.1f)，不满游戏人数(real=%d AI=%d total=%d)，继续等待",
// 					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
		}
		else {
			if (writeRealLog_ > 0) {
				//满足游戏人数，开始游戏
				_LOG_INFO("tableID[%d][%s]匹配真人时间(dt=%.1f|%.1f)，满足游戏人数(real=%d AI=%d total=%d)，开始游戏!",
					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
			GameTimerReadyOver();
		}
	}
	//匹配真人超时，补充机器人时间
	else if (totalMatchSeconds_ > timeoutMatchSeconds_ &&
		(totalMatchSeconds_ <= (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_))) {
		if (table_->GetPlayerCount() < GAME_PLAYER) {
			if (writeRealLog_ > 0) {
				//不满游戏人数，继续等待
// 				_LOG_INFO("tableID[%d][%s]补充机器人(max=%d)时间(dt=%.1f|%.1f)，不满游戏人数(real=%d AI=%d total=%d)，继续等待",
// 					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
		}
		else {
			if (writeRealLog_ > 0) {
				//满足游戏人数，开始游戏
				_LOG_INFO("tableID[%d][%s]补充机器人(max=%d)时间(dt=%.1f|%.1f)，满足游戏人数(real=%d AI=%d total=%d)，开始游戏!",
					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
			GameTimerReadyOver();
		}
	}
	//补充机器人超时
	else /*if (totalMatchSeconds_ > (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_))*/ {
		if (table_->GetPlayerCount() >= MIN_GAME_PLAYER) {
			if (writeRealLog_ > 0) {
				//满足最小游戏人数，开始游戏
				_LOG_INFO("tableID[%d][%s]补充机器人(max=%d)超时(dt=%.1f|%.1f)，满足最小游戏人数(real=%d AI=%d total=%d)，开始游戏!",
					table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
			}
			GameTimerReadyOver();
		}
		else {
			//匹配很久，不满最小游戏人数，重置桌子
			if (totalMatchSeconds_ > (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) + 10 && table_->GetRealPlayerCount() == 0) {
				if (writeRealLog_ > 0) {
					_LOG_INFO("tableID[%d][%s] 中止匹配 重置桌子 real=%d AI=%d total=%d",
						table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
						table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
				}
				//totalMatchSeconds_ = 0;
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (!table_->ExistUser(i)) {
						continue;
					}
					bPlaying_[i] = false;
					m_bRoundEndExit[i] = false;
					ByChairId(i)->SetUserStatus(sOffline);
					table_->ClearTableUser(i, true, true);
				}
				ClearAllTimer();
				table_->SetGameStatus(GAME_STATUS_INIT);
				gameStatus_ = GAME_STATUS_INIT;
				writeRealLog_ = false;
			}
			else {
				if (writeRealLog_ > 0) {
					//不满最小游戏人数，继续等待
// 					_LOG_INFO("tableID[%d][%s]补充机器人(max=%d)超时(dt=%.1f|%.1f)，不满最小游戏人数(real=%d AI=%d total=%d)，继续等待\n",
// 						table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
				}
			}
		}
	}
}

void CGameTable::CheckGameStart() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i);
		if (player) {
			//离线，踢出玩家
			if (player->GetUserStatus() == sOffline) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				table_->ClearTableUser(i, true, true);
			}
			//账号停用，踢出玩家
			else if (player->GetUserStatus() == sStop) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				player->SetUserStatus(sOffline);
				table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_STOP_CUR_USER);
			}
			//积分不足，踢出玩家
			else if (player->GetUserScore() < EnterMinScore) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				player->SetUserStatus(sOffline);
				table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
			//积分太多，踢出玩家
 			//else if (EnterMaxScore > 0 && player->GetUserScore() > EnterMaxScore) {
			//	bPlaying_[i] = false;
 			//	m_bRoundEndExit[i] = false;
 			//	player->SetUserStatus(sOffline);
 			//	table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORELIMIT);
 			//}
		}
	}
}

//结束准备，开始游戏
void CGameTable::GameTimerReadyOver() {
	writeRealLog_ = (table_->GetRealPlayerCount() > 0);
	ThisThreadTimer->cancel(timerIdGameReadyOver_);
	CheckGameStart();
	//有真人玩家且够游戏人数，开始游戏
	if (table_->GetRealPlayerCount() > 0 && table_->GetPlayerCount() >= MIN_GAME_PLAYER) {
		InitGameData();
		//游戏状态
		table_->SetGameStatus(GAME_STATUS_START);
		gameStatus_ = GAME_STATUS_START;
		//玩家状态
		for (int i = 0; i < GAME_PLAYER; ++i) {
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i);
			if (player) {
				//非离线状态\
				//_LOG_WARN("%d %d %s", i, player->GetUserId(), StringPlayerStat(player->GetUserStatus()).c_str());
				//assert(player->GetUserStatus() != sOffline);
				player->SetUserStatus(sPlaying);
				bPlaying_[i] = true;
			}
			else {
				bPlaying_[i] = false;
			}
		}
		//开始游戏
		OnGameStart();
	}
	else if (table_->GetRealPlayerCount() > 0) {
	//else if (table_->GetPlayerCount() > 0) {
		if (writeRealLog_) {
			_LOG_INFO("tableID[%d][%s][%s] 不满最小游戏人数(real=%d AI=%d total=%d)，重新匹配!",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		//ClearGameData();
		//table_->SetGameStatus(GAME_STATUS_READY);
		//gameStatus_ = GAME_STATUS_READY;
		//totalMatchSeconds_ = 0;
		maxAndroid_ = randomMaxAndroidCount();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CGameTable::OnTimerGameReadyOver, this));
	}
	//继续下一局可能走该流程
	else {
		if (writeRealLog_) {
			_LOG_INFO("tableID[%d][%s] 中断游戏 重置桌子 real=%d AI=%d total=%d",
				table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		//没有真人玩家或不够游戏人数(<MIN_GAME_PLAYER)，清理腾出桌子
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!table_->ExistUser(i)) {
				continue;
			}
			bPlaying_[i] = false;
			m_bRoundEndExit[i] = false;
			ByChairId(i)->SetUserStatus(sOffline);
			table_->ClearTableUser(i, true, true);
		}
		ClearAllTimer();
		table_->SetGameStatus(GAME_STATUS_INIT);
		gameStatus_ = GAME_STATUS_INIT;
		writeRealLog_ = false;
	}
}

void CGameTable::OnGameStart() {
	ReadConfigInformation();
	//牌局编号
    strRoundID_ = table_->NewRoundId();
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	//系统当前库存
	//table_->GetStorageScore(storageInfo_);
	if (writeRealLog_) {
		_LOG_INFO("tableID[%d][%s][%s] 当前库存=%s 开始游戏(real=%d AI=%d total=%d)!",
			table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(), std::to_string(StockScore).c_str(),
			table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
	}
	//本局开始时间
    roundStartTime_ = std::chrono::system_clock::now();
	aniPlay_.add_START_time(AnimatePlay::GroupE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::OpenE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::PreEndE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::NextE, roundStartTime_);
	//
	aniPlay_.add_DELAY_time(AnimatePlay::GroupE, groupTime_);
	aniPlay_.add_DELAY_time(AnimatePlay::OpenE, openCardTime_);
	aniPlay_.add_DELAY_time(AnimatePlay::PreEndE, winLostTime_);
	aniPlay_.add_DELAY_time(AnimatePlay::NextE, nextTime_);
    //配置牌型
    int enum_group_sz = 3;
	std::string strFile = "./Card/S13sCards.ini";
	int flag = 0;
	if (!strFile.empty() && boost::filesystem::exists(strFile)) {
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(strFile, pt);
		//1->文件读取手牌 0->随机牌
		flag = pt.get<int>("StartDeal.ReadCardsFromFile", 0);
		if (flag > 0) {
			//构造玩家手牌
			int idx = 0;
			std::vector<std::string> v;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!bPlaying_[i]) {
					continue;
				}
				char chair[256];
				snprintf(chair, sizeof(chair), "StartDeal.Chair%d", idx++);
				std::string s = pt.get<std::string>(chair, "");
				boost::split(v,
					s,
					boost::is_any_of(" "), boost::token_compress_on);
				assert(v.size() == MAX_COUNT);
				int n = S13S::CGameLogic::MakeCardList(
					s,
					&(handCards_[i])[0], MAX_COUNT);
				assert(n == MAX_COUNT);
				std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
				m_replay.addPlayer(player->GetUserId(), player->GetNickName(), player->GetUserScore(), i);
			}
		}
	}
	if (!flag) {
		//给各个玩家发牌
	restart:
		assert(table_->GetPlayerCount() <= GAME_PLAYER);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			//余牌不够则重新洗牌，然后重新分发给各个玩家
			if (g.Remaining() < MAX_COUNT) {
				g.ShuffleCards();
				goto restart;
			}
			g.DealCards(MAX_COUNT, &(handCards_[i])[0]);
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			m_replay.addPlayer(player->GetUserId(), player->GetNickName(), player->GetUserScore(), i);
		}
    }
	//各个玩家手牌分析
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
		//机器人AI
		if (table_->IsRobot((uint32_t)i)) {
			_LOG_INFO("[%s] 机器人 %d %d 手牌 [%s]", strRoundID_.c_str(), i, player->GetUserId(), S13S::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT).c_str());
		}
		else {
			_LOG_INFO("[%s] 玩家 %d %d 手牌 [%s]", strRoundID_.c_str(), i, player->GetUserId(), S13S::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT).c_str());
		}
		//一副手牌
		//S13S::CGameLogic::PrintCardList(&(handCards_[i])[0], MAX_COUNT);
		//手牌排序
		S13S::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT, true, true, true);
		//手牌牌型分析
		int c = S13S::CGameLogic::AnalyseHandCards(&(handCards_[i])[0], MAX_COUNT, enum_group_sz, handInfos_[i],
			ThisRoomId, ThisTableId, i);
		phandInfos_[i] = &handInfos_[i];
		//有特殊牌型时
		//if (handInfos_[i].specialTy_ == S13S::Tysp) {
		//	goto restart;
		//}
		//查看所有枚举牌型
		//handInfos_[i].rootEnumList->PrintEnumCards(false, S13S::Ty123sc);
		//查看手牌特殊牌型
		//handInfos_[i].PrintSpecCards();
		//查看手牌枚举三墩牌型
		//handInfos_[i].PrintEnumCards();
		//查看重复牌型和散牌
		//handInfos_[i].classify.PrintCardList();
		//_LOG_INFO("c = %d %s\n\n\n\n", c, phandInfos_[i]->StringSpecialTy().c_str());
	}
    //换牌策略分析
	AnalysePlayerCards();
	//给机器人发送所有人的牌数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有玩家
        if (!bPlaying_[i]) {
			continue;
		}
		//跳过真人玩家
		if (!table_->IsRobot((uint32_t)i)) {
			continue;
		}
		//给机器人发数据
 		s13s::CMD_S_AndroidCard reqdata;
		//牌局编号
		reqdata.set_roundid(strRoundID_);
 		for (int j = 0; j < GAME_PLAYER; ++j) {
 			//座椅有玩家
            if (!bPlaying_[j]) {
 				continue;
 			}
			//跳过自己，自己的手牌数据从CMD_S_GameStart中传递
			if (j == i) {
				continue;
			}
 			//其它玩家数据(真实玩家/其它机器人)
 			s13s::AndroidPlayer* player = reqdata.add_players();
 			//桌椅ID
 			player->set_chairid(j);
 			//玩家手牌
 			s13s::HandCards* handcards = player->mutable_handcards();
 			//一副13张手牌
 			handcards->set_cards(&(handCards_[j])[0], MAX_COUNT);
 			//标记手牌特殊牌型
 			handcards->set_specialty(phandInfos_[j]->SpecialTy());
 			//
 		}
		//发送机器人数据
 		std::string content = reqdata.SerializeAsString();
 		table_->SendTableData(i, s13s::SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
	}
	//理牌剩余时间
	uint32_t wTimeLeft = groupTime_ + TIME_GAME_START_DELAY;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有玩家
        if (!bPlaying_[i]) {
			continue;
		}
		//真实/机器人玩家
		//if (table_->IsRobot(i)) {
		//	continue;
		//}
		//printf("\n\n========================================================================\n");
		//printf("--- *** chairID = [%d]\n", i);
		//一副手牌
		//S13S::CGameLogic::PrintCardList(&(handCards_[i])[0], MAX_COUNT);
		//查看重复牌型和散牌
		//phandInfos_[i]->classify.PrintCardList();
		//查看所有枚举牌型
		//phandInfos_[i]->rootEnumList->PrintEnumCards(false, S13S::Ty123sc);
		//查看手牌特殊牌型
		//phandInfos_[i]->PrintSpecCards();
		//查看手牌枚举三墩牌型
		//phandInfos_[i]->PrintEnumCards();
		//printf("--- *** c = %d %s\n\n\n\n", phandInfos_[i]->groups.size(), phandInfos_[i]->StringSpecialTy().c_str());
		//游戏开始，填充相关数据
 		s13s::CMD_S_GameStart reqdata;
		reqdata.set_tableid(ThisTableId);
        for (int j = 0; j < GAME_PLAYER; j++)
        {
            //玩家个人信息
            std::shared_ptr<IPlayer> userItem = table_->GetChairPlayer(j);
            if (userItem)
            {
                s13s::PlayerItem* player = reqdata.add_players();
                player->set_chairid(j);
              //  player->set_isingame(bPlaying_[j]);
                player->set_userid(userItem->GetUserId());      //userId
                player->set_nickname(userItem->GetNickName());  //nickname
                player->set_headid(userItem->GetHeaderId());    //headID
                player->set_status(userItem->GetUserStatus());  //userstatus
                player->set_score(userItem->GetUserScore());    //userScore
                player->set_location(userItem->GetLocation());  //location
				//player->set_viplevel(userItem->GetVip());
				//player->set_headboxindex(userItem->GetHeadboxId());
				//player->set_headimgurl(userItem->GetHeadImgUrl());
                //玩家手牌/是否理牌
                player->set_selected(phandInfos_[i]->HasSelected() ? 1 : 0); //selected
            }
        }
		//一副手牌
		s13s::HandCards* handcards = reqdata.mutable_handcards();
		//一副13张手牌
		handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
		//标记手牌特殊牌型
		handcards->set_specialty(phandInfos_[i]->SpecialTy());
		int j = 0;
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[i]->spec_groups.begin();
			it != phandInfos_[i]->spec_groups.end(); ++it) {
			assert(phandInfos_[i]->spec_groups.size() == 1);
			//特殊牌型放在枚举几组最优解前面
			s13s::GroupDunData* group = handcards->add_groups();
			//从哪墩开始的
			group->set_start(it->start);
			//总体对应特殊牌型
			group->set_specialty(it->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			//printf("第[%d]组\t=>>\t", j++ + 1);
			for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* dun_i = group->add_duns();
				//标记0-头/1-中/2-尾
				dun_i->set_id(k);
				//墩对应普通牌型
				dun_i->set_ty(it->duns[k].ty_);
				//墩对应牌数c(3/5/5)
				dun_i->set_c(it->duns[k].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
                //printf("dun[%d] c:=%d\t", k, it->duns[k].c);
			}
			//printf("\n\n");
		}
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[i]->enum_groups.begin();
			it != phandInfos_[i]->enum_groups.end(); ++it) {
			//枚举几组最优墩
			s13s::GroupDunData* group = handcards->add_groups();
			//从哪墩开始的
			group->set_start(it->start);
			//总体对应特殊牌型
			group->set_specialty(it->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			//printf("第[%d]组\t=>>\t", j++ + 1);
			for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* dun_i = group->add_duns();
				//标记0-头/1-中/2-尾
				dun_i->set_id(k);
				//墩对应普通牌型
				dun_i->set_ty(it->duns[k].ty_);
				//墩对应牌数c(3/5/5)
				dun_i->set_c(it->duns[k].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
				//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
			}
			//printf("\n\n");
		}
		//局数编号
		reqdata.set_roundid(strRoundID_);
		//基础积分
		reqdata.set_ceilscore(FloorScore);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			int64_t userscore = 0;
			int playerstatus = 0;
            if (bPlaying_[i]) {
				std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
				userscore = player->GetUserScore();
				playerstatus = player->GetUserStatus();
			}
					
			//玩家当前状态
			reqdata.add_playstatus(playerstatus);
					
			//玩家当前积分
			reqdata.add_userscore(userscore);
		}
		//理牌剩余时间
		reqdata.set_wtimeleft(wTimeLeft);
		//序列化std::string
		//std::string content = reqdata.SerializeAsString();
		//table_->SendTableData(i, s13s::SUB_S_GAME_START, (uint8_t *)content.data(), content.size());
		//序列化bytes
		int len = reqdata.ByteSize();//len
		uint8_t *data = new uint8_t[len];
		reqdata.SerializeToArray(data, len);//data
		std::string const& typeName = reqdata.GetTypeName();//typename
		table_->SendTableData(i, s13s::SUB_S_GAME_START, data, len);
		delete[] data;
		//转换json格式
		//std::string jsonstr;
		//PB2JSON::Pb2Json::PbMsg2JsonStr(reqdata, jsonstr, true);
		//snprintf(msg, sizeof(msg), "\n--- *** %s\n", typeName.c_str());
		/////LOG_WARN << __FUNCTION__ << " ["<< strRoundID_<< "] " << msg << jsonstr << "\n\n";
		//printf("%s\n\n", jsonstr.c_str());
		
	//	}
		//机器人
	//	else {
			//给机器人发送所有人牌数据
			//CMD_S_AndroidCard AndroidCard;
			//for (int j = 0; j < GAME_PLAYER; ++j) {
			//	for (int k = 0; k < MAX_COUNT; ++k) {
			//		//handCards_[i][j];
			//	}
			//}
			//std::string content = AndroidCard.SerializeAsString();
			//table_->SendTableData(i, SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
	//	}
	}
	//理牌开始时间
	aniPlay_.add_START_time(AnimatePlay::GroupE, std::chrono::system_clock::now());
	//修改桌子状态
	table_->SetGameStatus(GAME_STATUS_GROUP);
	gameStatus_ = GAME_STATUS_GROUP;
	//游戏开始，开始理牌(25s)
	IsTrustee();
}

bool CGameTable::OnGameScene(uint16_t chairId, bool lookon) {
	_LOG_INFO("tableID[%d][%s][%s] chairId[%d]",
		table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(), strRoundID_.c_str(), chairId);
	assert(chairId >= 0);
	assert(chairId < GAME_PLAYER);
	switch (/*table_->GetGameStatus()*/gameStatus_)
	{
		//空闲状态
	case GAME_STATUS_INIT:
	case GAME_STATUS_READY:
	case GAME_STATUS_START:
	case GAME_STATUS_NEXT: {
		s13s::CMD_S_StatusFree StatusFree;
		StatusFree.set_tableid(ThisTableId);
		StatusFree.set_ceilscore(FloorScore);//ceilscore
		if (table_->GetGameStatus() == GAME_STATUS_NEXT) {
			StatusFree.set_wtimeleft(aniPlay_.Get_LEFT_time(GAME_STATUS_NEXT, std::chrono::system_clock::now()));
		}
		else {
			StatusFree.set_wtimeleft(0);
		}
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			std::shared_ptr<IPlayer> userItem = table_->GetChairPlayer(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusFree.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userId
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				//player->set_isingame(bPlaying_[i]);
				//player->set_viplevel(userItem->GetVip());
				//player->set_headboxindex(userItem->GetHeadboxId());
				//player->set_headimgurl(userItem->GetHeadImgUrl());
			}
		}
        //发送场景
        std::string content = StatusFree.SerializeAsString();
        table_->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_FREE, (uint8_t *)content.data(), content.size());
        break;
    }
		//理牌状态
	case GAME_STATUS_GROUP: {
		s13s::CMD_S_StatusGroup StatusGroup;
		StatusGroup.set_tableid(ThisTableId);
		StatusGroup.set_roundid(strRoundID_);//roundId
		StatusGroup.set_ceilscore(FloorScore);//ceilscore
		StatusGroup.set_wtimeleft(aniPlay_.Get_LEFT_time(GAME_STATUS_GROUP, std::chrono::system_clock::now()));
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			std::shared_ptr<IPlayer> userItem = table_->GetChairPlayer(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusGroup.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userId
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				player->set_isingame(bPlaying_[i]);
				//player->set_viplevel(userItem->GetVip());
				//player->set_headboxindex(userItem->GetHeadboxId());
				//player->set_headimgurl(userItem->GetHeadImgUrl());
				if (bPlaying_[i]) {
					//玩家手牌/是否理牌
					player->set_selected(phandInfos_[i]->HasSelected() ? 1 : 0); //selected
				}
			}
		}
		//玩家没有确认牌型，发送手牌
		if (bPlaying_[chairId] && !phandInfos_[chairId]->HasSelected()) {
			//玩家手牌
			s13s::HandCards* handcards = StatusGroup.mutable_handcards();
			//一副13张手牌
			handcards->set_cards(&(handCards_[chairId])[0], MAX_COUNT);
			//标记手牌特殊牌型
			handcards->set_specialty(phandInfos_[chairId]->SpecialTy());
			int j = 0;
			for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[chairId]->spec_groups.begin();
				it != phandInfos_[chairId]->spec_groups.end(); ++it) {
				assert(phandInfos_[chairId]->spec_groups.size() == 1);
				//特殊牌型放在枚举几组最优解前面
				s13s::GroupDunData* group = handcards->add_groups();
				//从哪墩开始的
				group->set_start(it->start);
				//总体对应特殊牌型
				group->set_specialty(it->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				//printf("第[%d]组\t=>>\t", j++ + 1);
				for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* dun_i = group->add_duns();
					//标记0-头/1-中/2-尾
					dun_i->set_id(k);
					//墩对应普通牌型
					dun_i->set_ty(it->duns[k].ty_);
					//墩对应牌数c(3/5/5)
					dun_i->set_c(it->duns[k].c);
					//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
					dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
					//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
				}
				//printf("\n\n");
			}
			for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[chairId]->enum_groups.begin();
				it != phandInfos_[chairId]->enum_groups.end(); ++it) {
				//枚举几组最优墩
				s13s::GroupDunData* group = handcards->add_groups();
				//从哪墩开始的
				group->set_start(it->start);
				//总体对应特殊牌型
				group->set_specialty(it->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				//printf("第[%d]组\t=>>\t", j++ + 1);
				for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* dun_i = group->add_duns();
					//标记0-头/1-中/2-尾
					dun_i->set_id(k);
					//墩对应普通牌型
					dun_i->set_ty(it->duns[k].ty_);
					//墩对应牌数c(3/5/5)
					dun_i->set_c(it->duns[k].c);
					//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
					dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
					//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
				}
				//printf("\n\n");
			}
		}
		//发送场景
		std::string content = StatusGroup.SerializeAsString();
		table_->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_GROUP, (uint8_t*)content.data(), content.size());
		break;
	}
		//开牌状态
	case GAME_STATUS_OPEN: {
		s13s::CMD_S_StatusOpen StatusOpen;
		StatusOpen.set_tableid(ThisTableId);
		StatusOpen.set_roundid(strRoundID_);//roundId
		StatusOpen.set_ceilscore(FloorScore);//ceilscore
		StatusOpen.set_wtimeleft(aniPlay_.Get_LEFT_time(GAME_STATUS_OPEN, std::chrono::system_clock::now()));
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			std::shared_ptr<IPlayer> userItem = table_->GetChairPlayer(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusOpen.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userId
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				player->set_isingame(bPlaying_[i]);
				//player->set_viplevel(userItem->GetVip());
				//player->set_headboxindex(userItem->GetHeadboxId());
				//player->set_headimgurl(userItem->GetHeadImgUrl());
				if (bPlaying_[i]) {
					//玩家手牌/是否理牌
					player->set_selected(phandInfos_[i]->HasSelected() ? 1 : 0); //selected
				}
			}
		}
		if (bPlaying_[chairId]) {
			//玩家比牌数据
			s13s::CMD_S_CompareCards* compareCards = StatusOpen.mutable_cmp();
			compareCards->CopyFrom(compareCards_[chairId]);
		}
		else {
			//随机玩家作为参考比牌对象
			short nextChairId = 0;
			for (; nextChairId < GAME_PLAYER; ++nextChairId) {
				if (bPlaying_[nextChairId]) {
					break;
				}
			}
			//玩家比牌数据
			s13s::CMD_S_CompareCards* compareCards = StatusOpen.mutable_cmp();
			compareCards->CopyFrom(compareCards_[nextChairId]);
		}
		//发送场景
		std::string content = StatusOpen.SerializeAsString();
		table_->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_OPEN, (uint8_t*)content.data(), content.size());
		break;
	}
		//结束状态
	case GAME_STATUS_END:
	case GAME_STATUS_PREEND: {
		s13s::CMD_S_StatusEnd StatusEnd;
		StatusEnd.set_tableid(ThisTableId);
		StatusEnd.set_roundid(strRoundID_);//roundId
		StatusEnd.set_ceilscore(FloorScore);//ceilscore
		StatusEnd.set_wtimeleft(aniPlay_.Get_LEFT_time(GAME_STATUS_PREEND, std::chrono::system_clock::now()));
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			std::shared_ptr<IPlayer> userItem = table_->GetChairPlayer(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusEnd.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userId
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				player->set_isingame(bPlaying_[i]);
				//player->set_viplevel(userItem->GetVip());
				//player->set_headboxindex(userItem->GetHeadboxId());
				//player->set_headimgurl(userItem->GetHeadImgUrl());
				if (bPlaying_[i]) {
					//玩家手牌/是否理牌
					player->set_selected(phandInfos_[i]->HasSelected() ? 1 : 0); //selected
					//输赢积分
					s13s::GameEndScore* score = StatusEnd.add_scores();
					score->set_chairid(i);
					score->set_score(score_[i].score());
					score->set_userscore(score_[i].userscore());
				}
			}
		}
		if (bPlaying_[chairId]) {
			//玩家比牌数据
			s13s::CMD_S_CompareCards* compareCards = StatusEnd.mutable_cmp();
			compareCards->CopyFrom(compareCards_[chairId]);
		}
		else {
			//随机玩家作为参考比牌对象
			short nextChairId = 0;
			for (; nextChairId < GAME_PLAYER; ++nextChairId) {
				if (bPlaying_[nextChairId]) {
					break;
				}
			}
			//玩家比牌数据
			s13s::CMD_S_CompareCards* compareCards = StatusEnd.mutable_cmp();
			compareCards->CopyFrom(compareCards_[nextChairId]);
		}
		//发送场景
		std::string content = StatusEnd.SerializeAsString();
		table_->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_END, (uint8_t*)content.data(), content.size());
        break;
    }
    default:
		assert(false);
        break;
    }
    return true;
}

//理牌
void CGameTable::OnTimerGroupCard() {
	ThisThreadTimer->cancel(timerGroupID_);


// 	char msg[1024];

// 	


	
	//定时器到期，未组牌的强制组牌
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//过滤非参与玩家
		if (!bPlaying_[i]) {
			continue;
		}
		//默认第0组墩
		OnUserSelect(i, 0, true);
	}
}

//定牌
void CGameTable::OnUserSelect(uint16_t chairId, int groupIndex, bool timeout) {
	//玩家已经确认过牌型
	if (phandInfos_[chairId]->HasSelected()) {
		return;
	}
	//是否进行过手动任意摆牌
	if (groupIndex == -1 && !phandInfos_[chairId]->HasManualGroup()) {
		assert(phandInfos_[chairId]->GetManualC() < MAX_COUNT);
		return;
	}
	switch (timeout) {
	case true:
		_LOG_INFO("[%s][%d][%s] %d %d %s %s",
			strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
			chairId, UserIdBy(chairId), (ByChairId(chairId)->IsRobot() ? "robot" : "real"),
			"理牌超时，强制定牌 ...");
	}
	//确认玩家手牌三墩牌型
	if (!phandInfos_[chairId]->Select(groupIndex)) {
		return;
	}
	//对局日志详情
	{
		//单墩牌(3/5/5)
		std::string strDunCards;
		for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
			if (strDunCards.empty()) {
				strDunCards = S13S::CGameLogic::StringCards(
					phandInfos_[chairId]->GetSelected()->duns[d].cards,
					phandInfos_[chairId]->GetSelected()->duns[d].GetC());
			}
			else {
				strDunCards += "," +
					S13S::CGameLogic::StringCards(
						phandInfos_[chairId]->GetSelected()->duns[d].cards,
						phandInfos_[chairId]->GetSelected()->duns[d].GetC());
			}
		}
		m_replay.addStep((uint32_t)time(NULL) - std::chrono::system_clock::to_time_t(roundStartTime_), strDunCards, -1, 0, chairId, chairId);
	}
	//广播确定牌型消息
	s13s::CMD_S_MakesureDunHandTy rspdata;
	rspdata.set_chairid(chairId);
	std::string content = rspdata.SerializeAsString();
	table_->SendTableData(INVALID_CHAIR, s13s::SUB_S_MAKESUREDUNHANDTY, (uint8_t*)content.data(), content.size());
	//检查是否都确认牌型了
	size_t userCount = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			++userCount;
		}
	}
	//都组牌了，开始摊牌
	if (++selectcount == userCount) {
		//玩家之间两两比牌
		StartCompareCards();
		//摊牌动画开始时间
		aniPlay_.add_START_time(AnimatePlay::OpenE, std::chrono::system_clock::now());
		//修改桌子状态
		table_->SetGameStatus(GAME_STATUS_OPEN);
		gameStatus_ = GAME_STATUS_OPEN;
		//都下注了，开始摊牌(5s)
		IsTrustee();
	}
}

//摊牌
void CGameTable::OnTimerOpenCard() {
	_LOG_INFO("[%s][%d][%s] 摊牌结束，开始结算 ......",
		strRoundID_.c_str(),table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
	//结算
	OnGameConclude(INVALID_CHAIR, GER_NORMAL);
	//IsTrustee();
	//通知框架结束游戏
	//table_->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	//table_->SetGameStatus(GAME_STATUS_END);
	gameStatus_ = GAME_STATUS_END;
	OnTimerGameEnd();
}

//结束/下一局
void CGameTable::OnTimerGameEnd() {
	//通知框架结束游戏
	table_->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	table_->SetGameStatus(GAME_STATUS_END);
	ClearAllTimer();
	clearKickUsers();
	//有真人玩家且够游戏人数，继续下一局游戏
	if (table_->GetRealPlayerCount() > 0 && table_->GetPlayerCount() >= MIN_GAME_PLAYER) {
		if (writeRealLog_) {
			_LOG_INFO("[%s][%d][%s] 继续下一局游戏!",
				strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
		}
		ClearGameData();
		table_->SetGameStatus(GAME_STATUS_READY);
	    gameStatus_ = GAME_STATUS_READY;
		aniPlay_.add_START_time(AnimatePlay::NextE, std::chrono::system_clock::now());
		maxAndroid_ = randomMaxAndroidCount();
		timerIdGameReadyOver_ = ThisThreadTimer->runAfter(aniPlay_.Get_DELAY_time(GAME_STATUS_NEXT), boost::bind(&CGameTable::GameTimerReadyOver, this));
	}
	else if (table_->GetRealPlayerCount() > 0) {
	//else if (table_->GetPlayerCount() > 0) {
		if (writeRealLog_) {
			_LOG_INFO("[%s][%d][%s] 不满最小游戏人数(real=%d AI=%d total=%d)，重新匹配!",
				strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		ClearGameData();
		table_->SetGameStatus(GAME_STATUS_READY);
	    gameStatus_ = GAME_STATUS_READY;
		//totalMatchSeconds_ = 0;
		maxAndroid_ = randomMaxAndroidCount();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CGameTable::OnTimerGameReadyOver, this));
	}
	else {
		//没有真人玩家或不够游戏人数(<MIN_GAME_PLAYER)，清理腾出桌子
		if (writeRealLog_) {
			_LOG_INFO("[%s][%d][%s] 终止游戏并退出(real=%d AI=%d total=%d)",
				strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
				table_->GetRealPlayerCount(), table_->GetRobotPlayerCount(), table_->GetPlayerCount());
		}
		ClearGameData();
		//清理房间内玩家
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!table_->ExistUser(i)) {
				continue;
			}
			bPlaying_[i] = false;
			m_bRoundEndExit[i] = false;
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			player->SetUserStatus(sOffline);
			table_->ClearTableUser(i, true, true);
		}
		//重置游戏初始化
		table_->SetGameStatus(GAME_STATUS_INIT);
		gameStatus_ = GAME_STATUS_INIT;
		writeRealLog_ = false;
	}
}

//踢人用户清理
void CGameTable::clearKickUsers() {
	//有真人的场机器人离开概率小，没有真人的场机器人离开概率大
	int randValue = (table_->GetRealPlayerCount() > 0) ? 35 : 85;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i);
		if (player) {
			//离线，踢出玩家-掐断网关需要游戏服更改用户离线
			if (player->GetUserStatus() == sOffline) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				table_->ClearTableUser(i, true, true);
			}
			//账号停用，踢出玩家
// 			else if (player->GetUserStatus() == sStop) {
// 				bPlaying_[i] = false;
// 				m_bRoundEndExit[i] = false;
// 				player->SetUserStatus(sOffline);
// 				table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_STOP_CUR_USER);
// 			}
			//积分不足，踢出玩家
			else if (player->GetUserScore() < EnterMinScore) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				player->SetUserStatus(sOffline);
				table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
			//积分太多，踢出玩家
// 			else if (EnterMaxScore > 0 && player->GetUserScore() > EnterMaxScore) {
// 				bPlaying_[i] = false;
// 				m_bRoundEndExit[i] = false;
// 				player->SetUserStatus(sOffline);
// 				table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORELIMIT);
// 			}
// 			else if (bPlaying_[i] && !player->IsRobot()) {
// 				////LOG_INFO << __FUNCTION__ << " table[" << table_->GetTableId() << "][" << StringStat(table_->GetGameStatus()).c_str() << "][" << strRoundID_ << "] "
// 				//	<< player->GetChairId() << " m_bPlayerOperated[" << player->GetUserId() << "]=" << (m_bPlayerOperated[player->GetChairId()] ? "true" : "false");
// 				//本局结束自动离开桌子
// 				if (m_bRoundEndExit[i]) {
// 					bPlaying_[i] = false;
// 					m_bRoundEndExit[i] = false;
// 					player->SetUserStatus(sOffline);
// 					table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_USER_AUTO_EXIT);
// 				}
// 				//长时间未操作，踢出玩家
// 				else if (!m_bPlayerOperated[i]) {
// 					bPlaying_[i] = false;
// 					m_bRoundEndExit[i] = false;
// 					player->SetUserStatus(sOffline);
// 					table_->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
// 				}
// 			}
			static STD::Random r(1, 100);
			if (player->IsRobot()  && table_->GetPlayerCount() > MIN_GAME_PLAYER && r.randInt_mt() <= randValue) {
				OnUserLeft(player->GetUserId(), true);
			}
		}
	}
}

//设置托管，理牌/摊牌/结算
bool CGameTable::IsTrustee(void) {
	//理牌动画
	if (table_->GetGameStatus() == GAME_STATUS_GROUP) {
		_LOG_INFO("[%s][%d][%s] 开始理牌动画 ...",
			strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
		timerGroupID_ = ThisThreadTimer->runAfter(aniPlay_.Get_DELAY_time(GAME_STATUS_GROUP) + 1 + TIME_GAME_START_DELAY, boost::bind(&CGameTable::OnTimerGroupCard, this));
	}
	//摊牌动画
	else if (table_->GetGameStatus() == GAME_STATUS_OPEN) {
		_LOG_INFO("[%s][%d][%s] 开始摊牌动画 ...",
			strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
		ThisThreadTimer->cancel(timerGroupID_);
		timerOpenCardID_ = ThisThreadTimer->runAfter(aniPlay_.Get_DELAY_time(GAME_STATUS_OPEN) + 1, boost::bind(&CGameTable::OnTimerOpenCard, this));
	}
	//结算动画
	else if (table_->GetGameStatus() == GAME_STATUS_PREEND) {
		_LOG_INFO("[%s][%d][%s] 开始结算动画 ...",
			strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
		ThisThreadTimer->cancel(timerOpenCardID_);
		timerGameEndID_ = ThisThreadTimer->runAfter(aniPlay_.Get_DELAY_time(GAME_STATUS_PREEND) + 1, boost::bind(&CGameTable::OnTimerGameEnd, this));
	}
	return true;
}

static std::string StringSubId(uint8_t subId) {
	switch (subId) {
	case s13s::SUB_C_MANUALCARDS:
		return "手动摆牌 ...";
	case s13s::SUB_C_CANCELCARDS:
		return "清空牌墩 ...";
	case s13s::SUB_C_MAKESUREDUNHANDTY:
		return "确定牌型 ...";
	default:
		return "ERROR";
	}
}

bool CGameTable::OnGameMessage(uint16_t chairId, uint8_t subId, uint8_t const* data, size_t len) {
	if (chairId == INVALID_CHAIR || !bPlaying_[chairId]) {
		return false;
	}
	_LOG_INFO("[%s][%d][%s] %d %d %s %s",
		strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str(),
		chairId, UserIdBy(chairId), (ByChairId(chairId)->IsRobot() ? "robot" : "real"),
		StringSubId(subId).c_str());
    switch (subId) {
        //手动摆牌
	case s13s::SUB_C_MANUALCARDS: {
        std::shared_ptr<IPlayer> player = table_->GetChairPlayer(chairId);
        assert(player != NULL);
		if (table_->GetGameStatus() != GAME_STATUS_GROUP) {
            return false;
        }
		if (bPlaying_[chairId] && !player->IsRobot()) {
			m_bPlayerOperated[chairId] = true;
			////LOG_INFO << __FUNCTION__ << " table[" << table_->GetTableId() << "][" << StringStat(table_->GetGameStatus()).c_str() << "][" << strRoundID_ << "] "
			//	<< player->GetChairId() << " m_bPlayerOperated[" << player->GetUserId() << "]=" << (m_bPlayerOperated[player->GetChairId()] ? "true" : "false");
		}
        //////////////////////////////////////////////////////////////
        //客户端请求数据
        s13s::CMD_C_ManualCards reqdata;
		if (!reqdata.ParseFromArray(data, len)) {
			return false;
		}
		
        //////////////////////////////////////////////////////////////
        //服务器应答数据
        s13s::CMD_S_ManualCards rspdata;
        //////////////////////////////////////////////////////////////
        S13S::CGameLogic::EnumTree enumList;
        S13S::CGameLogic::EnumTree* rootEnumList = NULL;
        //////////////////////////////////////////////////////////////
        //客户端选择了哪一墩
        S13S::DunTy dt = (S13S::DunTy)(reqdata.dt());
        //客户端选择了哪些牌
        uint8_t * cards = (uint8_t *)reqdata.cards().c_str();
        //客户端选择了几张牌
        int len = reqdata.cards().size();

        if (len > 0)
        {
            S13S::CGameLogic::SortCards(cards, len, true, true, true);
            S13S::CGameLogic::PrintCardList(cards, len);
            //////////////////////////////////////////////////////////////
            //对客户端选择的一组牌，进行单墩牌型判断
            S13S::HandTy ty = S13S::CGameLogic::GetDunCardHandTy(dt, cards, len);
            //////////////////////////////////////////////////////////////
            //手动摆牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
            if (dt == S13S::DunFirst && (ty == S13S::Ty123sc || ty == S13S::Tysc || ty == S13S::Ty123))
            {
                ty = S13S::Tysp;
            }
            //////////////////////////////////////////////////////////////
            //判断手动摆牌是否倒水
            if (phandInfos_[chairId]->IsInverted(dt, cards, len, ty)) {
                phandInfos_[chairId]->ResetManual();
                rspdata.set_result(1);
                rspdata.set_errmsg("手动摆牌倒水");
                //序列化std::string
                std::string content = rspdata.SerializeAsString();//data
                table_->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, (uint8_t *)content.data(), content.size());
                std::string const& typeName = rspdata.GetTypeName();//typename
                return false;
            }
            //////////////////////////////////////////////////////////////
            //手动选牌组墩，给指定墩(头/中/尾墩)选择一组牌
            if (!phandInfos_[chairId]->SelectAs(dt, cards, len, ty))
            {
                return false;
            }
            //返回组墩后剩余牌
            uint8_t cpy[S13S::MaxSZ] = { 0 };
            int cpylen = 0;
            phandInfos_[chairId]->GetLeftCards(&(handCards_[chairId])[0], MAX_COUNT, cpy, cpylen);
            //3/8/10
            len = phandInfos_[chairId]->GetManualC() < 10 ? 5 : 3;
            //////////////////////////////////////////////////////////////
            //从余牌中枚举所有牌型
            S13S::CGameLogic::classify_t classify = { 0 };
            S13S::CGameLogic::EnumCards(cpy, cpylen, len, classify, enumList, dt);
            //////////////////////////////////////////////////////////////
            //指向新的所有枚举牌型
            rootEnumList = &enumList;
            //查看所有枚举牌型
            //rootEnumList->PrintEnumCards(false, S13S::TyAllBase);
            //////////////////////////////////////////////////////////////
            //客户端选择了哪一墩，标记0-头/1-中/2-尾
            rspdata.set_dt(dt);
            //墩对应牌型
            rspdata.set_ty(ty);
            //剩余牌
            rspdata.set_cpy(cpy, cpylen);
        }
        else
        {
            //////////////////////////////////////////////////////////////
            //重新摆牌重置手动摆牌
            phandInfos_[chairId]->ResetManual();
            assert(phandInfos_[chairId]->GetManualC() == 0);
            //////////////////////////////////////////////////////////////
            //指向初始所有枚举牌型
            rootEnumList = phandInfos_[chairId]->rootEnumList;
        }
        //////////////////////////////////////////////////////////////
        //所有枚举牌型
        s13s::EnumCards* cards_enums = rspdata.mutable_enums();
        assert(rootEnumList != NULL);
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123sc.begin();
            it != rootEnumList->v123sc.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_v123sc(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v40.begin();
            it != rootEnumList->v40.end(); ++it)
        {
            cards_enums->add_v40(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v32.begin();
            it != rootEnumList->v32.end(); ++it)
        {
            cards_enums->add_v32(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->vsc.begin();
            it != rootEnumList->vsc.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_vsc(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123.begin();
            it != rootEnumList->v123.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_v123(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v30.begin();
            it != rootEnumList->v30.end(); ++it)
        {
            cards_enums->add_v30(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v22.begin();
            it != rootEnumList->v22.end(); ++it)
        {
            cards_enums->add_v22(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v20.begin();
            it != rootEnumList->v20.end(); ++it)
        {
            cards_enums->add_v20(&it->front(), it->size());
        }

         //序列化bytes
         int length = rspdata.ByteSize();//len
         uint8_t *data = new uint8_t[length];
         rspdata.SerializeToArray(data, length);//data
         std::string const& typeName = rspdata.GetTypeName();//typename
         table_->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, data, length);
         delete[] data;

        break;
    }
    //手动摆牌清空重置指定墩的牌
	case s13s::SUB_C_CANCELCARDS: {
		std::shared_ptr<IPlayer> player = table_->GetChairPlayer(chairId); assert(player != NULL);
		//////////////////////////////////////////////////////////////
		//客户端请求数据
		s13s::CMD_C_CancelCards reqdata;
		if (!reqdata.ParseFromArray(data, len)) {
			return false;
		}
		if (reqdata.dt() < S13S::DunFirst || reqdata.dt() > S13S::DunLast) {
			return false;
		}
		if (bPlaying_[chairId] && !player->IsRobot()) {
			m_bPlayerOperated[chairId] = true;
			////LOG_INFO << __FUNCTION__ << " table[" << table_->GetTableId() << "][" << StringStat(table_->GetGameStatus()).c_str() << "][" << strRoundID_ << "] "
			//	<< player->GetChairId() << " m_bPlayerOperated[" << player->GetUserId() << "]=" << (m_bPlayerOperated[player->GetChairId()] ? "true" : "false");
		}
        //////////////////////////////////////////////////////////////
        //服务器应答数据
        s13s::CMD_S_CancelCards rspdata;
        //////////////////////////////////////////////////////////////
        S13S::CGameLogic::EnumTree enumList;
        S13S::CGameLogic::EnumTree* rootEnumList = NULL;
        //重置指定墩的牌
		if (!phandInfos_[chairId]->ResetManual(S13S::DunTy(reqdata.dt()))) {
            rspdata.set_result(1);
            rspdata.set_errmsg("无效的一墩牌");
            //序列化std::string
            std::string content = rspdata.SerializeAsString();//data
            table_->SendTableData(chairId, s13s::SUB_S_CANCELCARDS, (uint8_t *)content.data(), content.size());
            std::string const& typeName = rspdata.GetTypeName();//typename
            return false;
        }
		if (phandInfos_[chairId]->GetManualC() > 0) {
            //返回取消牌墩后剩余牌
            uint8_t cpy[S13S::MaxSZ] = { 0 };
            int cpylen = 0;
            phandInfos_[chairId]->GetLeftCards(&(handCards_[chairId])[0], MAX_COUNT, cpy, cpylen);
            //3/8/10
            int len = phandInfos_[chairId]->GetManualC() < 10 ? 5 : 3;
            S13S::DunTy dt = len == 5 ? S13S::DunLast : S13S::DunFirst;
            //////////////////////////////////////////////////////////////
            //从余牌中枚举所有牌型
            S13S::CGameLogic::classify_t classify = { 0 };
            S13S::CGameLogic::EnumCards(cpy, cpylen, len, classify, enumList, dt);
            //////////////////////////////////////////////////////////////
            //指向新的所有枚举牌型
            rootEnumList = &enumList;
            //查看所有枚举牌型
            //rootEnumList->PrintEnumCards(false, S13S::TyAllBase);
            //////////////////////////////////////////////////////////////
            //客户端选择了哪一墩，标记0-头/1-中/2-尾
            rspdata.set_dt(reqdata.dt());
            //剩余牌
            rspdata.set_cpy(cpy, cpylen);
        }
        else
        {
            //////////////////////////////////////////////////////////////
            //指向初始所有枚举牌型
            rootEnumList = phandInfos_[chairId]->rootEnumList;
        }
        //////////////////////////////////////////////////////////////
        //所有枚举牌型
        s13s::EnumCards* cards_enums = rspdata.mutable_enums();
        assert(rootEnumList != NULL);
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123sc.begin();
            it != rootEnumList->v123sc.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_v123sc(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v40.begin();
            it != rootEnumList->v40.end(); ++it)
        {
            cards_enums->add_v40(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v32.begin();
            it != rootEnumList->v32.end(); ++it)
        {
            cards_enums->add_v32(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->vsc.begin();
            it != rootEnumList->vsc.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_vsc(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123.begin();
            it != rootEnumList->v123.end(); ++it)
        {
			if (it->size() == 5)
            cards_enums->add_v123(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v30.begin();
            it != rootEnumList->v30.end(); ++it)
        {
            cards_enums->add_v30(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v22.begin();
            it != rootEnumList->v22.end(); ++it)
        {
            cards_enums->add_v22(&it->front(), it->size());
        }
        for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v20.begin();
            it != rootEnumList->v20.end(); ++it) {
            cards_enums->add_v20(&it->front(), it->size());
        }

        {
            //序列化std::string
            //std::string content = rspdata.SerializeAsString();
            //table_->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, (uint8_t *)content.data(), content.size());
            //序列化bytes
            int len = rspdata.ByteSize();//len
            uint8_t *data = new uint8_t[len];
            rspdata.SerializeToArray(data, len);//data
            std::string const& typeName = rspdata.GetTypeName();//typename
            table_->SendTableData(chairId, s13s::SUB_S_CANCELCARDS, data, len);
            delete[] data;
        }
        break;
    }
    //确定牌型
    case s13s::SUB_C_MAKESUREDUNHANDTY:
    {
		assert(chairId != INVALID_CHAIR);
        assert(chairId < GAME_PLAYER);
        std::shared_ptr<IPlayer> player = table_->GetChairPlayer(chairId); assert(player != NULL);
		//////////////////////////////////////////////////////////////
        //客户端请求数据
        s13s::CMD_C_MakesureDunHandTy reqdata;
        if (!reqdata.ParseFromArray(data, len))
        {
            return false;
        }
		if (!reqdata.isautoopertor() && bPlaying_[chairId] && !player->IsRobot()) {
			m_bPlayerOperated[chairId] = true;
			////LOG_INFO << __FUNCTION__ << " table[" << table_->GetTableId() << "][" << StringStat(table_->GetGameStatus()).c_str() << "][" << strRoundID_ << "] "
			//	<< player->GetChairId() << " m_bPlayerOperated[" << player->GetUserId() << "]=" << (m_bPlayerOperated[player->GetChairId()] ? "true" : "false");
		}
		OnUserSelect(chairId, reqdata.groupindex());
        break;
    }
    case s13s::SUB_C_ROUND_END_EXIT:
    {
        ////LOG_INFO<<"***************************USER ROUND END EXIT***************************";

        //变量定义
        s13s::CMD_C_RoundEndExit  roundEndExit;
        roundEndExit.ParseFromArray(data, len);
        m_bRoundEndExit[chairId] = roundEndExit.bexit();

		s13s::CMD_C_RoundEndExitResult roundEndExitResult;
        roundEndExitResult.set_bexit(m_bRoundEndExit[chairId]);
        //发送结果
        std::string data = roundEndExitResult.SerializeAsString();
        table_->SendTableData(chairId, s13s::SUB_C_ROUND_END_EXIT_RESULT, (uint8_t*)data.c_str(), data.size());
        break;
    }
    }
    return true;
}

//玩家之间两两比牌
void CGameTable::StartCompareCards() {
	_LOG_INFO("[%s][%d][%s] 开始比牌 ...", strRoundID_.c_str(), table_->GetTableId(), StringStat(table_->GetGameStatus()).c_str());
	//////////////////////////////////////////////////////////////
	//玩家之间两两比牌，头墩与头墩比，中墩与中墩比，尾墩与尾墩比
	//s13s::CMD_S_CompareCards compareCards_[GAME_PLAYER];
	for (int i = 0; i < GAME_PLAYER; ++i) {
		compareCards_[i].Clear();
	}
	//////////////////////////////////////////////////////////////
	//所有玩家牌型都确认了，比牌
	int c = 0, c1 = 0;
	int chairIDs[GAME_PLAYER] = { 0 };
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(chairId); assert(player);
			//用于求两两组合
			chairIDs[c++] = i;
			//比牌玩家桌椅ID
			compareCards_[i].mutable_player()->set_chairid(i);
			compareCards_[i].mutable_player()->set_isingame(bPlaying_[i]);
			//获取座椅玩家确定的三墩牌型
			S13S::CGameLogic::groupdun_t const* player_select = phandInfos_[i]->GetSelected();
			//桌椅玩家选择一组墩(含头墩/中墩/尾墩)
			s13s::GroupDunData* player_group = compareCards_[i].mutable_player()->mutable_group();
			//从哪墩开始的
			player_group->set_start(S13S::DunFirst);
			//总体对应特殊牌型 ////////////
			player_group->set_specialty(player_select->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* player_dun_i = player_group->add_duns();
				//标记0-头/1-中/2-尾
				player_dun_i->set_id(d);
				//墩对应普通牌型
				player_dun_i->set_ty(
					player_select->specialTy >= S13S::TyThreesc ?
					player_select->specialTy :
					player_select->duns[d].ty_);
				//墩对应牌数c(3/5/5)
				player_dun_i->set_c(player_select->duns[d].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				player_dun_i->set_cards(player_select->duns[d].cards, player_select->duns[d].c);
				player_dun_i->set_max(player_select->specialTy < S13S::TyThreesc);
			}
			if (player_select->specialTy >= S13S::TyThreesc) {
				++c1;
			}
		}
	}
	std::set<int> losers[S13S::DunMax];
	assert(c >= MIN_GAME_PLAYER);
	std::vector<std::vector<int>> vec;
	CFuncC fnC;
	fnC.FuncC(c, 2, vec);
	//CFuncC::Print(vec);
	for (std::vector<std::vector<int>>::const_iterator it = vec.begin();
		it != vec.end(); ++it) {
		assert(it->size() == 2);//两两比牌
		int src_chairid = chairIDs[(*it)[0]];//src椅子ID
		int dst_chairid = chairIDs[(*it)[1]];//dst椅子ID
		assert(bPlaying_[src_chairid]);
		assert(bPlaying_[dst_chairid]);
		assert(src_chairid < GAME_PLAYER);
		assert(dst_chairid < GAME_PLAYER);
		//获取src确定的三墩牌型
		S13S::CGameLogic::groupdun_t const* src = phandInfos_[src_chairid]->GetSelected();
		//获取dst确定的三墩牌型
		S13S::CGameLogic::groupdun_t const* dst = phandInfos_[dst_chairid]->GetSelected();
		{
			//追加src比牌对象 ////////////
			s13s::ComparePlayer* src_peer = compareCards_[src_chairid].add_peers();
			{
				//比牌对象桌椅ID
				src_peer->set_chairid(dst_chairid);
				//比牌对象选择一组墩
				s13s::GroupDunData* src_peer_select = src_peer->mutable_group();
				//从哪墩开始的
				src_peer_select->set_start(S13S::DunFirst);
				//总体对应特殊牌型 ////////////
				src_peer_select->set_specialty(dst->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				for (int i = S13S::DunFirst; i <= S13S::DunLast; ++i) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* src_peer_dun_i = src_peer_select->add_duns();
					//标记0-头/1-中/2-尾
					src_peer_dun_i->set_id(i);
					//墩对应普通牌型
					src_peer_dun_i->set_ty(
						dst->specialTy >= S13S::TyThreesc ?
						dst->specialTy :
						dst->duns[i].ty_);
					//墩对应牌数c(3/5/5)
					src_peer_dun_i->set_c(dst->duns[i].c);
					//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
					src_peer_dun_i->set_cards(dst->duns[i].cards, dst->duns[i].c);
					src_peer_dun_i->set_max(dst->specialTy < S13S::TyThreesc);
				}
			}
			//追加src比牌结果 ////////////
			s13s::CompareResult* src_result = compareCards_[src_chairid].add_results();
		}
		{
			//追加dst比牌对象 ////////////
			s13s::ComparePlayer* dst_peer = compareCards_[dst_chairid].add_peers();
			{
				//比牌对象桌椅ID
				dst_peer->set_chairid(src_chairid);
				//比牌对象选择一组墩
				s13s::GroupDunData* dst_peer_select = dst_peer->mutable_group();
				//从哪墩开始的
				dst_peer_select->set_start(S13S::DunFirst);
				//总体对应特殊牌型 ////////////
				dst_peer_select->set_specialty(src->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				for (int i = S13S::DunFirst; i <= S13S::DunLast; ++i) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* dst_peer_dun_i = dst_peer_select->add_duns();
					//标记0-头/1-中/2-尾
					dst_peer_dun_i->set_id(i);
					//墩对应普通牌型
					dst_peer_dun_i->set_ty(
						src->specialTy >= S13S::TyThreesc ?
						src->specialTy :
						src->duns[i].ty_);
					//墩对应牌数c(3/5/5)
					dst_peer_dun_i->set_c(src->duns[i].c);
					//墩牌数据(头墩3张)
					dst_peer_dun_i->set_cards(src->duns[i].cards, src->duns[i].c);
					dst_peer_dun_i->set_max(src->specialTy < S13S::TyThreesc);
				}
			}
			//追加dst比牌结果 ////////////
			s13s::CompareResult* dst_result = compareCards_[dst_chairid].add_results();
		}
		//////////////////////////////////////////////////////////////
		//比较特殊牌型
		if (src->specialTy >= S13S::TyThreesc || dst->specialTy >= S13S::TyThreesc) {
			int winner = -1, loser = -1;
			if (src->specialTy != dst->specialTy) {
				//牌型不同比牌型
				if (src->specialTy > dst->specialTy) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->specialTy < dst->specialTy) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同，和了
			}
			if (winner == -1) {
				//和了
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//src输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//dst输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//至尊青龙获胜赢32水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyZZQDragon) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(108/*32*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-108/*-32*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//一条龙获胜赢30水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyOneDragon) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(36/*30*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-36/*-30*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//十二皇族获胜赢24水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::Ty12Royal) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(24);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-24);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三同花顺获胜赢20水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree123sc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(22/*20*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-22/*-20*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三分天下获胜赢20水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree40) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(20);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-20);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//全大获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllBig) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(15/*10*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-15/*-10*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//全小获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllSmall) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(12/*10*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-12/*-10*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//凑一色获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllOneColor) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(10);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-10);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//双怪冲三获胜赢8水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyTwo3220) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(8);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-8);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//四套三条获胜赢6水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyFour30) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(6);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-6);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//五对三条获胜赢5水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyFive2030) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(5);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-5);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//六对半获胜赢4水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TySix20) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(4);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-4);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三顺子获胜赢4水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree123) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(3/*4*/);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-3/*-4*/);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三同花获胜赢3水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThreesc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(3);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-3);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			continue;
		}
		//////////////////////////////////////////////////////////////
		//比较头墩
		{
			//单墩比牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
			if (src->duns[S13S::DunFirst].ty_ == S13S::Ty123sc ||
				src->duns[S13S::DunFirst].ty_ == S13S::Tysc ||
				src->duns[S13S::DunFirst].ty_ == S13S::Ty123) {
				//assert(src->duns[S13S::DunFirst].ty_ == S13S::Tysp);
				const_cast<S13S::CGameLogic::groupdun_t*>(src)->duns[S13S::DunFirst].ty_ = S13S::Tysp;
			}
			//单墩比牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
			if (dst->duns[S13S::DunFirst].ty_ == S13S::Ty123sc ||
				dst->duns[S13S::DunFirst].ty_ == S13S::Tysc ||
				dst->duns[S13S::DunFirst].ty_ == S13S::Ty123) {
				//assert(dst->duns[S13S::DunFirst].ty_ == S13S::Tysp);
				const_cast<S13S::CGameLogic::groupdun_t*>(dst)->duns[S13S::DunFirst].ty_ = S13S::Tysp;
			}
			int winner = -1, loser = -1;
			int dt = S13S::DunFirst;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 3);
				assert(dst->duns[dt].GetC() == 3);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//头墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//头墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				losers[dt].insert(loser);
				//乌龙/对子获胜赢1水
				if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//三条摆头墩获胜赢3水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(3);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-3);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				else {
					assert(false);
				}
			}
		}
		//////////////////////////////////////////////////////////////
		//比较中墩
		{
			int winner = -1, loser = -1;
			int dt = S13S::DunSecond;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 5);
				assert(dst->duns[dt].GetC() == 5);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//中墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//中墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				losers[dt].insert(loser);
				//乌龙/对子/两对/三条/顺子/同花获胜赢1水
				if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty22 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysc) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//葫芦摆中墩获胜赢2水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty32) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(3/*2*/);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-3/*-2*/);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//铁支摆中墩获胜赢8水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty40) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(8);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-8);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//同花顺摆中墩获胜赢10水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123sc) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(10);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-10);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				else {
					assert(false);
				}
			}
		}
		//////////////////////////////////////////////////////////////
		//比较尾墩
		{
			int winner = -1, loser = -1;
			int dt = S13S::DunLast;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 5);
				assert(dst->duns[dt].GetC() == 5);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//尾墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//尾墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				losers[dt].insert(loser);
				//乌龙/对子/两对/三条/顺子/同花/葫芦获胜赢1水
				if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty22 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123 ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysc ||
					phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty32) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//铁支摆尾墩获胜赢4水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty40) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(4);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-4);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				//同花顺摆尾墩获胜赢5水
				else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123sc) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(5);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-5);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
				}
				else {
					{
						assert(false);
					}
				}
			}
		}
	}
	if ((c - c1) == 1) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (bPlaying_[i]) {
				for (int dt = S13S::DunFirst; dt <= S13S::DunLast; ++dt) {
					compareCards_[i].mutable_player()->mutable_group()->mutable_duns(dt)->set_max(false);
					for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
						assert(bPlaying_[compareCards_[i].mutable_peers(j)->chairid()]);
						compareCards_[i].mutable_peers(j)->mutable_group()->mutable_duns(dt)->set_max(false);
					}
				}
			}
		}
	}
	else {
		for (int dt = S13S::DunFirst; dt <= S13S::DunLast; ++dt) {
			for (std::set<int>::const_iterator it = losers[dt].begin(); it != losers[dt].end(); ++it) {
				int loser = *it;
				compareCards_[loser].mutable_player()->mutable_group()->mutable_duns(dt)->set_max(false);
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (bPlaying_[i] && i != loser) {
						for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
							assert(bPlaying_[compareCards_[i].mutable_peers(j)->chairid()]);
							if (compareCards_[i].mutable_peers(j)->chairid() == loser) {
								compareCards_[i].mutable_peers(j)->mutable_group()->mutable_duns(dt)->set_max(false);
								break;
							}
						}
					}
				}
			}
		}
	} 
	enum {
		TOSHOOT = 0,
		FROMSHOOT = 1,
		MAX_
	};
	//玩家对其它玩家打枪
	std::map<uint8_t, std::set<uint8_t>> shootIds[MAX_];
	//统计判断打枪/全垒打
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			//判断是否全垒打
			int shootc = 0;
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				s13s::CompareResult const& result = compareCards_[i].results(j);
				int winc = 0, lostc = 0, sumscore = 0;
				//自己特殊牌型或对方特殊牌型
				assert(result.items_size() == (
					player.group().specialty() >= S13S::TyThreesc ||
					peer.group().specialty() >= S13S::TyThreesc) ? 0 : 3);
				//玩家与当前比牌对象比头/中/尾三墩输赢得水总分，不考虑打枪
				for (int d = 0; d < result.items_size(); ++d) {
					if (result.items(d).winlost() == 1) {
						++winc;
					}
					else if (result.items(d).winlost() == -1) {
						++lostc;
					}
					sumscore += result.items(d).score();
				}
				//三墩不考虑打枪输赢得水总分 赢分+/和分0/输分-
				const_cast<s13s::CompareResult&>(result).set_score(sumscore);
				//特殊牌型不参与打枪
				if (player.group().specialty() >= S13S::TyThreesc ||
					peer.group().specialty() >= S13S::TyThreesc) {
					const_cast<s13s::CompareResult&>(result).set_shoot(0);//-1被打枪/0不打枪/1打枪
					continue;
				}
				if (winc == result.items_size()) {
					//玩家三墩全部胜过比牌对象，则玩家对比牌对象打枪(i->peer=1)，中枪者付给打枪者2倍的水
					const_cast<s13s::CompareResult&>(result).set_shoot(1);//-1被打枪/0不打枪/1打枪
					//统计当前玩家打枪次数
					++shootc;
					//玩家对比牌对象打枪(peer->i=-1)
					shootIds[TOSHOOT][i].insert(peer.chairid());
					shootIds[FROMSHOOT][peer.chairid()].insert(i);
					for (int l = 0; l < compareCards_[peer.chairid()].peers_size(); ++l) {
						s13s::ComparePlayer const& peer_ = compareCards_[peer.chairid()].peers(l);
						s13s::CompareResult const& result_ = compareCards_[peer.chairid()].results(l);
						if (peer_.chairid() == i) {
							const_cast<s13s::CompareResult&>(result_).set_shoot(-1);
							break;
						}
					}
				}
				else if (lostc == result.items_size()) {
					//比牌对象三墩全部胜过玩家，则比牌对象对玩家打枪(i->peer=-1)，中枪者付给打枪者2倍的水
					const_cast<s13s::CompareResult&>(result).set_shoot(-1);//-1被打枪/0不打枪/1打枪
					//比牌对象对玩家打枪(peer->i=1)
					shootIds[TOSHOOT][peer.chairid()].insert(i);
					shootIds[FROMSHOOT][i].insert(peer.chairid());
					for (int l = 0; l < compareCards_[peer.chairid()].peers_size(); ++l) {
						s13s::ComparePlayer const& peer_ = compareCards_[peer.chairid()].peers(l);
						s13s::CompareResult const& result_ = compareCards_[peer.chairid()].results(l);
						if (peer_.chairid() == i) {
							const_cast<s13s::CompareResult&>(result_).set_shoot(1);
							break;
						}
					}
				}
				else {
					const_cast<s13s::CompareResult&>(result).set_shoot(0);//-1被打枪/0不打枪/1打枪
				}
			}
			if (shootc == compareCards_[i].peers_size() && compareCards_[i].peers_size() > 2/*1*/) {
				//全垒打，玩家三墩全部胜过其它玩家，且至少打2枪，中枪者付给打枪者4倍的水
				compareCards_[i].set_allshoot(1);//-1被全垒打/0无全垒打/1全垒打
				//其它比牌对象都是被全垒打
				for (int k = 0; k < GAME_PLAYER; ++k) {
					if (k != i) {
						if (bPlaying_[k]) {
							//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(k); assert(player);
							//-1被全垒打/0无全垒打/1全垒打
							compareCards_[k].set_allshoot(-1);
							//allshoot=-1被全垒打，记下全垒打桌椅ID
							compareCards_[k].set_fromchairid(i);
						}
					}
				}
			}
		}
	}
	//其它玩家之间打枪
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				//s13s::CompareResult const& result = compareCards_[i].results(j);
				//toshootIds 作为打枪者(shoot=1)，被打枪者座椅ID列表
				{
					std::map<uint8_t, std::set<uint8_t>>::const_iterator it = shootIds[TOSHOOT].find(peer.chairid());
					if (it != shootIds[TOSHOOT].end()) {
						for (std::set<uint8_t>::const_iterator ir = it->second.begin();
							ir != it->second.end(); ++ir) {
							const_cast<s13s::ComparePlayer&>(peer).add_toshootids(*ir);
						}
					}
				}
				//fromshootIds 作为被打枪者(shoot=-1)，来自打枪者座椅ID列表
				{
					std::map<uint8_t, std::set<uint8_t>>::const_iterator it = shootIds[FROMSHOOT].find(peer.chairid());
					if (it != shootIds[FROMSHOOT].end()) {
						for (std::set<uint8_t>::const_iterator ir = it->second.begin();
							ir != it->second.end(); ++ir) {
							const_cast<s13s::ComparePlayer&>(peer).add_fromshootids(*ir);
						}
					}
				}
#if 0
				std::map<uint8_t, std::set<uint8_t>>::const_iterator it = shootIds.find(peer.chairid());
				if (it != shootIds.end()) {
					for (std::set<uint8_t>::const_iterator ir = it->second.begin();
						ir != it->second.end(); ++ir) {
						//排除当前玩家
						if (*ir != i) {
							assert(player.chairid() == i);
							assert(bPlaying_[*ir]);
							const_cast<s13s::ComparePlayer&>(peer).add_shootids(*ir);
						}
					}
				}
#endif
			}
		}
	}
	memset(eachWinLostScore_, 0, GAME_PLAYER* GAME_PLAYER * sizeof(int));
	//计算包括打枪/全垒打在内输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			//玩家输赢得水总分，不含特殊牌型输赢分
			int deltascore = 0;
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				s13s::CompareResult const& result = compareCards_[i].results(j);
				//1打枪(对当前比牌对象打枪)
				if (result.shoot() == 1) {
					//1全垒打
					if (compareCards_[i].allshoot() == 1) {
						//-1被全垒打/0无全垒打/1全垒打
						deltascore += 4 * result.score();
						eachWinLostScore_[i][peer.chairid()] = 4 * result.score();
					}
					else {
						//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
						if (compareCards_[i].allshoot() == -1) {
						}
						else {
						}
						//-1被打枪/0不打枪/1打枪
						deltascore += 2 * result.score();
						eachWinLostScore_[i][peer.chairid()] = 2 * result.score();
					}
				}
				//-1被打枪(被当前比牌对象打枪)
				else if (result.shoot() == -1) {
					//-1被全垒打
					if (compareCards_[i].allshoot() == -1) {
						//被当前比牌对象全垒打
						if (peer.chairid() == compareCards_[i].fromchairid()) {
							//-1被全垒打/0无全垒打/1全垒打
							deltascore += 4 * result.score();
							eachWinLostScore_[i][peer.chairid()] = 4 * result.score();
						}
						//被另外比牌对象全垒打
						else {
							//-1被打枪/0不打枪/1打枪
							deltascore += 2 * result.score();
							eachWinLostScore_[i][peer.chairid()] = 2 * result.score();
						}
					}
					else {
						//-1被打枪/0不打枪/1打枪
						deltascore += 2 * result.score();
						eachWinLostScore_[i][peer.chairid()] = 2 * result.score();
					}
				}
				//0不打枪(与当前比牌对象互不打枪)
				else {
					//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
					if (compareCards_[i].allshoot() == -1) {
					}
					else {
						//一定不是全垒打，-1被打枪/0不打枪/1打枪
						//assert(compareCards_[i].allshoot() == 0);
					}
					//assert(result.shoot() == 0);
					deltascore += result.score();
					eachWinLostScore_[i][peer.chairid()] = result.score();
				}
			}
			//玩家输赢得水总分，不含特殊牌型输赢分
			compareCards_[i].set_deltascore(deltascore);
		}
	}
	//座椅玩家与其余玩家按墩比输赢分
	//不含打枪/全垒打与打枪/全垒打分开计算
	//计算特殊牌型输赢分
	//统计含打枪/全垒打/特殊牌型输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			//三墩打枪/全垒打输赢算水
			int allshootscore = 0;
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历各墩(头墩/中墩/尾墩)
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				//按墩比输赢分
				int sumscore = 0;
				//按墩计算打枪输赢分
				int sumshootscore = 0;
				{
					//自己特殊牌型，不按墩比牌，输赢水数在specitem中
					if (player.group().specialty() >= S13S::TyThreesc) {
					}
					else {
						//遍历比牌对象
						for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
							assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
							s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
							s13s::CompareResult const& result = compareCards_[i].results(j);
							//对方特殊牌型，不按墩比牌，输赢水数在specitem中
							if (peer.group().specialty() >= S13S::TyThreesc) {
								continue;
							}
							//累加指定墩输赢得水积分，不含打枪/全垒打
							sumscore += result.items(d).score();
							//1打枪(对当前比牌对象打枪)
							if (result.shoot() == 1) {
								//1全垒打
								if (compareCards_[i].allshoot() == 1) {
									//-1被全垒打/0无全垒打/1全垒打
									sumshootscore += 3/*4*/ * result.items(d).score();
								}
								else {
									//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
									if (compareCards_[i].allshoot() == -1) {
									}
									else {
									}
									//-1被打枪/0不打枪/1打枪
									sumshootscore += 1/*2*/ * result.items(d).score();
								}
							}
							//-1被打枪(被当前比牌对象打枪)
							else if (result.shoot() == -1) {
								//-1被全垒打
								if (compareCards_[i].allshoot() == -1) {
									//被当前比牌对象全垒打
									if (peer.chairid() == compareCards_[i].fromchairid()) {
										//-1被全垒打/0无全垒打/1全垒打
										sumshootscore += 3/*4*/ * result.items(d).score();
									}
									//被另外比牌对象全垒打
									else {
										//-1被打枪/0不打枪/1打枪
										sumshootscore += 1/*2*/ * result.items(d).score();
									}
								}
								else {
									//-1被打枪/0不打枪/1打枪
									sumshootscore += 1/*2*/ * result.items(d).score();
								}
							}
							//0不打枪(与当前比牌对象互不打枪)
							else {
								//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
								if (compareCards_[i].allshoot() == -1) {
								}
								else {
									//一定不是全垒打，-1被打枪/0不打枪/1打枪
									//assert(compareCards_[i].allshoot() == 0);
								}
								//assert(result.shoot() == 0);
								//sumshootscore += 0/*1*/ * result.items(d).score();
							}
						}
					}
				}
				//座椅玩家输赢得水积分(头/中/尾/特殊)
				compareCards_[i].add_itemscores(sumscore);
				//按墩计算打枪/全垒打输赢分
				compareCards_[i].add_itemscorepure(sumshootscore);
				//三墩打枪/全垒打输赢算水
				allshootscore += sumshootscore;
			}
			{
				//特殊牌型输赢算水(无打枪/全垒打)
				int sumspecscore = 0;
				{
					//遍历比牌对象
					for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
						assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
						s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
						s13s::CompareResult const& result = compareCards_[i].results(j);
						//自己普通牌型，对方特殊牌型
						//自己特殊牌型，对方普通牌型
						//自己特殊牌型，对方特殊牌型
						if (result.has_specitem()) {
							//累加特殊比牌算水
							sumspecscore += result.specitem().score();
							eachWinLostScore_[i][peer.chairid()] += result.specitem().score();
						}
					}
				}
				//座椅玩家输赢得水积分(头/中/尾/特殊)
				compareCards_[i].add_itemscores(sumspecscore);
				//三墩打枪/全垒打输赢算水 + 特殊牌型输赢算水(无打枪/全垒打)
				compareCards_[i].add_itemscorepure(allshootscore + sumspecscore);
				//玩家输赢得水总分，含打枪/全垒打，含特殊牌型输赢分
				int32_t deltascore = compareCards_[i].deltascore();
				compareCards_[i].set_deltascore(deltascore + sumspecscore);
				if (allshootscore > 0) {
					aniPlay_.add_DELAY_time(AnimatePlay::OpenE, openCardTime_+ shootTime_);
				}
			}
		}
	}
	//比牌对方输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				assert(bPlaying_[compareCards_[i].peers(j).chairid()]);
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				int32_t deltascore = compareCards_[peer.chairid()].deltascore();
				const_cast<s13s::ComparePlayer&>(peer).set_deltascore(deltascore);
			}
		}
	}
	//发送比牌数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (bPlaying_[i]) {
			//开牌剩余时间
			compareCards_[i].set_wtimeleft(aniPlay_.Get_DELAY_time(GAME_STATUS_OPEN));

			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(compareCards_[i], jsonstr, true);
			//std::string const& typeName = compareCards_[i].GetTypeName();//typename
			//snprintf(msg, sizeof(msg), "\n --- *** %s[%d:%lld]\n", typeName.c_str(), i, UserIdBy(i));
			////LOG_WARN << __FUNCTION__ << " ["<< strRoundID_<< "] " << msg << jsonstr << "\n\n";

			std::string content = compareCards_[i].SerializeAsString();
			table_->SendTableData(i, s13s::SUB_S_COMPARECARDS, (uint8_t*)content.data(), content.size());
		}
		else if (table_->ExistUser(i)) {
			//随机玩家作为参考比牌对象
			short nextChairId = 0;
			for (; nextChairId < GAME_PLAYER; ++nextChairId) {
				if (bPlaying_[nextChairId]) {
					break;
				}
			}
			//开牌剩余时间
			compareCards_[i].set_wtimeleft(aniPlay_.Get_DELAY_time(GAME_STATUS_OPEN));
			std::string content = compareCards_[nextChairId].SerializeAsString();
			table_->SendTableData(i, s13s::SUB_S_COMPARECARDS, (uint8_t*)content.data(), content.size());
		}
	}
}

bool CGameTable::OnGameConclude(uint16_t chairId, uint8_t flags) {
// 	for (int i = 0; i < GAME_PLAYER; ++i) {
// 		std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i);
// 		if (player && !player->IsRobot()) {
// 			table_->RefreshRechargeScore(player->GetUserId(), i);
// 		}
// 	}
	//游戏结束时间
	roundEndTime_ = std::chrono::system_clock::now();
	//结算动画开始时间，飘金币
	aniPlay_.add_START_time(AnimatePlay::PreEndE, roundEndTime_);
	//游戏结束命令
	s13s::CMD_S_GameEnd GameEnd;
	//计算玩家积分
	std::vector<tagScoreInfo> scoreInfos;
	//玩家手牌字符串，根据选择的墩牌型拼接手牌
	//3b2c110,04352607384,323d2d1d0d7,3;342b0b1,2336062a1a2,22123929096,0;
	//一个分号";"分割一个玩家，每个分号内用逗号","隔开玩家的3墩牌，分号前一位表示玩家座位号(0/1/2/3)
	//两位表示1张牌，第1位是花色，第2位数是牌值，头墩3张，中墩和尾墩各5张，每墩最后一位字符表示牌型
	std::string strCardsList;
	switch (flags)
	{
		//正常退出
	case GER_NORMAL: {
		//系统抽水
		int64_t revenue[GAME_PLAYER] = { 0 };
		//代理抽水
		int64_t agentRevenue[GAME_PLAYER] = { 0 };
		//携带积分
		int64_t userScore[GAME_PLAYER] = { 0 };
		//输赢积分
		int64_t userWinScore[GAME_PLAYER] = { 0 };
		//输赢积分，扣除系统抽水
		int64_t userWinScorePure[GAME_PLAYER] = { 0 };
		//剩余积分
		int64_t userLeftScore[GAME_PLAYER] = { 0 };
		//系统输赢积分
		int64_t systemWinScore = 0;
		//桌面输赢积分
		int64_t totalWinScore = 0, totalLostScore = 0;
		//桌面输赢人数
		int winners = 0, losers = 0;
		int winLostFlag[GAME_PLAYER] = { 0 };
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			{
				//携带积分
				userScore[i] = player->GetUserScore();
				//输赢积分
				userWinScore[i] = compareCards_[i].deltascore() * FloorScore;
				//输赢分取绝对值最小
				if (userScore[i] < llabs(userWinScore[i])) {
					//玩家输赢积分
					userWinScore[i] = userWinScore[i] < 0 ?
						-userScore[i] : userScore[i];
				}
				if (userWinScore[i] < 0) {
					//桌面输的积分/人数
					totalLostScore += userWinScore[i];
					++losers;
					winLostFlag[i] = -1;
				}
				else if (userWinScore[i] > 0) {
					//桌面赢的积分/人数
					totalWinScore += userWinScore[i];
					++winners;
					winLostFlag[i] = 1;
				}
				else {
					winLostFlag[i] = 0;
				}
			}
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			{
				//玩家手牌字符串，根据选择的墩牌型拼接手牌
				S13S::CGameLogic::groupdun_t const* group_select = phandInfos_[i]->GetSelected();
				std::string strCards/*, handcards*/;
				for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
					//单墩牌(3/5/5)
					std::string cards, ty;
					{
						//牌数据
						for (int k = 0; k < group_select->duns[d].c; ++k) {
							char ch[10] = { 0 };
							uint8_t color = S13S::CGameLogic::GetCardColor(group_select->duns[d].cards[k]);
							uint8_t value = S13S::CGameLogic::GetCardValue(group_select->duns[d].cards[k]);
							uint8_t card = S13S::CGameLogic::MakeCardWith(color - 0x10, value);
							sprintf(ch, "%02X", card);
							cards += ch;
						}
						//牌型
						ty = std::to_string(group_select->duns[d].ty_);
					}
					if (d == S13S::DunFirst) {
						strCards += cards + ty;
						//handcards += ty + ":" + cards;//ty:cards
					}
					else {
						strCards += "," + cards + ty;
						//handcards += "|" + ty + ":" + cards;//ty:cards|ty:cards|ty:cards
					}
				}
				//座位号
				strCards += "," + std::to_string(i) + "," + std::to_string(winLostFlag[i]);
				//选牌结果
				//strCards += "," + std::to_string((int)phandInfos_[i]->GetSelectResult());
				//座椅玩家手牌信息
				if (strCardsList.empty()) {
					strCardsList += strCards;
				}
				else {
					strCardsList += ";" + strCards;
				}
			}
		}
		assert(totalWinScore >= 0);
		assert(totalLostScore <= 0);
		//桌面输赢分取绝对值最小(|X|和|Y|比较大小)
		if (llabs(totalLostScore) < totalWinScore) {
			//|X| < |Y| 玩家赢分换算
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!bPlaying_[i]) {
					continue;
				}
				std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
				{
					if (userWinScore[i] > 0) {
						//以桌面输分|X|为标准按赢分比例计算赢分玩家赢的积分
						//玩家实际赢分 = |X| * 玩家赢分占比(玩家赢分 / |Y|)
						userWinScore[i] = llabs(totalLostScore) * userWinScore[i] / totalWinScore;
					}
				}
			}
		}
		else {
			//|X| > |Y| 玩家输分换算
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!bPlaying_[i]) {
					continue;
				}
				std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
				{
					if (userWinScore[i] < 0) {
						//以桌面赢分|Y|为标准按输分比例计算输分玩家输的积分
						//玩家实际输分 = |Y| * 玩家输分占比(玩家输分 / |X|)
						userWinScore[i] = totalWinScore * userWinScore[i] / llabs(totalLostScore);
					}
				}
			}
		}
		//计算输赢积分
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			std::shared_ptr<IPlayer> player = table_->GetChairPlayer(i); assert(player);
			{
				//携带积分
				//userScore[i] = player->GetUserScore();
				//输赢积分
				//userWinScore[i] = compareCards_[i].deltascore() * FloorScore;
				//盈利玩家
				if (userWinScore[i] > 0) {
					//系统抽水，真实玩家和机器人都按照5%抽水计算，前端显示和玩家扣分一致
					if (table_->IsRobot((uint32_t)i) || table_->IsOfficial((uint32_t)i)) {
						revenue[i] = 0;
					}
					else {
						revenue[i] = table_->CalculateRevenue(userWinScore[i]);
					}
					//agentRevenue[i] = table_->CalculateAgentRevenue(userWinScore[i]);
				}
				else {
					//agentRevenue[i] = table_->CalculateAgentRevenue(-userWinScore[i]);
				}
				//输赢积分，扣除系统抽水
				userWinScorePure[i] = userWinScore[i] - revenue[i];
				//剩余积分
				userLeftScore[i] = userScore[i] + userWinScorePure[i];
				//若是机器人AI
				if (table_->IsRobot((uint32_t)i) || table_->IsOfficial((uint32_t)i)) {
#ifdef _STORAGESCORE_SEPARATE_STAT_
					//系统输赢积分，扣除系统抽水
					systemWinScore += userWinScorePure[i];
#else
					//系统输赢积分
					systemWinScore += userWinScore[i];
#endif
				}
				//计算积分
				tagScoreInfo scoreInfo;
				scoreInfo.chairId = i;//座椅ID
				scoreInfo.cardValue = strCardsList;//本局开牌
				//scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
				scoreInfo.addScore = userWinScorePure[i];//本局输赢
				scoreInfo.betScore = llabs(userWinScore[i]);//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = revenue[i];//本局税收
				scoreInfo.startTime = roundStartTime_;//本局开始时间
				//scoreInfo.validBetScore = llabs(userWinScore[i]);
				//scoreInfo.winLostScore = userWinScore[i];
                //scoreInfo.cellScore.push_back(llabs(scoreInfo.winLostScore));
                //scoreInfo.agentRevenue = agentRevenue[i];
				if (!table_->IsRobot((uint32_t)i) && !table_->IsOfficial((uint32_t)i)) {//真实玩家
					//当前玩家相对系统平台输赢
					int64_t platformWinLost = 0;
					for (int j = 0; j < GAME_PLAYER; ++j) {
						if (!bPlaying_[j]) {
							continue;
						}
						if (table_->IsRobot((uint32_t)j) || table_->IsOfficial((uint32_t)j)) {//机器人
							platformWinLost -= eachWinLostScore_[i][j];
						}
					}
					//scoreInfo.platformWinScore = platformWinLost * FloorScore;
				}
				scoreInfos.push_back(scoreInfo);
				//玩家积分
				s13s::GameEndScore* score = GameEnd.add_scores();
				score->set_chairid(i);
				//前端显示机器人和真实玩家抽水一致
				if (userWinScore[i] > 0) {
					if (table_->IsRobot((uint32_t)i) || table_->IsOfficial((uint32_t)i)) {
						revenue[i] = table_->CalculateRevenue(userWinScore[i]);
						//输赢积分，扣除系统抽水
						userWinScorePure[i] = userWinScore[i] - revenue[i];
						//剩余积分
						userLeftScore[i] = userScore[i] + userWinScorePure[i];
					}
				}
				score->set_score(userWinScorePure[i]);
				score->set_userscore(userLeftScore[i]);
				score_[i].set_chairid(i);
				score_[i].set_score(userWinScorePure[i]);
				score_[i].set_userscore(userLeftScore[i]);
				//跑马灯信息
				if (userWinScorePure[i] > table_->GetRoomInfo()->broadcastScore) {
				//if (userWinScorePure[i] >= m_lMarqueeMinScore) {
					table_->SendGameMessage(i, "", SMT_GLOBAL | SMT_SCROLL, userWinScorePure[i]);
					////LOG_INFO << " --- *** [" << strRoundID_ << "] 跑马灯信息 userid = " << UserIdBy(i)
					//	<< " score = " << userWinScorePure[i];
				}
				//各墩牌型
				std::string str;
				if (phandInfos_[i]->GetSelected()->specialTy >= S13S::TyThreesc) {
					//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->specialTy);
					str += std::to_string(phandInfos_[i]->GetSelected()->specialTy);
				}
				else {
					for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
						if (str.empty()) {
							//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->duns[d].ty_);
							str += std::to_string(phandInfos_[i]->GetSelected()->duns[d].ty_);
						}
						else {
							//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->duns[d].ty_);
							str += std::string("|") + std::to_string(phandInfos_[i]->GetSelected()->duns[d].ty_);
						}
					}
				}
				//输赢得水(含打枪/全垒打)
				str += std::string(",") + std::to_string(compareCards_[i].deltascore());
				//座位号，下注分数，输赢分数，各墩牌型，输赢得水
				m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore, str, false);
				//输赢积分
				//player[i].put("winLostScore", userWinScorePure[i]);
				//details.mutable_detail(i)->set_winlostscore(userWinScorePure[i]);
			}
			//items.push_back(std::make_pair("", player[i]));
		}
		GameEnd.set_wtimeleft(aniPlay_.Get_DELAY_time(GAME_STATUS_PREEND));
		std::string content = GameEnd.SerializeAsString();
		table_->SendTableData(INVALID_CHAIR, s13s::SUB_S_GAME_END, (uint8_t*)content.data(), content.size());
#ifndef _STORAGESCORE_SEPARATE_STAT_
		//系统输赢积分，扣除系统抽水1%
		if (systemWinScore > 0) {
			systemWinScore -= table_->CalculateRevenue(systemWinScore);
			//systemWinScore -= CalculateAndroidRevenue(systemWinScore);
		}
#endif
		//更新系统库存
		//table_->UpdateStorageScore(systemWinScore);
		//系统当前库存变化
		//StockScore += systemWinScore;
		//写入玩家积分
		table_->WriteUserScore(&scoreInfos[0], scoreInfos.size(), strRoundID_);

		//对局记录详情json
		//if (!m_replay.saveAsStream) {
			//root.put("gameid", ThisGameId);
			//root.add_child("list", items);
			//std::stringstream s;
			//boost::property_tree::json_parser::write_json(s, root, false);
			//m_replay.detailsData = s.str();
			//LOG_ERROR << "\n" << m_replay.detailsData.c_str();
			//boost::replace_all<std::string>(m_replay.detailsData, "\n", "");
		//}
		//对局记录详情stream
		//else {
		//	m_replay.detailsData = details.SerializeAsString();
		//}
		//保存对局结果
		table_->SaveReplay(m_replay);
		break;
	}
				   //玩家强制退出，执行托管流程
	case GER_USER_LEFT: {
		break;
	}
					  //游戏解散(内部解散/外部解散)
	case GER_DISMISS: {
		break;
	}
	default: {
		assert(false);
		break;
	}
	}
	return true;
}

static bool GlobalCompareCardsByScore(S13S::CGameLogic::groupdun_t const* src, S13S::CGameLogic::groupdun_t const* dst) {
    int bv = S13S::CGameLogic::CompareCardsByScore(src, dst, true, true, true);
    return bv > 0;
}

//换牌策略分析
//收分时概率性玩家拿小牌，AI拿大牌
//放水时概率性AI拿小牌，玩家拿大牌
void CGameTable::AnalysePlayerCards() {
	if (table_->GetRobotPlayerCount() == 0) {
		return;
	}
	//库存低于下限值，需要收分
	if (StockScore < StockLowLimit) {
		//概率收分
		int64_t weight = ((double)llabs(StockLowLimit - StockScore))
			/ llabs(StockLowLimit - StockSecondLowLimit)
			* (100 - m_i32LowerChangeRate);
		if (rand_.betweenInt64(0, 99).randFloat_mt() > m_i32LowerChangeRate + weight) {
			_LOG_INFO("StockLowLimit:%ld StockScore:%ld StockHighLimit:%ld 算法 随机", StockLowLimit, StockScore, StockHighLimit);
			return;
		}
		_LOG_INFO("StockLowLimit:%ld StockScore:%ld StockHighLimit:%ld 算法 吸分", StockLowLimit, StockScore, StockHighLimit);
		LetSysWin(true);
	}
	//库存高于上限值，需要放水
	else if (StockScore > StockHighLimit) {
		//概率放水
		int64_t weight = ((double)llabs(StockScore - StockHighLimit))
			/ llabs(StockSecondHighLimit - StockHighLimit)
			* (100 - m_i32HigherChangeRate);
		if (rand_.betweenInt64(0, 99).randFloat_mt() > m_i32HigherChangeRate + weight) {
			_LOG_INFO("StockLowLimit:%ld StockScore:%ld StockHighLimit:%ld 算法 随机", StockLowLimit, StockScore, StockHighLimit);
			return;
		}
		_LOG_INFO("StockLowLimit:%ld StockScore:%ld StockHighLimit:%ld 算法 吐分", StockLowLimit, StockScore, StockHighLimit);
		LetSysWin(false);
	}
	else {
		_LOG_INFO("StockLowLimit:%ld StockScore:%ld StockHighLimit:%ld 算法 随机", StockLowLimit, StockScore, StockHighLimit);
	}
}

//让系统赢或输
void CGameTable::LetSysWin(bool sysWin) {
	if (table_->GetRobotPlayerCount() == 0 || table_->GetRealPlayerCount() == 0) {
		return;
	}
	std::vector<S13S::CGameLogic::groupdun_t const*> v;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有人
		if (!bPlaying_[i]) {
			continue;
		}
#if 0 //没有考虑特殊牌型
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = handInfos_[i].enum_groups.begin();
			it != handInfos_[i].enum_groups.end(); ++it) {
			it->chairID = i;
		}
		//获取当前玩家的一组最优枚举解
		v.push_back(&handInfos_[i].enum_groups[0]);
#else //考虑了特殊牌型
		for (std::vector<S13S::CGameLogic::groupdun_t const*>::iterator it = handInfos_[i].groups.begin();
			it != handInfos_[i].groups.end(); ++it) {
			const_cast<S13S::CGameLogic::groupdun_t*>(*it)->chairID = i;
		}
		//获取当前玩家的一组最优枚举解
		v.push_back(handInfos_[i].groups[0]);
#endif
	}
	//按输赢得水排序(不计打枪/全垒打)
	std::sort(&v.front(), &v.front() + v.size(), GlobalCompareCardsByScore);
	int c = 0;
	//换牌前按牌大小座位排次
	int info[GAME_PLAYER] = { 0 };
	//换牌后按牌大小座位排次(1.系统赢：AI排前面，真实玩家排后面 2.系统输：真实玩家排前面，AI排后面)
	std::vector<int> vinfo;
	for (std::vector<S13S::CGameLogic::groupdun_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		info[c++] = (*it)->chairID;
		if (sysWin) {
			//如果是机器人
			if (table_->IsRobot((uint32_t)(*it)->chairID) > 0) {
				vinfo.push_back((*it)->chairID);
				_LOG_WARN(" 吸分 %d %d 机器人", (*it)->chairID, UserIdBy((*it)->chairID));
			}
		}
		else {
			//如果是真实玩家
			if (table_->IsRobot((uint32_t)(*it)->chairID) == 0) {
				vinfo.push_back((*it)->chairID);
				_LOG_WARN(" 吐分 %d %d 玩家", (*it)->chairID, UserIdBy((*it)->chairID));
			}
		}
	}
	for (std::vector<S13S::CGameLogic::groupdun_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		if (sysWin) {
			//如果是真实玩家
			if (table_->IsRobot((uint32_t)(*it)->chairID) == 0) {
				vinfo.push_back((*it)->chairID);
				_LOG_WARN(" 吸分 %d %d 玩家", (*it)->chairID, UserIdBy((*it)->chairID));
			}
		}
		else {
			//如果是机器人
			if (table_->IsRobot((uint32_t)(*it)->chairID) > 0) {
				vinfo.push_back((*it)->chairID);
				_LOG_WARN(" 吐分 %d %d 机器人", (*it)->chairID, UserIdBy((*it)->chairID));
			}
		}
	}
	assert(c == vinfo.size());
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌
		if (info[i] != vinfo[i]) {
			//同是机器人或真实玩家，保持原来位置不变
			if ((
				table_->IsRobot((uint32_t)info[i]) > 0 &&
				table_->IsRobot((uint32_t)vinfo[i]) > 0) ||
				(table_->IsRobot((uint32_t)info[i]) == 0 &&
				table_->IsRobot((uint32_t)vinfo[i]) == 0)) {
				int j = 0;
				for (; j < vinfo.size(); ++j) {
					//找出info[i]在vinfo中的位置
					if (vinfo[j] == info[i]) {
						break;
					}
				}
				//_LOG_WARN("恢复 %d -> %d", vinfo[i], vinfo[j]);
				//恢复原来的位置
				std::swap(vinfo[i], vinfo[j]);
			}
		}
	}
	char msg[1024] = { 0 };
	std::string strmsg, strtmp, strtmp2;
	for (int i = 0; i < c; ++i) {
		snprintf(msg, sizeof(msg), "[%d] chairID = %d %s\n",
			i, info[i],
			table_->IsRobot((uint32_t)info[i]) > 0 ? "机器人" : "玩家");
		strtmp += msg;
	}
	strmsg += std::string("\n\n换牌前按牌大小座位排次\n") + strtmp;
	for (int i = 0; i < vinfo.size(); ++i) {
		snprintf(msg, sizeof(msg), "[%d] chairID = %d %s\n",
			i, vinfo[i],
			table_->IsRobot((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家");
		strtmp2 += msg;
	}
	strmsg += std::string("换牌后按牌大小座位排次\n") + strtmp2;
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌，并且换牌后是机器人则换牌
		if (info[i] != vinfo[i]) {
			//系统赢，玩家牌换成机器人牌
			//系统输，机器人牌换成玩家牌
			if ((sysWin && (table_->IsRobot((uint32_t)info[i]) == 0 && table_->IsRobot((uint32_t)vinfo[i]) > 0)) ||
				(!sysWin && (table_->IsRobot((uint32_t)info[i]) > 0 && table_->IsRobot((uint32_t)vinfo[i]) == 0))) {
				//交换手牌信息
				std::swap(phandInfos_[info[i]], phandInfos_[vinfo[i]]);
				//交换手牌
				uint8_t handcards[MAX_COUNT];
				memcpy(handcards, &(handCards_[info[i]])[0], MAX_COUNT);
				snprintf(msg, sizeof(msg), "换牌前，%s %d 手牌 [%s]\n", (table_->IsRobot((uint32_t)info[i]) > 0 ? "机器人" : "玩家"), info[i], S13S::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
				strmsg += msg;
				memcpy(&(handCards_[info[i]])[0], &(handCards_[vinfo[i]])[0], MAX_COUNT);
				snprintf(msg, sizeof(msg), "换牌后，%s %d 手牌 [%s]\n", (table_->IsRobot((uint32_t)info[i]) > 0 ? "机器人" : "玩家"), info[i], S13S::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
				strmsg += msg;
				snprintf(msg, sizeof(msg), "换牌前，%s %d 手牌 [%s]\n", (table_->IsRobot((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家"), vinfo[i], S13S::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
				strmsg += msg;
				memcpy(&(handCards_[vinfo[i]])[0], handcards, MAX_COUNT);
				snprintf(msg, sizeof(msg), "换牌后，%s %d 手牌 [%s]\n", (table_->IsRobot((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家"), vinfo[i], S13S::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
				strmsg += msg;
			}
		}
	}
	if (writeRealLog_)
		_LOG_INFO("[%s] %s", strRoundID_.c_str(), strmsg.c_str());
}

void CGameTable::ReadConfigInformation() {
	static STD::Random r;
	time_t now = time(NULL);
	uint32_t elapsed = now - lastReadCfgTime_;
	if (elapsed > (readIntervalTime_ + r.betweenInt(0, 10).randInt_mt())) {
		assert(table_);
		if (!boost::filesystem::exists(INI_FILENAME)) {
			_LOG_ERROR("%s not exists", INI_FILENAME);
			return;
		}
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(INI_FILENAME, pt);

		std::string strRoomName = "GameServer_" + std::to_string(table_->GetRoomInfo()->roomId);
		m_lMarqueeMinScore = pt.get<double>(strRoomName + ".MarqueeMinScore", 1000);

		//系统当前库存
		//table_->GetStorageScore(storageInfo_);

		//分片匹配时长
		sliceMatchSeconds_ = pt.get<double>("MatchRule.sliceMatchSeconds_", 0.4);
		//匹配真人超时时长
		timeoutMatchSeconds_ = pt.get<double>("MatchRule.timeoutMatchSeconds_", 2.0);
		//补充机器人超时时长
		timeoutAddAndroidSeconds_ = pt.get<double>("MatchRule.timeoutAddAndroidSeconds_", 1.0);
		//空闲踢人间隔时间
		kickPlayerIdleTime_ = pt.get<int>("Global.kickPlayerIdleTime_", 30);
		//更新配置间隔时间
		readIntervalTime_ = pt.get<int>("Global.readIntervalTime_", 300);

		//理牌时间
		groupTime_ = pt.get<int>("Global.groupTime_", 25);
		//开牌比牌时间
		openCardTime_ = pt.get<int>("Global.openCardTime_", 3);
		//打枪/全垒打时间
		shootTime_ = pt.get<int>("Global.shootTime_", 2);
		//结算飘金币时间
		winLostTime_ = pt.get<int>("Global.winLostTime_", 3);
		//下一局倒计时间
		nextTime_ = pt.get<int>("Global.nextTime_", 3);

		m_i32LowerChangeRate = pt.get<int32_t>("GAME_CONFIG.LowerChangeRate", 80);
		m_i32HigherChangeRate = pt.get<int32_t>("GAME_CONFIG.HigherChangeRate", 60);

		lastReadCfgTime_ = now;
	}
}

//得到桌子实例
extern "C" std::shared_ptr<ITableDelegate> CreateTableDelegate() {
	return std::shared_ptr<ITableDelegate>(new CGameTable());
}

//删除桌子实例
extern "C" void DeleteTableDelegate(std::shared_ptr<ITableDelegate>& tableDelegate) {
	tableDelegate.reset();
}