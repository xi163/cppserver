#include <stdio.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdarg.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>

#include <glog/logging.h>

#include "proto/texas.Message.pb.h"

#include "cfg.h"
#include "StdRandom.h"

#include "GlobalFunc.h"
#include "ToolTime.h"

#include "TableFrameSink.h"

class CGLogInit
{
public:
	CGLogInit()
	{
		string logPath;
		GlobalFunc::getLogPath(GAME_CONF_PATH, logPath);
		logPath += "/TEXAS/";

		FLAGS_colorlogtostderr = true;
		FLAGS_log_dir = logPath;
		FLAGS_logbufsecs = 1;
		if (!boost::filesystem::exists(FLAGS_log_dir))
		{
			boost::filesystem::create_directories(FLAGS_log_dir);
		}
		google::InitGoogleLogging("texas");
		// set std output special log content value.
		google::SetStderrLogging(google::GLOG_INFO);
		mkdir(logPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	}

	virtual ~CGLogInit()
	{
		//google::ShutdownGoogleLogging();
	}
};

CGLogInit glog_init;

#define TIME_GAME_ADD_SCORE       15 //下注时长(second)
#define TIME_GAME_COMPARE_DELAY   4  //比牌动画时长
#define TIME_GAME_START_DELAY     2

CTableFrameSink::CTableFrameSink(void)
{
    bankerUser_ = INVALID_CHAIR;
    currentUser_ = INVALID_CHAIR;
    firstUser_ = INVALID_CHAIR;
	smallBlindUser_ = INVALID_CHAIR;
	bigBlindUser_ = INVALID_CHAIR;
	nextUser_ = INVALID_CHAIR;
    currentWinUser_ = INVALID_CHAIR;
	memset(takeScore_, 0, sizeof(takeScore_));
	memset(userWinScorePure_, 0, sizeof(userWinScorePure_));
	memset(tableScore_, 0, sizeof(tableScore_));
	memset(cfgTakeScore_, 0, sizeof cfgTakeScore_);
	memset(needTip_, 0, sizeof needTip_);
	memset(setscore_, 0, sizeof setscore_);
	currentTurn_ = 0;
	isNewTurn_ = false;
	isExceed_ = false;
    tableAllScore_ = 0;
    opEndTime_ = 0;
    opTotalTime_ = 0;
	lastReadCfgTime_ = 0;
	readIntervalTime_ = 0;
	memset(handCards_,  0, sizeof(handCards_));
	memset(bPlaying_, 0, sizeof(bPlaying_));
	g.InitCards();
	g.ShuffleCards();
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//匹配真人超时时长(可配置)
	timeoutMatchSeconds_ = 0.8f;
	//补充机器人超时时长(可配置)
	timeoutAddAndroidSeconds_ = 0.4f;
	// 
	freeLookover_ = false;
	freeGiveup_ = false;
	openAllCards_ = false;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		allInTurn_[i] = -1;
		m_bPlayerOperated[i] = false;
		m_bPlayerCanOpt[i] = false;
	}
	gameStatus_ = GAME_STATUS_INIT;
}

CTableFrameSink::~CTableFrameSink(void)
{
}

//复位桌子
void CTableFrameSink::RepositionSink()
{

}

//设置指针
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
	//桌子指针
	m_pTableFrame = pTableFrame; assert(m_pTableFrame);
	//对局日志
	m_replay.cellscore = FloorScore;
	m_replay.roomname = ThisRoomName;
	//m_replay.gameid = ThisGameId;//游戏类型
	//m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	ReadConfigInformation();
    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
	for (int i = 0; i < GAME_PLAYER; ++i) {
		tableScore_[i] = 0;
		tableChip_[i].clear();
		noGiveUpTimeout_[i] = true;
		all_[i].Reset();
		cover_[i].Reset();
		allInTurn_[i] = -1;
		m_bPlayerOperated[i] = false;
		m_bPlayerCanOpt[i] = false;
		m_bShowCard[i] = false;
	}
	tableAllChip_.clear();
	settleChip_.clear();
	userinfos_.clear();
	tableAllScore_ = 0;
	deltaAddScore_ = 0;
	//bankerUser_ = INVALID_CHAIR;
	currentUser_ = INVALID_CHAIR;
	referenceUser_ = INVALID_CHAIR;
	firstUser_ = INVALID_CHAIR;
	smallBlindUser_ = INVALID_CHAIR;
	bigBlindUser_ = INVALID_CHAIR;
	nextUser_ = INVALID_CHAIR;
	currentWinUser_ = INVALID_CHAIR;
	currentTurn_ = 0;
	isNewTurn_ = false;
	isExceed_ = false;
	opEndTime_ = 0;
	opTotalTime_ = 0;
	memset(bPlaying_, 0, sizeof(bPlaying_));
	memset(handCards_, 0, sizeof(handCards_));
	memset(combCards_, 0, sizeof(combCards_));
	memset(cardsC_, 0, sizeof cardsC_);
	memset(operate_, 0, sizeof(operate_));
	memset(addScore_, 0, sizeof(addScore_));
	memset(isLooked_, false, sizeof isLooked_);
	memset(isGiveup_, false, sizeof isGiveup_);
	memset(isLost_, false, sizeof isLost_);
	memset(isCompared_, false, sizeof isCompared_);
	m_replay.clear();
	offset_ = 0;
	m_GameParticipant.clear();
	pots_.clear();
	potsT_.clear();
	showPots_.clear();
	strRoundID_.clear();
}

//初始化游戏数据
void CTableFrameSink::InitGameData()
{
	assert(m_pTableFrame);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		tableScore_[i] = 0;
		tableChip_[i].clear();
		noGiveUpTimeout_[i] = true;
		all_[i].Reset();
		cover_[i].Reset();
		allInTurn_[i] = -1;
		m_bPlayerOperated[i] = false;
		m_bPlayerCanOpt[i] = false;
		m_bShowCard[i] = false;
	}
	tableAllChip_.clear();
	settleChip_.clear();
	userinfos_.clear();
    tableAllScore_ = 0;
	deltaAddScore_ = 0;
	//bankerUser_ = INVALID_CHAIR;
	currentUser_ = INVALID_CHAIR;
	referenceUser_ = INVALID_CHAIR;
	firstUser_ = INVALID_CHAIR;
	smallBlindUser_ = INVALID_CHAIR;
	bigBlindUser_ = INVALID_CHAIR;
	nextUser_ = INVALID_CHAIR;
	currentWinUser_ = INVALID_CHAIR;
	currentTurn_ = 0;
	isNewTurn_ = false;
	isExceed_ = false;
    opEndTime_ = 0;
    opTotalTime_ = 0;
	memset(bPlaying_, 0, sizeof(bPlaying_));
    memset(handCards_, 0, sizeof(handCards_));
	memset(combCards_, 0, sizeof(combCards_));
	memset(cardsC_, 0, sizeof cardsC_);
	memset(operate_, 0, sizeof(operate_));
	memset(addScore_, 0, sizeof(addScore_));
	memset(isLooked_, false, sizeof isLooked_);
	memset(isGiveup_, false, sizeof isGiveup_);
	memset(isLost_, false, sizeof isLost_);
	memset(isCompared_, false, sizeof isCompared_);
	m_replay.clear();
	offset_ = 0;
	m_GameParticipant.clear();
	pots_.clear();
	potsT_.clear();
	showPots_.clear();
}


//清除所有定时器
void CTableFrameSink::ClearAllTimer()
{
	ThisThreadTimer->cancel(timerIdGameReadyOver_);
    ThisThreadTimer->cancel(timerIdWaiting_);
	ThisThreadTimer->cancel(timerIdGameEnd_);
}

//用户进入
bool CTableFrameSink::OnUserEnter(int64_t userId, bool islookon)
{
	assert(m_pTableFrame);
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);
	assert(userItem);
	writeRealLog_ = (m_pTableFrame->GetRealPlayerCount() > 0);
	char msg[1024];
	//if (!bPlaying_[userItem->GetChairId()]) {
		m_bPlayerOperated[userItem->GetChairId()] = true;
		m_bRoundEndExit[userItem->GetChairId()] = false;
		m_bShowCard[userItem->GetChairId()] = false;
	//}
	initUserTakeScore(userItem);
	//准备时启动定时器
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_READY) {
		//必须真人玩家
		//assert(m_pTableFrame->GetPlayerCount() == 1);
		//assert(m_pTableFrame->GetRealPlayerCount() == 1);
		if (!userItem->IsAndroidUser()) {
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] %s %d %lld 首次入桌(real=%d AI=%d total=%d) userScore[%lld] takeScore[%lld]\n",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(userItem->IsAndroidUser() ? "AI" : "真人"), userItem->GetChairId(), userId,
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount(),
				userItem->GetUserScore(), takeScore_[userItem->GetChairId()]);
			LOG(INFO) << __FUNCTION__ << msg;
		}
		else if (writeRealLog_) {
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] %s %d %lld 首次入桌(real=%d AI=%d total=%d) userScore[%lld] takeScore[%lld]\n",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(userItem->IsAndroidUser() ? "AI" : "真人"), userItem->GetChairId(), userId,
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount(),
				userItem->GetUserScore(), takeScore_[userItem->GetChairId()]);
			LOG(INFO) << __FUNCTION__ << msg;
		}
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		gameStatus_ = GAME_STATUS_READY;
		totalMatchSeconds_ = 0;
		if (maxAndroid_ == 0)
			maxAndroid_ = rand_.betweenInt(1, 8/*m_pTableFrame->GetGameRoomInfo()->maxAndroidCount*/).randInt_mt();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		OnEventGameScene(userItem->GetChairId(), false);
	}
	else {
		if (!userItem->IsAndroidUser()) {
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] %s %d %lld %s入桌(real=%d AI=%d total=%d) userScore[%lld] takeScore[%lld]\n",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(userItem->IsAndroidUser() ? "AI" : "真人"), userItem->GetChairId(), userId, (bPlaying_[userItem->GetChairId()] ? "重连" : "新加"),
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount(),
				userItem->GetUserScore(), takeScore_[userItem->GetChairId()]);
			LOG(INFO) << __FUNCTION__ << msg;
		}
		else if (writeRealLog_) {
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] %s %d %lld %s入桌(real=%d AI=%d total=%d) userScore[%lld] takeScore[%lld]\n",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(),
				(userItem->IsAndroidUser() ? "AI" : "真人"), userItem->GetChairId(), userId, (bPlaying_[userItem->GetChairId()] ? "重连" : "新加"),
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount(),
				userItem->GetUserScore(), takeScore_[userItem->GetChairId()]);
			LOG(INFO) << __FUNCTION__ << msg;
		}
		OnEventGameScene(userItem->GetChairId(), false);
	}
	//机器人中途加入
// 	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START &&
// 		userItem->IsAndroidUser() && !bPlaying_[userItem->GetChairId()]) {
// 		texas::CMD_S_AndroidEnter rspdata;
// 		rspdata.set_chairid(userItem->GetChairId());//发空包协议，机器人收不到消息
// 		string content = rspdata.SerializeAsString();
// 		//m_pTableFrame->SendTableData(userItem->GetChairId(), texas::SUB_S_ANDROID_ENTER, NULL, 0);
// 		m_pTableFrame->SendTableData(userItem->GetChairId(), texas::SUB_S_ANDROID_ENTER, (uint8_t*)content.data(), content.size());
// 	}
	return true;
}

//判断桌子能否进人
bool CTableFrameSink::CanJoinTable(int64_t userId) {
	//达到房间最大人数不能进了
	if (m_pTableFrame->GetPlayerCount() >= GAME_PLAYER) {
		if (userId >= MIN_SYS_USER_ID) {//断线重连可以进
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);
			if (userItem) {
 				return true;
			}
		}
		return false;
	}
	if (userId == -1) { //new android enter
		//机器人先入桌的话要初始化
		if (maxAndroid_ == 0)
			maxAndroid_ = rand_.betweenInt(1, 8/*m_pTableFrame->GetGameRoomInfo()->maxAndroidCount*/).randInt_mt();
		//游戏开始了机器人新玩家不准进入
		if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
			if (m_pTableFrame->GetAndroidPlayerCount() >= maxAndroid_) {
				return false;
			}
			static STD::Random r(1, 100);
			if (r.randInt_mt() <= 20) {
				return true;
			}
			return false;
		}
		//匹配真人时间或没有真人玩家，机器人不准进入
		if (totalMatchSeconds_ < timeoutMatchSeconds_ || m_pTableFrame->GetRealPlayerCount() < 1) {
			return false;
		}
		//根据房间机器人配置来决定补充多少机器人
		return m_pTableFrame->GetAndroidPlayerCount() < maxAndroid_;
	}
	else if (userId == 0) { //new real user enter
		return true;
	}
	else if (userId >= MIN_SYS_USER_ID) {//断线重连
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);
		if (userItem) {
			return true;
		}
	}
	return false;
}

//梭哈、德州使用当前携带金币
bool CTableFrameSink::CanJionGame(int64_t userId, int64_t score) {
// 	char msg[1024] = { 0 };
// 	if (userId >= MIN_SYS_USER_ID) {
// 		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);
// 		if (userItem) {
// 			assert(score == userItem->GetCurTakeScore());
// 			LOG(INFO) << __FUNCTION__ << msg;
// 			if (userItem->GetCurTakeScore() > 0 &&
// 				userItem->GetCurTakeScore() <= userItem->GetUserScore() &&
// 				userItem->GetCurTakeScore() >= EnterMinScore) {
// 				if ((userItem->GetTakeMinScore() > 0 && userItem->GetCurTakeScore() < userItem->GetTakeMinScore()) ||
// 					(userItem->GetTakeMaxScore() > 0 && userItem->GetCurTakeScore() > userItem->GetTakeMaxScore())) {
// 					snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %d %d 携带金额=%lld false 1-----------------------",
// 						m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(),
// 						userItem->GetChairId(), userId, score);
// 					LOG(INFO) << __FUNCTION__ << msg;
// 					return false;
// 				}
// 				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %d %d 携带金额=%lld true-----------------------",
// 					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(),
// 					userItem->GetChairId(), userId, score);
// 				LOG(INFO) << __FUNCTION__ << msg;
// 				return true;
// 			}
// 			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %d %d 携带金额=%lld false 2-----------------------",
// 				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(),
// 				userItem->GetChairId(), userId, userItem->GetCurTakeScore());
// 			LOG(INFO) << __FUNCTION__ << msg;
// 		}
// 		else {
			if (score > 0 &&
				score >= EnterMinScore) {
				return true;
			}
// 			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %d 携带金额=%lld false 3-----------------------",
// 				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), userId, score);
// 			LOG(INFO) << __FUNCTION__ << msg;
// 			return false;
// 		}
// 	}
// 	else {
// 		snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %d 携带金额=%lld false 4-----------------------",
// 			m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), userId, score);
// 		LOG(INFO) << __FUNCTION__ << msg;
// 	}
	return false;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
	shared_ptr<CServerUserItem> userItem = ByUserId(userId);
	if (!userItem) {
		return true;
	}
	uint32_t chairId = userItem->GetChairId();
	if (/*m_pTableFrame->GetGameStatus()*/gameStatus_ < GAME_STATUS_START ||
		/*m_pTableFrame->GetGameStatus()*/gameStatus_ >= GAME_STATUS_END ||
		!bPlaying_[chairId] || isGiveup_[chairId]) {
		return true;
	}
	return false;
}

bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    return true;
}

//用户离开
bool CTableFrameSink::OnUserLeft(int64_t userId, bool bIsLookUser)
{
	char msg[1024];
	assert(m_pTableFrame);
	shared_ptr<CServerUserItem> userItem = ByUserId(userId); //assert(userItem);
	if (!userItem) {
		//assert(false);
		return false;
	}
	assert(userItem->GetChairId() >= 0);
	assert(userItem->GetChairId() < GAME_PLAYER);
	bool isAndroid = userItem->IsAndroidUser();
	uint32_t chairId = userItem->GetChairId();
	//游戏未开始/已结束/非参与玩家
	if (/*m_pTableFrame->GetGameStatus()*/gameStatus_ < GAME_STATUS_START ||
		/*m_pTableFrame->GetGameStatus()*/gameStatus_ >= GAME_STATUS_END ||
		!bPlaying_[chairId] || isGiveup_[chairId]) {
		if (writeRealLog_) {
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %s[%d][%d]准备离桌 real=%d AI=%d total=%d",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
			LOG(INFO) << __FUNCTION__ << msg;
		}
		bPlaying_[chairId] = false;
		m_bRoundEndExit[chairId] = false;
		m_bShowCard[chairId] = false;
		userItem->SetUserStatus(sOffline);
		m_pTableFrame->ClearTableUser(chairId, true, true);
		//匹配中没有真人玩家，清理腾出桌子
		if (/*m_pTableFrame->GetGameStatus()*/gameStatus_ == GAME_STATUS_READY &&
			/*m_pTableFrame->GetRealPlayerCount()*/m_pTableFrame->GetPlayerCount() == 0) {
			if (writeRealLog_) {
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %s[%d][%d]已经离桌(real=%d AI=%d total=%d) 重置桌子",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
					m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				bPlaying_[chairId] = false;
				m_bRoundEndExit[chairId] = false;
				m_bShowCard[chairId] = false;
				ByChairId(i)->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true);
			}
			ClearAllTimer();
			m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
			gameStatus_ = GAME_STATUS_INIT;
			writeRealLog_ = false;
		}
		else {
			if (writeRealLog_) {
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] %s[%d][%d]已经离桌(real=%d AI=%d total=%d) 等待重置",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), (isAndroid ? "AI" : "真人"), chairId, userId,
					m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
		}
		UserInfoList::iterator it = userinfos_.find(userId);
		if (it != userinfos_.end()) {
			it->second.isLeave = true;
			assert(!bPlaying_[it->second.chairId]);
		}
		return true;
	}
	return false;
}

//准备定时器
void CTableFrameSink::OnTimerGameReadyOver() {
	writeRealLog_ = (m_pTableFrame->GetRealPlayerCount() > 0);
	char msg[1024];
	//匹配中有真人玩家
	//assert(m_pTableFrame->GetRealPlayerCount() > 0);
	//匹配中准备状态
	assert((int)m_pTableFrame->GetGameStatus() == GAME_STATUS_READY);
	//累计匹配时间
	totalMatchSeconds_ += sliceMatchSeconds_;
	//匹配真人时间
	if (totalMatchSeconds_ <= timeoutMatchSeconds_) {
		if (m_pTableFrame->GetPlayerCount() < GAME_PLAYER) {
			if (writeRealLog_) {
				//不满游戏人数，继续等待
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]匹配真人时间(dt=%.1f|%.1f)，不满游戏人数(real=%d AI=%d total=%d)，继续等待\n",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
		}
		else {
			if (writeRealLog_) {
				//满足游戏人数，开始游戏
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]匹配真人时间(dt=%.1f|%.1f)，满足游戏人数(real=%d AI=%d total=%d)，开始游戏!\n",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
			GameTimerReadyOver();
		}
	}
	//匹配真人超时，补充机器人时间
	else if (totalMatchSeconds_ > timeoutMatchSeconds_ &&
		(totalMatchSeconds_ <= (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_))) {
		if (m_pTableFrame->GetPlayerCount() < GAME_PLAYER) {
			if (writeRealLog_) {
				//不满游戏人数，继续等待
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]补充机器人(max=%d)时间(dt=%.1f|%.1f)，不满游戏人数(real=%d AI=%d total=%d)，继续等待\n",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
		}
		else {
			if (writeRealLog_) {
				//满足游戏人数，开始游戏
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]补充机器人(max=%d)时间(dt=%.1f|%.1f)，满足游戏人数(real=%d AI=%d total=%d)，开始游戏!\n",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
			GameTimerReadyOver();
		}
	}
	//补充机器人超时
	else /*if (totalMatchSeconds_ > (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_))*/ {
		if (m_pTableFrame->GetPlayerCount() >= MIN_GAME_PLAYER) {
			if (writeRealLog_) {
				//满足最小游戏人数，开始游戏
				snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]补充机器人(max=%d)超时(dt=%.1f|%.1f)，满足最小游戏人数(real=%d AI=%d total=%d)，开始游戏!\n",
					m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
				LOG(INFO) << __FUNCTION__ << msg;
			}
			GameTimerReadyOver();
		}
		else {
			//匹配很久，不满最小游戏人数，重置桌子
			if (totalMatchSeconds_ > (timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) + 10 && m_pTableFrame->GetRealPlayerCount() == 0) {
				if (writeRealLog_) {
					snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] 中止匹配 重置桌子 real=%d AI=%d total=%d",
						m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(),
						m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
					LOG(INFO) << __FUNCTION__ << msg;
				}
				//totalMatchSeconds_ = 0;
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (!m_pTableFrame->IsExistUser(i)) {
						continue;
					}
					bPlaying_[i] = false;
					m_bRoundEndExit[i] = false;
					ByChairId(i)->SetUserStatus(sOffline);
					m_pTableFrame->ClearTableUser(i, true, true);
				}
				ClearAllTimer();
				m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
				gameStatus_ = GAME_STATUS_INIT;
				writeRealLog_ = false;
			}
			else {
				if (writeRealLog_) {
					//不满最小游戏人数，继续等待
					snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s]补充机器人(max=%d)超时(dt=%.1f|%.1f)，不满最小游戏人数(real=%d AI=%d total=%d)，继续等待\n",
						m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), maxAndroid_, sliceMatchSeconds_, totalMatchSeconds_, m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
					LOG(INFO) << __FUNCTION__ << msg;
				}
			}
		}
	}
}

