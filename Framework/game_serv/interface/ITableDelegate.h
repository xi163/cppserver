#ifndef INCLUDE_ITABLEDELEGATE_H
#define INCLUDE_ITABLEDELEGATE_H

#include "Logger/src/Macro.h"

class ITable;
class IPlayer;

/// <summary>
/// 桌子代理接口
/// </summary>
class ITableDelegate {
public:
	ITableDelegate() = default;
	virtual ~ITableDelegate() = default;

	/// <summary>
	/// 重置归位
	/// </summary>
	virtual void Reposition() = 0;

	/// <summary>
	/// 获取牌局编号
	/// </summary>
	/// <returns></returns>
	virtual std::string GetRoundId() = 0;

	/// <summary>
	/// 绑定桌子
	/// </summary>
	/// <param name="table"></param>
	/// <returns></returns>
	virtual bool SetTable(std::shared_ptr<ITable> const& table) = 0;

	/// <summary>
	/// 能否加入
	/// </summary>
	/// <param name="player"></param>
	/// <returns></returns>
	virtual bool CanJoinTable(std::shared_ptr<IPlayer> const& player) = 0;

	/// <summary>
	/// 能否离开
	/// </summary>
	/// <param name="userId"></param>
	/// <returns></returns>
	virtual bool CanLeftTable(int64_t userId) = 0;

	/// <summary>
	/// 玩家入场
	/// </summary>
	/// <param name="userId"></param>
	/// <param name="lookon"></param>
	/// <returns></returns>
	virtual bool OnUserEnter(int64_t userId, bool lookon) = 0;

	/// <summary>
	/// 玩家准备
	/// </summary>
	/// <param name="userId"></param>
	/// <param name="lookon"></param>
	/// <returns></returns>
	virtual bool OnUserReady(int64_t userId, bool lookon) = 0;

	/// <summary>
	/// 玩家离场
	/// </summary>
	/// <param name="userId"></param>
	/// <param name="lookon"></param>
	/// <returns></returns>
	virtual bool OnUserLeft(int64_t userId, bool lookon) = 0;

	/// <summary>
	/// 游戏开始
	/// </summary>
	virtual void OnGameStart() = 0;

	/// <summary>
	/// 游戏结束
	/// </summary>
	/// <param name="chairId"></param>
	/// <param name="flags"></param>
	/// <returns></returns>
	virtual bool OnGameConclude(uint16_t chairId, uint8_t flags) = 0;

	/// <summary>
	/// 游戏场景
	/// </summary>
	/// <param name="chairId"></param>
	/// <param name="lookon"></param>
	/// <returns></returns>
	virtual bool OnGameScene(uint16_t chairId, bool lookon) = 0;

	/// <summary>
	/// 游戏逻辑消息
	/// </summary>
	/// <param name="chairId"></param>
	/// <param name="subId"></param>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <returns></returns>
	virtual bool OnGameMessage(uint16_t chairId, uint8_t subId, uint8_t const* data, size_t len) = 0;
};

typedef std::shared_ptr<ITableDelegate>(*TableDelegateCreator)(void);
typedef void* (*TableDelegateDeleter)(std::shared_ptr<ITableDelegate>& tableDelegate);

#define NameCreateTableDelegate ("CreateTableDelegate")
#define NameDeleteTableDelegate ("DeleteTableDelegate")

#endif