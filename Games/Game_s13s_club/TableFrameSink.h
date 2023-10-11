#pragma once

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "public/errorCode.h"

#include "ITableDelegate.h"
#include "ITable.h"
#include "IPlayer.h"

#include "s13s.h"

#define GAME_STATUS_INIT                        GameStatus::GAME_STATUS_INIT //初始状态
#define GAME_STATUS_READY						GameStatus::GAME_STATUS_FREE //空闲状态
#define GAME_STATUS_START						GameStatus::GAME_STATUS_START//进行状态
#define GAME_STATUS_GROUP						(GAME_STATUS_START+1) //理牌状态
#define GAME_STATUS_OPEN						(GAME_STATUS_START+2) //开牌状态
#define GAME_STATUS_PREEND						(GAME_STATUS_START+3) //结束前状态
#define GAME_STATUS_END				    	    GameStatus::GAME_STATUS_END  //游戏结束
#define GAME_STATUS_NEXT			            (GAME_STATUS_END+1) //下一局

#include "AnimatePlay.h"

#define FloorScore		(table_->GetRoomInfo()->floorScore)//底注
#define CeilScore		(table_->GetRoomInfo()->ceilScore)//顶注
#define CellScore		(FloorScore)//底注
#define JettionList     (table_->GetRoomInfo()->jettons)//筹码表

#define ThisTableId		(table_->GetTableId())
#define ThisGameId		(table_->GetRoomInfo()->gameId)
#define ThisRoomId		(table_->GetRoomInfo()->roomId)
#define ThisRoomName	(table_->GetRoomInfo()->roomName)
#define ThisThreadTimer	(table_->GetLoop())

#define EnterMinScore (table_->GetRoomInfo()->enterMinScore)//进入最小分
#define EnterMaxScore (table_->GetRoomInfo()->enterMaxScore)//进入最大分

#define ByChairId(chairId)	(table_->GetChairPlayer(chairId))
#define ByUserId(userId)	(table_->GetPlayer(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

#define StockScore table_->GetRoomInfo()->totalStock//系统当前库存
#define StockLowLimit table_->GetRoomInfo()->totalStockLowerLimit//系统输分不得低于库存下限，否则赢分
#define StockHighLimit table_->GetRoomInfo()->totalStockHighLimit//系统赢分不得大于库存上限，否则输分

#define StockSecondLowLimit table_->GetRoomInfo()->totalStockSecondLowerLimit
#define StockSecondHighLimit table_->GetRoomInfo()->totalStockSecondHighLimit

//配置文件
#define INI_FILENAME "./conf/s13s_config.ini"
//#define INI_CARDLIST "./conf/s13s_cardList.ini"

static std::string StringStat(uint8_t status) {
	switch (status) {
	case GAME_STATUS_INIT: return "eInit";
	case GAME_STATUS_READY: return "eReady";
	case GAME_STATUS_START: return "ePlaying";
	case GAME_STATUS_GROUP: return "eGroup";
	case GAME_STATUS_OPEN: return "eOpen";
	case GAME_STATUS_PREEND:
	case GAME_STATUS_END: return "eEnd";
	}
	return "nil";
}

static std::string StringPlayerStat(uint8_t status) {
	switch (status) {
	case sGetout: return "sGetout";
	case sFree: return "sFree";
	//case sStop: return "sStop";
	case sSit: return "sSit";
	case sReady: return "sReady";
	case sPlaying: return "sPlaying";
	case sOffline: return "sOffline";
	case sLookon: return "sLookon";
	//case sGetoutAtplaying: return "sGetoutAtplaying";
	}
	return "nil";
}