//游戏开局前检查
void CTableFrameSink::CheckGameStart() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
		if (userItem) {
			//离线，踢出玩家
			if (userItem->GetUserStatus() == sOffline) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				m_pTableFrame->ClearTableUser(i, true, true);
			}
			//账号停用，踢出玩家
			else if (userItem->GetUserStatus() == sStop) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				userItem->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_STOP_CUR_USER);
			}
			//积分不足，踢出玩家
			else if (userItem->GetUserScore() < EnterMinScore) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				userItem->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
			//积分太多，踢出玩家
 			//else if (EnterMaxScore > 0 && userItem->GetUserScore() > EnterMaxScore) {
			//	bPlaying_[i] = false;
 			//	m_bRoundEndExit[i] = false;
 			//	userItem->SetUserStatus(sOffline);
 			//	m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORELIMIT);
 			//}
		}
	}
}

//结束准备，开始游戏
void CTableFrameSink::GameTimerReadyOver() {
	writeRealLog_ = (m_pTableFrame->GetRealPlayerCount() > 0);
	ThisThreadTimer->cancel(timerIdGameReadyOver_);
	CheckGameStart();
	//有真人玩家且够游戏人数，开始游戏
	if (/*m_pTableFrame->GetRealPlayerCount() > 0 &&*/ m_pTableFrame->GetPlayerCount() >= MIN_GAME_PLAYER) {
		InitGameData();
		//游戏状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_START);
		gameStatus_ = GAME_STATUS_START;
		//玩家状态
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				//非离线状态
				//LOG(INFO) << __FUNCTION__ << " --- *** chairID=" << i << " stat=" << StringPlayerStat(userItem->GetUserStatus());
				//assert(userItem->GetUserStatus() != sOffline);
				userItem->SetUserStatus(sPlaying);
				bPlaying_[i] = true;
				//玩家出局(弃牌/比牌输)，若离开房间，信息将被清理，用户数据副本用来写记录
				GSUserBaseInfo& baseinfo = userItem->GetUserBaseInfo();
				assert(baseinfo.userId == userItem->GetUserId());
				userinfo_t& userinfo = userinfos_[userItem->GetUserId()];
				userinfo.init();//不用构造，防止被隐式析构
				userinfo.chairId = i;
				userinfo.userId = baseinfo.userId;
				userinfo.promoterId = userItem->GetPromoterId();
				userinfo.bankScore = userItem->GetBankScore();
				userinfo.headerId = baseinfo.headId;
				userinfo.nickName = baseinfo.nickName;
				userinfo.location = baseinfo.location;
				userinfo.vipLevel = baseinfo.vip;
				userinfo.headboxId = baseinfo.headboxId;
				userinfo.headImgUrl = baseinfo.headImgUrl;
				//userinfo.account = baseinfo.account;
				//userinfo.agentId = baseinfo.agentId;
				//userinfo.lineCode = baseinfo.lineCode;
				userinfo.takeScore = takeScore_[i];
				userinfo.userScore = userItem->GetUserScore();
				userinfo.isAndroidUser = m_pTableFrame->IsAndroidUser((uint32_t)i);
				userinfo.isSystemUser = m_pTableFrame->IsSystemUser((uint32_t)i);
				if (!together_ && userinfo.isAndroidUser == 0) {
					m_GameParticipant.insert(baseinfo.userId);
					m_pTableFrame->SetUserTogetherPlaying(m_GameParticipant);
				}
			}
			else {
				bPlaying_[i] = false;
			}
		}
		assert(userinfos_.size() > 0);
		//开始游戏
		OnGameStart();
	}
	else if (m_pTableFrame->GetPlayerCount() > 0) {
		if (writeRealLog_) {
			char msg[1024];
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] 不满最小游戏人数(real=%d AI=%d total=%d)，重新匹配!",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(),
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
			LOG(INFO) << __FUNCTION__ << msg;
		}
		//ClearGameData();
		//m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		//gameStatus_ = GAME_STATUS_READY;
		//totalMatchSeconds_ = 0;
		maxAndroid_ = rand_.betweenInt(1, 8/*m_pTableFrame->GetGameRoomInfo()->maxAndroidCount*/).randInt_mt();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
	}
	//继续下一局可能走该流程
	else {
		if (writeRealLog_) {
			char msg[1024];
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s] 中断游戏 重置桌子 real=%d AI=%d total=%d",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(),
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
			LOG(INFO) << __FUNCTION__ << msg;
		}
		//没有真人玩家或不够游戏人数(<MIN_GAME_PLAYER)，清理腾出桌子
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			bPlaying_[i] = false;
			m_bRoundEndExit[i] = false;
			m_bShowCard[i] = false;
			ByChairId(i)->SetUserStatus(sOffline);
			m_pTableFrame->ClearTableUser(i, true, true);
		}
		ClearAllTimer();
		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		gameStatus_ = GAME_STATUS_INIT;
		writeRealLog_ = false;
	}
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
	ReadConfigInformation();
	//牌局编号
    strRoundID_ = m_pTableFrame->GetNewRoundId();
	roundId_ = m_pTableFrame->GetNewRoundInt64Id();
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	//系统当前库存
	//m_pTableFrame->GetStorageScore(storageInfo_);
	if (writeRealLog_) {
		char msg[1024];
		snprintf(msg, sizeof(msg), "\n--- *** tableID[%d][%s][%s][%d] 当前库存=%s 开始游戏(real=%d AI=%d total=%d)!\n",
			m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, std::to_string(StockScore).c_str(),
			m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
		LOG(INFO) << __FUNCTION__ << msg;
	}
	//本局开始时间
    roundStartTime_ = chrono::system_clock::now();
	//配置牌型
 	string strFile = "./Card/TexasCards.ini";
	int flag = 0;
	if (!strFile.empty() && boost::filesystem::exists(strFile)) {
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(strFile, pt);
		//1->文件读取手牌 0->随机牌
		flag = pt.get<int>("StartDeal.ReadCardsFromFile", 0);
		if (flag > 0) {
			//构造桌面公共牌
			std::vector<std::string> v;
			std::string s = pt.get<std::string>("StartDeal.public", "");
			boost::split(v,
				s,
				boost::is_any_of(" "), boost::token_compress_on);
			assert(v.size() == TBL_CARDS);
			TEXAS::CGameLogic::MakeCardList(
				s,
				&tableCards_[0], TBL_CARDS);
			//构造玩家手牌
			int idx = 0;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!bPlaying_[i]) {
					continue;
				}
				char chair[256];
				snprintf(chair, sizeof(chair), "StartDeal.Chair%d", idx++);
				std::string s1 = pt.get<std::string>(chair, "");
				boost::split(v,
					s1,
					boost::is_any_of(" "), boost::token_compress_on);
				assert(v.size() == MAX_COUNT);
				TEXAS::CGameLogic::MakeCardList(
					s1,
					&(handCards_[i])[0], MAX_COUNT);
			}
		}
	}
	if (!flag) {
		//给各个玩家发牌
	restart:
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
		}
		if (g.Remaining() < TBL_CARDS) {
			g.ShuffleCards();
			goto restart;
		}
		//桌面扑克(翻牌3/转牌1/河牌1)
		g.DealCards(TBL_CARDS, &tableCards_[0]);
	}
	//各个玩家手牌分析
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		//合并手牌&桌面牌
		memcpy(&(combCards_[i])[0], &(handCards_[i])[0], MAX_COUNT);
		memcpy(&(combCards_[i])[MAX_COUNT], &tableCards_[0], TBL_CARDS);
		TEXAS::CGameLogic::AnalyseCards(&(combCards_[i])[0], MAX_TOTAL, all_[i]);
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
		//m_replay.addPlayer(userItem->GetUserId(), userItem->GetNickName(), takeScore_[i], i);
	}
	//换牌策略分析
	AnalysePlayerCards();
	//当前最大牌用户
	currentWinUser_ = INVALID_CHAIR;
	//比牌判断赢家
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (currentWinUser_ == INVALID_CHAIR) {
			currentWinUser_ = i;
			continue;
		}
		if (TEXAS::CGameLogic::CompareCards(all_[i], all_[currentWinUser_]) > 0) {
			currentWinUser_ = i;
		}
	}
	assert(currentWinUser_ != INVALID_CHAIR);
	ListPlayerCards();
	//给机器人发送所有人的牌数据
	{
		texas::CMD_S_AndroidCard reqdata;
		reqdata.set_roundid(strRoundID_);
		reqdata.set_winuser(currentWinUser_);
		texas::HandCards* handcards = reqdata.mutable_tablecards();
		handcards->set_cards(&tableCards_[0], TBL_CARDS);
 		texas::AndroidConfig* cfg = reqdata.mutable_cfg();
		cfg->set_freelookover_(freeLookover_);
		cfg->set_freegiveup_(freeGiveup_);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			texas::AndroidPlayer* player = reqdata.add_players();
			player->set_chairid(i);
			texas::HandCards* handcards = player->mutable_handcards();
			handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
			player->set_userscore(takeScore_[i]);
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (m_pTableFrame->IsAndroidUser(i) == 0) {
				continue;
			}
			string content = reqdata.SerializeAsString();
			m_pTableFrame->SendTableData(i, texas::SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
		}
	}
	//确定庄家
	if (bankerUser_ == INVALID_CHAIR) {
		bankerUser_ = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
	}
	else {
		bankerUser_ = (bankerUser_ + 1) % GAME_PLAYER;
	}
	while (!m_pTableFrame->IsExistUser(bankerUser_) || !bPlaying_[bankerUser_]) {
		bankerUser_ = (bankerUser_ + 1) % GAME_PLAYER;
	}
	//小盲注玩家
	smallBlindUser_ = (bankerUser_ + ((userinfos_.size() == MIN_GAME_PLAYER) ? 0 : 1)) % GAME_PLAYER;
	while (!m_pTableFrame->IsExistUser(smallBlindUser_) || !bPlaying_[smallBlindUser_]) {
		smallBlindUser_ = (smallBlindUser_ + 1) % GAME_PLAYER;
	}
	//小盲注
	updateDeltaAddScore(smallBlindUser_, FloorScore / 2, OP_INVALID);
	tableScore_[smallBlindUser_] += FloorScore / 2;
	tableAllScore_ += FloorScore / 2;
	updateReferenceUser(smallBlindUser_, OP_INVALID);
	updateMainSidePots(smallBlindUser_, tableScore_[smallBlindUser_]);
	//大盲注玩家
	bigBlindUser_ = (smallBlindUser_ + 1) % GAME_PLAYER;
	while (!m_pTableFrame->IsExistUser(bigBlindUser_) || !bPlaying_[bigBlindUser_]) {
		bigBlindUser_ = (bigBlindUser_ + 1) % GAME_PLAYER;
	}
	//大盲注
	updateDeltaAddScore(bigBlindUser_, FloorScore, OP_INVALID);
	tableScore_[bigBlindUser_] += FloorScore;
	tableAllScore_ += FloorScore;
	updateReferenceUser(bigBlindUser_, OP_INVALID);
	updateMainSidePots(bigBlindUser_, tableScore_[bigBlindUser_]);
	//待操作玩家
	currentUser_ = (/*(userinfos_.size() == MIN_GAME_PLAYER) ? smallBlindUser_ : */(bigBlindUser_ + 1)) % GAME_PLAYER;
	while (!m_pTableFrame->IsExistUser(currentUser_) || !bPlaying_[currentUser_]) {
		currentUser_ = (currentUser_ + 1) % GAME_PLAYER;
	}
	std::map<int, int64_t> chips;
	//广播游戏开始消息
	texas::CMD_S_GameStart rspdata;
	rspdata.set_roundid(strRoundID_);
	rspdata.set_cellscore(FloorScore);
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		shared_ptr<CServerUserItem> userItem = ByChairId(i); assert(userItem);
		int64_t userScore = ScoreByChairId(i) - tableScore_[i];
		int64_t takeScore = takeScore_[i] - tableScore_[i];
		if (takeScore <= 0) {
			LOG(ERROR) << __FUNCTION__
				<< " " << i
				<< " " << UserIdBy(i)
				<< " " << " userScore=" << ScoreByChairId(i)
				<< " " << " takeScore=" << takeScore_[i]
				<< " " << " tableScore=" << tableScore_[i]
				<< " " << " leftScore=" << takeScore;
		}
		assert(takeScore > 0);
		assert(userScore > 0);
		addScoreToChips(i, tableScore_[i], chips);
		texas::PlayerItem* player = rspdata.add_players();
		player->set_chairid(i);
		player->set_userid(ByChairId(i)->GetUserId());
		player->set_takescore(takeScore);
		player->set_userscore(userScore);
		player->set_tablescore(tableScore_[i]);
		player->set_playstatus(bPlaying_[i]);
		player->set_viplevel(userItem->GetVip());
		player->set_headid(userItem->GetHeaderId());
		player->set_headboxindex(userItem->GetHeadboxId());
		player->set_headimgurl(userItem->GetHeadImgUrl());
		//对局日志
		//m_replay.addStep(nowsec, to_string(tableScore_[i]), currentTurn_ + 1, opStart, i, i);
	}
	
	firstUser_ = currentUser_;
	rspdata.set_bankeruser(bankerUser_);
	rspdata.set_tableallscore(tableAllScore_);
	for (std::map<int, int64_t>::const_iterator it = chips.begin();
		it != chips.end(); ++it) {
		texas::ChipInfo* chip = rspdata.add_chips();
		chip->set_chip(it->first);
		chip->set_count(it->second);
	}
	for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
		it != showPots_.end(); ++it) {
		//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
		rspdata.add_pots(it->second.score);
	}
	m_bPlayerCanOpt[currentUser_] = true;
	texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
	nextStep->set_nextuser(currentUser_);
	nextStep->set_nextuserid(GetUserId(currentUser_));
	nextStep->set_currentturn(currentTurn_ + 1);
	texas::OptContext* ctx = nextStep->mutable_ctx();
	{
		ctx->set_minaddscore(MinAddScore(currentUser_));
		ctx->set_followscore(CurrentFollowScore(currentUser_));
		ctx->set_canpass(canPassScore(currentUser_));
		ctx->set_canallin(canAllIn(currentUser_));
		ctx->set_canfollow(canFollowScore(currentUser_));
		ctx->set_canadd(canAddScore(currentUser_));
		if (canAddScore(currentUser_)) {
			int64_t deltaScore;
			int minIndex, maxIndex;
			int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
			assert(takeScore >= 0);
			if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
				ctx->add_range(minIndex);
				ctx->add_range(maxIndex);
				for (std::vector<int64_t>::const_iterator it = JettionList.begin();
					it != JettionList.end(); ++it) {
					ctx->add_jettons(*it + deltaScore);
				}
			}
		}
	}
	//操作剩余时间
	uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
	//操作总的时间
	opTotalTime_ = wTimeLeft;
	//操作开始时间
	opStartTime_ = (uint32_t)time(NULL);
	//操作结束时间
	opEndTime_ = opStartTime_ + wTimeLeft + TIME_GAME_START_DELAY;
	//操作剩余时间
	nextStep->set_wtimeleft(wTimeLeft);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		for (int j = 0; j < rspdata.players_size(); ++j) {
			texas::PlayerItem* player = rspdata.mutable_players(j);
			if (player->chairid() == i) {
				texas::HandCards* handcards = player->mutable_handcards();
				handcards->set_cards(&(handCards_[i])[0], 2);
				TEXAS::CGameLogic::AnalyseCards(&(handCards_[i])[0], 2, cover_[i]);
				handcards->set_ty(cover_[i].ty_);
			}
			else {
				player->clear_handcards();
			}
		}
		rspdata.set_needtip(needTip_[i]);
		rspdata.set_setscoretip(setscore_[i]);
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, texas::SUB_S_GAME_START, (uint8_t*)content.data(), content.size());
		needTip_[i] = false;
		setscore_[i] = false;
	}
	//等待操作定时器
	ThisThreadTimer->cancel(timerIdWaiting_);
	timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1 + TIME_GAME_START_DELAY, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
}

//换牌策略分析
//收分时概率性玩家拿小牌，AI拿大牌
//放水时概率性AI拿小牌，玩家拿大牌
void CTableFrameSink::AnalysePlayerCards() {
	if (m_pTableFrame->GetAndroidPlayerCount() == 0) {
		return;
	}
	//库存低于下限值，需要收分
	if (StockScore < StockLowLimit) {
		//概率收分
		int64_t weight = ((double)llabs(StockLowLimit - StockScore))
			/ llabs(StockLowLimit - StockSecondLowLimit)
			* (100 - m_i32LowerChangeRate);
		if (GlobalFunc::RandomInt64(0, 99) > m_i32LowerChangeRate + weight) {
			//LOG(INFO) << "+++++++++++++++++++++算法 随机++++++++++++++++++";
			return;
		}
		//LOG(INFO) << "+++++++++++++++++++++算法 吸分++++++++++++++++++";
		LetSysWin(true);
	}
	//库存高于上限值，需要放水
	else if (StockScore > StockHighLimit) {
		//概率放水
		int64_t weight = ((double)llabs(StockScore - StockHighLimit))
			/ llabs(StockSecondHighLimit - StockHighLimit)
			* (100 - m_i32HigherChangeRate);
		if (GlobalFunc::RandomInt64(0, 99) > m_i32HigherChangeRate + weight) {
			//LOG(INFO) << "+++++++++++++++++++++算法 随机++++++++++++++++++";
			return;
		}
		//LOG(INFO) << "+++++++++++++++++++++算法 吐分++++++++++++++++++";
		LetSysWin(false);
	}
	else {
		//LOG(INFO) << "+++++++++++++++++++++算法 随机++++++++++++++++++";
	}
}

