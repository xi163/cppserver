#include <glog/logging.h>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Atomic.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include "ITableFrame.h"
#include "ITableFrameSink.h"
#include "IServerUserItem.h"
#include "IAndroidUserItemSink.h"
#include "ISaveReplayRecord.h"

#include "proto/texas.Message.pb.h"

#include "../Game_Texas/texas.h"
#include "../Game_Texas/funcC.h"
#include "../Game_Texas/cfg.h"
#include "../Game_Texas/StdRandom.h"

#include "AndroidUserItemSink.h"

#define TIME_GAME_ADD_SCORE       15 //下注时长(second)
#define TIME_GAME_COMPARE_DELAY   4  //比牌动画时长
#define TIME_GAME_START_DELAY     2

static std::string StringOpCode(int op) {
	switch (op)
	{
	case OP_PASS: return "过牌";
	case OP_ALLIN: return "梭哈";
	case OP_FOLLOW: return "跟注";
	case OP_ADD: return "加注";
	case OP_GIVEUP: return  "弃牌";
	case OP_LOOKOVER: return "看牌";
	default: break;
	}
}

std::map<int, CAndroidUserItemSink::weight_t> CAndroidUserItemSink::weights_;
//STD::Weight CAndroidUserItemSink::weightLookCard_[Win_Lost_Max][Stock_Sataus_Max][MAX_ROUND];

muduo::AtomicInt32 CAndroidUserItemSink::int32_;

CAndroidUserItemSink::CAndroidUserItemSink()
    : m_pTableFrame(NULL)
    , m_pAndroidUserItem(NULL)
{
	currentUser_ = INVALID_CHAIR;
	currentWinUser_ = INVALID_CHAIR;
	saveWinUser_ = INVALID_CHAIR;

	currentOp_ = OP_INVALID;

	memset(isAndroidUser_, 0, sizeof(isAndroidUser_));
	memset(isSystemUser_, 0, sizeof(isSystemUser_));
	memset(isLooked_, 0, sizeof isLooked_);
	memset(isGiveup_, 0, sizeof isGiveup_);
	memset(isLost_, 0, sizeof isLost_);

	//memset(isLooked_, 0, sizeof isLooked_);
	//memset(isGiveup_, 0, sizeof isGiveup_);
	//memset(isLost_, 0, sizeof isLost_);
	memset(bPlaying_, 0, sizeof bPlaying_);
	memset(lookCardTime_, 0, sizeof lookCardTime_);
	memset(operateTime_, 0, sizeof operateTime_);
	memset(dtLookCard_, 0, sizeof dtLookCard_);
	memset(dtOperate_, 0, sizeof dtOperate_);
	memset(tableScore_, 0, sizeof tableScore_);
	tableAllScore_ = 0;
	currentTurn_ = 0;
	offset_ = 0;
	public_.Reset();
	handTy_ = TEXAS::Tysp;
	freeLookover_ = false;
	freeGiveup_ = false;
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{
	//ThisThreadTimer->cancel(timerIdLeave_);
}

bool CAndroidUserItemSink::RepositionSink()
{
	currentUser_ = INVALID_CHAIR;
	currentWinUser_ = INVALID_CHAIR;
	saveWinUser_ = INVALID_CHAIR;

	currentOp_ = OP_INVALID;

	memset(isAndroidUser_, 0, sizeof(isAndroidUser_));
	memset(isSystemUser_, 0, sizeof(isSystemUser_));
	memset(isLooked_, 0, sizeof isLooked_);
	memset(isGiveup_, 0, sizeof isGiveup_);
	memset(isLost_, 0, sizeof isLost_);

	//memset(isLooked_, 0, sizeof isLooked_);
	//memset(isGiveup_, 0, sizeof isGiveup_);
	//memset(isLost_, 0, sizeof isLost_);
	memset(bPlaying_, 0, sizeof bPlaying_);
	memset(lookCardTime_, 0, sizeof lookCardTime_);
	memset(operateTime_, 0, sizeof operateTime_);
	memset(dtLookCard_, 0, sizeof dtLookCard_);
	memset(dtOperate_, 0, sizeof dtOperate_);
	memset(tableScore_, 0, sizeof tableScore_);
	tableAllScore_ = 0;
	currentTurn_ = 0;
	offset_ = 0;
	public_.Reset();
	handTy_ = TEXAS::Tysp;
	//是否已重置等待时长
	//randWaitResetFlag_ = false;
	//累计等待时长
	//totalWaitSeconds_ = 0;
	//分片等待时间
	//sliceWaitSeconds_ = 0.2f;
	//随机等待时长
	//randWaitSeconds_ = randWaitSeconds(ThisChairId);
    return true;
}

//桌子初始化
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pAndroidUserItem)
{
	if (!pTableFrame || !pAndroidUserItem) {
		return false;
	}
	//桌子指针
	m_pTableFrame = pTableFrame;
	//机器人对象
	m_pAndroidUserItem = pAndroidUserItem;
}

//用户指针
bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pAndroidUserItem)
{
	if (!pAndroidUserItem) {
		return false;
	}
	//机器人对象
	m_pAndroidUserItem = pAndroidUserItem;
}

//桌子指针
void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
	if (!pTableFrame) {
		return;
	}
	//桌子指针
	m_pTableFrame = pTableFrame;
	//读取机器人配置
	ReadConfig();
}

//消息处理
bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize) {
	switch (wSubCmdID)
	{
	case texas::SUB_S_ANDROID_ENTER: {//中途加入 - 服务端回复
		return onAndroidEnter(pData, wDataSize);
	}
	case texas::SUB_S_ANDROID_CARD: { //机器人牌 - 服务端广播
		return onAndroidCard(pData, wDataSize);
	}
	case texas::SUB_S_GAME_START: {	//游戏开始 - 服务端广播
		return onGameStart(pData, wDataSize);
	}
	case texas::SUB_S_PASS_SCORE: { //过牌结果 - 服务端回复
		return resultPass(pData, wDataSize);
	}
	case texas::SUB_S_ALL_IN: { //梭哈结果 - 服务端回复
		return resultAllIn(pData, wDataSize);
	}
	case texas::SUB_S_ADD_SCORE: { //跟注/加注结果 - 服务端回复
		return resultFollowAdd(pData, wDataSize);
	}
	case texas::SUB_S_GIVE_UP: { //弃牌结果 - 服务端回复
		return resultGiveup(pData, wDataSize);
	}
	case texas::SUB_S_LOOK_CARD: { //看牌结果 - 服务端回复
		return resultLookCard(pData, wDataSize);
	}
	case texas::SUB_S_SEND_CARD: { //发牌结果 - 服务端回复
		return resultSendCard(pData, wDataSize);
	}
	case texas::SUB_S_GAME_END: { //游戏结束 - 服务端广播
		return onGameEnd(pData, wDataSize);
	}
	}
    return true;
}

