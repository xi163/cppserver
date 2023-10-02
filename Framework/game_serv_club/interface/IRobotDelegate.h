#ifndef INCLUDE_IROBOTDELEGATE_H
#define INCLUDE_IROBOTDELEGATE_H

#include "Logger/src/Macro.h"

class ITable;
class IPlayer;

struct tagAndroidStrategyParam;

/// <summary>
/// 机器人代理接口
/// </summary>
class IRobotDelegate {
public:
	IRobotDelegate() = default;
	virtual ~IRobotDelegate() = default;
public:
	/// <summary>
	/// 重置归位
	/// </summary>
	/// <returns></returns>
	virtual bool Reposition() = 0;

	/// <summary>
	/// 初始化
	/// </summary>
	/// <param name="table"></param>
	/// <param name="player"></param>
	/// <returns></returns>
	virtual bool Init(std::shared_ptr<ITable> const& table, std::shared_ptr<IPlayer> const& player) = 0;

	/// <summary>
	/// 绑定桌子
	/// </summary>
	/// <param name="table"></param>
	virtual void SetTable(std::shared_ptr<ITable> const& table) = 0;

	/// <summary>
	/// 绑定机器人
	/// </summary>
	/// <param name="player"></param>
	/// <returns></returns>
	virtual bool SetPlayer(std::shared_ptr<IPlayer> const& player) = 0;

	/// <summary>
	/// 定时器消息
	/// </summary>
	/// <param name="timerId"></param>
	/// <param name="dt"></param>
	/// <returns></returns>
	//virtual bool OnTimerMessage(uint32_t timerId, uint32_t dt) = 0;

	/// <summary>
	/// 游戏逻辑消息
	/// </summary>
	/// <param name="subId"></param>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool OnGameMessage(uint8_t subId, uint8_t const* data, size_t len) = 0;

	/// <summary>
	/// 指定机器人策略
	/// </summary>
	/// <param name="strategy"></param>
	virtual void SetStrategy(tagAndroidStrategyParam* strategy) = 0;

	/// <summary>
	/// 获取机器人策略
	/// </summary>
	/// <returns></returns>
	virtual tagAndroidStrategyParam* GetStrategy() = 0;
};

typedef std::shared_ptr<IRobotDelegate>(*RobotDelegateCreator)(void);
typedef void* (*RobotDelegateDeleter)(std::shared_ptr<IRobotDelegate>& robotDelegate);

#define NameCreateRobotDelegate	("CreateRobotDelegate")
#define NameDeleteRobotDelegate	("DeleteRobotDelegate")

#endif