//让系统赢或输
void CTableFrameSink::LetSysWin(bool sysWin) {
	if (m_pTableFrame->GetAndroidPlayerCount() == 0 || m_pTableFrame->GetRealPlayerCount() == 0) {
		return;
	}
	std::vector<TEXAS::CGameLogic::handinfo_t const*> v;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		all_[i].chairID = i;
		v.push_back(&all_[i]);
	}
	std::sort(&v.front(), &v.front() + v.size(), [](TEXAS::CGameLogic::handinfo_t const* src, TEXAS::CGameLogic::handinfo_t const* dst) -> bool {
		int bv = TEXAS::CGameLogic::CompareCards(*src, *dst);
		return bv > 0;
		});
	int c = 0;
	//换牌前按牌大小座位排次
	int info[GAME_PLAYER] = { 0 };
	//换牌后按牌大小座位排次(1.系统赢：AI排前面，真实玩家排后面 2.系统输：真实玩家排前面，AI排后面)
	std::vector<int> vinfo;
	for (std::vector<TEXAS::CGameLogic::handinfo_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		info[c++] = (*it)->chairID;
		if (sysWin) {
			//如果是机器人
			if (m_pTableFrame->IsAndroidUser((uint32_t)(*it)->chairID) > 0) {
				vinfo.push_back((*it)->chairID);
				//LOG(INFO) << " 吸分 " << (*it)->chairID << " 机器人";
			}
		}
		else {
			//如果是真实玩家
			if (m_pTableFrame->IsAndroidUser((uint32_t)(*it)->chairID) == 0) {
				vinfo.push_back((*it)->chairID);
				//LOG(INFO) << " 吐分 " << (*it)->chairID << " 玩家";
			}
		}
	}
	for (std::vector<TEXAS::CGameLogic::handinfo_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		if (sysWin) {
			//如果是真实玩家
			if (m_pTableFrame->IsAndroidUser((uint32_t)(*it)->chairID) == 0) {
				vinfo.push_back((*it)->chairID);
				//LOG(INFO) << " 吸分 " << (*it)->chairID << " 玩家";
			}
		}
		else {
			//如果是机器人
			if (m_pTableFrame->IsAndroidUser((uint32_t)(*it)->chairID) > 0) {
				vinfo.push_back((*it)->chairID);
				//LOG(INFO) << " 吐分 " << (*it)->chairID << " 机器人";
			}
		}
	}
	assert(c == vinfo.size());
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌
		if (info[i] != vinfo[i]) {
			//同是机器人或真实玩家，保持原来位置不变
			if ((
				m_pTableFrame->IsAndroidUser((uint32_t)info[i]) > 0 &&
				m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) > 0) ||
				(m_pTableFrame->IsAndroidUser((uint32_t)info[i]) == 0 &&
					m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) == 0)) {
				int j = 0;
				for (; j < vinfo.size(); ++j) {
					//找出info[i]在vinfo中的位置
					if (vinfo[j] == info[i]) {
						break;
					}
				}
				//LOG(INFO) << " 恢复 " << vinfo[i] << " -> " << vinfo[j];
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
			m_pTableFrame->IsAndroidUser((uint32_t)info[i]) > 0 ? "机器人" : "玩家");
		strtmp += msg;
	}
	strmsg += std::string("\n\n--- *** 换牌前按牌大小座位排次\n") + strtmp;
	for (int i = 0; i < vinfo.size(); ++i) {
		snprintf(msg, sizeof(msg), "[%d] chairID = %d %s\n",
			i, vinfo[i],
			m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家");
		strtmp2 += msg;
	}
	strmsg += std::string("--- *** 换牌后按牌大小座位排次\n") + strtmp2;
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌，并且换牌后是机器人则换牌
		if (info[i] != vinfo[i]) {
			//系统赢，玩家牌换成机器人牌
			//系统输，机器人牌换成玩家牌
			if ((sysWin && (m_pTableFrame->IsAndroidUser((uint32_t)info[i]) == 0 && m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) > 0)) ||
				(!sysWin && (m_pTableFrame->IsAndroidUser((uint32_t)info[i]) > 0 && m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) == 0))) {
				
				//交换手牌信息
				{
					TEXAS::CGameLogic::handinfo_t t(all_[info[i]]);
					all_[info[i]] = all_[vinfo[i]];
					all_[vinfo[i]] = t;
				}
				//交换手牌
				{
					uint8_t handcards[MAX_COUNT];
					memcpy(handcards, &(handCards_[info[i]])[0], MAX_COUNT);
					snprintf(msg, sizeof(msg), "--- *** 换牌前，%s %d 手牌 [%s]\n", (m_pTableFrame->IsAndroidUser((uint32_t)info[i]) > 0 ? "机器人" : "玩家"), info[i], TEXAS::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
					strmsg += msg;
					memcpy(&(handCards_[info[i]])[0], &(handCards_[vinfo[i]])[0], MAX_COUNT);
					snprintf(msg, sizeof(msg), "--- *** 换牌后，%s %d 手牌 [%s]\n", (m_pTableFrame->IsAndroidUser((uint32_t)info[i]) > 0 ? "机器人" : "玩家"), info[i], TEXAS::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
					strmsg += msg;
					snprintf(msg, sizeof(msg), "--- *** 换牌前，%s %d 手牌 [%s]\n", (m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家"), vinfo[i], TEXAS::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
					strmsg += msg;
					memcpy(&(handCards_[vinfo[i]])[0], handcards, MAX_COUNT);
					snprintf(msg, sizeof(msg), "--- *** 换牌后，%s %d 手牌 [%s]\n", (m_pTableFrame->IsAndroidUser((uint32_t)vinfo[i]) > 0 ? "机器人" : "玩家"), vinfo[i], TEXAS::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
					strmsg += msg;
				}
				{
					uint8_t combcards[MAX_TOTAL];
					memcpy(combcards, &(combCards_[info[i]])[0], MAX_TOTAL);
					memcpy(&(combCards_[info[i]])[0], &(combCards_[vinfo[i]])[0], MAX_TOTAL);
					memcpy(&(combCards_[vinfo[i]])[0], combcards, MAX_TOTAL);
				}
			}
		}
	}
	if (writeRealLog_)
		LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] " << strmsg;
}

//输出玩家手牌/对局日志初始化
void CTableFrameSink::ListPlayerCards() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		//对局日志
		//m_replay.addPlayer(UserIdBy(i), ByChairId(i)->GetNickName(), takeScore_[i], i);
		//机器人AI
		if (m_pTableFrame->IsAndroidUser((uint32_t)i) > 0) {
			if (writeRealLog_) {
				LOG(INFO) << "\n========================================================================\n"
					<< "--- *** [" << strRoundID_ << "] 机器人 [" << i << "] " << UserIdBy(i) << " 手牌 ["
					<< TEXAS::CGameLogic::StringCards(&(combCards_[i])[0], MAX_TOTAL)
					<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[i].ty_)) << "] takeScore_[" << takeScore_[i] << "]\n";
			}
		}
		else {
			if (writeRealLog_) {
				LOG(INFO) << "\n========================================================================\n"
					<< "--- *** [" << strRoundID_ << "] 玩家 [" << i << "] " << UserIdBy(i) << " 手牌 ["
					<< TEXAS::CGameLogic::StringCards(&(combCards_[i])[0], MAX_TOTAL)
					<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[i].ty_)) << "] takeScore_[" << takeScore_[i] << "]\n";
			}
		}
	}
	assert(currentWinUser_ != INVALID_CHAIR);
	if (m_pTableFrame->IsAndroidUser(currentWinUser_) > 0) {
		if (writeRealLog_) {
			LOG(INFO) << "\n--- *** [" << strRoundID_ << "] 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 手牌 ["
				<< TEXAS::CGameLogic::StringCards(&(combCards_[currentWinUser_])[0], MAX_TOTAL)
				<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[currentWinUser_].ty_)) << "] 最大牌\n\n\n";
		}
	}
	else {
		if (writeRealLog_) {
			LOG(INFO) << "\n--- *** [" << strRoundID_ << "] 玩家 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 手牌 ["
				<< TEXAS::CGameLogic::StringCards(&(combCards_[currentWinUser_])[0], MAX_TOTAL)
				<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[currentWinUser_].ty_)) << "] 最大牌\n\n\n";
		}
	}
}

//拼接各玩家手牌cardValue
std::string CTableFrameSink::StringCardValue(bool flag) {
	char ch[20] = { 0 };
	std::string str;
	for (UserInfoList::iterator it = userinfos_.begin();
		it != userinfos_.end(); ++it) {
		int i = it->second.chairId;
		memcpy(&(combCards_[i])[0], &(handCards_[i])[0], MAX_COUNT);
		memcpy(&(combCards_[i])[MAX_COUNT], &tableCards_[0], cardsC_[i]);
		string cards, ty;
		for (int j = 0; j < MAX_COUNT + cardsC_[i]; ++j) {
			sprintf(ch, "%02X", combCards_[i][j]);
			cards += ch;
		}
		ty = std::to_string(cover_[i].ty_);
		if (str.empty()) {
			str += std::to_string(i) + std::to_string(/*MAX_COUNT + */cardsC_[i]) + cards + ty + std::to_string(isGiveup_[i]);
		}
		else {
			str += "|" + std::to_string(i) + std::to_string(/*MAX_COUNT + */cardsC_[i]) + cards + ty + std::to_string(isGiveup_[i]);
		}
	}
	for (std::vector<uint32_t>::const_iterator it = winUsers_.begin();
		it != winUsers_.end(); ++it) {
		sprintf(ch, ",%d", *it);
		str += ch;
	}
	return str;
}

//判断当轮操作
bool CTableFrameSink::hasOperate(int k, eOperate opcode) {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (isGiveup_[i] || isLost_[i]) {
			continue;
		}
		if (operate_[k][i] == opcode) {
			return true;
		}
	}
	return false;
}

//判断是否梭哈
bool CTableFrameSink::hasAllIn() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (isGiveup_[i] || isLost_[i]) {
			continue;
		}
		if (hasAllIn(i)) {
			return true;
		}
	}
	return false;
}

//判断是否梭哈
bool CTableFrameSink::hasAllIn(uint32_t chairId) {
	if (allInTurn_[chairId] >= 0) {
		assert(allInTurn_[chairId] < MAX_ROUND);
		assert(operate_[allInTurn_[chairId]][chairId] == OP_ALLIN);
		assert(addScore_[allInTurn_[chairId]][chairId] > 0);
		return true;
	}
	return false;
}

//能否过牌操作
bool CTableFrameSink::canPassScore(uint32_t chairId) {
	int64_t followScore = CurrentFollowScore(chairId);
	return followScore == 0;
}

//能否跟注操作
bool CTableFrameSink::canFollowScore(uint32_t chairId) {
	int64_t followScore = CurrentFollowScore(chairId);
	return followScore > 0 && (followScore + tableScore_[chairId]) < takeScore_[chairId];
}

//能否加注操作
bool CTableFrameSink::canAddScore(uint32_t chairId) {
#if 0
	if (hasOperate(CurrentTurn(), OP_ALLIN)) {
		return false;
	}
#else
	if (canOptPlayerCount() < 2) {
		return false;
	}
#endif
	//assert(MinAddScore(chairId) >= FloorScore / 2);
	return MinAddScore(chairId) + tableScore_[chairId] < takeScore_[chairId];
}

//能否梭哈操作
bool CTableFrameSink::canAllIn(uint32_t chairId) {
	int64_t takeScore = takeScore_[chairId] - tableScore_[chairId];
	return takeScore > 0/* && !canFollowScore(chairId) && !canAddScore(chairId)*/;
}

//可操作玩家数
int CTableFrameSink::canOptPlayerCount() {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (isGiveup_[i] || isLost_[i]) {
			continue;
		}
		//if (operate_[CurrentTurn()][i] == OP_ALLIN) {
		if (hasAllIn(i)) {
			continue;
		}
		++c;
	}
	return c;
}

//剩余游戏人数
int CTableFrameSink::leftPlayerCount(bool includeAndroid, uint32_t* currentUser) {
	int c = 0;
	for (int32_t i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (isGiveup_[i] || isLost_[i]) {
			continue;
		}
		if (!includeAndroid && m_pTableFrame->IsAndroidUser((uint32_t)i) > 0) {
			continue;
		}
		if (currentUser) {
			*currentUser = i;
		}
		++c;
	}
	return c;
}

//判断当轮是否完成
bool CTableFrameSink::checkFinished(int k) {
	if (k < MAX_ROUND) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			if (isGiveup_[i] || isLost_[i]) {
				continue;
			}
			//if (operate_[k][i] == OP_ALLIN) {
			if (hasAllIn(i)) {
				continue;
			}
			if (operate_[k][i] == OP_INVALID) {
				return false;
			}
			if (CurrentFollowScore(i) != 0) {
				return false;
			}
		}
	}
	return true;
}

//下一个操作用户
uint32_t CTableFrameSink::GetNextUser(bool& newflag, bool& exceed) {
	newflag = false;
	exceed = false;
	assert(currentUser_ != INVALID_CHAIR);
	for (int i = 1; i < GAME_PLAYER; ++i) {
		uint32_t nextUser = (currentUser_ + i) % GAME_PLAYER;
		if (!bPlaying_[nextUser]) {
			continue;
		}
		if (nextUser == currentUser_) {
			continue;
		}
		if (isGiveup_[nextUser] || isLost_[nextUser]) {
			continue;
		}
		//if (operate_[CurrentTurn()][nextUser] == OP_ALLIN) {
		if (hasAllIn(nextUser)) {
			continue;
		}
		if (checkFinished(CurrentTurn())) {
			referenceUser_ = INVALID_CHAIR;
			newflag = true;
			++currentTurn_;
			if (currentTurn_ >= MAX_ROUND) {
				exceed = true;
			}
		}
		//return (canOptPlayerCount() > 1) ? nextUser : INVALID_CHAIR;
		return nextUser;
	}
	if (checkFinished(CurrentTurn())) {
		referenceUser_ = INVALID_CHAIR;
		newflag = true;
		++currentTurn_;
		if (currentTurn_ >= MAX_ROUND) {
			exceed = true;
		}
	}
	return INVALID_CHAIR;
}

//下一个操作用户，从小盲注开始查找
uint32_t CTableFrameSink::GetNextUserBySmallBlindUser() {
	//assert(!isExceed_ && isNewTurn_);
	assert(smallBlindUser_ != INVALID_CHAIR);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		uint32_t nextUser = (smallBlindUser_ + i) % GAME_PLAYER;
		if (!bPlaying_[nextUser]) {
			continue;
		}
		if (isGiveup_[nextUser] || isLost_[nextUser]) {
			continue;
		}
		//if (operate_[CurrentTurn()][nextUser] == OP_ALLIN) {
		if (hasAllIn(nextUser)) {
			continue;
		}
		return (canOptPlayerCount() > 1) ? nextUser : INVALID_CHAIR;
	}
	return INVALID_CHAIR;
}

//最近一轮加注分
int64_t CTableFrameSink::GetLastAddScore(uint32_t chairId) {
	return addScore_[LastTurn()][chairId];
}

//最近一轮
int CTableFrameSink::LastTurn() {
	int n = isNewTurn_ ? (CurrentTurn() == 0 ? 0 : (CurrentTurn() - 1)) : CurrentTurn();
	if (n < 0) {
		n = 0;
	}
	else if (n >= MAX_ROUND) {
		n = MAX_ROUND - 1;
	}
	//assert(n >= 0 && n < MAX_ROUND);
	return n;
}

//有效当前轮
int CTableFrameSink::CurrentTurn() {
#if 0
	assert(currentTurn_ >= 0);
	assert(currentTurn_ <= MAX_ROUND);
	int n = isExceed_ ? (currentTurn_ - 1) : currentTurn_;
	assert(n >= 0 && n < MAX_ROUND);
	return n;
#else
	return (currentTurn_ >= MAX_ROUND) ? (MAX_ROUND - 1) : currentTurn_;
#endif
}

//筹码表可加注筹码范围[minIndex, maxIndex]
bool CTableFrameSink::GetCurrentAddRange(int64_t minAddScore, int64_t userScore, int& minIndex, int& maxIndex, int64_t& deltaScore) {
	minIndex = maxIndex = -1;
	deltaScore = 0;
	assert(minAddScore > 0);
restart:
	for (int i = 0; i < JettionList.size(); ++i) {
		if (minAddScore <= JettionList[i] + deltaScore) {
			minIndex = i;
			break;
		}
	}
	if (minIndex >= 0/* && minIndex < JettionList.size()*/) {
		assert(JettionList[minIndex] + deltaScore >= minAddScore);
		for (int i = minIndex + 1; i < JettionList.size(); ++i) {
			assert(JettionList[i - 1] < JettionList[i]);
		}
		for (int i = JettionList.size() - 1; i >= minIndex; --i) {
			if (userScore > JettionList[i] + deltaScore) {
				maxIndex = i;
				break;
			}
		}
		if (minIndex >= 0 && maxIndex >= minIndex) {
			return true;
		}
	}
	if (minIndex >= 0 && maxIndex == -1) {
		minIndex = -1;
	}
	else {
		deltaScore += JettionList[JettionList.size() - 1];
		goto restart;
	}
	return false;
}

//最小加注分
int64_t CTableFrameSink::MinAddScore(uint32_t chairId) {
	assert(deltaAddScore_ > 0);
// 	LOG(INFO) << __FUNCTION__ << "\n"
// 		<< " referenceUser_=" << referenceUser_
// 		<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 		<< " chairId=" << chairId
// 		<< " tableScore=" << tableScore_[chairId] << "\n"
// 		<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 		<< " deltaAddScore_=" << deltaAddScore_
//		<< " MinAddScore=" << CurrentFollowScore(chairId) + deltaAddScore_;
	return CurrentFollowScore(chairId) + deltaAddScore_;
}

//当前跟注分
int64_t CTableFrameSink::CurrentFollowScore(uint32_t chairId) {
	if (referenceUser_ != INVALID_CHAIR) {
// 		LOG(INFO) << __FUNCTION__ << "\n"
// 			<< " referenceUser_=" << referenceUser_
// 			<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 			<< " chairId=" << chairId
//			<< " tableScore=" << tableScore_[chairId];
		int64_t followScore = tableScore_[referenceUser_] - tableScore_[chairId];
		assert(followScore >= 0);
		return followScore;
	}
	return 0;
}

//跟注参照用户
void CTableFrameSink::updateReferenceUser(uint32_t chairId, eOperate op) {
	switch (op) {
	case OP_INVALID:
		if (referenceUser_ == INVALID_CHAIR) {
			if (tableScore_[chairId] > 0) {
				referenceUser_ = chairId;
// 				LOG(INFO) << __FUNCTION__ << "\n"
// 					<< " referenceUser_=" << referenceUser_
// 					<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 					<< " chairId=" << chairId
// 					<< " tableScore=" << tableScore_[chairId] << "\n"
// 					//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 					//<< " MinAddScore=" << MinAddScore(chairId)
//					<< " op=" << "OP_INVALID";
			}
		}
		else if (tableScore_[chairId] > tableScore_[referenceUser_]) {
			referenceUser_ = chairId;
// 			LOG(INFO) << __FUNCTION__ << "\n"
// 				<< " referenceUser_=" << referenceUser_
// 				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 				<< " chairId=" << chairId
// 				<< " tableScore=" << tableScore_[chairId] << "\n"
// 				//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 				//<< " MinAddScore=" << MinAddScore(chairId)
//				<< " op=" << "OP_INVALID";
		}
		break;
	case OP_ADD:
		if (referenceUser_ == INVALID_CHAIR) {
			referenceUser_ = chairId;
// 			LOG(INFO) << __FUNCTION__ << "\n"
// 				<< " referenceUser_=" << referenceUser_
// 				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 				<< " chairId=" << chairId
// 				<< " tableScore=" << tableScore_[chairId] << "\n"
// 				//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 				//<< " MinAddScore=" << MinAddScore(chairId)
// 				<< " op=" << "OP_ADD;
		}
		else if (tableScore_[chairId] > tableScore_[referenceUser_]) {
			referenceUser_ = chairId;
//			LOG(INFO) << __FUNCTION__ << "\n"
// 				<< " referenceUser_=" << referenceUser_
// 				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 				<< " chairId=" << chairId
// 				<< " tableScore=" << tableScore_[chairId] << "\n"
// 				//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 				//<< " MinAddScore=" << MinAddScore(chairId)
//				<< " op=" << "OP_ADD";
		}
		break;
	case OP_ALLIN:
		if (referenceUser_ == INVALID_CHAIR) {
			referenceUser_ = chairId;
// 			LOG(INFO) << __FUNCTION__ << "\n"
// 				<< " referenceUser_=" << referenceUser_
// 				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 				<< " chairId=" << chairId
// 				<< " tableScore=" << tableScore_[chairId] << "\n"
// 				//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 				//<< " MinAddScore=" << MinAddScore(chairId)
// 				<< " op=" << "OP_ALLIN";
		}
		else if (tableScore_[chairId] > tableScore_[referenceUser_]) {
			referenceUser_ = chairId;
// 			LOG(INFO) << __FUNCTION__ << "\n"
// 				<< " referenceUser_=" << referenceUser_
// 				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 				<< " chairId=" << chairId
// 				<< " tableScore=" << tableScore_[chairId] << "\n"
// 				//<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 				//<< " MinAddScore=" << MinAddScore(chairId)
//				<< " op=" << "OP_ALLIN";
		}
		break;
	}
}