//中途加入 - 服务端回复
bool CAndroidUserItemSink::onAndroidEnter(const void* pBuffer, int32_t wDataSize) {
	//texas::CMD_S_AndroidEnter rspdata;
	//if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
	//	assert(false);
	//	return false;
	//}
// 	assert(!bPlaying_[ThisChairId]);
// 	//LOG(INFO) << __FUNCTION__ << " 机器人 " << ThisChairId << " 中途加入，监听离开 ...";
// 	ThisThreadTimer->cancel(timerIdLeave_);
// 	timerIdLeave_ = ThisThreadTimer->runAfter(
// 		rand_.betweenFloat(6, 20.0).randFloat_mt(),
// 		boost::bind(&CAndroidUserItemSink::onTimerAndroidLeave, this));
	return true;
}

//机器人牌 - 服务端广播
bool CAndroidUserItemSink::onAndroidCard(const void * pBuffer, int32_t wDataSize) {
	texas::CMD_S_AndroidCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//LOG(INFO) << __FUNCTION__ << " [" << rspdata.roundid() << "] 机器人牌 ......";
	strRoundID_ = rspdata.roundid();
	currentWinUser_ = rspdata.winuser();
	saveWinUser_ = currentWinUser_;
	assert(m_pTableFrame->IsExistUser(currentWinUser_));
	memset(takeScore_, 0, sizeof takeScore_);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		all_[i].Reset();
		cover_[i].Reset();
	}
	memcpy(tableCards_, rspdata.tablecards().cards().data(), TBL_CARDS);
	for (int i = 0; i < rspdata.players_size(); ++i) {
		texas::AndroidPlayer const& player = rspdata.players(i);
		assert(m_pTableFrame->IsExistUser(player.chairid()));
		memcpy(&(handCards_[player.chairid()])[0], player.handcards().cards().data(), MAX_COUNT);
		memcpy(&(combCards_[player.chairid()])[0], &(handCards_[player.chairid()])[0], MAX_COUNT);
		memcpy(&(combCards_[player.chairid()])[MAX_COUNT], &tableCards_[0], TBL_CARDS);
		TEXAS::CGameLogic::AnalyseCards(&(combCards_[player.chairid()])[0], MAX_TOTAL, all_[player.chairid()]);
		TEXAS::CGameLogic::AnalyseCards(&(handCards_[player.chairid()])[0], MAX_COUNT, cover_[player.chairid()]);
		isAndroidUser_[player.chairid()] = m_pTableFrame->IsAndroidUser((uint32_t)player.chairid()) > 0;
		isSystemUser_[player.chairid()] = m_pTableFrame->IsSystemUser((uint32_t)player.chairid()) > 0;
		takeScore_[player.chairid()] = player.userscore();
	}
	handTy_ = cover_[ThisChairId].ty_;
	return true;
}

//游戏开始 - 服务端广播
bool CAndroidUserItemSink::onGameStart(const void * pBuffer, int32_t wDataSize) {
	texas::CMD_S_GameStart rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//LOG(INFO) << __FUNCTION__ << " [" << rspdata.roundid() << "] 游戏开始 !!!!!!";
	strRoundID_ = rspdata.roundid();
	//当前最大牌用户
	assert(currentWinUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(currentWinUser_));
	memset(isLooked_, 0, sizeof isLooked_);
	memset(isGiveup_, 0, sizeof isGiveup_);
	memset(isLost_, 0, sizeof isLost_);
	memset(bPlaying_, 0, sizeof bPlaying_);
	memset(lookCardTime_, 0, sizeof lookCardTime_);
	memset(operateTime_, 0, sizeof operateTime_);
	memset(dtLookCard_, 0, sizeof dtLookCard_);
	memset(dtOperate_, 0, sizeof dtOperate_);
	offset_ = 0;
	currentTurn_ = 0;
	public_.Reset();
	memset(tableScore_, 0, sizeof tableScore_);
	tableAllScore_ = 0;
	for (int i = 0; i < rspdata.players_size(); ++i) {
		texas::PlayerItem const& player = rspdata.players(i);
		tableScore_[player.chairid()] = player.tablescore();
		tableAllScore_ += tableScore_[player.chairid()];
		bPlaying_[player.chairid()] = true;
		//LOG(INFO) << __FUNCTION__ << " [" << rspdata.roundid() << "] takeScore_[" << player.chairid() << "]=" << takeScore_[player.chairid()];
	}
	assert(tableAllScore_ == rspdata.tableallscore());
	if (rspdata.has_nextstep()) {
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//是否已重置等待时长
		//randWaitResetFlag_ = false;
		//累计等待时长
		//totalWaitSeconds_ = 0;
		//分片等待时间
		//sliceWaitSeconds_ = 0.2f;
		//随机等待时长
		//randWaitSeconds_ = randWaitSeconds(ThisChairId, 1, -1);
		//sendLookCardReq();
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate(TIME_GAME_START_DELAY);
		}
	}
	else {
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}
    return true;
}

//过牌结果 - 服务端回复
bool CAndroidUserItemSink::resultPass(const void* pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_PassScore rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	assert(rspdata.passuser() == currentUser_);
	operateTime_[rspdata.passuser()] = time(NULL);
// 	if (ThisChairId == currentUser_ &&
// 		(isLooked_[currentTurn_][ThisChairId] || (currentOp_ != OP_ALLIN && currentOp_ != OP_GIVEUP))) {
// 		sendLookCardReq();
// 	}
	if (rspdata.has_nextstep()) {
		//assert(leftPlayerCount() >= 2);
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate();
		}
	}
	else {
		//assert(leftPlayerCount() == 1);
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}
	return true;
}

//梭哈结果 - 服务端回复
bool CAndroidUserItemSink::resultAllIn(const void* pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_AllIn rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	assert(rspdata.allinuser() == currentUser_);
	operateTime_[rspdata.allinuser()] = time(NULL);
// 	if (ThisChairId == currentUser_ &&
// 		(isLooked_[currentTurn_][ThisChairId] || (currentOp_ != OP_ALLIN && currentOp_ != OP_GIVEUP))) {
// 		sendLookCardReq();
// 	}
	tableScore_[currentUser_] += rspdata.addscore();
	tableAllScore_ += rspdata.addscore();
	assert(tableScore_[currentUser_] == rspdata.tablescore());
	assert(tableAllScore_ == rspdata.tableallscore());
	int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
	assert(takeScore == rspdata.takescore());
	if (rspdata.has_nextstep()) {
		//assert(leftPlayerCount() >= 2);
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate();
		}
	}
	else {
		//assert(leftPlayerCount() == 1);
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}
	return true;
}

