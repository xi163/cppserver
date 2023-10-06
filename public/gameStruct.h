#ifndef INCLUDE_GAMESTRUCT_H
#define INCLUDE_GAMESTRUCT_H

#include "Logger/src/time/time.h"
#include "gameConst.h"

struct UserClubInfo {
	int64_t userId;
	int64_t clubId;//俱乐部Id 当玩家userId与clubId相同时为盟主
	int64_t superiorId;//我的上级推广代理
	int32_t iconId;//俱乐部图标
	std::string clubName;//俱乐部名称
	int status;//1:会员 2:合伙人 3.盟主
	int clubStatus;//0-未启用 1-活动 2-弃用 3-禁用
	uint32_t invitationCode;//邀请码 会员:0 合伙人或盟主 8位数邀请码
	uint32_t playerNum;//俱乐部人数
	int ratio;//提成比例 会员:0 合伙人或盟主:75 表示75%
	int autoPartnerRatio;//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
	std::string url;//会员:"" 合伙人或盟主:url不为空 当玩家userId与clubId相同时为盟主
	int64_t promoterId;//俱乐部盟主/发起人
	int64_t creatorId;//俱乐部创建者
	STD::time_point joinTime;//加入俱乐部时间
	STD::time_point updateTime;//俱乐部更新时间
	STD::time_point createTime;//俱乐部创建时间
};

//#pragma pack(1)
struct tagGameInfo
{
	uint32_t gameId;
	std::string gameName;
	uint32_t sortId;//游戏排序0 1 2 3 4
	uint8_t gameType;//0-百人场 1-对战类
	std::string serviceName;
	uint8_t revenueRatio;  // revenue
	bool matchforbids[10];// forbid match types
	uint32_t updatePlayerNum;
	uint8_t serverStatus;//-1:关停 0:暂未开放 1：正常开启 2：敬请期待 3: 正在维护
};

struct tagAndroidUserParameter
{
	int64_t    userId;
	std::string     account;
	std::string     nickName;
	uint8_t    headId;

	int64_t    score;

	std::string     enterTime;
	std::string     leaveTime;
	int64_t    takeMinScore;
	int64_t    takeMaxScore;

	std::string    location;
};

struct AndroidStrategyArea
{
	int32_t weight;
	int32_t lowTimes;
	int32_t highTimes;
};

//机器人策略
struct tagAndroidStrategyParam
{
	int32_t gameId;
	int32_t roomId;
	int64_t exitLowScore;
	int64_t exitHighScore;
	int64_t minScore;
	int64_t maxScore;
	std::vector<AndroidStrategyArea> areas;
};

struct tagGameRoomInfo
{
	tagGameRoomInfo() {
		usedCount = 0;
	}
	uint32_t    gameId;                // game id.
	uint32_t    roomId;                // room kind id.
	std::string      roomName;              // room kind name.

	uint16_t    tableCount;            // table count.
	uint16_t    usedCount;

	int64_t     floorScore;            // cell score.
	int64_t     ceilScore;             // cell score.
	int64_t     enterMinScore;         // enter min score.
	int64_t     enterMaxScore;         // enter max score.

	uint32_t    minPlayerNum;          // start min player.
	uint32_t    maxPlayerNum;          // start max player.

	uint32_t    androidCount;          // start android count
	uint32_t    maxAndroidCount;   // real user

	int64_t     broadcastScore;        // broadcast score.
	int64_t     maxJettonScore;        // max Jetton Score

	int64_t     totalStock;
	int64_t     totalStockLowerLimit;
	int64_t     totalStockHighLimit;

	int64_t     totalStockSecondLowerLimit;
	int64_t     totalStockSecondHighLimit;

	uint32_t    systemKillAllRatio;
	uint32_t    systemReduceRatio;
	uint32_t    changeCardRatio;

	uint8_t     serverStatus;          // server status.
	uint8_t     bEnableAndroid;       // is enable android.

	std::vector<int64_t> jettons;

	uint32_t    updatePlayerNumTimes;
	std::vector<float> enterAndroidPercentage;  //Control the number of android entering according to the time
	uint32_t    realChangeAndroid; //join 'realChangeAndroid' real user change out one android user (n:1)

	int64_t     totalJackPot[5];          //预存N个奖池信息

