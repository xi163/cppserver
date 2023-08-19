#ifndef INCLUDE_ITALBE_H
#define INCLUDE_ITALBE_H

#include "public/Inc.h"

#include "IReplayRecord.h"

class IPlayer;
class ITableDelegate;
class ITableContext;
class IReplayRecord;

struct TableState;
struct tagGameInfo;
struct tagGameRoomInfo;
struct tagGameClubInfo;

struct tagScoreInfo;
struct tagSpecialScoreInfo;
struct tagGameReplay;
struct tagUserInfo;
struct tagStorageInfo;

class ITable : public std::enable_shared_from_this<ITable>, public IReplayRecord {
public:
	ITable() = default;
	virtual ~ITable() = default;
	virtual void Reset() = 0;
	virtual uint32_t GetTableId() = 0;
	virtual void GetTableInfo(TableState& TableInfo) = 0;
	virtual std::shared_ptr<muduo::net::EventLoopThread> GetLoopThread() = 0;
	virtual void assertThisThread() = 0;
	virtual std::string const& ServId() = 0;
	virtual std::string NewRoundId() = 0;
	virtual std::string GetRoundId() = 0;
	virtual bool DismissGame() = 0;
	virtual bool ConcludeGame(uint8_t gameStatus) = 0;
	virtual int64_t CalculateRevenue(int64_t score) = 0;
	virtual std::shared_ptr<IPlayer> GetChairPlayer(uint32_t chairId) = 0;
	virtual std::shared_ptr<IPlayer> GetPlayer(int64_t userId) = 0;
	virtual bool ExistUser(uint32_t chairId) = 0;
	virtual void SetGameStatus(uint8_t status) = 0;
	virtual uint8_t GetGameStatus() = 0;
	virtual std::string StrGameStatus() = 0;
	virtual void SetUserTrustee(uint32_t chairId, bool trustee) = 0;
	virtual bool GetUserTrustee(uint32_t chairId) = 0;
	virtual void SetUserReady(uint32_t chairId) = 0;
	virtual uint32_t GetPlayerCount() = 0;
	virtual uint32_t GetPlayerCount(bool includeRobot) = 0;
	virtual uint32_t GetRobotPlayerCount() = 0;
	virtual uint32_t GetRealPlayerCount() = 0;
	virtual void GetPlayerCount(uint32_t& realCount, uint32_t& robotCount) = 0;
	virtual uint32_t GetMaxPlayerCount() = 0;
	virtual tagGameRoomInfo* GetRoomInfo() = 0;
	virtual tagGameClubInfo* GetClubInfo() = 0;
	virtual bool IsRobot(uint32_t chairId) = 0;
	virtual bool IsOfficial(uint32_t chairId) = 0;
	virtual bool OnGameEvent(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len) = 0;
	virtual void OnStartGame() = 0;
	virtual bool IsGameStarted() = 0;
	virtual bool CheckGameStart() = 0;
	virtual bool SendTableData(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len) = 0;
	virtual bool SendTableData(uint32_t chairId, uint8_t subId, ::google::protobuf::Message* msg) = 0;
	virtual bool SendUserData(std::shared_ptr<IPlayer> const& player, uint8_t subId, uint8_t const* data, size_t len) = 0;
	virtual bool SendGameMessage(uint32_t chairId, std::string const& msg, uint8_t msgType, int64_t score = 0) = 0;
	virtual void ClearTableUser(uint32_t chairId, bool sendState = true, bool sendToSelf = true, uint8_t sendErrCode = 0) = 0;
	virtual void BroadcastUserInfoToOther(std::shared_ptr<IPlayer> const& player) = 0;
	virtual void SendAllOtherUserInfoToUser(std::shared_ptr<IPlayer> const& player) = 0;
	virtual void SendOtherUserInfoToUser(std::shared_ptr<IPlayer> const& player, tagUserInfo& userInfo) = 0;
	virtual void BroadcastUserScore(std::shared_ptr<IPlayer> const& player) = 0;
	virtual void BroadcastUserStatus(std::shared_ptr<IPlayer> const& player, bool sendToSelf = false) = 0;
	virtual bool WriteUserScore(tagScoreInfo* scoreInfo, uint32_t count, std::string& strRound) = 0;
	virtual bool WriteSpecialUserScore(tagSpecialScoreInfo* scoreInfo, uint32_t count, std::string& strRound) = 0;
	virtual bool UpdateUserScoreToDB(int64_t userId, tagScoreInfo* scoreInfo) = 0;
	virtual bool UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo) = 0;
	virtual int  UpdateStorageScore(int64_t changeStockScore) = 0;
	virtual bool GetStorageScore(tagStorageInfo& storageInfo) = 0;
	virtual void RefreshRechargeScore(std::shared_ptr<IPlayer> const& player) = 0;
	virtual int64_t CalculateAgentRevenue(uint32_t chairId, int64_t revenue) = 0;
};

#endif