//加注递增量
void CTableFrameSink::updateDeltaAddScore(uint32_t chairId, int64_t addScore, eOperate op) {
	switch (op) {
	case OP_INVALID: {
		if (addScore > 0) {
//			LOG(INFO) << __FUNCTION__ << "\n"
//				<< " referenceUser_=" << referenceUser_
//				<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
//				<< " chairId=" << chairId
//				<< " tableScore=" << tableScore_[chairId] << "\n"
//				<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
//				//<< " MinAddScore=" << MinAddScore(chairId)
//				<< " addScore=" << addScore
//				<< " op=" << "OP_INVALID";
			//assert(addScore > CurrentFollowScore(chairId));
			//assert(chairId != referenceUser_);
			int64_t delta = addScore - CurrentFollowScore(chairId);
			if (delta > deltaAddScore_) {
				deltaAddScore_ = delta;
			}
		}
		break;
	}
	case OP_ADD: {
// 		LOG(INFO) << __FUNCTION__ << "\n"
// 			<< " referenceUser_=" << referenceUser_
// 			<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 			<< " chairId=" << chairId
// 			<< " tableScore=" << tableScore_[chairId] << "\n"
// 			<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 			//<< " MinAddScore=" << MinAddScore(chairId)
// 			<< " addScore=" << addScore
// 			<< " op=" << "OP_ADD";
		assert(addScore > 0);
		assert(addScore >= MinAddScore(chairId));
		assert(MinAddScore(chairId) > CurrentFollowScore(chairId));
		//assert(chairId != referenceUser_);
		int64_t delta = addScore - CurrentFollowScore(chairId);
		if (delta > deltaAddScore_) {
			deltaAddScore_ = delta;
		}
		break;
	}
	case OP_ALLIN: {
// 		LOG(INFO) << __FUNCTION__ << "\n"
// 			<< " referenceUser_=" << referenceUser_
// 			<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 			<< " chairId=" << chairId
// 			<< " tableScore=" << tableScore_[chairId] << "\n"
// 			<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 			//<< " MinAddScore=" << MinAddScore(chairId)
// 			<< " addScore=" << addScore
// 			<< " op=" << "OP_ALLIN";
		assert(addScore > 0);
		//assert(addScore >= MinAddScore(chairId));
		//assert(MinAddScore(chairId) > CurrentFollowScore(chairId));
		//assert(chairId != referenceUser_);
		int64_t delta = addScore - CurrentFollowScore(chairId);
		if (delta > deltaAddScore_) {
			deltaAddScore_ = delta;
		}
		break;
	}
	}
}

//计算主(底)池，边池
void CTableFrameSink::updateMainSidePots(uint32_t chairId, int64_t addScore) {
	if (isNewTurn_) {
		updateMainSidePots(pots_);
		updateMainSidePots(potsT_);
		//标准pots_
		showPots_.clear();
		for (std::map<int, pot_t>::const_iterator it = pots_.begin();
			it != pots_.end(); ++it) {
			pot_t& pot = showPots_[it->first];
			pot.score += it->second.score;
		}
	}
	else {
		updateMainSidePots(pots_);
		if (chairId != INVALID_CHAIR && addScore > 0) {
			//添加到主池
			pot_t& pot = potsT_[0];
			pot.score += addScore;
		}
		//展示potsT_
		showPots_.clear();
		for (std::map<int, pot_t>::const_iterator it = potsT_.begin();
			it != potsT_.end(); ++it) {
			pot_t& pot = showPots_[it->first];
			pot.score += it->second.score;
		}
	}
}

//计算主(底)池，边池
void CTableFrameSink::updateMainSidePots(std::map<int, pot_t>& pots) {
	char msg[1024] = { 0 };
	//测试用
	{
		std::vector<bet_t> v;
		int64_t giveupScore = 0;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			if (tableScore_[i] > 0) {
				v.emplace_back(bet_t(i, tableScore_[i]));
			}
		}
		std::sort(&v.front(), &v.front() + v.size(), [](bet_t const& src, bet_t const& dst) -> bool {
			return src.score < dst.score;
			});
		std::string ss;
		{
			int64_t cursor;
			for (std::vector<bet_t>::const_iterator it = v.begin();
				it != v.end(); ++it) {
				if (it == v.begin()) {
					cursor = it->score;
				}
				else {
					assert(cursor <= it->score);
					cursor = it->score;
				}
				if(isGiveup_[it->id]) {
					assert(!hasAllIn(it->id));
					if (ss.empty()) {
						snprintf(msg, sizeof(msg), "[%d]%lld|G", it->id, it->score);
						ss += msg;
					}
					else {
						snprintf(msg, sizeof(msg), " [%d]%lld|G", it->id, it->score);
						ss += msg;
					}
				}
				else if (hasAllIn(it->id)) {
					assert(!isGiveup_[it->id]);
					if (ss.empty()) {
						snprintf(msg, sizeof(msg), "[%d]%lld|AIN", it->id, it->score);
						ss += msg;
					}
					else {
						snprintf(msg, sizeof(msg), " [%d]%lld|AIN", it->id, it->score);
						ss += msg;
					}
				}
				else {
					if (ss.empty()) {
						snprintf(msg, sizeof(msg), "[%d]%lld", it->id, it->score);
						ss += msg;
					}
					else {
						snprintf(msg, sizeof(msg), " [%d]%lld", it->id, it->score);
						ss += msg;
					}
				}
			}
			//LOG(INFO) << __FUNCTION__ << " ---*** 当前各玩家下注筹码如下:\n" << ss;
		}
	}
	//主逻辑
	std::vector<bet_t> v;
	int64_t giveupScore = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (tableScore_[i] > 0) {
			if (!isGiveup_[i]) {
				v.emplace_back(bet_t(i, tableScore_[i]));
			}
			else {
				giveupScore += tableScore_[i];
			}
		}
	}
	std::sort(&v.front(), &v.front() + v.size(), [](bet_t const& src, bet_t const& dst) -> bool {
		return src.score < dst.score;
		});
	{
		int64_t cursor;
		for (std::vector<bet_t>::const_iterator it = v.begin();
			it != v.end(); ++it) {
			if (it == v.begin()) {
				cursor = it->score;
			}
			else {
				assert(cursor <= it->score);
				cursor = it->score;
			}
		}
	}
	std::string ss;
	pots.clear();
	uint32_t id[2];
	int64_t pivot[2];
	int c = 0;
	for (size_t i = 0; i < v.size(); ++i) {
		if (c == 0) {
			//主(底)池
			pot_t& pot = pots[c];
			//最小已投筹码
			id[0] = v[i].id;
			pivot[0] = v[i].score;
			//参与底池人数
			std::string s;
			for (size_t k = i; k < v.size(); ++k) {
				pot.ids.insert(v[k].id);
				s += "[" + std::to_string(v[k].id) + "]";
			}
			//主(底)池 = 最小已投筹码 * 参与底池人数 + 弃牌筹码
			pot.score = pivot[0] * pot.ids.size() + giveupScore;
			if (giveupScore > 0) {
				snprintf(msg, sizeof(msg), "---*** 主池%d = [%d]%lld * %d + %lld = %lld 组成:%s\n", c, id[0], pivot[0], pot.ids.size(), giveupScore, pot.score, s.c_str());
				ss += msg;
			}
			else {
				snprintf(msg, sizeof(msg), "---*** 主池%d = [%d]%lld * %d = %lld 组成:%s\n", c, id[0], pivot[0], pot.ids.size(), pot.score, s.c_str());
				ss += msg;
			}
			++c;
		}
		else {
			assert(v[i].score >= pivot[0]);
			if (v[i].score > pivot[0]) {
				//边池
				pot_t& pot = pots[c];
				//次小已投筹码
				id[1] = v[i].id;
				pivot[1] = v[i].score;
				//参与底池人数
				std::string s;
				for (size_t k = i; k < v.size(); ++k) {
					pot.ids.insert(v[k].id);
					s += "[" + std::to_string(v[k].id) + "]";
				}
				//边池 = (次小已投筹码 - 最小已投筹码) * 参与底池人数
				pot.score = (pivot[1] - pivot[0]) * pot.ids.size();
				snprintf(msg, sizeof(msg), "---*** 边池%d = ([%d]%lld - [%d]%lld) * %d = %lld 组成:%s\n", c, id[1], pivot[1], id[0], pivot[0], pot.ids.size(), pot.score, s.c_str());
				ss += msg;
				id[0] = id[1];
				pivot[0] = pivot[1];
				++c;
			}
		}
	}
	//LOG(INFO) << __FUNCTION__ << "\n" << ss;
}

//积分转换成筹码
void CTableFrameSink::addScoreToChips(uint32_t chairId, int64_t score, std::map<int, int64_t>& chips) {
	//
	// 	1,2,5,
	// 	10,20,50,
	// 	100,200,500,
	// 	1000,2000,5000,
	// 	10000,20000,50000,
	// 	100000,200000,500000,
	// 	1000000,2000000,5000000,
	// 	10000000,20000000,50000000,
	// 	100000000,200000000,500000000
	//
	// 	1,5,
	// 	10,25,50,
	// 	100,500,
	// 	1000,5000,
	// 	10000,50000
	//
	//assert(score > 0);
	if (score <= 0) {
		return;
	}
	int64_t leftScore = score / 100;
	for (std::vector<int>::const_reverse_iterator it = chipList_.rbegin();
		it != chipList_.rend(); ++it) {
		int chip = *it;
		int64_t count = leftScore / chip;
		if (count > 0) {
			leftScore -= count * chip;
			{
				std::map<int, int64_t>::iterator it = chips.find(chip);
				if (it == chips.end())
					chips[chip] = count;
				else
					it->second += count;
			}
			{
				std::map<int, int64_t>::iterator it = tableChip_[chairId].find(chip);
				if (it == tableChip_[chairId].end())
					tableChip_[chairId][chip] = count;
				else
					it->second += count;
			}
			{
				std::map<int, int64_t>::iterator it = tableAllChip_.find(chip);
				if (it == tableAllChip_.end())
					tableAllChip_[chip] = count;
				else
					it->second += count;
			}
			{
				std::map<int, int64_t>::iterator it = settleChip_.find(chip);
				if (it == settleChip_.end())
					settleChip_[chip] = count;
				else
					it->second += count;
			}
		}
		assert(leftScore >= 0);
		if (leftScore == 0) {
			break;
		}
	}
	if (leftScore > 0) {
		int chip = 1;
		int64_t count = leftScore;
		{
			std::map<int, int64_t>::iterator it = chips.find(chip);
			if (it == chips.end())
				chips[chip] = count;
			else
				it->second += count;
		}
		{
			std::map<int, int64_t>::iterator it = tableChip_[chairId].find(chip);
			if (it == tableChip_[chairId].end())
				tableChip_[chairId][chip] = count;
			else
				it->second += count;
		}
		{
			std::map<int, int64_t>::iterator it = tableAllChip_.find(chip);
			if (it == tableAllChip_.end())
				tableAllChip_[chip] = count;
			else
				it->second += count;
		}
		{
			std::map<int, int64_t>::iterator it = settleChip_.find(chip);
			if (it == settleChip_.end())
				settleChip_[chip] = count;
			else
				it->second += count;
		}
	}
}

//结算筹码
void CTableFrameSink::settleChips(int64_t score, std::map<int, int64_t>& chips) {
	assert(score > 0);
	int64_t leftScore = score / 100;
	for (std::map<int, int64_t>::reverse_iterator it = settleChip_.rbegin();
		it != settleChip_.rend(); ++it) {
		int chip = it->first;
		int64_t total = it->second;
		int64_t count = leftScore / chip;
		if (count > 0 && total > 0) {
			count = std::min(count, total);
			it->second -= count;
			leftScore -= count * chip;
			{
				std::map<int, int64_t>::iterator it = chips.find(chip);
				if (it == chips.end())
					chips[chip] = count;
				else
					it->second += count;
			}
			//LOG(INFO) << __FUNCTION__ << " chip=" << chip << " total=" << total << " count=" << count << " left=" << it->second;
		}
		assert(leftScore >= 0);
		if (leftScore == 0) {
			break;
		}
	}
}

//等待操作定时器
void CTableFrameSink::OnTimerWaitingOver() {
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1);
	//LOG(INFO) << __FUNCTION__ << msg;
	ThisThreadTimer->cancel(timerIdWaiting_);
	if (currentUser_ == INVALID_CHAIR) {
		return;
	}
	if (!bPlaying_[currentUser_]) {
		return;
	}
	if (isGiveup_[currentUser_]) {
		return;
	}
	if (canPassScore(currentUser_)) {
		OnUserPassScore(currentUser_);
	}
	else {
		OnUserGiveUp(currentUser_, true);
	}
}

//操作错误通知消息
void CTableFrameSink::SendNotify(uint32_t chairId, int opcode, std::string const& errmsg) {
	texas::CMD_S_Operate_Notify  notify;
	notify.set_opcode(opcode);
	notify.set_errmsg(errmsg);
	string data = notify.SerializeAsString();
	m_pTableFrame->SendTableData(chairId, texas::SUB_S_OPERATE_NOTIFY, (uint8_t*)data.data(), data.size());
}

//新一轮开始发牌
void CTableFrameSink::SendCard(int left) {
	//char msg[1024];
	//assert(isNewTurn_);
	if (left > 0) {
		texas::CMD_S_SendCard rspdata;
		rspdata.set_cards(&tableCards_[offset_++], 1);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			if (isGiveup_[i] || isLost_[i]) {
				continue;
			}
			++cardsC_[i];
		}
		//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] turn=[%d] left=[%d]",
		//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, left);
		//LOG(INFO) << __FUNCTION__ << msg;
		//当轮牌已发完
		if (--left == 0) {
			for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
				it != showPots_.end(); ++it) {
				//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
				rspdata.add_pots(it->second.score);
			}
			//没有超出轮数，且没有人梭哈操作
			if (!isExceed_ && (nextUser_ = GetNextUserBySmallBlindUser()) != INVALID_CHAIR) {
				assert(nextUser_ != INVALID_CHAIR);
				assert(!isGiveup_[nextUser_]);
				if (userinfos_.size() == MIN_GAME_PLAYER) {
					nextUser_ = (nextUser_ + 1) % GAME_PLAYER;
					while (!m_pTableFrame->IsExistUser(nextUser_) || !bPlaying_[nextUser_]) {
						nextUser_ = (nextUser_ + 1) % GAME_PLAYER;
					}
					assert(nextUser_ != INVALID_CHAIR);
					assert(!isGiveup_[nextUser_]);
				}
				currentUser_ = nextUser_;
				firstUser_ = currentUser_;
				m_bPlayerCanOpt[currentUser_] = true;
				texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
				nextStep->set_nextuser(currentUser_);
				nextStep->set_nextuserid(GetUserId(currentUser_));
				nextStep->set_currentturn(currentTurn_ + 1);
				texas::OptContext* ctx = nextStep->mutable_ctx();
				{
					ctx->set_minaddscore(MinAddScore(currentUser_));
					ctx->set_followscore(CurrentFollowScore(currentUser_));
					ctx->set_canpass(canPassScore(currentUser_));
					ctx->set_canallin(canAllIn(currentUser_));
					ctx->set_canfollow(canFollowScore(currentUser_));
					ctx->set_canadd(canAddScore(currentUser_));
					if (canAddScore(currentUser_)) {
						int64_t deltaScore;
						int minIndex, maxIndex;
						int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
						assert(takeScore >= 0);
						if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
							ctx->add_range(minIndex);
							ctx->add_range(maxIndex);
							for (std::vector<int64_t>::const_iterator it = JettionList.begin();
								it != JettionList.end(); ++it) {
								ctx->add_jettons(*it + deltaScore);
							}
						}
					}
				}
				//操作剩余时间
				uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
				//操作总的时间
				opTotalTime_ = wTimeLeft;
				//操作开始时间
				opStartTime_ = (uint32_t)time(NULL);
				//操作结束时间
				opEndTime_ = opStartTime_ + wTimeLeft;
				nextStep->set_wtimeleft(wTimeLeft);
				//开启等待定时器
				timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
			}
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i))
				continue;
			rspdata.clear_ty();
			if (bPlaying_[i] && !isGiveup_[i] && !isLost_[i] && offset_ >= 3) {
				memcpy(&(combCards_[i])[0], &(handCards_[i])[0], MAX_COUNT);
				memcpy(&(combCards_[i])[MAX_COUNT], &tableCards_[0], offset_);
				TEXAS::CGameLogic::AnalyseCards(&(combCards_[i])[0], MAX_COUNT + offset_, cover_[i]);
				rspdata.set_ty(cover_[i].ty_);
			}
			string content = rspdata.SerializeAsString();
			m_pTableFrame->SendTableData(i, texas::SUB_S_SEND_CARD, (uint8_t*)content.data(), content.size());
		}
		if (left > 0) {
			timerIdSendCard_ = ThisThreadTimer->runAfter(0.2f, boost::bind(&CTableFrameSink::SendCardOnTimer, this, left));
		}
	}
	if (left == 0) {
		//超出轮数或有人梭哈了，结束游戏
		if (isExceed_ || nextUser_ == INVALID_CHAIR || canOptPlayerCount() < 2) {
			OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
		}
	}
}

void CTableFrameSink::SendCardOnTimer(int left) {
	ThisThreadTimer->cancel(timerIdSendCard_);
	SendCard(left);
}

