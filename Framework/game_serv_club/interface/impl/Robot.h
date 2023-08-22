#ifndef INCLUDE_ROBOT_H
#define INCLUDE_ROBOT_H

#include "Logger/src/Macro.h"

#include "Player.h"
#include "IRobotDelegate.h"

class CRobot : public CPlayer {
public:
	CRobot();
	virtual ~CRobot();
	virtual void Init(std::shared_ptr<IRobotDelegate> const& robotDelegate);
	virtual void Reset();
	virtual inline bool IsRobot() { return true; }
	virtual inline bool IsOfficial() { return false; }
	virtual inline std::shared_ptr<IRobotDelegate> GetDelegate() { return robotDelegate_; }
	/// <summary>
	/// IRobotDelegate消息回调
	/// </summary>
	virtual bool SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len);
	/// <summary>
	/// ITableDelegate消息回调
	/// </summary>
	virtual bool SendTableMessage(uint8_t subId, uint8_t const* data, size_t len);
	virtual void setReady();
protected:
	std::shared_ptr<IRobotDelegate> robotDelegate_;
};

#endif