class CGameTable : public ITableDelegate {
public:
    CGameTable(void);
    ~CGameTable(void);
public:
	virtual std::string GetRoundId();
	virtual std::string GetGameStatusStr();
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnGameConclude(uint16_t chairId, uint8_t flags);
    //发送场景
    virtual bool OnGameScene(uint16_t chairId, bool lookon);
    //游戏消息
    virtual bool OnGameMessage(uint16_t chairId, uint8_t subId, uint8_t const* data, size_t len);
    //用户进入
    virtual bool OnUserEnter(int64_t userId, bool lookon);
    //用户准备
    virtual bool OnUserReady(int64_t userId, bool lookon);
    //用户离开
    virtual bool OnUserLeft(int64_t userId, bool lookon);
    //能否加入
    virtual bool CanJoinTable(std::shared_ptr<IPlayer> const& player);
    //能否离开
    virtual bool CanLeftTable(int64_t userId);
    //设置指针
    virtual bool SetTable(std::shared_ptr<ITable> const& table);
	//复位桌子
    virtual void Reposition() ;
private:
	int randomMaxAndroidCount();
	//计算机器人税收
	//int64_t CalculateAndroidRevenue(int64_t score);
    //清理游戏数据
    virtual void ClearGameData();
    //初始化游戏数据
    void InitGameData();
    //清除所有定时器
    inline void ClearAllTimer();
	//理牌
	void OnTimerGroupCard();
	//定牌
	void OnUserSelect(uint16_t chairId, int groupIndex, bool timeout = false);
	//摊牌
	void OnTimerOpenCard();
	//结束/下一局
    void OnTimerGameEnd();
	//踢人用户清理
	void clearKickUsers();
    //读取库存配置
    void ReadConfigInformation();
	//换牌策略分析
	//收分时概率性玩家拿小牌，AI拿大牌
	//放水时概率性AI拿小牌，玩家拿大牌
	void AnalysePlayerCards();
	//让系统赢或输
	void LetSysWin(bool sysWin);
	//玩家之间两两比牌
	void StartCompareCards();
private:
	//累计匹配时长
	double totalMatchSeconds_;
	//分片匹配时长(可配置)，比如0.1s做一次检查
	double sliceMatchSeconds_;
	//匹配真人超时时长(可配置)
	double timeoutMatchSeconds_;
	//补充机器人超时时长(可配置)
	double timeoutAddAndroidSeconds_;
	//准备定时器
	void OnTimerGameReadyOver();
	//结束准备，开始游戏
	void GameTimerReadyOver();
	//游戏开局前检查
	void CheckGameStart();
	//设置托管，理牌/摊牌/结算
	bool IsTrustee(void);
protected:
	//打印真人游戏局log
	bool writeRealLog_;
	time_t lastReadCfgTime_;
	int readIntervalTime_;//更新配置间隔时间
	//空闲踢人间隔时间
	int kickPlayerIdleTime_;
	STD::Random rand_;
	int maxAndroid_;
protected:
	uint8_t gameStatus_;
	//牌局编号
	std::string strRoundID_;
	//游戏逻辑
	S13S::CGameLogic g;
	//各个玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//玩家牌型分析结果
	S13S::CGameLogic::handinfo_t handInfos_[GAME_PLAYER];
	//直接使用
	S13S::CGameLogic::handinfo_t* phandInfos_[GAME_PLAYER];
	//各个玩家比牌结果
	s13s::CMD_S_CompareCards compareCards_[GAME_PLAYER];
	//各个玩家输赢积分
	s13s::GameEndScore score_[GAME_PLAYER];
	//彼此相对输赢/[i]输赢[j]得水
	int eachWinLostScore_[GAME_PLAYER][GAME_PLAYER];
	//本局开始时间/本局结束时间
	std::chrono::system_clock::time_point roundStartTime_;
	std::chrono::system_clock::time_point roundEndTime_;
	uint32_t groupTime_;//理牌时间
	uint32_t openCardTime_;//开牌比牌时间
	uint32_t shootTime_;//打枪/全垒打时间
	uint32_t winLostTime_;//结算飘金币时间
	uint32_t nextTime_;//下一局倒计时间
	//确定牌型的玩家数
	int selectcount;
    //内部使用玩家状态，false:无效/中途加入桌子
    bool bPlaying_[GAME_PLAYER];
	enum {
		Exchange = 0,	//换牌
		NoExchange,		//不换
		MaxExchange,
	};
	//对局日志
	tagGameReplay m_replay;
	AnimatePlay aniPlay_;
	//桌子指针
	std::shared_ptr<ITable> table_;
	//准备定时器
	muduo::net::TimerId timerIdGameReadyOver_;
	//理牌定时器
	muduo::net::TimerId timerGroupID_;
	//摊牌定时器
	muduo::net::TimerId timerOpenCardID_;
	//结束定时器
	muduo::net::TimerId timerGameEndID_;
	muduo::net::TimerId timerIdReadConfig_;

    int64_t m_lMarqueeMinScore; // 跑马灯最小分值
	bool m_bPlayerOperated[GAME_PLAYER]; //本局是否有操作
    bool m_bRoundEndExit[GAME_PLAYER]; //本局结束后退出

	int32_t m_i32LowerChangeRate;
	int32_t m_i32HigherChangeRate;
};
