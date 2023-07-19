#pragma once

#include "public/Inc.h"
#include "gameDefine.h"
#include "ITableDelegate.h"
#include "ITable.h"
#include "IPlayer.h"

#include "texas.h"

#define GAME_STATUS_INIT                        GameStatus::GAME_STATUS_INIT //初始状态
#define GAME_STATUS_READY						GameStatus::GAME_STATUS_FREE //空闲状态
#define GAME_STATUS_START						GameStatus::GAME_STATUS_START//进行状态
#define GAME_STATUS_END				    	    GameStatus::GAME_STATUS_END  //游戏结束

#define FloorScore		(table_->GetRoomInfo()->floorScore)//底注
#define CeilScore		(table_->GetRoomInfo()->ceilScore)//顶注
#define JettionList     (table_->GetRoomInfo()->jettons)//筹码表

#define ThisTableId		(table_->GetTableId())
#define ThisGameId		(table_->GetRoomInfo()->gameId)
#define ThisRoomId		(table_->GetRoomInfo()->roomId)
#define ThisRoomName	(table_->GetRoomInfo()->roomName)
#define ThisThreadTimer	(table_->GetLoopThread()->getLoop())

#define EnterMinScore (table_->GetRoomInfo()->enterMinScore)//进入最小分
#define EnterMaxScore (table_->GetRoomInfo()->enterMaxScore)//进入最大分
#define MIN_GAME_PLAYER (table_->GetRoomInfo()->minPlayerNum)

#define ByChairId(chairId)	(table_->GetChairPlayer(chairId))
#define ByUserId(userId)	(table_->GetPlayer(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

//#define TakeScoreByChairId(chairId) ByChairId(chairId)->GetCurTakeScore()
//#define TakeScoreByUserId(userId) ByUserId(userId)->GetCurTakeScore()

#define StockScore table_->GetRoomInfo()->totalStock//系统当前库存
#define StockLowLimit table_->GetRoomInfo()->totalStockLowerLimit//系统输分不得低于库存下限，否则赢分
#define StockHighLimit table_->GetRoomInfo()->totalStockHighLimit//系统赢分不得大于库存上限，否则输分

#define StockSecondLowLimit table_->GetRoomInfo()->totalStockSecondLowerLimit
#define StockSecondHighLimit table_->GetRoomInfo()->totalStockSecondHighLimit

//赋值tagSpecialScoreInfo基础数据
// #define SetScoreInfoBase(scoreInfo, x, p) { \
// 	if(!(p)) { \
// 		std::shared_ptr<IPlayer> userItem = ByChairId(x); \
// 		assert(userItem); \
// 		scoreInfo.chairId = (x); \
// 		scoreInfo.userId = userItem->GetUserId(); \
// 		scoreInfo.promoterId = userItem->GetPromoterId(); \
// 		scoreInfo.beforeScore = ScoreByChairId(x); \
// 		scoreInfo.bankScore = userItem->GetBankScore(); \
// 		scoreInfo.bIsAndroid = (userItem->IsRobot() > 0); \
// 	} \
// 	else { \
// 		assert(dynamic_cast<userinfo_t*>((userinfo_t*)(p))); \
// 		assert((x) == ((userinfo_t*)(p))->chairId); \
// 		scoreInfo.chairId = (x); \
// 		scoreInfo.userId = ((userinfo_t*)(p))->userId; \
// 		scoreInfo.promoterId = ((userinfo_t*)(p))->promoterId; \
// 		scoreInfo.beforeScore = ((userinfo_t*)(p))->userScore; \
// 		scoreInfo.bankScore = ((userinfo_t*)(p))->bankScore; \
// 		scoreInfo.bIsAndroid = (((userinfo_t*)(p))->IsRobot > 0); \
// 	} \
// }

#define SetScoreInfoBase(scoreInfo, x, p) { \
	if(!(p)) { \
		std::shared_ptr<IPlayer> userItem = ByChairId(x); \
		assert(userItem); \
		scoreInfo.chairId = (x); \
		scoreInfo.userId = userItem->GetUserId(); \
	} \
	else { \
		assert(dynamic_cast<userinfo_t*>((userinfo_t*)(p))); \
		assert((x) == ((userinfo_t*)(p))->chairId); \
		scoreInfo.chairId = (x); \
		scoreInfo.userId = ((userinfo_t*)(p))->userId; \
	} \
}

//配置文件
#define INI_FILENAME "./conf/Texas_config.ini"
//#define INI_CARDLIST "./conf/Texas_cardList.ini"

static std::string StringStat(uint8_t status) {
	switch (status) {
	case GAME_STATUS_INIT: return "eInit";
	case GAME_STATUS_READY: return "eReady";
	case GAME_STATUS_START: return "ePlaying";
		//case GAME_STATUS_GROUP: return "eGroup";
		//case GAME_STATUS_OPEN: return "eOpen";
	case GAME_STATUS_END: return "eEnd";
	}
	return "nil";
}

