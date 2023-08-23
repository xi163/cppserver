
#include "proto/s13s.Message.pb.h"

#include "s13s.h"

#include "AndroidUserItemSink.h"

#define TIME_GAME_START_DELAY     2

CRobot::CRobot()
    : table_(NULL)
    , robot_(NULL)
{
    srand((unsigned)time(NULL));
	Reposition();
}

CRobot::~CRobot()
{
}

//桌子重置
bool CRobot::Reposition()
{
	//确定牌型的真人玩家数
	realPlayerCount = 0;
	//是否重置随机思考时长
	randWaitResetFlag_ = false;
	//累计思考时长
	//totalWaitSeconds_ = 0;
	//分片思考时间
	//sliceWaitSeconds_ = 0.6f;
	//随机思考时长(1.0~18.0s之间)
	//randWaitSeconds_ = CalcWaitSeconds(ThisChairId, true);
    return true;
}

//桌子初始化
bool CRobot::Init(std::shared_ptr<ITable> const& table, std::shared_ptr<IPlayer> const& player)
{
	//桌子指针
    table_ = table;
	assert(table_);
	//机器人对象
	robot_ = player;
	assert(robot_);
	return false;
}

//桌子指针
void CRobot::SetTable(std::shared_ptr<ITable> const& table)
{
	//桌子指针
	table_ = table;
	assert(table_);
	//ReadConfigInformation();
}

//用户指针
bool CRobot::SetPlayer(std::shared_ptr<IPlayer> const& player)
{
	//机器人对象
	robot_ = player;
	assert(robot_);
	return true;
}

//消息处理
bool CRobot::OnGameMessage(uint8_t subId, uint8_t const* data, size_t len) {
	switch (subId)
	{
	case s13s::SUB_S_ANDROID_CARD: {//机器人牌
		s13s::CMD_S_AndroidCard reqdata;
		if (!reqdata.ParseFromArray(data, len)) {
			return false;
		}
		//LOG(INFO) << "--- *** [" << reqdata.roundid() << "] CRobot 机器人牌 ...";
		for (int i = 0; i < reqdata.players_size(); ++i) {
			s13s::AndroidPlayer const& player = reqdata.players(i);
			int j = player.chairid();
			//memcpy(&(handCards_[j])[0], player.handcards().cards().data(), MAX_COUNT);
		}
		break;
	}
	case s13s::SUB_S_GAME_START: {//游戏开始
		s13s::CMD_S_GameStart reqdata;
		if (!reqdata.ParseFromArray(data, len)) {
			return false;
		}
		//LOG(INFO) << "--- *** [" << reqdata.roundid() << "] CRobot 游戏开始 ...";
		//牌局编号
		strRoundID_ = reqdata.roundid();
		//剩余时间
		groupTotalTime_ = reqdata.wtimeleft();
		//设置状态
		//robot_->SetUserStatus(sPlaying);
		//确定牌型的真人玩家数
		realPlayerCount = 0;
		//是否重置随机思考时长
		randWaitResetFlag_ = false;
		//累计思考时长
		totalWaitSeconds_ = 0;
		//分片思考时间
		sliceWaitSeconds_ = 0.2f;
		//随机思考时长(1.0~18.0s之间)
		randWaitSeconds_ = (ThisRoomId >= 6305) ?
			CalcWaitSecondsSpeed(ThisChairId, TIME_GAME_START_DELAY, true) :
			CalcWaitSeconds(ThisChairId, TIME_GAME_START_DELAY, true);
		//char msg[1024] = { 0 };
		//snprintf(msg, sizeof(msg), "--- *** [%s] 游戏开始，机器人 %d 随机思考时长(%.2f) ...", strRoundID_.c_str(), ThisChairId, randWaitSeconds_);
		//LOG(INFO) << msg;
		//理牌定时器
		ThisThreadTimer->cancel(timerIdThinking);
		timerIdThinking = ThisThreadTimer->runAfter(sliceWaitSeconds_, boost::bind(&CRobot::ThinkingTimerOver, this));
		break;
	}
	case s13s::SUB_S_MANUALCARDS: {//手动摆牌
		//LOG(INFO) << "--- *** [" << strRoundID_ << "] CRobot 手动摆牌 ...";
		break;
	}
	case s13s::SUB_S_MAKESUREDUNHANDTY: {//确定牌型
		s13s::CMD_S_MakesureDunHandTy reqdata;
		if (!reqdata.ParseFromArray(data, len)) {
			return false;
		}
		//座椅玩家有效
		if (!table_->ExistUser(reqdata.chairid())) {
			return false;
		}
		//机器人
		if (table_->IsRobot((uint32_t)reqdata.chairid()) > 0) {
			//LOG(INFO) << "--- *** [" << strRoundID_ << "] CRobot 机器人 " << reqdata.chairid() << " 确定牌型 ...";
		}
		else {
			//确定牌型的真人玩家数量
			++realPlayerCount;
			//LOG(INFO) << "--- *** [" << strRoundID_ << "] CRobot 玩家 " << reqdata.chairid() << " 确定牌型 ...";
		}
		break;
	}
	case s13s::SUB_S_CANCELCARDS: {//重置墩牌
		//LOG(INFO) << "--- *** [" << strRoundID_ << "] CRobot 重置墩牌 ...";
		break;
	}
	case s13s::SUB_S_GAME_END: {//游戏结束
		//LOG(INFO) << "--- *** [" << strRoundID_ << "] CRobot 游戏结束 ...";
		s13s::CMD_S_GameEnd GameEnd;
		if (!GameEnd.ParseFromArray(data, len)) {
			return false;
		}
		ClearAllTimer();
		Reposition();
		break;
	}
	default: break;
	}
	return true;
}