//跟注/加注结果 - 服务端回复
bool CAndroidUserItemSink::resultFollowAdd(const void * pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_AddScore rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	assert(rspdata.opuser() == currentUser_);
	//跟注(OP_FOLLOW=3)
	if (rspdata.opvalue() == OP_FOLLOW) {
		assert(followScore_ == rspdata.addscore());
	}
	//加注(OP_ADD=4)
	else {
		assert(minAddScore_ <= rspdata.addscore());
	}
	operateTime_[rspdata.opuser()] = time(NULL);
// 	if (ThisChairId == currentUser_ &&
// 		(isLooked_[currentTurn_][ThisChairId] || (currentOp_ != OP_ALLIN && currentOp_ != OP_GIVEUP))) {
// 		sendLookCardReq();
// 	}
	tableScore_[currentUser_] += rspdata.addscore();
	tableAllScore_ += rspdata.addscore();
	assert(tableScore_[currentUser_] == rspdata.tablescore());
	assert(tableAllScore_ == rspdata.tableallscore());
	int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
	assert(takeScore == rspdata.takescore());
	if (rspdata.has_nextstep()) {
		//assert(leftPlayerCount() >= 2);
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate();
		}
	}
	else {
		//assert(leftPlayerCount() == 1);
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}
    return true;
}

//离开桌子定时器
void CAndroidUserItemSink::onTimerAndroidLeave() {
// 	ThisThreadTimer->cancel(timerIdLeave_);
// 	if (ThisChairId == INVALID_CHAIR) {
// 		return;
// 	}
// 	if (!bPlaying_[ThisChairId]) {
// 		static STD::Random r(1, 100);
// 		if (r.randInt_mt() <= 50) {
// 			//LOG(INFO) << __FUNCTION__ << " 机器人 " << ThisChairId << " 中途加入，请求离开 ...";
// 			texas::CMD_C_AndroidLeave reqdata;
// 			std::string content = reqdata.SerializeAsString();
// 			//reqdata.set_chairid(ThisChairId);
// 			//m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ANDROID_LEAVE, NULL, 0);
// 			m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ANDROID_LEAVE, (uint8_t*)content.data(), content.size());
// 		}
// 		else {
// 			timerIdLeave_ = ThisThreadTimer->runAfter(
// 				rand_.betweenFloat(3.0, 10.0).randFloat_mt(),
// 				boost::bind(&CAndroidUserItemSink::onTimerAndroidLeave, this));
// 		}
// 	}
// 	else if (isGiveup_[ThisChairId]) {
// 		static STD::Random r(1, 100);
// 		if (r.randInt_mt() <= 50) {
// 			//LOG(INFO) << __FUNCTION__ << " 机器人 " << ThisChairId << " 弃牌，请求离开 ...";
// 			texas::CMD_C_AndroidLeave reqdata;
// 			std::string content = reqdata.SerializeAsString();
// 			//reqdata.set_chairid(ThisChairId);
// 			//m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ANDROID_LEAVE, NULL, 0);
// 			m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ANDROID_LEAVE, (uint8_t*)content.data(), content.size());
// 		}
// 		else {
// 			timerIdLeave_ = ThisThreadTimer->runAfter(
// 				rand_.betweenFloat(3.0, 10.0).randFloat_mt(),
// 				boost::bind(&CAndroidUserItemSink::onTimerAndroidLeave, this));
// 		}
// 	}
}

//弃牌结果 - 服务端回复
bool CAndroidUserItemSink::resultGiveup(const void* pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_GiveUp rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	assert(m_pTableFrame->IsExistUser(currentUser_));
	assert(!isGiveup_[currentUser_]);
	assert(!isLost_[currentUser_]);
	assert(m_pTableFrame->IsExistUser(rspdata.giveupuser()));
	assert(!isGiveup_[rspdata.giveupuser()]);
	assert(!isLost_[rspdata.giveupuser()]);
	isGiveup_[rspdata.giveupuser()] = true;
	operateTime_[rspdata.giveupuser()] = time(NULL);
	if (rspdata.giveupuser() == ThisChairId) {
		//ThisThreadTimer->cancel(timerIdLookCard_);
// 		ThisThreadTimer->cancel(timerIdLeave_);
// 		timerIdLeave_ = ThisThreadTimer->runAfter(
// 			rand_.betweenFloat(1, 5.0).randFloat_mt(),
// 			boost::bind(&CAndroidUserItemSink::onTimerAndroidLeave, this));
	}
	if (rspdata.giveupuser() == currentWinUser_) {
		currentWinUser_ = INVALID_CHAIR;
	}
	if (rspdata.has_nextstep()) {
		//操作用户弃牌
		assert(rspdata.giveupuser() == currentUser_);
		//assert(leftPlayerCount() >= 2);
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate();
		}
	}
	else {
		//assert(leftPlayerCount() == 1);
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}

	return true;
}

//看牌结果 - 服务端回复
bool CAndroidUserItemSink::resultLookCard(const void* pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_LookCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	if (!isLooked_[currentTurn_][rspdata.lookcarduser()]) {
		isLooked_[currentTurn_][rspdata.lookcarduser()] = true;
	}
	//if (rspdata.lookcarduser() == ThisChairId) {
	//	cover_[ThisChairId].ty_ = (TEXAS::HandTy)rspdata.ty();
	//}
	lookCardTime_[rspdata.lookcarduser()] = time(NULL);
	return true;
}

//发牌结果 - 服务端回复
bool CAndroidUserItemSink::resultSendCard(const void* pBuffer, int32_t wDataSize) {
	if (!bPlaying_[ThisChairId]) {
		return false;
	}
	texas::CMD_S_SendCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	++offset_;
	assert(offset_ <= TBL_CARDS);
	if (offset_ >= 3) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i])
				continue;
			if (isGiveup_[i] || isLost_[i])
				continue;
			memcpy(&(combCards_[i])[0], &(handCards_[i])[0], MAX_COUNT);
			memcpy(&(combCards_[i])[MAX_COUNT], &tableCards_[0], offset_);
			TEXAS::CGameLogic::AnalyseCards(&(combCards_[i])[0], MAX_COUNT + offset_, cover_[i]);
			TEXAS::CGameLogic::AnalyseCards(tableCards_, offset_, public_);
		}
	}
	if (rspdata.has_nextstep()) {
		//assert(leftPlayerCount() >= 2);
		currentUser_ = rspdata.nextstep().nextuser();
		assert(currentUser_ != INVALID_CHAIR);
		assert(m_pTableFrame->IsExistUser(currentUser_));
		assert(!isGiveup_[currentUser_]);
		assert(!isLost_[currentUser_]);
		currentTurn_ = rspdata.nextstep().currentturn() - 1;
		minAddScore_ = rspdata.nextstep().ctx().minaddscore();
		followScore_ = rspdata.nextstep().ctx().followscore();
		can_[OP_PASS] = rspdata.nextstep().ctx().canpass();
		can_[OP_ALLIN] = rspdata.nextstep().ctx().canallin();
		can_[OP_FOLLOW] = rspdata.nextstep().ctx().canfollow();
		can_[OP_ADD] = rspdata.nextstep().ctx().canadd();
		//LOG(INFO) << __FUNCTION__ << " minAddScore_=" << minAddScore_ << " followScore_=" << followScore_;
		jettons_.clear();
		minIndex_ = maxIndex_ = -1;
		if (rspdata.nextstep().ctx().range_size() > 0) {
			minIndex_ = rspdata.nextstep().ctx().range(0);
			maxIndex_ = rspdata.nextstep().ctx().range(1);
			for (int i = 0; i < rspdata.nextstep().ctx().jettons_size(); ++i) {
				jettons_.push_back(rspdata.nextstep().ctx().jettons(i));
			}
			std::stringstream ss;
			ss << "[";
			for (std::vector<int64_t>::const_iterator it = jettons_.begin();
				it != jettons_.end(); ++it) {
				if (it == jettons_.begin()) {
					ss << *it;
				}
				else {
					ss << ", " << *it;
				}
			}
			ss << "]";
			//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
			//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		}
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作用户是自己
		if (ThisChairId == currentUser_) {
			//等待机器人操作
			waitingOperate();
		}
	}
	else {
		//assert(leftPlayerCount() == 1);
		minAddScore_ = 0;
		followScore_ = 0;
		can_[OP_PASS] = false;
		can_[OP_ALLIN] = false;
		can_[OP_FOLLOW] = false;
		can_[OP_ADD] = false;
		minIndex_ = maxIndex_ = -1;
		jettons_.clear();
	}
	return true;
}