static std::string StringPlayerStat(uint8_t status) {
	switch (status) {
	case sGetout: return "sGetout";
	case sFree: return "sFree";
	case sStop: return "sStop";
	case sSit: return "sSit";
	case sReady: return "sReady";
	case sPlaying: return "sPlaying";
	case sOffline: return "sOffline";
	case sLookon: return "sLookon";
	//case sGetoutAtplaying: return "sGetoutAtplaying";
	}
	return "nil";
}

//操作类型
enum eOperate {
	OP_INVALID  = 0,	//无效
	OP_PASS     = 1,    //过牌
	OP_ALLIN    = 2,	//梭哈
	OP_FOLLOW   = 3,    //跟注
	OP_ADD      = 4,    //加注
	OP_GIVEUP   = 5,    //弃牌
	OP_LOOKOVER = 6,    //看牌
	OP_MAX,
};

//游戏流程
class CGameTable : public ITableDelegate {
	//各玩家投注结构
	struct bet_t {
		bet_t() {
			Reset();
		}
		bet_t(uint32_t id, int64_t score) :id(id), score(score) {
		}
		void Reset() {
			id = -1;
			score = 0;
		}
		bet_t(bet_t const& ref) {
			id = ref.id;
			score = ref.score;
		}
		bet_t& operator=(bet_t const& ref) {
			id = ref.id;
			score = ref.score;
		}
		uint32_t id;
		int64_t score;
	};
	//主(底)池、边池结构
	struct pot_t {
		pot_t() {
			reset();
		}
		void reset() {
			score = 0;
			ids.clear();
		}
		//奖池积分
		int64_t score;
		//参与人数
		std::set<uint32_t> ids;
	};
public:
    CGameTable(void);
    ~CGameTable(void);
public:
	virtual std::string GetRoundId();
	//游戏开始
	virtual void OnGameStart();
	//游戏结束
	virtual bool OnGameConclude(uint32_t chairId, uint8_t flags);
	//发送场景
	virtual bool OnGameScene(uint32_t chairId, bool lookon);
	//游戏消息
	virtual bool OnGameMessage(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len);
	//用户进入
	virtual bool OnUserEnter(int64_t userId, bool lookon);
	//用户准备
	virtual bool OnUserReady(int64_t userId, bool lookon);
	//用户离开
	virtual bool OnUserLeft(int64_t userId, bool lookon);
	//能否加入
	virtual bool CanJoinTable(std::shared_ptr<IPlayer> const& player);
	//梭哈、德州使用当前携带金币
	virtual bool CanJionTableSpecial(int64_t userId, int64_t score);
	//能否离开
	virtual bool CanLeftTable(int64_t userId);
	//设置指针
	virtual bool SetTable(std::shared_ptr<ITable> const& table);
	//复位桌子
	virtual void Reposition();
private:
	int randomMaxAndroidCount();
	void BroadcastTakeScore(uint32_t chairId, int64_t userId);
	//计算机器人税收
	int64_t CalculateAndroidRevenue(int64_t score);
    //清理游戏数据
    void ClearGameData();
    //初始化游戏数据
    void InitGameData();
    //清除所有定时器
    void ClearAllTimer();
	//准备定时器
	void OnTimerGameReadyOver();
	//结束准备，开始游戏
	void GameTimerReadyOver();
	//游戏开局前检查
	void CheckGameStart();
	//换牌策略分析
	//收分时概率性玩家拿小牌，AI拿大牌
	//放水时概率性AI拿小牌，玩家拿大牌
	void AnalysePlayerCards();
	//让系统赢或输
	void LetSysWin(bool sysWin);
	//输出玩家手牌/对局日志初始化
	void ListPlayerCards();
	//游戏结束，清理数据
    void OnTimerGameEnd();
	//初始化玩家当前携带
	void initUserTakeScore(std::shared_ptr<IPlayer> player);
	//更新玩家当前携带
	void upateUserTakeScore(std::shared_ptr<IPlayer> player);
	//踢人用户清理
	void clearKickUsers();
	//读取库存配置
	void ReadConfigInformation();
private:
	//累计匹配时长
	double totalMatchSeconds_;
	//分片匹配时长(可配置)，比如0.1s做一次检查
	double sliceMatchSeconds_;
	//匹配真人超时时长(可配置)
	double timeoutMatchSeconds_;
	//补充机器人超时时长(可配置)
	double timeoutAddAndroidSeconds_;
private:
	//拼接各玩家手牌cardValue
	std::string StringCardValue(bool flag = false);
	//判断当轮操作
	bool hasOperate(int k, eOperate opcode);
	//判断是否梭哈
	bool hasAllIn();
	bool hasAllIn(uint32_t chairId);
	//能否过牌操作
	bool canPassScore(uint32_t chairId);
	//能否跟注操作
	bool canFollowScore(uint32_t chairId);
	//能否加注操作
	bool canAddScore(uint32_t chairId);
	//能否梭哈操作
	bool canAllIn(uint32_t chairId);
	//可操作玩家数
	int canOptPlayerCount();
	//剩余游戏人数
	int leftPlayerCount(bool includeAndroid = true, uint32_t* currentUser = NULL);
	//判断当轮是否完成
	bool checkFinished(int k);
	//下一个操作用户
	uint32_t GetNextUser(bool& newflag, bool& exceed);
	//下一个操作用户，从小盲注开始查找
	uint32_t GetNextUserBySmallBlindUser();
	//最近一轮加注分
	int64_t GetLastAddScore(uint32_t chairId);
	//最近一轮
	int LastTurn();
	//有效当前轮
	int CurrentTurn();
	//筹码表可加注筹码范围[minIndex, maxIndex]
	bool GetCurrentAddRange(int64_t minAddScore, int64_t userScore, int& minIndex, int& maxIndex, int64_t& deltaScore);
	//最小加注分
	int64_t MinAddScore(uint32_t chairId);
	//当前跟注分
	int64_t CurrentFollowScore(uint32_t chairId);
	//跟注参照用户
	void updateReferenceUser(uint32_t chairId, eOperate op);
	//加注递增量
	void updateDeltaAddScore(uint32_t chairId, int64_t addScore, eOperate op);
	//计算主(底)池，边池
	void updateMainSidePots(uint32_t chairId = INVALID_CHAIR, int64_t addScore = 0);
	void updateMainSidePots(std::map<int, pot_t>& pots);
	//积分转换成筹码
	void addScoreToChips(uint32_t chairId, int64_t score, std::map<int, int64_t>& chips);
	//结算筹码
	void settleChips(int64_t score, std::map<int, int64_t>& chips);
private:
	//等待操作定时器
	void OnTimerWaitingOver();
	//操作错误通知消息
	void SendNotify(uint32_t chairId, int opcode, std::string const& errmsg);
	//新一轮开始发牌
	void SendCard(int left);
	void SendCardOnTimer(int left);
	//用户过牌
	bool OnUserPassScore(uint32_t chairId);
	//用户梭哈
	bool OnUserAllIn(uint32_t chairId);
	//用户跟注/加注
	bool OnUserAddScore(uint32_t chairId, int opValue, int64_t addScore);
	//用户弃牌
	bool OnUserGiveUp(uint32_t chairId, bool timeout = false);
	//用户看牌
	bool OnUserLookCard(uint32_t chairId);
protected:
	//打印真人游戏局log
	bool writeRealLog_;
	time_t lastReadCfgTime_;
	int readIntervalTime_;//更新配置间隔时间
	//空闲踢人间隔时间
	int kickPlayerIdleTime_;
	STD::Random rand_;
	int maxAndroid_;
	//牌局编号
	string strRoundID_;
	int64_t roundId_;
	//游戏逻辑
	TEXAS::CGameLogic g;
	//桌面扑克(翻牌3/转牌1/河牌1)
	uint8_t tableCards_[TBL_CARDS];
	//玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	uint8_t combCards_[GAME_PLAYER][MAX_TOTAL];
	//玩家牌型分析结果
	TEXAS::CGameLogic::handinfo_t all_[GAME_PLAYER], cover_[GAME_PLAYER];
	int offset_;
	int cardsC_[GAME_PLAYER];
	//本局开始时间/本局结束时间
	std::chrono::system_clock::time_point roundStartTime_;
	std::chrono::system_clock::time_point roundEndTime_;
	//庄家用户
	uint32_t bankerUser_;
	//小盲注玩家
	uint32_t smallBlindUser_;
	//大盲注玩家
	uint32_t bigBlindUser_;
	//下一个操作用户
	uint32_t nextUser_;
	//操作用户
	uint32_t currentUser_;
	//跟注参照用户
	uint32_t referenceUser_;
	//首发用户
	uint32_t firstUser_;
	//当前最大牌用户
	uint32_t currentWinUser_;
	//最终赢家列表
	std::vector<uint32_t> winUsers_;
	//当前第几轮(0,1,2,3,...)
	int currentTurn_;
	//是否新的一轮
	bool isNewTurn_;
	//是否超出轮数
	bool isExceed_;
	//记录每轮玩家操作
	eOperate operate_[MAX_ROUND][GAME_PLAYER];
	int64_t addScore_[MAX_ROUND][GAME_PLAYER];
	//记录玩家梭哈操作
	int allInTurn_[GAME_PLAYER];
	//主(底)池：只有一个 边池：可能没有或很多个
	std::map<int, pot_t> pots_, potsT_, showPots_;
	//struct opInfo_t {
	//	uint32_t chairId;
	//	eOperate op;
	//	int64_t score_;
	//};
	//std::vector<opInfo_t> opInfo_[MAX_ROUND];
	//操作总的时间/剩余时间
	uint32_t opTotalTime_;
	//操作开始时间
	uint32_t opStartTime_;
	//操作结束时间
	uint32_t opEndTime_;
private:
	uint8_t gameStatus_;
	//提示余额不够自动补充，请尽快充值
	bool needTip_[GAME_PLAYER];
	bool setscore_[GAME_PLAYER];
	//是否看牌
	bool isLooked_[GAME_PLAYER];
	//是否弃牌
	bool isGiveup_[GAME_PLAYER];
	//是否比牌输
	bool isLost_[GAME_PLAYER];
	//是否比过牌
	bool isCompared_[GAME_PLAYER];
	//防超时弃牌
	bool noGiveUpTimeout_[GAME_PLAYER];
	//是否展示牌
	bool isShowComparedCards_;
private:
	int64_t cfgTakeScore_[2][GAME_PLAYER];
	//玩家携带积分
	int64_t takeScore_[GAME_PLAYER];
	//玩家当局输赢(扣完税收)
	int64_t userWinScorePure_[GAME_PLAYER];
	//加注递增量
	int64_t deltaAddScore_;
	//各玩家下注
	int64_t tableScore_[GAME_PLAYER];
	//桌面总下注
	int64_t tableAllScore_;
	//各玩家下注筹码
	std::map<int, int64_t> tableChip_[GAME_PLAYER];
	//桌面总下注筹码
	std::map<int, int64_t> tableAllChip_, settleChip_;
	//桌面显示筹码配置
	std::vector<int> chipList_;
private:
	enum {
		Exchange = 0,	//换牌
		NoExchange,		//不换
		MaxExchange,
	};
private:
	//准备定时器
	muduo::net::TimerId timerIdGameReadyOver_;
	//等待操作定时器
	muduo::net::TimerId timerIdWaiting_;
	//结束定时器，清理数据
	muduo::net::TimerId timerIdGameEnd_;
	muduo::net::TimerId timerIdSendCard_;
	muduo::net::TimerId timerIdLookCardNotify_;
	//桌子指针
	std::shared_ptr<ITable> table_;
	//造牌配置文件路径
	std::string INI_CARDLIST_;
	//对局日志
	tagGameReplay m_replay;
	//玩家出局(弃牌/比牌输)，若离开房间，信息将被清理，用户数据副本用来写记录
	struct userinfo_t {
		//不用构造，防止被隐式析构
		void init() {
			chairId = INVALID_CHAIR;
			userId = 0;
			headerId = 0;
			vipLevel = 0;
			headboxId = 0;
			promoterId = 0;
			bankScore = 0;
			takeScore = 0;
			userScore = 0;
			IsRobot = false;
			IsOfficial = false;
			isLeave = false;
		}
		uint32_t chairId;
		int64_t userId;
		uint8_t headerId;
		std::string nickName;
		std::string location;
		std::string headImgUrl;
		uint8_t vipLevel;
		uint8_t headboxId;
		int64_t promoterId;
		int64_t bankScore;
		int64_t takeScore;
		//携带积分
		int64_t userScore;
		//是否机器人
		int8_t IsRobot;
		//是否官方账号
		int8_t IsOfficial;
		bool isLeave;
	};
	typedef std::map<int64_t, userinfo_t> UserInfoList;
	UserInfoList userinfos_;
	inline int64_t GetUserId(uint32_t chairId) {
		for (UserInfoList::const_iterator it = userinfos_.begin();
			it != userinfos_.end(); ++it) {
			if (it->second.chairId == chairId) {
				return it->first;
			}
		}
		return 0;
	}
	//开启随时看牌
	bool freeLookover_;
	//开启随时弃牌
	bool freeGiveup_;
	//游戏结束是否全部摊牌
	bool openAllCards_;
	//是否参与游戏
	bool bPlaying_[GAME_PLAYER];
private:
	//调试模式不写分
	bool debug_, together_, take_;
	int64_t m_lMarqueeMinScore; // 跑马灯最小分值
	bool m_bPlayerOperated[GAME_PLAYER]; //本局是否有操作
	bool m_bPlayerCanOpt[GAME_PLAYER];
	bool m_bRoundEndExit[GAME_PLAYER]; //本局结束后退出
	bool m_bShowCard[GAME_PLAYER]; //结算时玩家是否明牌开关
	int32_t m_i32LowerChangeRate;
	int32_t m_i32HigherChangeRate;
	std::set<int64_t> m_GameParticipant; //游戏参与者
};