	tagAndroidStrategyParam robotStrategy_;
	//    uint8_t     bisKeepAndroidin;       // is keep android in room.
	//    uint8_t     bisLeaveAnyTime;        // is user can leave game any time.
	//    uint8_t     bisAndroidWaitList;     // DDZ: android have to do wait list.
	//    uint8_t     bisDynamicJoin;         // is game can dynamic join.
	//    uint8_t     bisAutoReady;           // is game auto ready.
	//    uint8_t     bisEnterIsReady;		// is enter is ready.
	//    uint8_t     bisQipai;               // need to wait player ready.
};

struct tagGameClubInfo {
	int64_t clubId;//俱乐部Id 当玩家userId与clubId相同时为盟主
	int32_t iconId;//俱乐部图标
	std::string clubName;//俱乐部名称
	int status;//0-未启用 1-活动 2-弃用 3-禁用
	uint32_t playerNum;//俱乐部人数
	int ratio;//提成比例 会员:0 合伙人或盟主:75 表示75%
	int autoPartnerRatio;//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
	std::string url;//会员:"" 合伙人或盟主:url不为空 当玩家userId与clubId相同时为盟主
	int64_t promoterId;//俱乐部盟主/发起人
	int64_t creatorId;//俱乐部创建者
	STD::time_point joinTime;//加入俱乐部时间
	STD::time_point updateTime;//俱乐部更新时间
	STD::time_point createTime;//俱乐部创建时间
};
//#pragma pack()

struct TableState
{
	uint16_t    tableId;
	uint8_t		locked;
	uint8_t		lookon;
};

struct agent_info_t {
	agent_info_t() {
		score = 0;
		status = 0;
		agentId = 0;
		cooperationtype = 0;
	}
	int64_t		score;              //代理分数 
	int32_t		status;             //是否被禁用 0正常 1停用
	int32_t		agentId;            //agentId
	int32_t		cooperationtype;    //合作模式  1 买分 2 信用
	std::string descode;            //descode 
	std::string md5code;            //MD5 
};

struct agent_user_t {
	agent_user_t() {
		userId = 0;
		score = 0;
		onlinestatus = 0;
	}
	int64_t userId;
	int64_t score;
	int32_t onlinestatus;
	std::string linecode;
};

struct UserBaseInfo {
	UserBaseInfo() {
		userId = INVALID_USER;
		account = "";
		headId = 0;
		privilege = 0;
		nickName = "";
		userScore = 0;
		agentId = 0;
		lineCode = "";
		status = 0;
		location = "";

		takeMaxScore = 0;
		takeMinScore = 0;
		ip = 0;

		alladdscore = 0;
		allsubscore = 0;
		winlostscore = 0;
		allbetscore = 0;
	}

	int64_t userId;
	std::string account;
	uint8_t headId;
	int32_t privilege;
	std::string nickName;
	int64_t userScore;
	uint32_t agentId;
	std::string lineCode;
	uint32_t status;
	uint32_t ip;
	std::string location;
	int64_t takeMaxScore;
	int64_t takeMinScore;
	int64_t alladdscore;
	int64_t allsubscore;
	int64_t winlostscore;
	int64_t allbetscore;

	STD::time_point registertime;
	std::string registerip;
	STD::time_point lastlogintime;
	std::string lastloginip;
	int activedays;
	int keeplogindays;
	int addscoretimes;
	int subscoretimes;
	int32_t onlinestatus;
	int64_t gamerevenue;
	int gender;
	int64_t integralvalue;
};


struct tagScoreInfo
{
	tagScoreInfo()
	{
		//        userId = -1;
		chairId = INVALID_CHAIR;
		isBanker = 0;
		//        beforeScore = -1;
		addScore = 0;
		betScore = 0;
		revenue = 0;
		rWinScore = 0;
		cellScore.clear();
		cardValue = "";

		//        bWriteScore = true;
		//        bWriteRecord = true;
	}

	void clear()
	{
		//        userId = -1;
		chairId = INVALID_CHAIR;
		isBanker = 0;
		//        beforeScore = -1;
		addScore = 0;
		betScore = 0;
		revenue = 0;
		rWinScore = 0;
		cellScore.clear();
		cardValue = "";
	}