//游戏结束 - 服务端广播
bool CAndroidUserItemSink::onGameEnd(const void * pBuffer, int32_t wDataSize) {
	//if (!bPlaying_[ThisChairId]) {
	//	return false;
	//}
	RepositionSink();
	//ThisThreadTimer->cancel(timerIdLookCard_);
	//ThisThreadTimer->cancel(timerIdLeave_);
	texas::CMD_S_GameEnd rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
	//	//currentUser_已不再更新了
	//	return true;
	//}
    return true;
}

//推断当前最大牌用户
bool CAndroidUserItemSink::EstimateWinner(eEstimateTy ty) {
	switch (ty) {
	case EstimateWin: {//当前最大牌用户
		if (currentWinUser_ != INVALID_CHAIR) {
			assert(!isGiveup_[currentWinUser_]);
			assert(!isLost_[currentWinUser_]);
			return saveWinUser_ != currentWinUser_;
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!bPlaying_[i]) {
				continue;
			}
			if (isGiveup_[i] || isLost_[i]) {
				continue;
			}
			if (currentWinUser_ == INVALID_CHAIR) {
				currentWinUser_ = i;
				continue;
			}
#if 1
			//i
			std::string s1 = TEXAS::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT);
			std::string t1 = TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[i].ty_));
			//winner
			std::string s2 = TEXAS::CGameLogic::StringCards(&(handCards_[currentWinUser_])[0], MAX_COUNT);
			std::string t2 = TEXAS::CGameLogic::StringHandTy(TEXAS::HandTy(all_[currentWinUser_].ty_));
			uint32_t j = currentWinUser_;
#endif
			int result = TEXAS::CGameLogic::CompareCards(all_[i], all_[currentWinUser_]);
			if (result >= 0) {
				currentWinUser_ = i;
			}
#if 1
			std::string cc = (result > 0) ? ">" : ((result == 0) ? "=" : "<");
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%s][%s] %s [%d][%s][%s] currentWinUser_ = [%d]",
				//座椅ID/手牌/牌型
				i, s1.c_str(), t1.c_str(),
				cc.c_str(),
				//座椅ID/手牌/牌型
				j, s2.c_str(), t2.c_str(), currentWinUser_);
			//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] " << msg;
#endif
		}
		assert(currentWinUser_ != INVALID_CHAIR);
		return saveWinUser_ != currentWinUser_;
	}
	case EstimateRealWin: {//真人最大牌用户
		return true;
	}
	}
	assert(false);
	return false;
}

//推断当前操作
bool CAndroidUserItemSink::EstimateOperate() {
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	if (isGiveup_[currentUser_] || isLost_[currentUser_]) {
		return false;
	}
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		return false;
	}
	if (ThisChairId != currentUser_) {
		return false;
	}
	//当前最大牌用户
	bool changed = EstimateWinner(EstimateWin);
	if (currentOp_ == OP_INVALID || changed) {
		if (changed)
			saveWinUser_ = currentWinUser_;
		int i = IsSelfWinner();//IsPlatformWinner();
		int j = Stock_Normal;
		if (StockScore < StockLowLimit) {
			j = Stock_Floor;
		}
		else if (StockScore > StockHighLimit) {
			j = Stock_Ceil;
		}
		can_[OP_GIVEUP] = true;
		bool can_t_[OP_MAX] = { 0 };
		memcpy(can_t_, can_, sizeof can_);
	restart:
		int mask = 0;
		if (!can_t_[OP_PASS]) {
			//不能过牌
			mask |= MASK_PASS;
		}
#if 1
		if (!can_t_[OP_ALLIN]) {
			//不能梭哈
			mask |= MASK_ALLIN;
		}
#else
		if (!(can_t_[OP_ALLIN] = (can_t_[OP_ALLIN] && isLooked_[currentTurn_][ThisChairId]))) {
			//不能梭哈
			mask |= MASK_ALLIN;
		}
#endif
		if (!can_t_[OP_FOLLOW]) {
			//不能跟注
			mask |= MASK_FOLLOW;
		}
		if (!can_t_[OP_ADD]) {
			//不能加注
			mask |= MASK_ADD;
		}
#if 1
		if (!can_t_[OP_GIVEUP]) {
			//不能弃牌
			mask |= MASK_GIVEUP;
		}
#else
		if (!(can_t_[OP_GIVEUP] = isLooked_[currentTurn_][ThisChairId])) {
			//不能弃牌
			mask |= MASK_GIVEUP;
		}
#endif
		STD::Weight* p = NULL;
		if (mask > 0) {
			static int const assert_all = MASK_PASS | MASK_ALLIN | MASK_FOLLOW | MASK_ADD | MASK_GIVEUP;
			assert(mask != assert_all);
			//使用排除非使能操作权重后的随机概率
			std::map<int, weight_t>::iterator it = weights_.find(mask);
			assert(it != weights_.end());
			p = &it->second[i][j][currentTurn_];
		}
		else {
			std::map<int, weight_t>::iterator it = weights_.find(mask);
			assert(it != weights_.end());
			p = &it->second[i][j][currentTurn_];//&weights_[0][i][j][currentTurn_];
		}
#if 1
		int w[OP_MAX] = { 0 };
		p->recover(w, OP_MAX);
		std::string t, ctx;
		t += i ? "Win_Stock_" : "Lost_Stock_";
		t += j == Stock_Normal ? "Normal_Prob" : (j == Stock_Floor ? "Floor_Prob" : "Ceil_Prob");
		for (int k = OP_PASS; k <= OP_GIVEUP; ++k) {
			std::stringstream ss;
			ss << StringOpCode(k) << (can_[k] ? " [YES]" : " [NO]") << " prob_=" << w[k];
			ctx += "\n" + ss.str();
		}
		//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] " << t << ctx;
#endif
		int c = 0;
		do {
			++c;
			//权重概率获取当前操作
			assert(currentTurn_ >= 0);
			assert(currentTurn_ < MAX_ROUND);
			currentOp_ = (eOperate)p->getResult();
			assert(currentOp_ >= OP_PASS && currentOp_ <= OP_GIVEUP);
			if ((currentOp_ == OP_PASS && can_t_[OP_PASS]) ||//过牌
				(currentOp_ == OP_ALLIN && can_t_[OP_ALLIN]) ||//梭哈
				(currentOp_ == OP_FOLLOW && can_t_[OP_FOLLOW]) ||//跟注
				(currentOp_ == OP_ADD && can_t_[OP_ADD]) ||//加注
				(currentOp_ == OP_GIVEUP && can_t_[OP_GIVEUP])) {//弃牌
				break;
			}
		} while (c < 10);
		assert(c == 1);
		assert(currentOp_ >= OP_PASS && currentOp_ <= OP_GIVEUP);
		assert(can_t_[currentOp_]);
		//当前机器人赢了
		if (IsSelfWinner()) {
			
		}
		//当前机器人输了
		else {
			//合牌比公牌要大，且输
			if (cover_[ThisChairId].ty_ > public_.ty_) {
				switch (currentOp_) {
				case OP_PASS: {//OK
					if (public_.ty_ >= TEXAS::Ty30) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty40) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							//if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
							//}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty22) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty32) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							//if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
							//}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty20) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty30) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							//if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
							//}
						}
						else if (cover_[ThisChairId].ty_ >= TEXAS::Ty22) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							//if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
							//}
						}
					}
					else {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty20) {
							//if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) > TEXAS::CGameLogic::GetCardPoint(TEXAS::CGameLogic::MaxCard(show_[currentWinUser_].cards_))) {
							if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) >= TEXAS::J) {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								if (rand_op_.betweenInt(1, 100).randInt_mt() < 31) {
								}
								else {
									if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
										can_t_[currentOp_] = false;
										goto restart;
									}
								}
							}
							else {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								if (rand_op_.betweenInt(1, 100).randInt_mt() < 51) {
								}
								else {
									if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
										can_t_[currentOp_] = false;
										goto restart;
									}
								}
							}
							//}