//用户过牌
bool CTableFrameSink::OnUserPassScore(uint32_t chairId) {
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	if (!canPassScore(chairId)) {
		SendNotify(chairId, OP_PASS, "本轮有人加注，不能过牌");
		LOG(INFO) << __FUNCTION__ << " 本轮有人加注，不能过牌";
		return false;
	}
	ThisThreadTimer->cancel(timerIdWaiting_);
	texas::CMD_S_PassScore rspdata;
	assert(chairId == currentUser_);
	rspdata.set_passuser(chairId);
	operate_[CurrentTurn()][chairId] = OP_PASS;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//m_replay.addStep(nowsec, "", currentTurn_ + 1, opPass, chairId, -1);
	//返回下一个操作用户
	nextUser_ = GetNextUser(isNewTurn_, isExceed_);
	updateMainSidePots();
	for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
		it != showPots_.end(); ++it) {
		//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
		rspdata.add_pots(it->second.score);
	}
	//没有超出轮数，且非新一轮
	if (!isExceed_ && !isNewTurn_) {
		assert(nextUser_ != INVALID_CHAIR);
		assert(nextUser_ != currentUser_);
		assert(!isGiveup_[nextUser_]);
		currentUser_ = nextUser_;
		m_bPlayerCanOpt[currentUser_] = true;
		texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
		nextStep->set_nextuser(currentUser_);
		nextStep->set_nextuserid(GetUserId(currentUser_));
		nextStep->set_currentturn(currentTurn_ + 1);
		texas::OptContext* ctx = nextStep->mutable_ctx(); {
			ctx->set_minaddscore(MinAddScore(currentUser_));
			ctx->set_followscore(CurrentFollowScore(currentUser_));
			ctx->set_canpass(canPassScore(currentUser_));
			ctx->set_canallin(canAllIn(currentUser_));
			ctx->set_canfollow(canFollowScore(currentUser_));
			ctx->set_canadd(canAddScore(currentUser_));
			if (canAddScore(currentUser_)) {
				int64_t deltaScore;
				int minIndex, maxIndex;
				int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
				assert(takeScore >= 0);
				if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
					ctx->add_range(minIndex);
					ctx->add_range(maxIndex);
					for (std::vector<int64_t>::const_iterator it = JettionList.begin();
						it != JettionList.end(); ++it) {
						ctx->add_jettons(*it + deltaScore);
					}
				}
			}
		}
		//操作剩余时间
		uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		nextStep->set_wtimeleft(wTimeLeft);
		//开启等待定时器
		timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 当前轮 nextUser_=" << (int)nextUser_;
	}
	else {
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 新一轮 nextUser_=" << (int)nextUser_;
	}
	string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, texas::SUB_S_PASS_SCORE, (uint8_t*)content.data(), content.size());
	//新一轮开始发牌
	if (isNewTurn_) {
		SendCard((isExceed_ || nextUser_ == INVALID_CHAIR || canOptPlayerCount() < 2) ? (TBL_CARDS - offset_) : (offset_ == 0 ? 3 : 1));
	}
	else {
	}
	return true;
}

//用户梭哈
bool CTableFrameSink::OnUserAllIn(uint32_t chairId) {
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s] 第[%d]轮 [%d]梭哈!",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	ThisThreadTimer->cancel(timerIdWaiting_);
	texas::CMD_S_AllIn rspdata;
	assert(chairId == currentUser_);
	rspdata.set_allinuser(currentUser_);
	int64_t addScore = takeScore_[chairId] - tableScore_[chairId];
	assert(addScore > 0);
	int64_t maxAddScore = 4000000;
	if (/*ThisRoomId >= 4204 && */addScore > maxAddScore) {
		addScore = maxAddScore;
		LOG(INFO) << __FUNCTION__ << " room=" << ThisRoomId << " 梭哈限额=" << maxAddScore;
	}
	updateDeltaAddScore(chairId, addScore, OP_ALLIN);
	rspdata.set_addscore(addScore);
	tableScore_[chairId] += addScore;
	rspdata.set_tablescore(tableScore_[chairId]);
	tableAllScore_ += addScore;
	rspdata.set_tableallscore(tableAllScore_);
	operate_[CurrentTurn()][chairId] = OP_ALLIN;
	addScore_[CurrentTurn()][chairId] += addScore;
	allInTurn_[chairId] = CurrentTurn();
	updateReferenceUser(chairId, OP_ALLIN);
	int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
	assert(userScore >= 0);
	int64_t takeScore = takeScore_[chairId] - tableScore_[chairId];
	assert(takeScore >= 0);
	rspdata.set_takescore(takeScore);
	rspdata.set_userscore(userScore);
	std::map<int, int64_t> chips;
	addScoreToChips(chairId, addScore, chips);
	for (std::map<int, int64_t>::const_iterator it = chips.begin();
		it != chips.end(); ++it) {
		texas::ChipInfo* chip = rspdata.add_chips();
		chip->set_chip(it->first);
		chip->set_count(it->second);
		//LOG(INFO) << __FUNCTION__ << " chip=" << it->first << " count=" << it->second;
	}
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
	//m_replay.addStep(nowsec, to_string(addScore), currentTurn_ + 1, opCmprOrLook, chairId, -1);
	//返回下一个操作用户
	nextUser_ = GetNextUser(isNewTurn_, isExceed_);
	updateMainSidePots(chairId, addScore);
	for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
		it != showPots_.end(); ++it) {
		//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
		rspdata.add_pots(it->second.score);
	}
	//没有超出轮数，且非新一轮
	if (!isExceed_ && !isNewTurn_) {
		assert(nextUser_ != INVALID_CHAIR);
		assert(nextUser_ != currentUser_);
		assert(!isGiveup_[nextUser_]);
		currentUser_ = nextUser_;
		m_bPlayerCanOpt[currentUser_] = true;
		texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
		nextStep->set_nextuser(currentUser_);
		nextStep->set_nextuserid(GetUserId(currentUser_));
		nextStep->set_currentturn(currentTurn_ + 1);
		texas::OptContext* ctx = nextStep->mutable_ctx();
		{
			ctx->set_minaddscore(MinAddScore(currentUser_));
			ctx->set_followscore(CurrentFollowScore(currentUser_));
			ctx->set_canpass(canPassScore(currentUser_));
			ctx->set_canallin(canAllIn(currentUser_));
			ctx->set_canfollow(canFollowScore(currentUser_));
			ctx->set_canadd(canAddScore(currentUser_));
			if (canAddScore(currentUser_)) {
				int64_t deltaScore;
				int minIndex, maxIndex;
				int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
				assert(takeScore >= 0);
				if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
					ctx->add_range(minIndex);
					ctx->add_range(maxIndex);
					for (std::vector<int64_t>::const_iterator it = JettionList.begin();
						it != JettionList.end(); ++it) {
						ctx->add_jettons(*it + deltaScore);
					}
				}
			}
		}
		//操作剩余时间
		uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		nextStep->set_wtimeleft(wTimeLeft);
		//开启等待定时器
		timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 当前轮 nextUser_=" << (int)nextUser_;
	}
	else {
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 新一轮 nextUser_=" << (int)nextUser_;
	}
	std::string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, texas::SUB_S_ALL_IN, (uint8_t*)content.data(), content.size());
	//新一轮开始发牌
	if (isNewTurn_) {
		SendCard((isExceed_ || nextUser_ == INVALID_CHAIR || canOptPlayerCount() < 2) ? (TBL_CARDS - offset_) : (offset_ == 0 ? 3 : 1));
	}
	return true;
}

//用户跟注/加注
bool CTableFrameSink::OnUserAddScore(uint32_t chairId, int opValue, int64_t addScore) {
	char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	texas::CMD_S_AddScore rspdata;
	assert(chairId == currentUser_);
	rspdata.set_opuser(chairId);
	rspdata.set_opvalue(opValue);
	if (opValue == OP_ADD) {
		//assert(MinAddScore(chairId) >= FloorScore / 2);
#if 0
		if (hasOperate(CurrentTurn(), OP_ALLIN)) {
			SendNotify(chairId, OP_ADD, "有人ALLIN，不能加注");
			LOG(INFO) << __FUNCTION__ << " 有人ALLIN，不能加注";
			return false;
		}
#endif
		if (addScore < MinAddScore(chairId)) {
			snprintf(msg, sizeof(msg), "加注金额(%s)不得低于最小加注金额(%s)", std::to_string(addScore).c_str(), std::to_string(MinAddScore(chairId)).c_str());
			SendNotify(chairId, OP_ADD, msg);
			LOG(INFO) << __FUNCTION__ << " " << msg;
			return false;
		}
		if ((addScore + tableScore_[chairId]) > takeScore_[chairId]) {
			SendNotify(chairId, OP_ADD, "金币不足，不能加注");
			LOG(INFO) << __FUNCTION__ << " 金币不足，不能加注";
			return false;
		}
		else if ((addScore + tableScore_[chairId]) == takeScore_[chairId]) {
			return OnUserAllIn(chairId);
		}
		else {
			int64_t maxAddScore = 4000000;
			if (/*ThisRoomId >= 4204 && */addScore > maxAddScore) {
				addScore = maxAddScore;
				LOG(INFO) << __FUNCTION__ << " room=" << ThisRoomId << " 加注限额=" << maxAddScore;
			}
		}
// 		LOG(INFO) << __FUNCTION__ << "\n"
// 			<< " referenceUser_=" << referenceUser_
// 			<< " ReferenceScore=" << ((referenceUser_ == INVALID_CHAIR) ? 0 : tableScore_[referenceUser_]) << "\n"
// 			<< " chairId=" << chairId
// 			<< " tableScore=" << tableScore_[chairId] << "\n"
// 			<< " FollowScore=" << CurrentFollowScore(chairId) << "\n"
// 			<< " MinAddScore=" << MinAddScore(chairId)
//			<< " addScore=" << addScore;
		ThisThreadTimer->cancel(timerIdWaiting_);
		updateDeltaAddScore(chairId, addScore, OP_ADD);
		rspdata.set_addscore(addScore);
		tableScore_[chairId] += addScore;
//		printf("---*** 加注 tableScore_[%d]=%lld\n", chairId, tableScore_[chairId]);
		rspdata.set_tablescore(tableScore_[chairId]);
		tableAllScore_ += addScore;
		rspdata.set_tableallscore(tableAllScore_);
		operate_[CurrentTurn()][chairId] = OP_ADD;
		addScore_[CurrentTurn()][chairId] += addScore;
		updateReferenceUser(chairId, OP_ADD);
		uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundStartTime_);
		//对局日志
		//m_replay.addStep(nowsec, to_string(addScore), currentTurn_ + 1, opAddBet, chairId, -1);
	}
	else if (opValue == OP_FOLLOW) {
		int64_t followScore = CurrentFollowScore(chairId);
		if (!canFollowScore(chairId)) {
			if (followScore > 0 && (followScore + tableScore_[chairId]) == takeScore_[chairId]) {
				return OnUserAllIn(chairId);
			}
			SendNotify(chairId, OP_FOLLOW, "本轮没人加注或者积分不够，不能跟注");
			LOG(INFO) << __FUNCTION__ << " 本轮没人加注或者积分不够，不能跟注";
			return false;
		}
		addScore = followScore;
		ThisThreadTimer->cancel(timerIdWaiting_);
		rspdata.set_addscore(followScore);
		tableScore_[chairId] += followScore;
		rspdata.set_tablescore(tableScore_[chairId]);
		tableAllScore_ += followScore;
		rspdata.set_tableallscore(tableAllScore_);
		operate_[CurrentTurn()][chairId] = OP_FOLLOW;
		addScore_[CurrentTurn()][chairId] += followScore;
		uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundStartTime_);
		//对局日志
		//m_replay.addStep(nowsec, to_string(followScore), currentTurn_ + 1, opFollow, chairId, -1);
	}
	else {
		LOG(INFO) << __FUNCTION__ << " 操作码错误";
		return false;
	}
	std::map<int, int64_t> chips;
	addScoreToChips(chairId, rspdata.addscore(), chips);
	for (std::map<int, int64_t>::const_iterator it = chips.begin();
		it != chips.end(); ++it) {
		texas::ChipInfo* chip = rspdata.add_chips();
		chip->set_chip(it->first);
		chip->set_count(it->second);
		//LOG(INFO) << __FUNCTION__ << " chip=" << it->first << " count=" << it->second;
	}
	int64_t takeScore = takeScore_[chairId] - tableScore_[chairId];
	int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
	if (takeScore < 0) {
		LOG(ERROR) << __FUNCTION__
			<< " " << chairId
			<< " " << UserIdBy(chairId)
			<< " " << " userScore=" << ScoreByChairId(chairId)
			<< " " << " takeScore=" << takeScore_[chairId]
			<< " " << " tableScore=" << tableScore_[chairId]
			<< " " << " leftScore=" << takeScore;
	}
	assert(takeScore >= 0);
	if (userScore < 0) {
		LOG(ERROR) << __FUNCTION__
			<< " " << chairId
			<< " " << UserIdBy(chairId)
			<< " " << " userScore=" << ScoreByChairId(chairId)
			<< " " << " takeScore=" << takeScore_[chairId]
			<< " " << " tableScore=" << tableScore_[chairId]
			<< " " << " leftScore=" << takeScore;
	}
	assert(userScore >= 0);
	rspdata.set_takescore(takeScore);
	rspdata.set_userscore(userScore);
	//返回下一个操作用户
	nextUser_ = GetNextUser(isNewTurn_, isExceed_);
	updateMainSidePots(chairId, addScore);
	for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
		it != showPots_.end(); ++it) {
		//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
		rspdata.add_pots(it->second.score);
	}
	//没有超出轮数，且非新一轮
	if (!isExceed_ && !isNewTurn_) {
		assert(nextUser_ != INVALID_CHAIR);
		assert(nextUser_ != currentUser_);
		assert(!isGiveup_[nextUser_]);
		currentUser_ = nextUser_;
		m_bPlayerCanOpt[currentUser_] = true;
		texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
		nextStep->set_nextuser(currentUser_);
		nextStep->set_nextuserid(GetUserId(currentUser_));
		nextStep->set_currentturn(currentTurn_ + 1);
		texas::OptContext* ctx = nextStep->mutable_ctx();
		{
			ctx->set_minaddscore(MinAddScore(currentUser_));
			ctx->set_followscore(CurrentFollowScore(currentUser_));
			ctx->set_canpass(canPassScore(currentUser_));
			ctx->set_canallin(canAllIn(currentUser_));
			ctx->set_canfollow(canFollowScore(currentUser_));
			ctx->set_canadd(canAddScore(currentUser_));
			if (canAddScore(currentUser_)) {
				int64_t deltaScore;
				int minIndex, maxIndex;
				int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
				assert(takeScore >= 0);
				if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
					ctx->add_range(minIndex);
					ctx->add_range(maxIndex);
					for (std::vector<int64_t>::const_iterator it = JettionList.begin();
						it != JettionList.end(); ++it) {
						ctx->add_jettons(*it + deltaScore);
					}
				}
			}
		}
		//操作剩余时间
		uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		nextStep->set_wtimeleft(wTimeLeft);
		//开启等待定时器
		timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 当前轮 nextUser_=" << (int)nextUser_;
	}
	else {
		//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 新一轮 nextUser_=" << (int)nextUser_;
	}
	string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, texas::SUB_S_ADD_SCORE, (uint8_t*)content.data(), content.size());
	//新一轮开始发牌
	if (isNewTurn_) {
		SendCard((isExceed_ || nextUser_ == INVALID_CHAIR || canOptPlayerCount() < 2) ? (TBL_CARDS - offset_) : (offset_ == 0 ? 3 : 1));
	}
	return true;
}

//用户弃牌
bool CTableFrameSink::OnUserGiveUp(uint32_t chairId, bool timeout) {
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
	if (userItem && !userItem->IsAndroidUser()) {
		m_pTableFrame->RefreshRechargeScore(userItem->GetUserId(), chairId);
		UserInfoList::iterator it = userinfos_.find(userItem->GetUserId());
		if (it != userinfos_.end()) {
			it->second.userScore = userItem->GetUserScore();
		}
	}
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	assert(!isGiveup_[chairId]);
	isGiveup_[chairId] = true;
	operate_[CurrentTurn()][chairId] = OP_GIVEUP;
	if (m_pTableFrame->IsAndroidUser(chairId) > 0) {
		//机器人弃牌非超时弃牌
		//assert(!timeout);
		//机器人弃牌必须看过牌
		//assert(isLooked_[chairId]);
	}
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
	//m_replay.addStep(nowsec, "", currentTurn_ + 1, opQuitOrBCall, chairId, chairId);
	//更新庄家
	//if (bankerUser_ == chairId) {
	//	bankerUser_ = maxShowCardChair();
	//}
	//真实玩家
	/*if (m_pTableFrame->IsAndroidUser(chairId) == 0)*/ {
		int64_t userScore = takeScore_[chairId] - tableScore_[chairId];
		assert(userScore >= 0);
		//计算积分
		tagSpecialScoreInfo scoreInfo;
		//scoreInfo.cardValue = StringCardValue();				//本局开牌
		//scoreInfo.rWinScore = tableScore_[chairId];			//税前输赢
		scoreInfo.addScore = -tableScore_[chairId];				//本局输赢
		scoreInfo.betScore = tableScore_[chairId];				//总投注/有效投注/输赢绝对值(系统抽水前)
		scoreInfo.revenue = 0;									//本局税收
		scoreInfo.startTime = roundStartTime_;					//本局开始时间
		scoreInfo.cellScore.push_back(tableScore_[chairId]);	//玩家桌面分
		scoreInfo.bWriteScore = true;                           //只写积分
		scoreInfo.bWriteRecord = false;                         //不写记录
		scoreInfo.validBetScore = tableScore_[chairId];
		scoreInfo.winLostScore = -tableScore_[chairId];
        scoreInfo.agentRevenue = m_pTableFrame->CalculateAgentRevenue(labs(scoreInfo.winLostScore));
        SetScoreInfoBase(scoreInfo, chairId, NULL);				//基本信息
		//写入玩家积分
		if (!debug_)
			m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_, roundId_);
	}
	texas::CMD_S_GiveUp rspdata;
	rspdata.set_giveupuser(chairId);
	if (leftPlayerCount() >= 2) {
		if (chairId == currentUser_) {
			ThisThreadTimer->cancel(timerIdWaiting_);
			//返回下一个操作用户
			nextUser_ = GetNextUser(isNewTurn_, isExceed_);
			//updateMainSidePots();
			//没有超出轮数，且非新一轮
			if (!isExceed_ && !isNewTurn_) {
				assert(nextUser_ != INVALID_CHAIR);
				assert(nextUser_ != currentUser_);
				assert(!isGiveup_[nextUser_]);
				currentUser_ = nextUser_;
				m_bPlayerCanOpt[currentUser_] = true;
				texas::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
				nextStep->set_nextuser(currentUser_);
				nextStep->set_nextuserid(GetUserId(currentUser_));
				nextStep->set_currentturn(currentTurn_ + 1);
				texas::OptContext* ctx = nextStep->mutable_ctx();
				{
					ctx->set_minaddscore(MinAddScore(currentUser_));
					ctx->set_followscore(CurrentFollowScore(currentUser_));
					ctx->set_canpass(canPassScore(currentUser_));
					ctx->set_canallin(canAllIn(currentUser_));
					ctx->set_canfollow(canFollowScore(currentUser_));
					ctx->set_canadd(canAddScore(currentUser_));
					if (canAddScore(currentUser_)) {
						int64_t deltaScore;
						int minIndex, maxIndex;
						int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
						assert(takeScore >= 0);
						if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
							ctx->add_range(minIndex);
							ctx->add_range(maxIndex);
							for (std::vector<int64_t>::const_iterator it = JettionList.begin();
								it != JettionList.end(); ++it) {
								ctx->add_jettons(*it + deltaScore);
							}
						}
					}
				}
				//操作剩余时间
				uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
				//操作总的时间
				opTotalTime_ = wTimeLeft;
				//操作开始时间
				opStartTime_ = (uint32_t)time(NULL);
				//操作结束时间
				opEndTime_ = opStartTime_ + wTimeLeft;
				nextStep->set_wtimeleft(wTimeLeft);
				//开启等待定时器
				timerIdWaiting_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CTableFrameSink::OnTimerWaitingOver, this));
				//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 当前轮 nextUser_=" << (int)nextUser_;
			}
			else {
				//LOG(INFO) << __FUNCTION__ << " turn=[" << CurrentTurn() + 1 << "] isExceed_=" << isExceed_ << " isNewTurn_=" << isNewTurn_ << " 新一轮 nextUser_=" << (int)nextUser_;
			}
		}
		else {
			//if (canOptPlayerCount() < 2) {
			//	ThisThreadTimer->cancel(timerIdWaiting_);
			//}
		}
	}
	else {
		//ThisThreadTimer->cancel(timerIdWaiting_);
	}
	updateMainSidePots();
	for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
		it != showPots_.end(); ++it) {
		//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
		rspdata.add_pots(it->second.score);
	}
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i))
			continue;
		rspdata.clear_cards();
		rspdata.set_ty(0);
		if (chairId != i &&
			//别人弃牌，我不能看底牌
			(isGiveup_[chairId] ||
				//我弃牌，别人没有比牌，我不能看底牌
				(isGiveup_[i] && !isCompared_[chairId]))) {
			uint8_t t = handCards_[chairId][0];
			uint8_t z = handCards_[chairId][1];
			handCards_[chairId][0] = 0;//暗牌
			handCards_[chairId][1] = 0;//暗牌
			rspdata.set_cards(&(handCards_[chairId])[0], MAX_COUNT);
			handCards_[chairId][0] = t;//恢复
			handCards_[chairId][1] = z;//恢复
			rspdata.set_ty(0/*cover_[chairId].ty_*/);
		}
		else {
// 			if (chairId != i) {
// 				assert(!isGiveup_[chairId] && isCompared_[chairId]);
// 				assert(isGiveup_[i] || isCompared_[i]);
// 			}
			rspdata.set_cards(&(handCards_[chairId])[0], MAX_COUNT);
			rspdata.set_ty(cover_[chairId].ty_);
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, texas::SUB_S_GIVE_UP, (uint8_t*)content.data(), content.size());
	}
	if (leftPlayerCount() >= 2) {
		//新一轮开始发牌
		if (isNewTurn_) {
			SendCard((isExceed_ || nextUser_ == INVALID_CHAIR || canOptPlayerCount() < 2) ? (TBL_CARDS - offset_) : (offset_ == 0 ? 3 : 1));
		}
		//else if (canOptPlayerCount() < 2) {
		//	SendCard(TBL_CARDS - offset_);
		//}
	}
	else {
		OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
	}
	return true;
}

