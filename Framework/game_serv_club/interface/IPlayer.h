#ifndef INCLUDE_IPLAER_H
#define INCLUDE_IPLAER_H

#include "Logger/src/Macro.h"

class IRobotDelegate;
struct UserBaseInfo;

class IPlayer : public std::enable_shared_from_this<IPlayer> {
public:
	IPlayer() = default;
	virtual ~IPlayer() = default;
	virtual void Reset() = 0;
	virtual bool Valid() = 0;
	virtual bool IsRobot() = 0;
	virtual bool IsOfficial() = 0;
	virtual bool ExistOnlineInfo() = 0;
	virtual std::shared_ptr<IRobotDelegate> GetDelegate() = 0;
	/// <summary>
	/// IRobotDelegate消息回调
	/// </summary>
	/// <param name="mainId"></param>
	/// <param name="subId"></param>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len) = 0;
	/// <summary>
	/// ITableDelegate消息回调
	/// </summary>
	/// <param name="subId"></param>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool SendTableMessage(uint8_t subId, uint8_t const* data, size_t len) = 0;
	virtual int64_t GetUserId() = 0;
	virtual const std::string GetAccount() = 0;
	virtual const std::string GetNickName() = 0;
	virtual uint8_t GetHeaderId() = 0;
	virtual uint8_t GetHeadboxId() = 0;
	virtual uint8_t GetVip() = 0;
	virtual std::string GetHeadImgUrl() = 0;
	virtual uint16_t GetTableId() = 0;
	virtual void SetTableId(uint16_t tableId) = 0;
	virtual uint16_t GetChairId() = 0;
	virtual void SetChairId(uint16_t chairId) = 0;
	virtual int64_t GetUserScore() = 0;
	virtual void SetUserScore(int64_t userScore) = 0;
	virtual void SetCurTakeScore(int64_t score) = 0;
	virtual int64_t GetCurTakeScore() = 0;
	virtual void SetAutoSetScore(bool autoSet) = 0;
	virtual bool GetAutoSetScore() = 0;
	virtual const uint32_t GetIp() = 0;
	virtual const std::string GetLocation() = 0;
	virtual int GetUserStatus() = 0;
	virtual void SetUserStatus(uint8_t status) = 0;
	virtual UserBaseInfo& GetUserBaseInfo() = 0;
	virtual void SetUserBaseInfo(UserBaseInfo const& info) = 0;
	virtual int64_t GetTakeMaxScore() = 0;
	virtual int64_t GetTakeMinScore() = 0;
	virtual bool isGetout() = 0;
	virtual bool isSit() = 0;
	virtual bool isReady() = 0;
	virtual bool isPlaying() = 0;
	virtual bool lookon() = 0;
	virtual bool isBreakline() = 0;
	virtual bool isOffline() = 0;
	virtual void setUserReady() = 0;
	virtual void setUserSit() = 0;
	virtual void setOffline() = 0;
	virtual void setTrustee(bool trustship) = 0;
	virtual bool getTrustee() = 0;
};

#endif