// 							else {
// 								if (rand_op_.betweenInt(1, 100).randInt_mt() < 61) {
// 									if (can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
// 										can_t_[currentOp_] = false;
// 										goto restart;
// 									}
// 								}
// 							}
						}
					}
					break;
				}
				case OP_ALLIN: {//OK
					if (public_.ty_ >= TEXAS::Ty30) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty40) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty22) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty32) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 81) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty20) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty30) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
						else if (cover_[ThisChairId].ty_ >= TEXAS::Ty22) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 81) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty20) {
							//if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) > TEXAS::CGameLogic::GetCardPoint(TEXAS::CGameLogic::MaxCard(show_[currentWinUser_].cards_))) {
							if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) >= TEXAS::J) {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								//if (rand_op_.betweenInt(1, 100).randInt_mt() < 71) {
								//}
								//else {
									//if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
									//	can_t_[currentOp_] = false;
									//	goto restart;
									//}
								//}
							}
							else {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								//if (rand_op_.betweenInt(1, 100).randInt_mt() < 61) {
								//}
								//else {
								//	if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
								//		can_t_[currentOp_] = false;
								//		goto restart;
								//	}
								//}
							}
							//}
// 							else {
// 								//if (rand_op_.betweenInt(1, 100).randInt_mt() < 51) {
// 								//}
// 								//else {
// 									if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
// 										can_t_[currentOp_] = false;
// 										goto restart;
// 									}
// 								//}
// 							}
						}
					}
					break;
				}
				case OP_FOLLOW: {//OK
					//LOG(INFO) << __FUNCTION__ << " ---***";
					break;
				}
				case OP_ADD: {//OK
					if (public_.ty_ >= TEXAS::Ty30) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty40) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty22) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty32) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
						}
						else {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else if (public_.ty_ >= TEXAS::Ty20) {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty30) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
						else if (cover_[ThisChairId].ty_ >= TEXAS::Ty22) {
							//LOG(INFO) << __FUNCTION__ << " ---***";
							if (rand_op_.betweenInt(1, 100).randInt_mt() < 91) {
							}
							else {
								//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
								//	can_t_[currentOp_] = false;
								//	goto restart;
								//}
							}
						}
					}
					else {
						if (cover_[ThisChairId].ty_ >= TEXAS::Ty20) {
							//if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) > TEXAS::CGameLogic::GetCardPoint(TEXAS::CGameLogic::MaxCard(show_[currentWinUser_].cards_))) {
							if (TEXAS::CGameLogic::GetCardPoint(cover_[ThisChairId].cards_[0]) >= TEXAS::J) {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								if (rand_op_.betweenInt(1, 100).randInt_mt() < 81) {
								}
								else {
									//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
									//	can_t_[currentOp_] = false;
									//	goto restart;
									//}
								}
							}
							else {
								//LOG(INFO) << __FUNCTION__ << " ---***";
								if (rand_op_.betweenInt(1, 100).randInt_mt() < 71) {
								}
								else {
									//if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
									//	can_t_[currentOp_] = false;
									//	goto restart;
									//}
								}
							}
							//}
// 							else {
// 								if (rand_op_.betweenInt(1, 100).randInt_mt() < 51) {
// 								}
// 								else {
// 									if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
// 										can_t_[currentOp_] = false;
// 										goto restart;
// 									}
// 								}
// 							}
						}
					}
					break;
				}
				case OP_GIVEUP: {//OK
					//LOG(INFO) << __FUNCTION__ << " ---***";
					break;
				}
				}
			}
			//合牌和公牌一样，且输
			else {
				//♥Q ♠Q ♦4 ♠J ♣K ♦K ♠4
				//♦2 ♠2 ♦4 ♠J ♣K ♦K ♠4
				switch (currentOp_) {
				case OP_PASS: {
					//LOG(INFO) << __FUNCTION__ << " ---***";
					break;
				}
				case OP_ALLIN: {
					//LOG(INFO) << __FUNCTION__ << " ---***";
					if (can_t_[OP_PASS] || can_t_[OP_FOLLOW] || can_t_[OP_ADD] || can_t_[OP_GIVEUP]) {
						can_t_[currentOp_] = false;
						goto restart;
					}
					break;
				case OP_FOLLOW: {
					//LOG(INFO) << __FUNCTION__ << " ---***";
					break;
				}
				case OP_ADD: {
					if (TEXAS::CGameLogic::GetCardPoint(TEXAS::CGameLogic::MaxCard(&(handCards_[ThisChairId])[0], MAX_COUNT)) >= TEXAS::J) {
						//LOG(INFO) << __FUNCTION__ << " ---***";
						if (rand_op_.betweenInt(1, 100).randInt_mt() < 51) {
						}
						else {
							if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
								can_t_[currentOp_] = false;
								goto restart;
							}
						}
					}
					else {
						//LOG(INFO) << __FUNCTION__ << " ---***";
						if (can_t_[OP_PASS] || can_t_[OP_ALLIN] || can_t_[OP_FOLLOW] || can_t_[OP_GIVEUP]) {
							can_t_[currentOp_] = false;
							goto restart;
						}
					}
					break;
				}
				case OP_GIVEUP: {
					//LOG(INFO) << __FUNCTION__ << " ---***";
					break;
				}
				}
				}
			}
		}
	}
	return true;
}