//用户看牌
bool CTableFrameSink::OnUserLookCard(uint32_t chairId) {
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	if (!isLooked_[chairId]) {
		isLooked_[chairId] = true;
		uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundStartTime_);
		//对局日志
		//m_replay.addStep(nowsec, "", currentTurn_ + 1, opLkOrCall, chairId, -1);
	}
	texas::CMD_S_LookCard rspdata;
	rspdata.set_lookcarduser(chairId);
	if (isGiveup_[chairId]) {
		rspdata.set_cards(&(handCards_[chairId])[0], 1);
		rspdata.set_ty(cover_[chairId].ty_);
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_S_LOOK_CARD, (uint8_t*)content.data(), content.size());
		return true;
	}
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i))
			continue;
		if (i == chairId) {
			rspdata.set_cards(&(handCards_[chairId])[0], 1);
			rspdata.set_ty(cover_[chairId].ty_);
		}
		else if (rspdata.has_cards()) {
			rspdata.clear_cards();
			rspdata.clear_ty();
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, texas::SUB_S_LOOK_CARD, (uint8_t*)content.data(), content.size());
	}
	return true;
}

//游戏消息
bool CTableFrameSink::OnGameMessage(uint32_t chairId, uint8_t dwSubCmdID, const uint8_t* pDataBuffer, uint32_t dwDataSize)
{
	if (chairId == INVALID_CHAIR || chairId >= GAME_PLAYER || currentUser_ == INVALID_CHAIR) {
		return false;
	}
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//if (isGiveup_[currentUser_] || isLost_[currentUser_]) {
	//	return false;
	//}
	if (!bPlaying_[chairId]) {
		return false;
	}
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
	if (!userItem) {
		return false;
	}
	UserInfoList::const_iterator it = userinfos_.find(userItem->GetUserId());
	if (it == userinfos_.end() || chairId != it->second.chairId) {
		return false;
	}
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	switch (dwSubCmdID)
	{
	case texas::SUB_C_ANDROID_LEAVE: {//机器人离开
		//texas::CMD_C_AndroidLeave  reqdata;
		//reqdata.ParseFromArray(pDataBuffer, dwDataSize);
		//LOG(INFO) << __FUNCTION__ << " 机器人 " << chairId << " 离开处理 ...";
		assert(m_pTableFrame->IsAndroidUser(chairId) > 0);
		//assert(!bPlaying_[chairId] || isGiveup_[chairId]);
		return OnUserLeft(chairId, true);
	}
	case texas::SUB_C_PASS_SCORE: {	//过牌
		if (isGiveup_[chairId] || isLost_[chairId]) {
			return false;
		}
		if (chairId != currentUser_) {
			return false;
		}
		if (hasAllIn(chairId)) {
			return false;
		}
		if (takeScore_[chairId] <= tableScore_[chairId]) {
			return false;
		}
		if (bPlaying_[chairId] && !userItem->IsAndroidUser()) {
			m_bPlayerOperated[chairId] = true;
		}
		return OnUserPassScore(chairId);
	}
	case texas::SUB_C_ALL_IN: { //梭哈
		if (isGiveup_[chairId] || isLost_[chairId]) {
			return false;
		}
		if (chairId != currentUser_) {
			return false;
		}
		if (hasAllIn(chairId)) {
			return false;
		}
		if (takeScore_[chairId] <= tableScore_[chairId]) {
			return false;
		}
		if (bPlaying_[chairId] && !userItem->IsAndroidUser()) {
			m_bPlayerOperated[chairId] = true;
		}
		return OnUserAllIn(chairId);
	}
	case texas::SUB_C_ADD_SCORE: { //跟注/加注
		if (isGiveup_[chairId] || isLost_[chairId]) {
			return false;
		}
		if (chairId != currentUser_) {
			return false;
		}
		if (hasAllIn(chairId)) {
			return false;
		}
		if (takeScore_[chairId] <= tableScore_[chairId]) {
			return false;
		}
		texas::CMD_C_AddScore reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		if (reqdata.opvalue() != OP_FOLLOW && reqdata.opvalue() != OP_ADD) {
			return false;
		}
		if (bPlaying_[chairId] && !userItem->IsAndroidUser()) {
			m_bPlayerOperated[chairId] = true;
		}
		return OnUserAddScore(chairId, reqdata.opvalue(), reqdata.addscore());
	}
	case texas::SUB_C_GIVE_UP: { //弃牌
		if (isGiveup_[chairId] || isLost_[chairId]) {
			return false;
		}
		if (hasAllIn(chairId)) {
			return false;
		}
		if (bPlaying_[chairId] && !userItem->IsAndroidUser()) {
			m_bPlayerOperated[chairId] = true;
		}
		return OnUserGiveUp(chairId);
	}
	case texas::SUB_C_LOOK_CARD: { //看牌
		//if (bPlaying_[chairId] && !userItem->IsAndroidUser()) {
		//	m_bPlayerOperated[chairId] = true;
		//}
		return OnUserLookCard(chairId);
	}
	case texas::SUB_C_TAKESCORE: {
		texas::CMD_C_TakeScore reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		texas::CMD_S_TakeScore rspdata;
		if (reqdata.takescore() >= EnterMinScore &&
			reqdata.takescore() <= userItem->GetUserScore()) {
			userItem->SetAutoSetScore(reqdata.autoset());
			userItem->SetCurTakeScore(reqdata.takescore());
			cfgTakeScore_[1][chairId] = reqdata.takescore();
			rspdata.set_bok(1);
			rspdata.set_autoset(reqdata.autoset());
		}
		else {
			rspdata.set_bok(false);
		}
		std::string data = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_S_TAKESCORE, (uint8_t*)data.c_str(), data.size());
		return true;
	}
	case texas::SUB_C_SHOW_CARD: {
// 		if (isGiveup_[chairId] || isLost_[chairId]) {
// 			return false;
// 		}
		texas::CMD_C_ShowCardSwitch  reqdata;
		reqdata.ParseFromArray(pDataBuffer, dwDataSize);
		m_bShowCard[chairId] = reqdata.isshowcard();

		texas::CMD_C_ShowCardSwitch rspdata;
		rspdata.set_isshowcard(m_bShowCard[chairId]);
		//发送结果
		std::string data = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_S_SHOW_CARD, (uint8_t*)data.c_str(), data.size());

		break;
	}
	case texas::SUB_C_ROUND_END_EXIT: {  //本局结束后退出
		//if (isGiveup_[chairId] || isLost_[chairId]) {
		//	return false;
		//}
		texas::CMD_C_RoundEndExit  roundEndExit;
		roundEndExit.ParseFromArray(pDataBuffer, dwDataSize);
		m_bRoundEndExit[chairId] = roundEndExit.bexit();

		texas::CMD_S_RoundEndExitResult roundEndExitResult;
		roundEndExitResult.set_bexit(m_bRoundEndExit[chairId]);
		//发送结果
		std::string data = roundEndExitResult.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_S_ROUND_END_EXIT_RESULT, (uint8_t*)data.c_str(), data.size());

		return true;
	}
	case texas::NN_SUB_C_MESSAGE:  //发送消息
	{
		int64_t userId = userItem->GetUserId();
		//变量定义
		texas::NN_CMD_C_Message recMessage;
		recMessage.ParseFromArray(pDataBuffer, dwDataSize);

		string message = recMessage.message();
		int32_t faceId = recMessage.faceid();
		int32_t type = recMessage.type();
		texas::NN_CMD_S_MessageResult sendMessage;
		sendMessage.set_sendchairid(chairId);
		sendMessage.set_headerid(userItem->GetHeaderId());
		sendMessage.set_headboxid(userItem->GetHeadboxId());
		sendMessage.set_headimgurl(userItem->GetHeadImgUrl());
		sendMessage.set_vip(userItem->GetVip());
		sendMessage.set_nickname(userItem->GetNickName());
		sendMessage.set_senduserid(userId);

		sendMessage.set_message(message);
		sendMessage.set_faceid(faceId);
		sendMessage.set_type(type);
		//发送结果
		std::string data = sendMessage.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, texas::NN_SUB_S_MESSAGE_RESULT, (uint8_t*)data.c_str(), data.size());

		return true;
	}
	}
	return false;
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bIsLookUser) {
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] chairId[%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1, chairId);
	//LOG(INFO) << __FUNCTION__ << msg;
	if (chairId == INVALID_CHAIR || chairId >= GAME_PLAYER) {
		return false;
	}
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
	if (!userItem) {
		return false;
	}
	switch (/*m_pTableFrame->GetGameStatus()*/gameStatus_)
	{
	case GAME_STATUS_INIT:
	case GAME_STATUS_READY: {		//空闲状态
		texas::CMD_S_StatusFree rspdata;
		rspdata.set_cellscore(FloorScore);//ceilscore
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			::texas::PlayerItem* player = rspdata.add_players();
			player->set_chairid(i);//chairID
			player->set_userid(userItem->GetUserId());//userID
			player->set_nickname(userItem->GetNickName());//nickname
			player->set_headid(userItem->GetHeaderId());//headID
			if (!bPlaying_[i]) {
				player->set_takescore(takeScore_[i]);//takeScore
				player->set_userscore(userItem->GetUserScore());//userScore
			}
			else {
				int64_t userScore = userItem->GetUserScore() - tableScore_[i];
				int64_t takeScore = takeScore_[i] - tableScore_[i];
				assert(userScore >= 0);
				assert(takeScore >= 0);
				player->set_takescore(takeScore);//takeScore
				player->set_userscore(userScore);//userScore
			}
			player->set_location(userItem->GetLocation());//location
			player->set_playstatus(bPlaying_[i]);
			player->set_viplevel(userItem->GetVip());
			player->set_headboxindex(userItem->GetHeadboxId());
			player->set_headimgurl(userItem->GetHeadImgUrl());
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_SC_GAMESCENE_FREE, (uint8_t*)content.data(), content.size());
		break;
	}
	case GAME_STATUS_START: {	//游戏状态
		texas::CMD_S_StatusPlay rspdata;
		rspdata.set_roundid(strRoundID_);//牌局编号
		rspdata.set_cellscore(FloorScore);//底注
		rspdata.set_currentturn(currentTurn_ + 1);//轮数
		rspdata.set_bankeruser(bankerUser_);//庄家用户
		rspdata.set_currentuser(currentUser_);//操作用户
		rspdata.set_currentuserid(GetUserId(currentUser_));
		rspdata.set_firstuser(firstUser_);//首发用户
		rspdata.set_smallblinduser(smallBlindUser_);
		rspdata.set_smallblinduserid(GetUserId(smallBlindUser_));
		rspdata.set_bigblinduser(bigBlindUser_);
		rspdata.set_bigblinduserid(GetUserId(bigBlindUser_));
		texas::OptContext* ctx = rspdata.mutable_ctx();
		{
			ctx->set_minaddscore(MinAddScore(currentUser_));
			ctx->set_followscore(CurrentFollowScore(currentUser_));
			ctx->set_canpass(canPassScore(currentUser_));
			ctx->set_canallin(canAllIn(currentUser_));
			ctx->set_canfollow(canFollowScore(currentUser_));
			ctx->set_canadd(canAddScore(currentUser_));
			if (canAddScore(currentUser_)) {
				int64_t deltaScore;
				int minIndex, maxIndex;
				int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
				assert(takeScore >= 0);
				if (GetCurrentAddRange(MinAddScore(currentUser_), takeScore, minIndex, maxIndex, deltaScore)) {
					ctx->add_range(minIndex);
					ctx->add_range(maxIndex);
					for (std::vector<int64_t>::const_iterator it = JettionList.begin();
						it != JettionList.end(); ++it) {
						ctx->add_jettons(*it + deltaScore);
					}
				}
			}
		}
		for (std::map<int, pot_t>::const_iterator it = showPots_.begin();
			it != showPots_.end(); ++it) {
			//LOG(INFO) << __FUNCTION__ << " pot[" << it->first << "]";
			rspdata.add_pots(it->second.score);
		}
		rspdata.set_wtimeleft(opEndTime_ - (uint32_t)time(NULL));//操作剩余时间
		rspdata.set_wtotaltime(opTotalTime_);//操作总的时间
		if (offset_ > 0) {
			texas::HandCards* handcards = rspdata.mutable_tablecards();
			handcards->set_cards(&tableCards_[0], offset_);
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			if (!bPlaying_[i]) {
				::texas::PlayerItem* player = rspdata.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userID
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_takescore(takeScore_[i]);//takeScore
				player->set_userscore(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				player->set_playstatus(bPlaying_[i]);
				player->set_viplevel(userItem->GetVip());
				player->set_headboxindex(userItem->GetHeadboxId());
				player->set_headimgurl(userItem->GetHeadImgUrl());
			}
		}
		for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
			if (!it->second.isLeave) {
				uint32_t i = it->second.chairId;
				::texas::PlayerItem* player = rspdata.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(it->second.userId);//userID
				player->set_nickname(it->second.nickName);//nickname
				player->set_headid(it->second.headerId);//headID
				player->set_viplevel(it->second.vipLevel);
				player->set_headboxindex(it->second.headboxId);
				player->set_headimgurl(it->second.headImgUrl);
				int64_t takeScore = it->second.takeScore - tableScore_[i];
				assert(takeScore >= 0);
				player->set_takescore(takeScore);//takeScore
				int64_t userScore = it->second.userScore - tableScore_[i];
				assert(userScore >= 0);
				player->set_userscore(userScore);//userScore
				player->set_location(it->second.location);//location
				player->set_islooked(isLooked_[i]);
				player->set_isgiveup(isGiveup_[i]);
				player->set_isallin(hasAllIn(i));
				player->set_tablescore(tableScore_[i]);
				player->set_playstatus(bPlaying_[i]);
				if (i == chairId) {
					texas::HandCards* handcards = player->mutable_handcards();
					handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
					handcards->set_ty(cover_[i].ty_);
				}
				else {
					if (player->has_handcards()) player->clear_handcards();
				}
			}
			else {
				assert(!bPlaying_[it->second.chairId]);
			}
		}
		rspdata.set_tableallscore(tableAllScore_);
		for (std::map<int, int64_t>::const_iterator it = tableAllChip_.begin();
			it != tableAllChip_.end(); ++it) {
			texas::ChipInfo* chip = rspdata.add_chips();
			chip->set_chip(it->first);
			chip->set_count(it->second);
			//LOG(INFO) << __FUNCTION__ << " chip=" << it->first << " count=" << it->second;
		}
		rspdata.set_istimeoutgiveup(noGiveUpTimeout_[chairId]);
		rspdata.set_isautosetscore(userItem->GetAutoSetScore());
		rspdata.set_cfgtakescore(cfgTakeScore_[1][chairId]);
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_SC_GAMESCENE_PLAY, (uint8_t*)content.data(), content.size());
		break;
	}
	case GAME_STATUS_END: {//结束状态
		texas::CMD_S_StatusEnd rspdata;
		rspdata.set_cellscore(FloorScore);
		rspdata.set_wtimeleft(opEndTime_ - (uint32_t)time(NULL));
		string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, texas::SUB_SC_GAMESCENE_END, (uint8_t*)content.data(), content.size());
	}
						break;
	default:
		break;
	}
	BroadcastTakeScore(chairId, userItem->GetUserId());
	return true;
}

void CTableFrameSink::BroadcastTakeScore(uint32_t chairId, int64_t userId) {
	texas::CMD_S_BroadcastTakeScore rspdata;
	rspdata.set_chairid(chairId);
	rspdata.set_userid(userId);
	if (!bPlaying_[chairId]) {
		rspdata.set_takescore(takeScore_[chairId]);
		//LOG(WARNING) << __FUNCTION__ << " [" << strRoundID_ << "] "
		//	<< "takeScore_[" << userId << "]=" << takeScore_[chairId]
		//	<< " 非参与!";
	}
	else {
		int64_t takeScore = takeScore_[chairId] - tableScore_[chairId];
		assert(takeScore >= 0);
		rspdata.set_takescore(takeScore);
		//LOG(WARNING) << __FUNCTION__ << " [" << strRoundID_ << "] "
		//	<< "takeScore_[" << userId << "]=" << takeScore_[chairId]
		//	<< " tableScore_[" << userId << "]=" << tableScore_[chairId]
		//	<< " takeScore[" << userId << "]=" << takeScore
		//	<< " 参与者!";
	}
	std::string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, texas::SUB_S_BROADCASTTAKESCORE, (uint8_t*)content.data(), content.size());
}

