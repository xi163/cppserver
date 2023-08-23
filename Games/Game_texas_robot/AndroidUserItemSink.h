#pragma once

#ifndef isZero
#define isZero(a)        (((a)>-0.000001) && ((a)<0.000001))
#endif//isZero

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)//底注
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)//顶注
#define JettionList     (m_pTableFrame->GetGameRoomInfo()->jettons)//筹码表

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

#define ThisChairId		(m_pAndroidUserItem->GetChairId())
#define ThisUserId		(m_pAndroidUserItem->GetUserId())
#define ThisUserScore	(m_pAndroidUserItem->GetUserScore())

#define ByChairId(chairId)	(m_pTableFrame->GetTableUserItem(chairId))
#define ByUserId(userId)	(m_pTableFrame->GetUserItemByUserId(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

//#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
//#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

//#define TakeScoreByChairId(chairId) ByChairId(chairId)->GetCurTakeScore()
//#define TakeScoreByUserId(userId) ByUserId(userId)->GetCurTakeScore()

#define StockScore m_pTableFrame->GetGameRoomInfo()->totalStock//(storageInfo_.i64CurStock) //系统当前库存
#define StockLowLimit m_pTableFrame->GetGameRoomInfo()->totalStockLowerLimit//(storageInfo_.i64LowestStock)//系统输分不得低于库存下限，否则赢分
#define StockHighLimit m_pTableFrame->GetGameRoomInfo()->totalStockHighLimit//(storageInfo_.i64HighestStock)//系统赢分不得大于库存上限，否则输分

//操作类型
enum eOperate {
	OP_INVALID  = 0,	//无效
	OP_PASS		= 1,    //过牌
	OP_ALLIN	= 2,	//梭哈
	OP_FOLLOW	= 3,    //跟注
	OP_ADD		= 4,    //加注
	OP_GIVEUP	= 5,    //弃牌
	OP_LOOKOVER = 6,    //看牌
	OP_MAX,
};

enum eEstimateTy {
	EstimateWin,	//当前最大牌用户
	EstimateRealWin,//真人最大牌用户
};

//不同库存下机器人操作概率
enum eStockStatus
{
	Stock_Ceil   = 0,//放水
	Stock_Normal = 1,//正常
	Stock_Floor  = 2,//收分
	Stock_Sataus_Max
};

enum eWinLost {
	State_Lost   = 0,//当前机器人输
	State_Win    = 1,//当前机器人赢
	Win_Lost_Max = 2,
};

#define MASK_PASS   0x01 //不能过牌
#define MASK_ALLIN  0x02 //不能梭哈
#define MASK_FOLLOW 0x04 //不能跟注
#define MASK_ADD    0x08 //不能加注
#define MASK_GIVEUP 0x10 //不能弃牌

class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
	virtual ~CAndroidUserItemSink();
public:
	//桌子重置
	virtual bool RepositionSink();
	//桌子初始化
	virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
	//用户指针
	virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
	//桌子指针
	virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//消息处理
	virtual bool OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize);
	//机器人策略
	virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy) { }
	virtual tagAndroidStrategyParam* GetAndroidStrategy() { return NULL; }
protected:
	//随机等待时间
	double randWaitSeconds(uint16_t chairId, int32_t delay = 0, int32_t wTimeLeft = -1);
	//等待操作定时器
	void onTimerWaitingOver();
	//离开桌子定时器
	void onTimerAndroidLeave();
	//清理所有定时器
	void ClearAllTimer();
	//推断当前最大牌用户
	bool EstimateWinner(eEstimateTy ty = EstimateWin);
	//推断当前操作
	bool EstimateOperate();
	//等待机器人操作
	void waitingOperate(int32_t delay = 0, int32_t wTimeLeft = -1);
	//读取机器人配置
	void ReadConfig();
protected:
	//累计等待时长
	//double totalWaitSeconds_;
	//分片等待时长
	double sliceWaitSeconds_;
	//随机等待时长
	double randWaitSeconds_;
	//重置机器人等待时长
	//double resetWaitSeconds_;
	//是否已重置等待时长
	//bool randWaitResetFlag_;
protected:
	//中途加入 - 服务端回复
	bool onAndroidEnter(const void* pBuffer, int32_t wDataSize);
	//机器人牌 - 服务端广播
	bool onAndroidCard(const void * pBuffer, int32_t wDataSize);
	//游戏开始 - 服务端广播
	bool onGameStart(const void * pBuffer, int32_t wDataSize);
	//过牌结果 - 服务端回复
	bool resultPass(const void* pBuffer, int32_t wDataSize);
	//梭哈结果 - 服务端回复
	bool resultAllIn(const void* pBuffer, int32_t wDataSize);
	//跟注/加注结果 - 服务端回复
	bool resultFollowAdd(const void * pBuffer, int32_t wDataSize);
	//弃牌结果 - 服务端回复
	bool resultGiveup(const void* pBuffer, int32_t wDataSize);
	//看牌结果 - 服务端回复
	bool resultLookCard(const void* pBuffer, int32_t wDataSize);
	//发牌结果 - 服务端回复
	bool resultSendCard(const void* pBuffer, int32_t wDataSize);
	//游戏结束 - 服务端广播
	bool onGameEnd(const void * pBuffer, int32_t wDataSize);