//等待机器人操作
void CAndroidUserItemSink::waitingOperate(int32_t delay, int32_t wTimeLeft)
{
	ThisThreadTimer->cancel(timerIdWaiting_);
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	if (isGiveup_[currentUser_] || isLost_[currentUser_]) {
		return;
	}
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		return;
	}
	if (ThisChairId != currentUser_) {
		return;
	}
	currentOp_ = OP_INVALID;
	if (!EstimateOperate()) {
		return;
	}
	//是否已重置等待时长
	//randWaitResetFlag_ = false;
	//累计等待时长
	//totalWaitSeconds_ = 0;
	//随机等待时长
	randWaitSeconds_ = randWaitSeconds(ThisChairId, delay, wTimeLeft);
	//开启等待定时器
	timerIdWaiting_ = ThisThreadTimer->runAfter(randWaitSeconds_, boost::bind(&CAndroidUserItemSink::onTimerWaitingOver, this));
}

//随机等待时间
double CAndroidUserItemSink::randWaitSeconds(uint16_t chairId, int32_t delay, int32_t wTimeLeft) {
	assert(currentOp_ != OP_INVALID);
	if (!isLooked_[currentTurn_][ThisChairId] && (currentOp_ == OP_ALLIN || currentOp_ == OP_GIVEUP)) {
		//assert(dtLookCard_[ThisChairId] > 0);
		if (currentOp_ == OP_ALLIN) {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 60) {
				dtOperate_[ThisChairId] = dtLookCard_[ThisChairId] + rand_.betweenFloat(1.0, rand_.betweenFloat(1, 5.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = dtLookCard_[ThisChairId] + rand_.betweenFloat(1.0, rand_.betweenFloat(1, 3.0).randFloat_mt()).randFloat_mt();
			}
		}
		else if (currentOp_ == OP_GIVEUP) {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 75) {
				dtOperate_[ThisChairId] = dtLookCard_[ThisChairId] + rand_.betweenFloat(1.0, rand_.betweenFloat(1, 2.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = dtLookCard_[ThisChairId] + rand_.betweenFloat(1.0, rand_.betweenFloat(1, 3.0).randFloat_mt()).randFloat_mt();
			}
		}
	}
	else {
		if (currentOp_ == OP_ALLIN) {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 60) {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 3.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 2.0).randFloat_mt()).randFloat_mt();
			}
		}
		else if (currentOp_ == OP_GIVEUP) {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 75) {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 2.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 3.0).randFloat_mt()).randFloat_mt();
			}
		}
		else if (currentOp_ == OP_FOLLOW || currentOp_ == OP_ADD) {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 60) {
				dtOperate_[ThisChairId] = rand_.betweenFloat(2.0, rand_.betweenFloat(2, 5.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 5.0).randFloat_mt()).randFloat_mt();
			}
		}
		else /*if (currentOp_ == OP_PASS)*/ {
			if (rand_.betweenInt(1, 100).randInt_mt() <= 75) {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 2.0).randFloat_mt()).randFloat_mt();
			}
			else {
				dtOperate_[ThisChairId] = rand_.betweenFloat(1.0, rand_.betweenFloat(1, 3.0).randFloat_mt()).randFloat_mt();
			}
		}
		if (lookCardTime_[ThisChairId] > 0) {
			time_t now = time(NULL);
			assert(((double)now + dtOperate_[ThisChairId] - (double)lookCardTime_[ThisChairId]) > 0);
			if (((double)now + dtOperate_[ThisChairId] - (double)lookCardTime_[ThisChairId]) < 1.0) {
				if (rand_.betweenInt(1, 100).randInt_mt() <= 80) {
					dtOperate_[ThisChairId] += rand_.betweenFloat(1.0, rand_.betweenFloat(1.0, 2.0).randFloat_mt()).randFloat_mt();
				}
				else {
					dtOperate_[ThisChairId] += rand_.betweenFloat(1.0, rand_.betweenFloat(1.0, 3.0).randFloat_mt()).randFloat_mt();
				}
			}
		}
	}
	return dtOperate_[ThisChairId] + delay;
}

//等待操作定时器
void CAndroidUserItemSink::onTimerWaitingOver() {
	ThisThreadTimer->cancel(timerIdWaiting_);
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	if (isGiveup_[currentUser_] || isLost_[currentUser_]) {
		return;
	}
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		return;
	}
	if (ThisChairId != currentUser_) {
		return;
	}
	if (!EstimateOperate()) {
		return;
	}
	if (currentOp_ == OP_GIVEUP && can_[OP_PASS]) {
		currentOp_ = OP_PASS;
	}
// 	if (currentOp_ == OP_PASS && offset_ == TBL_CARDS && !IsSelfWinner()) {
// 		if (rand_op_.betweenInt(1, 100).randInt_mt() < 61) {
// 			currentOp_ = OP_GIVEUP;
// 		}
// 	}
	if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
		//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] 第[" << currentTurn_ + 1 << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
		//	<< " <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 玩家 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
	}
	else {
		if (IsSelfWinner()) {
			//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] 第[" << currentTurn_ + 1 << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
			//	<< " <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
		}
		else {
			//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] 第[" << currentTurn_ + 1 << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
			//	<< " <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
		}
	}
	sendOperateReq();
}

//请求操作
bool CAndroidUserItemSink::sendOperateReq() {
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	if (isGiveup_[currentUser_] || isLost_[currentUser_]) {
		return false;
	}
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return false;
	}
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		return false;
	}
