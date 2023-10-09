#ifndef INCLUDE_PLAYER_H
#define INCLUDE_PLAYER_H

#include "Logger/src/Macro.h"
#include "Logger/src/log/Assert.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"

#include "IPlayer.h"
#include "IRobotDelegate.h"

class CPlayer : public IPlayer {
public:
	CPlayer(bool robot = false);
	virtual ~CPlayer()/* = default*/;
	virtual void Reset();
	virtual void AssertReset();
	virtual bool ExistOnlineInfo();
	virtual inline bool Valid() { return baseInfo_->userId != INVALID_USER/* && tableId_ != INVALID_TABLE && chairId_ != INVALID_CHAIR*/; }
	virtual inline bool IsRobot() { return robot_/*false*/; }
	virtual inline bool IsOfficial() { return official_; }
	virtual inline std::shared_ptr<IRobotDelegate> GetDelegate() { return std::shared_ptr<IRobotDelegate>(); }
	/// <summary>
	/// IRobotDelegate消息回调
	/// </summary>
	/// <param name="mainId"></param>
	/// <param name="subId"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len);
	/// <summary>
	/// ITableDelegate消息回调
	/// </summary>
	/// <param name="subId"></param>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool SendTableMessage(uint8_t subId, uint8_t const* data, size_t len);
	virtual inline int64_t  GetUserId() { return baseInfo_->userId; }
	virtual inline const std::string GetAccount() { return baseInfo_->account; }
	virtual inline const std::string GetNickName() { return baseInfo_->nickName; }
	virtual inline uint8_t  GetHeaderId() { return baseInfo_->headId; }
	virtual inline uint8_t GetHeadboxId() { return 0; }
	virtual inline uint8_t GetVip() { return 0; }
	virtual inline std::string GetHeadImgUrl() { return ""; }
	virtual inline uint16_t GetTableId() { return tableId_; }
	virtual inline void SetTableId(uint16_t tableId) { tableId_ = tableId; }
	virtual inline uint16_t GetChairId() { return chairId_; }
	virtual inline void SetChairId(uint16_t chairId) { chairId_ = chairId; }
	virtual inline int64_t GetUserScore() { return baseInfo_->userScore; }
	virtual inline void SetUserScore(int64_t userScore) { baseInfo_->user
	/// <param name="data"></param>Score = userScore; }
	virtual inline void SetCurTakeScore(int64_t score) { }
	virtual inline int64_t GetCurTakeScore() { return 0; }
	virtual inline void SetAutoSetScore(bool autoSet) { }
	virtual inline bool GetAutoSetScore() { return false; }
	virtual inline const uint32_t GetIp() { return baseInfo_->ip; }
	virtual inline const std::string GetLocation() { return baseInfo_->location; }
	virtual inline int GetUserStatus() { return status_; }
	virtual inline void SetUserStatus(uint8_t status) { status_ = status; }
	virtual inline UserBaseInfo& GetUserBaseInfo() { return *ASSERT_NOTNULL(baseInfo_.get()); }
	virtual inline void SetUserBaseInfo(UserBaseInfo const& info) { *ASSERT_NOTNULL(baseInfo_.get()) = info; }
	virtual inline int64_t GetTakeMaxScore() { return baseInfo_->takeMaxScore; }
	virtual inline int64_t GetTakeMinScore() { return baseInfo_->takeMinScore; }
	virtual inline bool isGetout() { return sGetout == status_; }
	virtual inline bool isSit() { return sSit == status_; }
	virtual inline bool isReady() { return sReady == status_; }
	virtual inline bool isPlaying() { return sPlaying == status_; }
	virtual inline bool lookon() { return sLookon == status_; }
	virtual inline bool isBreakline() { return sBreakline == status_; }
	virtual inline bool isOffline() { return sOffline == status_; }
	virtual inline void setUserReady() { SetUserStatus(sReady); }
	virtual inline void setUserSit() { SetUserStatus(sSit); }
	virtual inline void setOffline() { SetUserStatus(sOffline); }
	virtual inline void setTrustee(bool trustship) { trustee_ = trustship; }
	virtual inline bool getTrustee() { return trustee_; }
protected:
	bool robot_;
	bool trustee_;//托管状态
	bool official_;//官方账号
	uint8_t status_; //玩家状态
	uint16_t tableId_;//桌子ID
	uint16_t chairId_;//座位ID
	std::unique_ptr<UserBaseInfo> baseInfo_;//基础数据
};

#endif