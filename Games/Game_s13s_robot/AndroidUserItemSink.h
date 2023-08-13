#pragma once

#include "public/Inc.h"
#include "pbulic/gameConst.h"
#include "public/gameStruct.h"

#include "IRobotDelegate.h"
#include "ITable.h"
#include "IPlayer.h"

#define FloorScore		(table_->GetRoomInfo()->floorScore)
#define CeilScore		(table_->GetRoomInfo()->ceilScore)
#define CellScore		(FloorScore)

#define ThisTableId		(table_->GetTableId())
#define ThisRoomId		(table_->GetRoomInfo()->roomId)
#define ThisRoomName	(table_->GetRoomInfo()->roomName)
#define ThisThreadTimer	(table_->GetLoopThread()->getLoop())

#define ThisChairId		(robot_->GetChairId())
#define ThisUserId		(robot_->GetUserId())

class CRobot : public IRobotDelegate {
public:
	CRobot();
	virtual ~CRobot();
public:
	//桌子重置
	virtual bool Reposition();
	//桌子初始化
	virtual bool Init(std::shared_ptr<ITable> const& table, std::shared_ptr<IPlayer> const& player);
	//桌子指针
	virtual void SetTable(std::shared_ptr<ITable> const& table);
	//用户指针
	virtual bool SetPlayer(std::shared_ptr<IPlayer> const& player);
	//消息处理
	virtual bool OnGameMessage(uint8_t subId, uint8_t const* data, size_t len);
	//机器人策略
	virtual void SetStrategy(tagAndroidStrategyParam* strategy) { strategy_ = strategy; }
	virtual tagAndroidStrategyParam* GetStrategy() { return strategy_; }
protected:
	//随机思考时间
	double CalcWaitSeconds(uint32_t chairId, int32_t delay = 0, bool isinit = false);
	//随机思考时间(极速房)
	double CalcWaitSecondsSpeed(uint32_t chairId, int32_t delay = 0, bool isinit = false);
	//思考定时器
	void ThinkingTimerOver();
	//清理所有定时器
	void ClearAllTimer();
protected:
	//累计思考时长
	double totalWaitSeconds_;
	//分片思考时长
	double sliceWaitSeconds_;
	//随机思考时长(1.0~18.0s之间)
	double randWaitSeconds_;
	//所有真人玩家理牌完毕，重置机器人随机思考时长，
	//若机器人随机思考时间太长的话，可以缩短随机思考时长
	double resetWaitSeconds_;
	//是否重置随机思考时长
	bool randWaitResetFlag_;
	STD::Random rand_;
private:
	//牌局编号
	std::string strRoundID_;
	//游戏逻辑
	//S13S::CGameLogic g;
	//各个玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//玩家牌型分析结果
	//S13S::CGameLogic::handinfo_t handInfos_[GAME_PLAYER];
	//理牌总的时间/理牌剩余时间
	uint32_t groupTotalTime_;
	//理牌结束时间
	uint32_t groupEndTime_;
	//确定牌型的真人玩家数
	int realPlayerCount;
	//桌子指针
	std::shared_ptr<ITable> table_;
	//机器人对象
    std::shared_ptr<IPlayer> robot_;
	//理牌定时器
	muduo::net::TimerId timerIdThinking;
	//定时器指针
	std::shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;
	//机器人策略
	tagAndroidStrategyParam* strategy_;
};