// 	if (ThisChairId == currentUser_ &&
// 		(isLooked_[currentTurn_][ThisChairId] || (currentOp_ != OP_ALLIN && currentOp_ != OP_GIVEUP))) {
// 		ThisThreadTimer->cancel(timerIdLookCard_);
// 	}
    switch (currentOp_) {
	case OP_PASS: {			//请求过牌
		sendPassReq();
		break;
	}
	case OP_ALLIN: {		//请求梭哈
		sendAllInReq();
		break;
	}
	case OP_FOLLOW: {		//请求跟注
		sendFollowReq();
		break;
	}
	case OP_ADD: {			//请求加注
		sendAddReq();
		break;
	}
	case OP_GIVEUP: {		//请求弃牌
		sendGiveupReq();
		break;
	}
	//case OP_LOOKOVER: {		//请求看牌
	//	sendLookCardReq();
	//	break;
	//}
	default: assert(false); break;
    }
    return true;
}

//请求过牌
void CAndroidUserItemSink::sendPassReq() {
	assert(currentOp_ == OP_PASS);
	assert(!isGiveup_[ThisChairId]);
	assert(!isLost_[ThisChairId]);
	if (ThisChairId != currentUser_) {
		return;
	}
	assert(can_[OP_PASS]);
	m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_PASS_SCORE, NULL, 0);
}

//请求梭哈
void CAndroidUserItemSink::sendAllInReq() {
	assert(currentOp_ == OP_ALLIN);
	assert(!isGiveup_[ThisChairId]);
	assert(!isLost_[ThisChairId]);
	//assert(isLooked_[currentTurn_][ThisChairId]);
	if (ThisChairId != currentUser_) {
		return;
	}
	assert(can_[OP_ALLIN]);
	m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ALL_IN, NULL, 0);
}

//请求跟注
void CAndroidUserItemSink::sendFollowReq()
{
	assert(currentOp_ == OP_FOLLOW);
	assert(!isGiveup_[ThisChairId]);
	assert(!isLost_[ThisChairId]);
	if (ThisChairId != currentUser_) {
		return;
	}
	assert(can_[OP_FOLLOW]);
	assert(followScore_ > 0);
	if ((followScore_ + tableScore_[currentUser_]) >= takeScore_[currentUser_]) {
		LOG(ERROR) << __FUNCTION__ << " [" << strRoundID_ << "] "
			<< " followScore_=" << followScore_
			<< " tableScore_[" << currentUser_ << "]=" << tableScore_[currentUser_]
			<< " takeScore_[" << currentUser_ << "]=" << takeScore_[currentUser_];
	}
	assert((followScore_ + tableScore_[currentUser_]) < takeScore_[currentUser_]);
	texas::CMD_C_AddScore reqdata;
	reqdata.set_opvalue(OP_FOLLOW);
	reqdata.set_addscore(followScore_);
	std::string content = reqdata.SerializeAsString();
	m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
}

//请求加注
void CAndroidUserItemSink::sendAddReq()
{
	assert(currentOp_ == OP_ADD);
	assert(!isGiveup_[ThisChairId]);
	assert(!isLost_[ThisChairId]);
	if (ThisChairId != currentUser_) {
		return;
	}
	assert(can_[OP_ADD]);
	assert(minAddScore_ >= 0/*CellScore*/);
	if (minAddScore_ + tableScore_[currentUser_] >= takeScore_[currentUser_]) {
		LOG(ERROR) << __FUNCTION__ << " [" << strRoundID_ << "] "
			<< " minAddScore_=" << minAddScore_
			<< " tableScore_[" << currentUser_ << "]=" << tableScore_[currentUser_]
			<< " takeScore_[" << currentUser_ << "]=" << takeScore_[currentUser_];
	}
	assert(minAddScore_ + tableScore_[currentUser_] < takeScore_[currentUser_]);
	int64_t takeScore = takeScore_[currentUser_] - tableScore_[currentUser_];
	assert(takeScore > 0);
	if (jettons_.size() > 0) {
		std::stringstream ss;
		ss << "[";
		for (std::vector<int64_t>::const_iterator it = jettons_.begin();
			it != jettons_.end(); ++it) {
			if (it == jettons_.begin()) {
				ss << *it;
			}
			else {
				ss << ", " << *it;
			}
		}
		ss << "]";
		//LOG(INFO) << __FUNCTION__ << " jettons " << ss.str();
		//LOG(INFO) << __FUNCTION__ << " range   [" << minIndex_ << "," << maxIndex_ << "]";
		assert(minIndex_ >= 0 && maxIndex_ >= minIndex_);
		int index = rand_.betweenInt(minIndex_, maxIndex_).randInt_mt();
		//LOG(INFO) << __FUNCTION__ << " index=" << index;
		assert(index >= minIndex_ && index <= maxIndex_);
		assert(index >= 0 && index < jettons_.size());
		int64_t addScore = jettons_[index];
		assert(addScore >= minAddScore_);
		assert(addScore + tableScore_[currentUser_] < takeScore_[currentUser_]);
		texas::CMD_C_AddScore reqdata;
		reqdata.set_opvalue(OP_ADD);
		reqdata.set_addscore(addScore);
		std::string content = reqdata.SerializeAsString();
		m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
	}
	else {
		int64_t addScore = minAddScore_;
		texas::CMD_C_AddScore reqdata;
		reqdata.set_opvalue(OP_ADD);
		reqdata.set_addscore(addScore);
		std::string content = reqdata.SerializeAsString();
		m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
	}
}

//请求弃牌
void CAndroidUserItemSink::sendGiveupReq() {
	assert(currentOp_ == OP_GIVEUP);
	assert(!isGiveup_[ThisChairId]);
	assert(!isLost_[ThisChairId]);
	//assert(isLooked_[currentTurn_][ThisChairId]);
	m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_GIVE_UP, NULL, 0);
}

//请求看牌
void CAndroidUserItemSink::sendLookCardReq() {
	if (isGiveup_[ThisChairId] || isLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return;
	}
	if (!isLooked_[currentTurn_][ThisChairId]) {
		dtLookCard_[ThisChairId] = rand_.betweenFloat(2.0, 6.0).randFloat_mt();
	}
	else {
		dtLookCard_[ThisChairId] = rand_.betweenFloat(2.0, 6.0).randFloat_mt();	
	}
	if (operateTime_[ThisChairId] > 0) {
		time_t now = time(NULL);
		//assert(((double)now + dtLookCard_[ThisChairId] - (double)operateTime_[ThisChairId]) > 0);
		if (((double)now + dtLookCard_[ThisChairId] - (double)operateTime_[ThisChairId]) < 1.0) {
			dtLookCard_[ThisChairId] += rand_.betweenFloat(1.0, rand_.betweenFloat(1.0, 3.0).randFloat_mt()).randFloat_mt();
		}
	}
	ThisThreadTimer->cancel(timerIdLookCard_);
	timerIdLookCard_ = ThisThreadTimer->runAfter(dtLookCard_[ThisChairId],
		boost::bind(&CAndroidUserItemSink::sendLookCardTimerOver, this));
}