	//    int64_t    userId;
	uint32_t   chairId;               // 椅子号
	uint32_t   isBanker;
	//    int64_t    beforeScore;
	int64_t    addScore;              // 当局输赢分数
	int64_t    betScore;              // 总压注
	int64_t    revenue;               // 当局税收
	int64_t    rWinScore;   //有效投注额：税前输赢
	std::vector<int64_t> cellScore;        // 每一方压注

	std::chrono::system_clock::time_point startTime;  //当局开始时间
	std::string     cardValue;             // 当局开牌

	//    bool       bWriteScore;           // 写分
	//    bool       bWriteRecord;          // 写记录
};

struct tagSpecialScoreInfo
{
	tagSpecialScoreInfo()
	{
		userId = -1;
		account = "";
		chairId = INVALID_CHAIR;
		isBanker = 0;
		agentId = 0;
		lineCode = "";
		beforeScore = -1;
		addScore = 0;
		betScore = 0;
		revenue = 0;
		rWinScore = 0;
		cellScore.clear();
		cardValue = "";

		bWriteScore = true;
		bWriteRecord = true;
	}

	void clear()
	{
		userId = -1;
		account = "";
		chairId = INVALID_CHAIR;
		isBanker = 0;
		agentId = 0;
		lineCode = "";
		beforeScore = -1;
		addScore = 0;
		betScore = 0;
		revenue = 0;
		rWinScore = 0;
		cellScore.clear();
		cardValue = "";
	}

	int64_t    userId;
	std::string     account;
	uint32_t   chairId;               // 椅子号
	uint32_t   isBanker;
	uint32_t   agentId;
	std::string     lineCode;
	int64_t    beforeScore;
	int64_t    addScore;              // 当局输赢分数
	int64_t    betScore;              // 总压注
	int64_t    revenue;               // 当局税收
	int64_t    rWinScore;   //有效投注额：税前输赢
	std::vector<int64_t> cellScore;        // 每一方压注

	std::chrono::system_clock::time_point startTime;  //当局开始时间
	std::string     cardValue;             // 当局开牌

	bool       bIsAndroid;
	bool       bWriteScore;           // 写分
	bool       bWriteRecord;          // 写记录
};

//记录水果机免费游戏剩余次数 add by caiqing
struct tagSgjFreeGameRecord
{
	int64_t     userId;             //UserID
	int64_t     betInfo;			//下注数量信息
	int32_t		freeLeft;			//剩余免费次数
	int32_t		marryLeft;			//剩余玛丽数
	int32_t		allMarry;			//总玛丽数
};

struct tagUserInfo
{
	int64_t    userId;
	std::string     account;
	std::string     nickName;
	uint8_t    headId;
	uint32_t   tableId;
	uint32_t   chairId;
	int64_t    score;

	uint8_t    status;
	std::string     location;
};
//对局单步操作
struct tagReplayStep
{
	int32_t time;
	std::string bet;
	int32_t round;
	int32_t ty;//操作类型
	int32_t chairId;//操作位置
	int32_t pos;//被操作位置，如比牌的被比方
	bool flag;//is valid

};
//对局结果
struct tagReplayResult
{
	int32_t chairId;
	int32_t pos;
	int64_t bet;
	int64_t win;
	std::string cardtype;
	bool isbanker;
	bool flag;// is valid
};
//对局玩家信息
struct  tagReplayPlayer
{
	int32_t userid;
	std::string accout;
	int64_t score;
	int32_t chairid;
	bool flag;// is valid
};
//对局记录
struct tagGameReplay
{
	uint16_t gameid;//游戏类型
	std::string gameinfoid;//对局id，对应gameids
	std::string roomname;//全名，如：炸金花高级场
	int32_t cellscore;//底注
	bool saveAsStream;//对局详情格式 true-binary false-jsondata
	std::string detailsData;//对局详情 binary/jsondata
	std::vector<tagReplayPlayer>    players;//玩家
	std::vector<tagReplayStep> steps;//游戏过程
	std::vector<tagReplayResult> results;//游戏结果
	void clear()
	{
		for (std::vector<tagReplayStep>::iterator it = steps.begin(); it != steps.end(); it++)
		{
			it->flag = false;
		}
		for (std::vector<tagReplayPlayer>::iterator it = players.begin(); it != players.end(); it++)
		{
			it->flag = false;
		}
		for (std::vector<tagReplayResult>::iterator it = results.begin(); it != results.end(); it++)
		{
			it->flag = false;
		}
	}