public:
	//请求操作
	bool sendOperateReq();
	//请求过牌
	void sendPassReq();
	//请求梭哈
	void sendAllInReq();
	//请求跟注
	void sendFollowReq();
	//请求加注
	void sendAddReq();
	//请求弃牌
	void sendGiveupReq();
	//请求看牌
	void sendLookCardReq();
	void sendLookCardTimerOver();
private:
	//桌面扑克(翻牌3/转牌1/河牌1)
	uint8_t tableCards_[TBL_CARDS];
	//玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	uint8_t combCards_[GAME_PLAYER][MAX_TOTAL];
	//玩家牌型分析结果
	TEXAS::CGameLogic::handinfo_t all_[GAME_PLAYER], cover_[GAME_PLAYER], public_;
	TEXAS::HandTy handTy_;
	int offset_;
	//牌局编号
	std::string strRoundID_;
	//是机器人
	bool isAndroidUser_[GAME_PLAYER];
	//是否官方账号
	bool isSystemUser_[GAME_PLAYER];
	//操作用户
	uint32_t currentUser_;
	//当前最大牌用户
	uint32_t saveWinUser_, currentWinUser_;
	//当前第几轮(0,1,2,3,...)
	int32_t currentTurn_;
	//操作总的时间/剩余时间
	uint32_t opTotalTime_;
	//操作开始时间
	uint32_t opStartTime_;
	//操作结束时间
	uint32_t opEndTime_;
	//是否看牌
	bool isLooked_[MAX_ROUND][GAME_PLAYER];
	//看牌时间
	time_t lookCardTime_[GAME_PLAYER];
	double dtLookCard_[GAME_PLAYER];
	//操作时间
	time_t operateTime_[GAME_PLAYER];
	double dtOperate_[GAME_PLAYER];
	//是否弃牌
	bool isGiveup_[GAME_PLAYER];
	//是否比牌输
	bool isLost_[GAME_PLAYER];
	//桌子指针
	shared_ptr<ITableFrame> m_pTableFrame;
	//机器人对象
	shared_ptr<IServerUserItem>	m_pAndroidUserItem;
	//等待定时器
	muduo::net::TimerId timerIdWaiting_;
	muduo::net::TimerId timerIdLookCard_;
	muduo::net::TimerId timerIdLeave_;
	//机器人操作
	eOperate currentOp_;
	//开启随时看牌
	bool freeLookover_;
	//开启随时弃牌
	bool freeGiveup_;
	//是否参与游戏
	bool bPlaying_[GAME_PLAYER];
	//各玩家下注
	int64_t tableScore_[GAME_PLAYER];
	//桌面总下注
	int64_t tableAllScore_;
	//最小加注分
	int64_t minAddScore_;
	//当前跟注分
	int64_t followScore_;
	//玩家携带积分
	int64_t takeScore_[GAME_PLAYER];
	//操作使能，能否过牌/跟注/加注/梭哈
	bool can_[OP_MAX];
	//加注筹码索引范围
	int minIndex_, maxIndex_;
	//可加注筹码
	std::vector <int64_t> jettons_;
	//概率计算
	STD::Random rand_, rand_op_;
	static muduo::AtomicInt32 int32_;
	//过牌/梭哈/跟注/加注/弃牌/看牌
	typedef int prob_t[Win_Lost_Max][Stock_Sataus_Max][MAX_ROUND][OP_MAX];
	//排除非使能操作权重后的随机概率
	typedef STD::Weight weight_t[Win_Lost_Max][Stock_Sataus_Max][MAX_ROUND];
	static std::map<int, weight_t> weights_;
	//static STD::Weight weightLookCard_[Win_Lost_Max][Stock_Sataus_Max][MAX_ROUND];
private:
	//排除非使能操作权重后的随机概率，非使能操作不能参与概率随机
	void initWeightMatrix(prob_t& prob);
	//剩余游戏人数
	int leftPlayerCount(bool includeAndroid = true, uint32_t* currentUser = NULL);
	//返回有效用户
	uint32_t GetNextUser(uint32_t startUser, bool ignoreCurrentUser = true);
	//是否自己赢家
	inline bool IsSelfWinner() {
		assert(currentWinUser_ != INVALID_CHAIR);
		return currentWinUser_ == ThisChairId;
	}
	//是否平台赢家
	inline bool IsPlatformWinner() {
		assert(currentWinUser_ != INVALID_CHAIR);
		return isAndroidUser_[currentWinUser_] || isSystemUser_[currentWinUser_];
	}
	//随机加注分
	inline int64_t RandomAddScore(uint16_t chairId) {
		assert(chairId == currentUser_);
		assert(can_[OP_ADD]);
		assert(minAddScore_ >= FloorScore);
		assert(minAddScore_ + tableScore_[currentUser_] < takeScore_[currentUser_]);
		int64_t userScore = takeScore_[currentUser_] - tableScore_[currentUser_];
		assert(userScore > 0);
		if (jettons_.size() > 0) {
			assert(minIndex_ >= 0 && maxIndex_ >= minIndex_);
			int index = rand_.betweenInt(minIndex_, maxIndex_).randInt_mt();
			assert(index >= minIndex_ && index <= maxIndex_);
			assert(index >= 0 && index < jettons_.size());
			int64_t addScore = jettons_[index];
			assert(addScore >= minAddScore_);
			assert(addScore + tableScore_[currentUser_] < takeScore_[currentUser_]);
			return addScore;
		}
		else {
			int64_t addScore = minAddScore_;
			return addScore;
		}
		return 0;
	}
};