void CAndroidUserItemSink::sendLookCardTimerOver() {
	ThisThreadTimer->cancel(timerIdLookCard_);
	if (!isLooked_[currentTurn_][ThisChairId]) {
		//LOG(INFO) << __FUNCTION__ << " [" << strRoundID_ << "] 第[" << currentTurn_ + 1 << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
		//	<< " <<看牌>> " << dtLookCard_[ThisChairId] << "(s)";
		//m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_LOOK_CARD, NULL, 0);
	}
	else {
		//当前最大牌用户
		EstimateWinner(EstimateWin);
		int i = IsSelfWinner();//IsPlatformWinner();
		int j = Stock_Normal;
		if (StockScore < StockLowLimit) {
			j = Stock_Floor;
		}
		else if (StockScore > StockHighLimit) {
			j = Stock_Ceil;
		}
// 		if (0 == weightLookCard_[i][j].getResult()) {
// 			//m_pTableFrame->OnGameEvent(ThisChairId, texas::SUB_C_LOOK_CARD, NULL, 0);
// 		}
	}
	sendLookCardReq();
}

//剩余游戏人数
int CAndroidUserItemSink::leftPlayerCount(bool includeAndroid, uint32_t* currentUser) {
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

//返回有效用户
uint32_t CAndroidUserItemSink::GetNextUser(uint32_t startUser, bool ignoreCurrentUser) {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		uint32_t nextUser = (startUser + i) % GAME_PLAYER;
		if (!m_pTableFrame->IsExistUser(nextUser)) {
			continue;
		}
		if (ignoreCurrentUser && nextUser == currentUser_) {
			continue;
		}
		if (isGiveup_[nextUser] || isLost_[nextUser]) {
			continue;
		}
		return nextUser;
	}
	return INVALID_CHAIR;
}

//清理所有定时器
void CAndroidUserItemSink::ClearAllTimer()
{
    ThisThreadTimer->cancel(timerIdWaiting_);
}

void CAndroidUserItemSink::ReadConfig()
{
	if (int32_.getAndSet(1) != 0)
		return;
	if (!boost::filesystem::exists("./conf/Texasrobot_config.ini"))
	{// init robot param
		abort();
	}
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/Texasrobot_config.ini", pt);
	std::vector<std::string> v;
	std::string mat;
	//过牌/梭哈/跟注/加注/弃牌/看牌
	int prob[Win_Lost_Max][Stock_Sataus_Max][MAX_ROUND][OP_MAX] = { 0 };
	//放水且赢牌
	{
		mat = pt.get<std::string>("Win_Stock_Ceil_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Ceil][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Ceil_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Ceil][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Ceil_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Ceil][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Ceil_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Ceil][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Ceil_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Ceil][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	//正常且赢牌
	{
		mat = pt.get<std::string>("Win_Stock_Normal_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Normal][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Normal_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Normal][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Normal_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Normal][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Normal_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Normal][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Normal_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Normal][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	//收分且赢牌
	{
		mat = pt.get<std::string>("Win_Stock_Floor_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Floor][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Floor_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Floor][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Floor_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Floor][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Floor_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Floor][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Win_Stock_Floor_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Win][Stock_Floor][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	//放水且输牌
	{
		mat = pt.get<std::string>("Lost_Stock_Ceil_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Ceil][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Ceil_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Ceil][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Ceil_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Ceil][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Ceil_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Ceil][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Ceil_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Ceil][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	//正常且输牌
	{
		mat = pt.get<std::string>("Lost_Stock_Normal_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Normal][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Normal_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Normal][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Normal_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Normal][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Normal_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Normal][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Normal_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Normal][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	//收分且输牌
	{
		mat = pt.get<std::string>("Lost_Stock_Floor_Prob.PassScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Floor][i][OP_PASS] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Floor_Prob.AllIn", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Floor][i][OP_ALLIN] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Floor_Prob.FollowScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Floor][i][OP_FOLLOW] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Floor_Prob.AddScore", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Floor][i][OP_ADD] = stol(v[i]);
		}

		mat = pt.get<std::string>("Lost_Stock_Floor_Prob.GiveUp", "");
		boost::split(v, mat, boost::is_any_of(","), boost::token_compress_on);
		assert(v.size() == MAX_ROUND);
		for (int i = 0; i < MAX_ROUND; ++i) {
			prob[State_Lost][Stock_Floor][i][OP_GIVEUP] = stol(v[i]);
		}
	}
	initWeightMatrix(prob);
}

//排除非使能操作权重后的随机概率，非使能操作不能参与概率随机
void CAndroidUserItemSink::initWeightMatrix(prob_t& prob) {
	//能过牌/能梭哈/能跟注/能加注/能弃牌
	{
		weight_t& ref = weights_[0];
		for (int i = State_Lost; i <= State_Win; ++i) {
			for (int j = Stock_Ceil; j <= Stock_Floor; ++j) {
				for (int r = 0; r < MAX_ROUND; ++r) {
					int w[OP_MAX] = { 0 };
					for (int k = OP_PASS; k <= OP_GIVEUP; ++k) {
						w[k] = prob[i][j][r][k];
						assert(w[k] > 0);
					}
					ref[i][j][r].init(w, OP_MAX);
					ref[i][j][r].shuffle();
				}
			}
		}
	}
	//不能过牌/不能梭哈/不能跟注/不能加注/不能弃牌
	static int const op[] = { MASK_PASS,MASK_ALLIN,MASK_FOLLOW,MASK_ADD,MASK_GIVEUP };
	static int const n = sizeof(op) / sizeof(op[0]);
	CFuncC fnC;
	for (int c = 1; c < n; ++c) {
		std::vector<std::vector<int>> v;
		fnC.FuncC(n, c, v);
		for (std::vector<std::vector<int>>::const_iterator it = v.begin();
			it != v.end(); ++it) {
			assert(it->size() == c);
			int mask = 0;
			for (int i = 0; i < c; ++i) {
				mask |= op[(*it)[i]];
			}
			weight_t& ref = weights_[mask];
			for (int i = State_Lost; i <= State_Win; ++i) {
				for (int j = Stock_Ceil; j <= Stock_Floor; ++j) {
					for (int r = 0; r < MAX_ROUND; ++r) {
						int w[OP_MAX] = { 0 };
						for (int k = OP_PASS; k <= OP_GIVEUP; ++k) {
							if (op[k - OP_PASS] && (mask & op[k - OP_PASS]) == op[k - OP_PASS]) {
								w[k] = 0;
							}
							else {
								w[k] = prob[i][j][r][k];
								assert(w[k] > 0);
							}
						}
						ref[i][j][r].init(w, OP_MAX);
						ref[i][j][r].shuffle();
					}
				}
			}
		}
	}
}

//CreateAndroidSink 创建实例
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

//DeleteAndroidSink 删除实例
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink>& pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    pSink.reset();
}