	// 添加结果
	void addResult(int32_t chairId, int32_t pos, int64_t bet, int64_t win, std::string cardtype, bool isBanker)
	{
		std::vector<tagReplayResult>::iterator it = results.begin();
		for (; it != results.end(); it++)
		{
			if (it->flag == false)
			{
				break;
			}
		}
		if (it == results.end())
		{
			tagReplayResult result;
			result.chairId = chairId;
			result.pos = pos;
			result.bet = bet;
			result.win = win;
			result.cardtype = cardtype;
			result.isbanker = isBanker;
			result.flag = true;
			results.push_back(result);
		}
		else {
			it->chairId = chairId;
			it->pos = pos;
			it->bet = bet;
			it->win = win;
			it->isbanker = isBanker;
			it->cardtype = cardtype;
			it->flag = true;
		}
	}
	// 添加玩家
	void addPlayer(int32_t userid, std::string account, int64_t score, int32_t chairid)
	{
		std::vector<tagReplayPlayer>::iterator it = players.begin();
		for (; it != players.end(); it++)
		{
			if (it->flag == false)
			{
				break;
			}
		}
		if (it == players.end())
		{
			tagReplayPlayer player;
			player.accout = account;
			player.userid = userid;
			player.score = score;
			player.chairid = chairid;
			player.flag = true;
			players.push_back(player);
		}
		else {
			it->accout = account;
			it->userid = userid;
			it->score = score;
			it->chairid = chairid;
			it->flag = true;
		}
	}
	// 添加步骤
	void addStep(int32_t time, std::string bet, int32_t round, int32_t ty, int32_t chairId, int32_t pos)
	{
		std::vector<tagReplayStep>::iterator it = steps.begin();
		for (; it != steps.end(); it++)
		{
			if (it->flag == false)
			{
				break;
			}
		}
		if (it == steps.end())
		{
			tagReplayStep step;
			step.time = time;
			step.bet = bet;
			step.round = round;
			step.ty = ty;
			step.chairId = chairId;
			step.pos = pos;
			step.flag = true;
			steps.push_back(step);
		}
		else {
			it->time = time;
			it->bet = bet;
			it->round = round;
			it->ty = ty;
			it->chairId = chairId;
			it->pos = pos;
			it->flag = true;
		}
	}
};

//黑名单信息
struct tagBlacklistInfo
{
	int64_t total;//total control value
	int64_t current;//current control value
	int64_t weight;// control weight
	short status;//control status:0 disabled; 1 undercontrol;other, it is other
	std::map<std::string, int16_t> listRoom;
	tagBlacklistInfo() {}
};

// stock information.
struct tagStockInfo
{
	int64_t   nStorageControl;            // 库存控制值,控牌对应值
	int64_t   nStorageLowerLimit;         // 库存控制下限值
	int64_t   nAndroidStorage;            // 机器人库存控制值
	int64_t   nAndroidStorageLowerLimit;  // 机器人库存
	uint32_t  wSystemAllKillRatio;        // 系统通杀概率(百分比)
};


struct tagStorageInfo
{
	int64_t lEndStorage;					// 当前库存
	int64_t lLowlimit;					    // 最小库存
	int64_t lUplimit;						// 最大库存
	int64_t lSysAllKillRatio;				// 系统通杀率
	int64_t lReduceUnit;                    // 库存衰减
	int64_t lSysChangeCardRatio;			// 系统换牌率
};


#define REC_MAXPLAYER   (5)         // max player count is 5.
struct tagRecPlayer
{
	tagRecPlayer()
	{
		userid = 0;
		account = 0;
		changed_score = 0;
	}

	int64_t changed_score;            // the score has been changed.
	int   account;                  // the special account id now.
	int   userid;                   // the userid of current chair.
};

// game record playback now.
struct tagGameRecPlayback
{
	tagGameRecPlayback()
	{
		// initialize value.
		rec_roundid = 0;
		banker_userid = 0;
	}

	int   rec_roundid;                      // output current game round id(set 0 if input).
	int banker_userid;                      // current banker user item id content value.
	tagRecPlayer player[REC_MAXPLAYER];     // the special player array content item.
	std::string content;                         // the content of record protobuf serial data.
};

#endif