//计算机器人税收
int64_t CTableFrameSink::CalculateAndroidRevenue(int64_t score)
{
	return m_pTableFrame->CalculateRevenue(score);
	//return score * stockWeak / 100;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t dchairId, uint8_t GETag)
{
	for (int i = 0; i < GAME_PLAYER; ++i) {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
		if (userItem && !userItem->IsAndroidUser()) {
			m_pTableFrame->RefreshRechargeScore(userItem->GetUserId(), i);
			UserInfoList::iterator it = userinfos_.find(userItem->GetUserId());
			if (it != userinfos_.end()) {
				it->second.userScore = userItem->GetUserScore();
			}
		}
	}
	//char msg[1024];
	//snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d]",
	//	m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1);
	//LOG(INFO) << __FUNCTION__ << msg;
	//清理所有定时器
	ClearAllTimer();
	//游戏结束时间
	roundEndTime_ = chrono::system_clock::now();
	//消息结构
	texas::CMD_S_GameEnd rspdata;
	//计算玩家积分
	std::vector<tagSpecialScoreInfo> scoreInfos;
	//记录当局游戏详情binary
	//texas::CMD_S_GameRecordDetail details;
	//details.set_gameid(ThisGameId);
	//系统抽水
	int64_t revenue[GAME_PLAYER] = { 0 };
	//代理抽水
	int64_t agentRevenue[GAME_PLAYER] = { 0 };
	//携带积分
	int64_t userScore[GAME_PLAYER] = { 0 }, takeScore[GAME_PLAYER] = { 0 };
	//输赢积分
	int64_t userWinScore[GAME_PLAYER] = { 0 };
	//输赢积分，扣除系统抽水
	int64_t userWinScorePure[GAME_PLAYER] = { 0 };
	//剩余积分
	int64_t leftUserScore[GAME_PLAYER] = { 0 }, leftTakeScore[GAME_PLAYER] = { 0 };
	//返还积分
	int64_t userBackScore[GAME_PLAYER] = { 0 };
	//系统输赢积分
	int64_t systemWinScore = 0;
	//玩家有效下注
	int64_t tableScore[GAME_PLAYER] = { 0 };
	memcpy(tableScore, tableScore_, sizeof(tableScore_));
	//桌面有效总注
	int64_t tableAllScore = tableAllScore_;
	//真人输赢积分
	int64_t realUserWinScore = 0;
	//判断是否比牌
	isShowComparedCards_ = (leftPlayerCount() > 1 && offset_ == TBL_CARDS);
	rspdata.set_showwincard(isShowComparedCards_);
	//用户赢的奖池
	int64_t userWinPot[GAME_PLAYER] = { 0 };
	//用户赢的奖池详情
	typedef std::vector<std::pair<int, int64_t>> UserWinPotDetail;
	UserWinPotDetail userWinPotDetail[GAME_PLAYER];
	//奖池分配原则
	// 1.只有参与了该奖池的玩家才能参与该奖池的分配
	// 2.每个奖池中，牌型最大者赢得该奖池，牌型相同平分奖池
	// 
	//遍历每个奖池
	for (std::map<int, pot_t>::const_iterator it = pots_.begin();
		it != pots_.end(); ++it) {
		//参与了该奖池的玩家
		std::vector<TEXAS::CGameLogic::handinfo_t const*> v;
		for (std::set<uint32_t>::const_iterator ir = it->second.ids.begin();
			ir != it->second.ids.end(); ++ir) {
			int i = *ir;
			assert(!isGiveup_[i]);
			cover_[i].chairID = i;
			v.push_back(&cover_[i]);
		}
		std::sort(&v.front(), &v.front() + v.size(), [](TEXAS::CGameLogic::handinfo_t const* src, TEXAS::CGameLogic::handinfo_t const* dst) -> bool {
			int bv = TEXAS::CGameLogic::CompareCards(*src, *dst);
			return bv > 0;
			});
		//计算赢家列表
		std::vector<uint32_t> winUsers;
		for (std::vector<TEXAS::CGameLogic::handinfo_t const*>::const_iterator it = v.begin();
			it != v.end(); ++it) {
			if (it == v.begin()) {
				winUsers.push_back((*it)->chairID);
			}
			else {
				uint32_t winUser = winUsers[0];
				int result = TEXAS::CGameLogic::CompareCards(cover_[(*it)->chairID], cover_[winUser]);
				if (result == 0) {
					winUsers.push_back((*it)->chairID);
				}
				else {
					assert(result < 0);
				}
			}
		}
		//该奖池的大小
		int64_t score = it->second.score;
		//赢家平分奖池
		for (std::vector<uint32_t>::const_iterator ir = winUsers.begin();
			ir != winUsers.end(); ++ir) {
			userWinPot[*ir] += score / winUsers.size();
			userWinPotDetail[*ir].push_back(std::make_pair(it->first, score / winUsers.size()));
		}
		/*if (it->first == 0) {
			int64_t giveupScore = 0;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!bPlaying_[i]) {
					continue;
				}
				if (isGiveup_[i] && tableScore_[i] > 0) {
					giveupScore += tableScore_[i];
				}
			}
			int64_t score = it->second.score - giveupScore;
			score / it->second.ids.size();
			for (std::set<uint32_t>::const_iterator ir = it->second.ids.begin();
				ir != it->second.ids.end(); ++ir) {
			}
		}
		else {
			int64_t score = it->second.score;
			score / it->second.ids.size();
			for(std::set<uint32_t>::const_iterator ir = it->second.ids.begin();
				ir != it->second.ids.end(); ++ir){
			}
		}*/
	}
	std::vector<TEXAS::CGameLogic::handinfo_t const*> v;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!bPlaying_[i]) {
			continue;
		}
		if (isGiveup_[i]) {
			continue;
		}
		if (isShowComparedCards_)
			isCompared_[i] = true;
		cover_[i].chairID = i;
		v.push_back(&cover_[i]);
	}
	std::sort(&v.front(), &v.front() + v.size(), [](TEXAS::CGameLogic::handinfo_t const* src, TEXAS::CGameLogic::handinfo_t const* dst) -> bool {
		int bv = TEXAS::CGameLogic::CompareCards(*src, *dst);
		return bv > 0;
		});
	assert(v.size() > 0);
	winUsers_.clear();
	for (std::vector<TEXAS::CGameLogic::handinfo_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		if (it == v.begin()) {
			winUsers_.push_back((*it)->chairID);
		}
		else {
			uint32_t winUser = winUsers_[0];
			int result = TEXAS::CGameLogic::CompareCards(cover_[(*it)->chairID], cover_[winUser]);
			if (result == 0) {
				winUsers_.push_back((*it)->chairID);
			}
			else {
				assert(result < 0);
			}
		}
	}
	currentWinUser_ = v[0]->chairID;
	assert(winUsers_.size() > 0);
	//assert(winUser != INVALID_CHAIR);
	//bankerUser_ = currentWinUser_/*winUser*/;
	//currentWinUser_ = winUser;
	//currentUser_ = INVALID_CHAIR;
	//int c = leftPlayerCount(true, &currentUser_);
	//assert(c == 1);
	//assert(currentWinUser_ == currentUser_);
	//bool bPlatformBanker = (m_pTableFrame->IsAndroidUser(bankerUser_) > 0 || m_pTableFrame->IsSystemUser(bankerUser_) > 0);
	//LOG(INFO) << __FUNCTION__ << " 第[" << CurrentTurn() + 1 << "]轮 赢家[" << LastTurn() + 1 << "]轮时下注=" << GetLastAddScore(winUser);
// 	for (int i = 0; i < GAME_PLAYER; ++i) {
// 		if (!bPlaying_[i])
// 		if (isGiveup_[i])
// 			continue;
// 		//当轮防以小博大处理
// 		if (i != winUser) {
// 			LOG(INFO) << __FUNCTION__ << " 第[" << CurrentTurn() + 1 << "]轮 输家[" << LastTurn() + 1 << "]轮时下注=" << GetLastAddScore(i);
// 			if (GetLastAddScore(i) > GetLastAddScore(winUser)) {
// 				//返还输家当轮积分
// 				userBackScore[i] = GetLastAddScore(i) - GetLastAddScore(winUser);
// 				LOG(INFO) << __FUNCTION__ << " 第[" << CurrentTurn() + 1 << "]轮 防以小博大返还输家积分=" << userBackScore[i];
// 				tableScore[i] -= userBackScore[i];
// 				tableAllScore -= userBackScore[i];
// 
// 			}
// 		}
// 		//真实玩家
// 		/*if (m_pTableFrame->IsAndroidUser((uint32_t)i) == 0)*/ {
// 			int64_t userScore = ScoreByChairId(i) - tableScore[i];
// 			assert(userScore >= 0);
// #if 0
// 			//计算积分
// 			tagSpecialScoreInfo scoreInfo;
// 			//scoreInfo.cardValue = StringCardValue();		//本局开牌
// 			//scoreInfo.rWinScore = tableScore[i];			//税前输赢
// 			scoreInfo.addScore = -tableScore[i];			//本局输赢
// 			scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
// 			scoreInfo.revenue = 0;							//本局税收
// 			scoreInfo.startTime = roundStartTime_;			//本局开始时间
// 			scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
// 			scoreInfo.bWriteScore = true;                   //只写积分
// 			scoreInfo.bWriteRecord = false;                 //不写记录
// 			scoreInfo.validBetScore = tableScore[i];
// 			scoreInfo.winLostScore = -tableScore[i];
// 			SetScoreInfoBase(scoreInfo, i, NULL);			//基本信息
// 			//写入玩家积分
// 			if (!debug_)
// 				m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_, roundId_);
// #endif
// 		}
// 	}
	for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		uint32_t i = it->second.chairId;
		//携带积分 >= 下注积分
		takeScore[i] = it->second.takeScore;
		if (takeScore[i] < tableScore[i])
			takeScore[i] = tableScore[i];
		//assert(takeScore[i] >= tableScore[i]);
		userScore[i] = it->second.userScore;
		if (userScore[i] < tableScore[i])
			userScore[i] = tableScore[i];
		//assert(userScore[i] >= tableScore[i]);
		//输赢积分
// 		if (isGiveup_[i] || isLost_[i]) {
// 			//输的积分
// 			userWinScore[i] = - tableScore[i];
// 		}
// 		else {
// 			//assert(i == currentWinUser_);
// 			//赢的积分 = 桌面总下注 - 玩家桌面分
// 			userWinScore[i] = tableAllScore - tableScore[i];
// 		}
// 		if (userWinScore[i] < 0) {
// 		}
// 		else if (userWinScore[i] == 0) {
// 		}
// 		else {
// 			//assert(i == currentWinUser_);
// 		}
		//输赢积分 = 赢的奖池 - 玩家桌面分
		userWinScore[i] = userWinPot[i] - tableScore_[i];
		if (userWinScore[i] < 0 && userWinPot[i] > 0) {
			tableScore[i] -= userWinPot[i];
			tableAllScore -= userWinPot[i];
		}
		if (it->second.isAndroidUser == 0 && it->second.isSystemUser == 0) {
			realUserWinScore += userWinScore[i];
		}
	}
	//
	//1.考虑平台赢的情况
	//		如果玩家赢了，玩家只能赢玩家的钱
	//		如果玩家输了，计算玩家输给平台的部分：
	//			如果 |平台赢钱| > |玩家输钱|
	//				平台输赢 = 玩家输钱
	//				平台输赢累减
	//			否则
	//				平台输赢 = 平台输赢累减剩余
	//				平台输赢累减
	//2.考虑平台输的情况
	//		如果玩家赢了，计算玩家赢平台的部分
	//			平台输赢 = 平台输赢/赢家人数
	//		如果玩家输了，玩家只能输给玩家
	//
	bool bPlatformWin = (realUserWinScore < 0);
	int64_t realUserWinScoreTotal = 0, realUserLostScoreTotal = 0;
	for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		uint32_t i = it->second.chairId;
		if (it->second.isAndroidUser == 0 && it->second.isSystemUser == 0) {
			if (userWinScore[i] > 0) {
				realUserWinScoreTotal += userWinScore[i];
			}
			else if (userWinScore[i] < 0) {
				realUserLostScoreTotal += userWinScore[i];
			}
		}
	}
	//计算输赢积分
    for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		uint32_t i = it->second.chairId;
		//盈利玩家
		if (userWinScore[i] > 0) {
			//assert(i == currentWinUser_);
			//系统抽水，真实玩家和机器人都按照5%抽水计算，前端显示和玩家扣分一致
			if (/*it->second.isAndroidUser > 0 ||*/ it->second.isSystemUser > 0) {
				revenue[i] = 0;
			}
			else {
				revenue[i] = m_pTableFrame->CalculateRevenue(userWinScore[i]);
			}
			agentRevenue[i] = m_pTableFrame->CalculateAgentRevenue(userWinScore[i]);
		}
		else {
			agentRevenue[i] = m_pTableFrame->CalculateAgentRevenue(-userWinScore[i]);
		}
		//输赢积分，扣除系统抽水
		userWinScorePure[i] = userWinScore[i] - revenue[i];
		//剩余积分 = 携带积分 + 输赢积分
		leftUserScore[i] = userScore[i] + userWinScorePure[i];
		leftTakeScore[i] = takeScore[i] + userWinScorePure[i];
		userWinScorePure_[i] = userWinScorePure[i];
		//跑马灯消息
		if (userWinScorePure[i] > m_pTableFrame->GetGameRoomInfo()->broadcastScore) {
		//if (userWinScorePure[i] >= m_lMarqueeMinScore) {
			std::string msg = std::to_string(cover_[i].ty_);
			m_pTableFrame->SendGameMessage(i, ""/*msg*/, SMT_GLOBAL | SMT_SCROLL, userWinScorePure[i]);
			//LOG(INFO) << " --- *** [" << strRoundID_ << "] 跑马灯信息 userid = " << it->second.userId
			//	<< " " << msg << " score = " << userWinScorePure[i];
		}
		//若是机器人AI
		if (it->second.isAndroidUser) {
#ifdef _STORAGESCORE_SEPARATE_STAT_
			//系统输赢积分，扣除系统抽水
			systemWinScore += userWinScorePure[i];
#else
			//系统输赢积分
			systemWinScore += userWinScore[i];
#endif
		}
		//赢家
		if (userWinScore[i] > 0/*i == currentWinUser_*/) {
			//真实玩家
			/*if(it->second.isAndroidUser == 0)*/ {
				//计算积分
				tagSpecialScoreInfo scoreInfo;
				scoreInfo.cardValue = StringCardValue(isShowComparedCards_);		//本局开牌
				//scoreInfo.rWinScore = llabs(userWinScore[i]);	//税前输赢
				//scoreInfo.rWinScore = tableScore[i];			//有效投注
				scoreInfo.addScore = userWinScorePure[i];		//本局输赢
				scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = revenue[i];					//本局税收
				scoreInfo.startTime = roundStartTime_;			//本局开始时间
				scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
				scoreInfo.bWriteScore = true;                   //写赢家分
				scoreInfo.bWriteRecord =
					(it->second.isAndroidUser == 0) ? true : false;//也写记录
				scoreInfo.validBetScore = llabs(userWinScore[i]);// tableScore[i];
				scoreInfo.winLostScore = userWinScore[i];
				scoreInfo.agentRevenue = agentRevenue[i];
				if (it->second.isAndroidUser == 0 && it->second.isSystemUser == 0) {
#if 0
					//平台赢，玩家只能赢玩家
					if (bPlatformWin) {
					}
					//平台输，计算玩家赢平台的部分
					else {
						//按照比例分摊
						int64_t winScore = realUserWinScore * userWinScore[i] / realUserWinScoreTotal;
						if (llabs(realUserWinScore) > llabs(winScore)) {
							scoreInfo.platformWinScore = -winScore;
							realUserWinScore -= winScore;
						}
						else if (llabs(realUserWinScore) > 0) {
							scoreInfo.platformWinScore = -realUserWinScore;
							realUserWinScore -= realUserWinScore;
						}
					}
#else
					//平台输
					//if (!bPlatformWin)
						scoreInfo.platformWinScore = -userWinScore[i];
#endif
				}
				SetScoreInfoBase(scoreInfo, i, &it->second);	//基本信息
				scoreInfos.push_back(scoreInfo);
				if (it->second.isAndroidUser == 0) {
					//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "]"
					//	<< "\n玩家得分=" << scoreInfo.winLostScore
					//	<< "\n玩家输赢=" << scoreInfo.addScore
					//	<< "\n游戏输赢=" << -scoreInfo.winLostScore
					//	<< "\n平台税收=" << scoreInfo.winLostScore - scoreInfo.addScore
					//	<< "\n平台营收=" << scoreInfo.platformWinScore + scoreInfo.revenue;
				}
			}
			//对局日志
			//m_replay.addResult(i, i, tableScore_[i], userWinScorePure[i],
			//	to_string(cover_[i].ty_) + ":" + GlobalFunc::converToHex(&(handCards_[i])[0], MAX_COUNT), false);
		}
		//输家
		else {
			//真实玩家
			/*if (it->second.isAndroidUser == 0)*/ {
				//计算积分
				tagSpecialScoreInfo scoreInfo;
				scoreInfo.cardValue = StringCardValue(isShowComparedCards_);		//本局开牌
				//scoreInfo.rWinScore = tableScore[i];			//税前输赢
				scoreInfo.addScore = userWinScorePure[i];		//本局输赢
				scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = 0/*revenue[i]*/;			//本局税收
				scoreInfo.startTime = roundStartTime_;			//本局开始时间
				scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
#if 0
				scoreInfo.bWriteScore = false;                  //不用写分，出局前写了
				scoreInfo.bWriteRecord =
					(it->second.isAndroidUser == 0) ? true : false;//仅写记录
#else
				if (isGiveup_[i]) {
					scoreInfo.bWriteScore = false;                  //不用写分，出局前写了
					scoreInfo.bWriteRecord =
						(it->second.isAndroidUser == 0) ? true : false;//仅写记录
				}
				else {
					//assert(isLost_[i]);
					scoreInfo.bWriteScore = true;                   //写输家分
					scoreInfo.bWriteRecord =
						(it->second.isAndroidUser == 0) ? true : false;//也写记录
				}
#endif
				scoreInfo.validBetScore = llabs(userWinScore[i]);
				scoreInfo.winLostScore = userWinScore[i];
				scoreInfo.agentRevenue = agentRevenue[i];
				if (it->second.isAndroidUser == 0 && it->second.isSystemUser == 0) {
#if 0
					//平台赢，计算玩家输给平台的部分
					if (bPlatformWin) {
						//按照比例分摊
						int64_t winScore = realUserWinScore * userWinScore[i] / realUserLostScoreTotal;
						if (llabs(realUserWinScore) > llabs(winScore)) {
							scoreInfo.platformWinScore = -winScore;
							realUserWinScore -= winScore;
						}
						else if (llabs(realUserWinScore) > 0) {
							scoreInfo.platformWinScore = -realUserWinScore;
							realUserWinScore -= realUserWinScore;
						}
					}
					//平台输，玩家只能输给玩家
					else {
					}
#else
					//平台赢
					//if (bPlatformWin)
						scoreInfo.platformWinScore = -userWinScore[i];
#endif
				}
				SetScoreInfoBase(scoreInfo, i, &it->second);	//基本信息
				scoreInfos.push_back(scoreInfo);
				if (it->second.isAndroidUser == 0) {
					//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "]"
					//	<< "\n玩家得分=" << scoreInfo.winLostScore
					//	<< "\n玩家输赢=" << scoreInfo.addScore
					//	<< "\n游戏输赢=" << -scoreInfo.winLostScore
					//	<< "\n平台税收=" << scoreInfo.winLostScore - scoreInfo.addScore
					//	<< "\n平台营收=" << scoreInfo.platformWinScore + scoreInfo.revenue;
				}
			}
			//对局日志
			//m_replay.addResult(i, i, tableScore_[i], -tableScore_[i],
			//	to_string(cover_[i].ty_) + ":" + GlobalFunc::converToHex(&(handCards_[i])[0], MAX_COUNT), false);
		}
// 		texas::PlayerRecordDetail* detail = details.add_detail();
// 		//账号/昵称
// 		detail->set_account(std::to_string(it->second.userId));
// 		//座椅ID
// 		detail->set_chairid(i);
// 		//是否庄家
// 		detail->set_isbanker(bankerUser_ == i ? true : false);
// 		//手牌/牌型
// 		texas::HandCards* pcards = detail->mutable_handcards();
// 		pcards->set_cards(&(handCards_[i])[0], cardsC_[i]);
// 		pcards->set_ty(cover_[i].ty_);
// 		//携带积分
// 		detail->set_userscore(userScore[i]);
// 		//房间底注
// 		detail->set_cellscore(FloorScore);
// 		//玩家下注
// 		detail->set_tablescore(tableScore_[i]);
// 		//输赢积分
// 		detail->set_winlostscore(userWinScorePure[i]);
	}
