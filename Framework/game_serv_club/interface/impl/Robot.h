#ifndef INCLUDE_ROBOT_H
#define INCLUDE_ROBOT_H

#include "Logger/src/Macro.h"

#include "Player.h"
#include "IRobotDelegate.h"

class CRobot : public CPlayer {
public:
	CRobot();
	virtual ~CRobot()/* = default*/;
public:
	/// <summary>
	/// 初始化
	/// </summary>
	/// <param name="robotDelegate"></param>
	virtual void Init(std::shared_ptr<IRobotDelegate> const& robotDelegate);
	/// <summary>
	/// 除了基础信息重置其它
	/// </summary>
	virtual void Reset();
	
	/// <summary>
	/// 除了基础信息重置其它
	/// </summary>
	virtual void AssertReset();
	
	/// <summary>
	/// 是否机器人
	/// </summary>
	/// <returns></returns>
	virtual inline bool IsRobot() { return robot_/*true*/; }
	
	/// <summary>
	/// 是否官方账号
	/// </summary>
	/// <returns></returns>
	virtual inline bool IsOfficial() { return false; }

	/// <summary>
	/// 是否游戏中，缓存检查
	/// </summary>
	/// <returns></returns>
	virtual inline bool ExistOnlineInfo() { return true; }

	/// <summary>
	/// 返回机器人代理
	/// </summary>
	/// <returns></returns>
	virtual inline std::shared_ptr<IRobotDelegate> GetDelegate() { return robotDelegate_; }
	
	/// <summary>
	/// IRobotDelegate消息回调
	/// </summary>
	/// <param name="mainId"></param>
	/// <param name="subId"></param>
	/// <param name="data"></param>
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
	
	/// <summary>
	/// 更改准备状态
	/// </summary>
	virtual void setReady();
protected:
	std::shared_ptr<IRobotDelegate> robotDelegate_;
};

#endif