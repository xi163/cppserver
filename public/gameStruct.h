#ifndef INCLUDE_GAMESTRUCT_H
#define INCLUDE_GAMESTRUCT_H

#include "Logger/src/Macro.h"

//#pragma pack(1)
struct tagGameInfo
{
	uint32_t gameId;       // game id.
	std::string gameName;
	uint32_t sortId;       // game sort id.
	uint8_t gameType;     // 0-bairen   1-duizhan
	std::string serviceName;
	uint8_t revenueRatio;  // revenue
	bool matchforbids[10];// forbid match types
	uint32_t updatePlayerNum;
	uint8_t serverStatus;
};

struct tagGameRoomInfo
{
	uint32_t    gameId;                // game id.
	uint32_t    roomId;                // room kind id.
	std::string      roomName;              // room kind name.

	uint16_t    tableCount;            // table count.

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

	//    uint8_t     bisKeepAndroidin;       // is keep android in room.
	//    uint8_t     bisLeaveAnyTime;        // is user can leave game any time.
	//    uint8_t     bisAndroidWaitList;     // DDZ: android have to do wait list.
	//    uint8_t     bisDynamicJoin;         // is game can dynamic join.
	//    uint8_t     bisAutoReady;           // is game auto ready.
	//    uint8_t     bisEnterIsReady;		// is enter is ready.
	//    uint8_t     bisQipai;               // need to wait player ready.
};
//#pragma pack()

struct TableState
{
	uint32_t    nTableID;
	uint8_t		bisLock;
	uint8_t		bisLookOn;
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
		userId = -1;
		account = "";
		headId = 0;
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
};

#endif