#ifndef _STORAGESCORE_SEPARATE_STAT_
	//系统输赢积分，扣除系统抽水1%
	if (systemWinScore > 0) {
		//systemWinScore -= m_pTableFrame->CalculateRevenue(systemWinScore);
		systemWinScore -= CalculateAndroidRevenue(systemWinScore);
	}
#endif
	//更新系统库存
	//m_pTableFrame->UpdateStorageScore(systemWinScore);
	//系统当前库存变化
	//StockScore += systemWinScore;
	//更新机器人配置
	//UpdateConfig();
	//写入玩家积分
	if (!debug_)
		m_pTableFrame->WriteSpecialUserScore(&scoreInfos[0], scoreInfos.size(), strRoundID_, roundId_);
	//对局记录详情json
	//if (!m_replay.saveAsStream) {
	//}
	//else {
	//	m_replay.detailsData = details.SerializeAsString();
	//}
	//保存对局结果
	//m_pTableFrame->SaveReplay(m_replay);
	//赢家用户
	rspdata.set_winuser(currentWinUser_);
	//玩家信息
	for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		uint32_t i = it->second.chairId;
		texas::CMD_S_GameEnd_Player* player = rspdata.add_players();
		player->set_chairid(i);
		player->set_userid(it->first);
		for (UserWinPotDetail::const_iterator it = userWinPotDetail[i].begin();
			it != userWinPotDetail[i].end(); ++it) {
			::texas::PlayerPotInfo* detail = player->add_winpots();
			detail->set_index(it->first);
			detail->set_score(it->second);
		}
// 		texas::HandCards* handcards = player->mutable_handcards();
// 		handcards->set_cards(&(handCards_[i])[0], cardsC_[i]);
// 		handcards->set_ty(cover_[i].ty_);
//		//机器人AI
// 		if (it->second.isAndroidUser > 0) {
// 			LOG(INFO) << __FUNCTION__
// 				<< " [" << strRoundID_ << "] 机器人 [" << i << "] " << it->second.userId << " 手牌 ["
// 				<< TEXAS::CGameLogic::StringCards(&(handCards_[i])[0], cardsC_[i])
// 				<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(cover_[i].ty_)) << "]\n";
// 		}
// 		else {
// 			LOG(INFO) << __FUNCTION__
// 				<< " [" << strRoundID_ << "] 玩家 [" << i << "] " << it->second.userId << " 手牌 ["
// 				<< TEXAS::CGameLogic::StringCards(&(handCards_[i])[0], cardsC_[i])
// 				<< "] 牌型 [" << TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(cover_[i].ty_)) << "]\n";
// 		}
		//前端显示机器人和真实玩家抽水一致
		if (userWinScore[i] > 0) {
			if (it->second.isAndroidUser > 0 || it->second.isSystemUser > 0) {
				revenue[i] = m_pTableFrame->CalculateRevenue(userWinScore[i]);
				//输赢积分，扣除系统抽水
				userWinScorePure[i] = userWinScore[i] - revenue[i];
				//剩余积分 = 携带积分 + 输赢积分
				leftUserScore[i] = userScore[i] + userWinScorePure[i];
				leftTakeScore[i] = takeScore[i] + userWinScorePure[i];
			}
		}
		//输赢得分
		player->set_deltascore(userWinScorePure[i]);
		//剩余积分
		player->set_userscore(leftUserScore[i]);
		player->set_takescore(leftTakeScore[i]);
		//返还筹码
		player->set_backscore(userBackScore[i]);
		if (userWinScorePure[i] > 0) {
// 			for (std::map<int, int64_t>::const_iterator it = tableAllChip_.begin();
// 				it != tableAllChip_.end(); ++it) {
// 				texas::ChipInfo* chip = player->add_chips();
// 				chip->set_chip(it->first);
// 				chip->set_count(it->second);
// 			}
		}
		else if (userBackScore[i] > 0) {
			std::map<int, int64_t> chips;
			settleChips(userBackScore[i], chips);
			for (std::map<int, int64_t>::const_iterator it = chips.begin();
				it != chips.end(); ++it) {
				texas::ChipInfo* chip = player->add_chips();
				chip->set_chip(it->first);
				chip->set_count(it->second);
			}
		}
	}
	//
	//自己 A                  别人 B        
	//    弃                    比牌  明牌
	//    弃                    未比  暗牌
	//    比                    比牌  明牌
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if(!m_pTableFrame->IsExistUser(i))
			continue;
		for (int j = 0; j < rspdata.players_size();++j) {
			texas::CMD_S_GameEnd_Player* player = rspdata.mutable_players(j);//rspdata.players(j);
			player->clear_handcards();
			texas::HandCards* handcards = player->mutable_handcards();
			if (player->chairid() != i && !m_bShowCard[player->chairid()] &&
				//别人弃牌，我不能看底牌
				(isGiveup_[player->chairid()] ||
					//我弃牌，别人没有比牌，我不能看底牌
					(isGiveup_[i] && !isCompared_[player->chairid()]))) {
				uint8_t t = handCards_[player->chairid()][0];
				uint8_t z = handCards_[player->chairid()][1];
				handCards_[player->chairid()][0] = 0;//暗牌
				handCards_[player->chairid()][1] = 0;//暗牌
				handcards->set_cards(&(handCards_[player->chairid()])[0], MAX_COUNT);
				handCards_[player->chairid()][0] = t;//恢复
				handCards_[player->chairid()][1] = z;//恢复
				handcards->set_ty(0/*cover_[player->chairid()].ty_*/);
			}
			else {
// 				if (player->chairid() != i) {
// 					assert(!isGiveup_[player->chairid()] && isCompared_[player->chairid()]);
// 					assert(isGiveup_[i] || isCompared_[i]);
// 				}
				handcards->set_cards(&(handCards_[player->chairid()])[0], MAX_COUNT);
				handcards->set_ty(cover_[player->chairid()].ty_);
				player->clear_handindexs();
				for (std::vector<int>::const_iterator it = cover_[player->chairid()].vec[0].begin();
					it != cover_[player->chairid()].vec[0].end(); ++it) {
					player->add_handindexs(*it);
				}
				player->clear_tblindexs();
				for (std::vector<int>::const_iterator it = cover_[player->chairid()].vec[1].begin();
					it != cover_[player->chairid()].vec[1].end(); ++it) {
					player->add_tblindexs(*it);
				}
			}
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, texas::SUB_S_GAME_END, (uint8_t*)content.data(), content.size());
	}
	//std::string content = rspdata.SerializeAsString();
	//m_pTableFrame->SendTableData(INVALID_CHAIR, texas::SUB_S_GAME_END, (uint8_t *)content.data(), content.size());
	//通知框架结束游戏
	//m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	//m_pTableFrame->SetGameStatus(GAME_STATUS_END);
	gameStatus_ = GAME_STATUS_END;
	//延时清理桌子数据
	timerIdGameEnd_ = ThisThreadTimer->runAfter((isShowComparedCards_ ? 4 : 2) + 1, boost::bind(&CTableFrameSink::OnTimerGameEnd, this));
	//OnTimerGameEnd();
	//clearKickUsers();
	return true;
}

//游戏结束，清理数据
void CTableFrameSink::OnTimerGameEnd() {
	//通知框架结束游戏
	m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_END);
	ClearAllTimer();
	clearKickUsers();
	//有真人玩家且够游戏人数，继续下一局游戏
	if (/*m_pTableFrame->GetRealPlayerCount() > 0 && */m_pTableFrame->GetPlayerCount() >= MIN_GAME_PLAYER) {
		if (writeRealLog_) {
			char msg[1024];
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] 继续下一局游戏!",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1);
			LOG(INFO) << __FUNCTION__ << msg;
		}
		ClearGameData();
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		gameStatus_ = GAME_STATUS_READY;
		maxAndroid_ = rand_.betweenInt(1, 8/*m_pTableFrame->GetGameRoomInfo()->maxAndroidCount*/).randInt_mt();
		timerIdGameReadyOver_ = ThisThreadTimer->runAfter((isShowComparedCards_ ? 0 : 0) + 1, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
	}
	//else if (m_pTableFrame->GetRealPlayerCount() > 0) {
	else if (m_pTableFrame->GetPlayerCount() > 0) {
		if (writeRealLog_) {
			char msg[1024];
			snprintf(msg, sizeof(msg), " --- *** tableID[%d][%s][%s][%d] 不满最小游戏人数(real=%d AI=%d total=%d)，重新匹配!",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1,
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
			LOG(INFO) << __FUNCTION__ << msg;
		}
		ClearGameData();
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		gameStatus_ = GAME_STATUS_READY;
		//totalMatchSeconds_ = 0;
		maxAndroid_ = rand_.betweenInt(1, 8/*m_pTableFrame->GetGameRoomInfo()->maxAndroidCount*/).randInt_mt();
		timerIdGameReadyOver_ = ThisThreadTimer->runEvery(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
	}
	else {
		//没有真人玩家或不够游戏人数(<MIN_GAME_PLAYER)，清理腾出桌子
		if (writeRealLog_) {
			char msg[1024];
			snprintf(msg, sizeof(msg), "\n--- *** tableID[%d][%s][%s][%d] 终止游戏并退出(real=%d AI=%d total=%d)",
				m_pTableFrame->GetTableId(), StringStat(m_pTableFrame->GetGameStatus()).c_str(), strRoundID_.c_str(), currentTurn_ + 1,
				m_pTableFrame->GetRealPlayerCount(), m_pTableFrame->GetAndroidPlayerCount(), m_pTableFrame->GetPlayerCount());
			LOG(INFO) << __FUNCTION__ << msg;
		}
		ClearGameData();
		//清理房间内玩家
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			bPlaying_[i] = false;
			m_bRoundEndExit[i] = false;
			m_bShowCard[i] = false;
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			userItem->SetUserStatus(sOffline);
			m_pTableFrame->ClearTableUser(i, true, true);
		}
		//重置游戏初始化
		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		gameStatus_ = GAME_STATUS_INIT;
		writeRealLog_ = false;
	}
}

//初始化玩家当前携带
void CTableFrameSink::initUserTakeScore(shared_ptr<CServerUserItem> userItem) {
	if (!bPlaying_[userItem->GetChairId()]) {
		if (take_) {
			if (userItem->IsAndroidUser()) {
				assert(userItem->GetUserScore() >= EnterMinScore);
				int64_t minScore = 0;
				if (EnterMaxScore >= EnterMinScore) {
					minScore = std::min(userItem->GetUserScore(), EnterMaxScore);
				}
				else {
					minScore = userItem->GetUserScore();
				}
				assert(EnterMinScore <= minScore);
				int64_t curScore = GlobalFunc::RandomInt64(EnterMinScore, minScore);
				if (curScore > 0) {
					curScore = 100 * (int64_t)(curScore / 100);
					minScore = std::min(userItem->GetUserScore(), curScore);
				}
				else {
					minScore = userItem->GetUserScore();
				}
				assert(curScore >= EnterMinScore);
				assert(curScore <= userItem->GetUserScore());
				if (EnterMaxScore >= EnterMinScore) {
					assert(curScore <= EnterMaxScore);
				}
				userItem->SetCurTakeScore(minScore);
				assert(userItem->GetCurTakeScore() >= EnterMinScore);
				assert(userItem->GetCurTakeScore() <= userItem->GetUserScore());
			}
			else {
				if (userItem->GetCurTakeScore() >= EnterMinScore &&
					userItem->GetCurTakeScore() <= userItem->GetUserScore()) {
				}
				else {
					userItem->SetCurTakeScore(userItem->GetUserScore());
					assert(userItem->GetCurTakeScore() >= EnterMinScore);
					assert(userItem->GetCurTakeScore() <= userItem->GetUserScore());
				}
			}
			for (int i = 0; i < 2; ++i) {
				cfgTakeScore_[i][userItem->GetChairId()] = userItem->GetCurTakeScore();
			}
			takeScore_[userItem->GetChairId()] = cfgTakeScore_[0][userItem->GetChairId()];
			setscore_[userItem->GetChairId()] = true;
		}
		else {
			takeScore_[userItem->GetChairId()] = userItem->GetUserScore();
		}
		assert(takeScore_[userItem->GetChairId()] >= EnterMinScore);
		assert(takeScore_[userItem->GetChairId()] <= userItem->GetUserScore());
	}
}

//更新玩家当前携带
void CTableFrameSink::upateUserTakeScore(shared_ptr<CServerUserItem> userItem) {
	if (take_) {
		if (cfgTakeScore_[0][userItem->GetChairId()] != cfgTakeScore_[1][userItem->GetChairId()]) {
			cfgTakeScore_[0][userItem->GetChairId()] = cfgTakeScore_[1][userItem->GetChairId()];
			takeScore_[userItem->GetChairId()] = cfgTakeScore_[0][userItem->GetChairId()];
			setscore_[userItem->GetChairId()] = true;
			//LOG(WARNING) << __FUNCTION__ << " 带入配置变更 通知 " << userItem->GetUserId() << " takeScore_=" << takeScore_[userItem->GetChairId()];
		}
		else if (userItem->GetAutoSetScore()) {
			takeScore_[userItem->GetChairId()] = cfgTakeScore_[0][userItem->GetChairId()];
			setscore_[userItem->GetChairId()] = true;
			//LOG(WARNING) << __FUNCTION__ << " 自动带入设置 通知 " << userItem->GetUserId() << " takeScore_=" << takeScore_[userItem->GetChairId()];
		}
		else {
			takeScore_[userItem->GetChairId()] += userWinScorePure_[userItem->GetChairId()];
			if (takeScore_[userItem->GetChairId()] < EnterMinScore) {
			//if (takeScore_[userItem->GetChairId()] <= FloorScore) {
				takeScore_[userItem->GetChairId()] = cfgTakeScore_[0][userItem->GetChairId()];
				setscore_[userItem->GetChairId()] = true;
				//LOG(WARNING) << __FUNCTION__ << " 携带低于准入分 通知 " << userItem->GetUserId() << " takeScore_=" << takeScore_[userItem->GetChairId()];
			}
			else {
				//LOG(WARNING) << __FUNCTION__ << " 手动带入显示变化 " << userItem->GetUserId() << " takeScore_=" << takeScore_[userItem->GetChairId()];
			}
		}
		//if (takeScore_[userItem->GetChairId()] < EnterMinScore) {
		//	takeScore_[userItem->GetChairId()] = userItem->GetUserScore();
		//}
		/*else */if (takeScore_[userItem->GetChairId()] > userItem->GetUserScore()) {
			takeScore_[userItem->GetChairId()] = 100 * (int64_t)(userItem->GetUserScore() / 100);
			//提示余额不够自动补充，请尽快充值
			needTip_[userItem->GetChairId()] = true;
			//LOG(WARNING) << __FUNCTION__ << " 余额不够自动补充 通知 " << userItem->GetChairId() << " takeScore_=" << takeScore_[userItem->GetChairId()];
		}
	}
	else {
		takeScore_[userItem->GetChairId()] = userItem->GetUserScore();
	}
}

//踢人用户清理
void CTableFrameSink::clearKickUsers() {
	//有真人的场机器人离开概率小，没有真人的场机器人离开概率大
	int randValue = (m_pTableFrame->GetRealPlayerCount() > 0) ? 35 : 90;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
		if (userItem) {
			upateUserTakeScore(userItem);
			//离线，踢出玩家
			if (userItem->GetUserStatus() == sOffline) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				m_bShowCard[i] = false;
				m_pTableFrame->ClearTableUser(i, true, true);
			}
			//账号停用，踢出玩家
			else if (userItem->GetUserStatus() == sStop) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				m_bShowCard[i] = false;
				userItem->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_STOP_CUR_USER);
			}
			//积分不足，踢出玩家
			else if (userItem->GetUserScore() < EnterMinScore) {
				bPlaying_[i] = false;
				m_bRoundEndExit[i] = false;
				m_bShowCard[i] = false;
				userItem->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
			//积分太多，踢出玩家
// 			else if (EnterMaxScore > 0 && userItem->GetUserScore() > EnterMaxScore) {
// 				bPlaying_[i] = false;
// 				m_bRoundEndExit[i] = false;
// 				m_bShowCard[i] = false;
// 				userItem->SetUserStatus(sOffline);
// 				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORELIMIT);
// 			}
			else if (bPlaying_[i] && !userItem->IsAndroidUser()) {
				//本局结束自动离开桌子
				if (m_bRoundEndExit[i]) {
					bPlaying_[i] = false;
					m_bRoundEndExit[i] = false;
					m_bShowCard[i] = false;
					userItem->SetUserStatus(sOffline);
					m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_USER_AUTO_EXIT);
				}
				//长时间未操作，踢出玩家
				else if (m_bPlayerCanOpt[i] && !m_bPlayerOperated[i]) {
					bPlaying_[i] = false;
					m_bRoundEndExit[i] = false;
					m_bShowCard[i] = false;
					userItem->SetUserStatus(sOffline);
					m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
				}
			}
			static STD::Random r(1, 100);
			if (userItem->IsAndroidUser() && m_pTableFrame->GetPlayerCount() > MIN_GAME_PLAYER && bPlaying_[i] && currentWinUser_ != i && r.randInt_mt() <= randValue) {
				OnUserLeft(userItem->GetUserId(), true);
			}
		}
	}
}

//读取配置
void CTableFrameSink::ReadConfigInformation() {
	static STD::Random r;
	time_t now = time(NULL);
	uint32_t elapsed = now - lastReadCfgTime_;
	if (elapsed > (readIntervalTime_ + r.betweenInt(0, 10).randInt_mt())) {
		assert(m_pTableFrame);
		if (!boost::filesystem::exists(INI_FILENAME)) {
			LOG(ERROR) << __FUNCTION__ << " " << INI_FILENAME << " not exists";
			return;
		}
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(INI_FILENAME, pt);

		string strRoomName = "GameServer_" + to_string(m_pTableFrame->GetGameRoomInfo()->roomId);
		m_lMarqueeMinScore = pt.get<double>(strRoomName + ".MarqueeMinScore", 1000);

		//系统当前库存
		//m_pTableFrame->GetStorageScore(storageInfo_);

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
		//调试模式不写分
		debug_ = pt.get<int>("Global.debug_", 0);
		together_ = pt.get<int>("Global.together_", 0);
		take_ = pt.get<int>("Global.take_", 0);
		chipList_.clear();
		//桌面显示筹码配置
		std::string chips = pt.get<std::string>("Global.chipList_", "");
		int pre = 0;
		vector<string> vec;
		boost::split(vec, chips, boost::is_any_of(","), boost::token_compress_on);
		for (std::vector<string>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
			int chip = stol(it->c_str());
			assert(chip > 0);
			if (it == vec.begin())
				pre = chip;
			else
				assert(pre < chip);
			chipList_.emplace_back(chip);
		}
		//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
		//ratioSwap_ = pt.get<int>("Probability.ratio_", 80);
		//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
		//ratioSwapLost_ = pt.get<int>("Probability.ratioLost_", 80);
		//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
		//ratioSwapWin_ = pt.get<int>("Probability.ratioWin_", 80);
		//发牌时尽量都发好牌的概率，玩起来更有意思
		//ratioDealGood_ = pt.get<int>("Probability.ratioDealGood_", 80);
		//点杀时尽量都发好牌的概率
		//ratioDealGoodKill_ = pt.get<int>("Probability.ratioDealGoodKill_", 80);

		m_i32LowerChangeRate = pt.get<int32_t>("GAME_CONFIG.LowerChangeRate", 80);
		m_i32HigherChangeRate = pt.get<int32_t>("GAME_CONFIG.HigherChangeRate", 60);

		lastReadCfgTime_ = now;
	}
}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink()
{
    shared_ptr<CTableFrameSink> pGameProcess(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFrameSink = dynamic_pointer_cast<ITableFrameSink>(pGameProcess);
    return pITableFrameSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pITableFrameSink)
{
    pITableFrameSink.reset();
}

