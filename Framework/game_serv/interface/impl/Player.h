#ifndef INCLUDE_PLAYER_H
#define INCLUDE_PLAYER_H

#include "public/Inc.h"
#include "gameDefine.h"

#include "IPlayer.h"
#include "IRobotDelegate.h"

class CPlayer : public IPlayer {
public:
	CPlayer();
	virtual ~CPlayer() = default;
	virtual void Reset();
	virtual inline bool Valid() { return /*tableId_ != INVALID_TABLE && chairId_ != INVALID_CHAIR && */GetUserId() > 0; }
	virtual inline bool IsRobot() { return false; }
	virtual inline bool IsOfficial() { return official_; }
	virtual inline std::shared_ptr<IRobotDelegate> GetDelegate() { return std::shared_ptr<IRobotDelegate>(); }
	/// <summary>
	/// IRobotDelegate消息回调
	/// </summary>
	virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len);
	/// <summary>
	/// ITableDelegate消息回调
	/// </summary>
	virtual bool SendTableMessage(uint8_t subId, uint8_t const* data, size_t len);
	virtual inline int64_t  GetUserId() { return baseInfo_.userId; }
	virtual inline const std::string GetAccount() { return baseInfo_.account; }
	virtual inline const std::string GetNickName() { return baseInfo_.nickName; }
	virtual inline uint8_t  GetHeaderId() { return baseInfo_.headId; }
	virtual inline uint8_t GetHeadboxId() { return 0; }
	virtual inline uint8_t GetVip() { return 0; }
	virtual inline std::string GetHeadImgUrl() { return ""; }
	virtual inline uint32_t GetTableId() { return tableId_; }
	virtual inline void SetTableId(uint32_t tableId) { tableId_ = tableId; }
	virtual inline uint32_t GetChairId() { return chairId_; }
	virtual inline void SetChairId(uint32_t chairId) { chairId_ = chairId; }
	virtual inline int64_t GetUserScore() { return baseInfo_.userScore; }
	virtual inline void SetUserScore(int64_t userScore) { baseInfo_.userScore = userScore; }
	virtual inline void SetCurTakeScore(int64_t score) { }
	virtual inline int64_t GetCurTakeScore() { return 0; }
	virtual inline void SetAutoSetScore(bool autoSet) { }
	virtual inline bool GetAutoSetScore() { return false; }
	virtual inline const uint32_t GetIp() { return baseInfo_.ip; }
	virtual inline const std::string GetLocation() { return baseInfo_.location; }
	virtual inline int GetUserStatus() { return status_; }
	virtual inline void SetUserStatus(uint8_t status) { status_ = status; }
	virtual inline UserBaseInfo& GetUserBaseInfo() { return baseInfo_; }
	virtual inline void SetUserBaseInfo(UserBaseInfo const& info) { baseInfo_ = info; }
	virtual inline int64_t GetTakeMaxScore() { return baseInfo_.takeMaxScore; }
	virtual inline int64_t GetTakeMinScore() { return baseInfo_.takeMinScore; }
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
	bool official_;//官方账号
	uint32_t tableId_;//桌子ID
	uint32_t chairId_;//座位ID
	uint8_t status_; //玩家状态
	bool trustee_;//托管状态
	UserBaseInfo baseInfo_;//基础数据
};

#endif