//随机思考时间
double CRobot::CalcWaitSeconds(uint16_t chairId, int32_t delay, bool isinit) {
	int j = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!table_->ExistUser(i)) {
			continue;
		}
		if (table_->IsRobot((uint32_t)i) > 0 && !randWaitResetFlag_) {
			++j;
		}
		if(chairId == i) {
			return isinit ?
				//随机思考时长(1.0~18.0s之间)
				delay + rand_.betweenInt(1, 15).randInt_mt() + 0.1f * rand_.betweenInt(5, 10).randInt_mt() * 1.5 * (j/* + 1*/):
				//随机思考时长(1.0~3.0s之间)
				delay + rand_.betweenInt(0,  1).randInt_mt() + 0.1f * rand_.betweenInt(5, 10).randInt_mt() * 1.5 * (j/* + 1*/);
		}
	}
}

//随机思考时间(极速房)
double CRobot::CalcWaitSecondsSpeed(uint16_t chairId, int32_t delay, bool isinit) {
	int j = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!table_->ExistUser(i)) {
			continue;
		}
		if (table_->IsRobot((uint32_t)i) > 0 && !randWaitResetFlag_) {
			++j;
		}
		if (chairId == i) {
			return isinit ?
				//随机思考时长(1.0~18.0s之间)
				delay + rand_.betweenInt(0, 1).randInt_mt() + 0.1f * rand_.betweenInt(5, 10).randInt_mt() * (j/* + 1*/) :
				//随机思考时长(1.0~3.0s之间)
				delay + /*rand_.betweenInt(0, 1) + */0.1f * rand_.betweenInt(5, 10).randInt_mt() * (j/* + 1*/);
		}
	}
}

//思考定时器
void CRobot::ThinkingTimerOver() {
	//char msg[1024] = { 0 };
	//snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 思考中(%.2f) ...", strRoundID_.c_str(), ThisChairId, randWaitSeconds_);
	//LOG(INFO) << msg;
	ThisThreadTimer->cancel(timerIdThinking);
	totalWaitSeconds_ += sliceWaitSeconds_;
	//思考时长不够
	if (totalWaitSeconds_ < randWaitSeconds_) {
		//所有真人玩家都确定牌型，减少AI思考时间
		if (!randWaitResetFlag_ &&
			realPlayerCount == table_->GetRealPlayerCount()) {
			//随机思考时长(1.0~3.0秒之间)
			resetWaitSeconds_ = (ThisRoomId >= 6305) ?
				CalcWaitSecondsSpeed(ThisChairId, 0, false) :
				CalcWaitSeconds(ThisChairId, 0, false);
			//累计思考时长 + 重置思考时长
			randWaitSeconds_ = totalWaitSeconds_ + resetWaitSeconds_;
			//重置随机思考时长
			randWaitResetFlag_ = true;
			//memset(msg, 0, sizeof(msg));
			//snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 重置随机思考时长(%.2f) ...", strRoundID_.c_str(), ThisChairId, resetWaitSeconds_);
			//LOG(INFO) << msg;
		}
		//重启思考定时器
		timerIdThinking = ThisThreadTimer->runAfter(sliceWaitSeconds_, boost::bind(&CRobot::ThinkingTimerOver, this));
	}
	else {
		//memset(msg, 0, sizeof(msg));
		//snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 理牌完成(%.2f) ...", strRoundID_.c_str(), ThisChairId, totalWaitSeconds_);
		//LOG(INFO) << msg;
		//机器人确定牌型
		s13s::CMD_C_MakesureDunHandTy rspdata;
		rspdata.set_groupindex(0);
        rspdata.set_isautoopertor(false);
        std::string content = rspdata.SerializeAsString();
        table_->OnGameEvent(ThisChairId, s13s::SUB_C_MAKESUREDUNHANDTY, (uint8_t *)content.data(), content.size());
    }
}

void CRobot::ClearAllTimer()
{
	ThisThreadTimer->cancel(timerIdThinking);
}

extern "C" std::shared_ptr<IRobotDelegate> CreateRobotDelegate() {
    return std::shared_ptr<IRobotDelegate>(new CRobot());
}

extern "C" void DeleteRobotDelegate(std::shared_ptr<IRobotDelegate>& robotDelegate) {
	robotDelegate.reset();
}
