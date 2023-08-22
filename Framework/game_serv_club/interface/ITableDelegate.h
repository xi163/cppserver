#ifndef INCLUDE_ITABLEDELEGATE_H
#define INCLUDE_ITABLEDELEGATE_H

#include "Logger/src/Macro.h"

class ITable;
class IPlayer;

class ITableDelegate {
public:
	ITableDelegate() = default;
	virtual ~ITableDelegate() = default;
	virtual void Reposition() = 0;
	virtual std::string GetRoundId() = 0;
	virtual bool SetTable(std::shared_ptr<ITable> const& table) = 0;
	virtual bool CanJoinTable(std::shared_ptr<IPlayer> const& player) = 0;
	virtual bool CanLeftTable(int64_t userId) = 0;
	virtual bool OnUserEnter(int64_t userId, bool lookon) = 0;
	virtual bool OnUserReady(int64_t userId, bool lookon) = 0;
	virtual bool OnUserLeft(int64_t userId, bool lookon) = 0;
	virtual void OnGameStart() = 0;
	virtual bool OnGameConclude(uint32_t chairId, uint8_t flags) = 0;
	virtual bool OnGameScene(uint32_t chairId, bool lookon) = 0;
	virtual bool OnGameMessage(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len) = 0;
};

typedef std::shared_ptr<ITableDelegate>(*TableDelegateCreator)(void);
typedef void* (*TableDelegateDeleter)(std::shared_ptr<ITableDelegate>& tableDelegate);

#define NameCreateTableDelegate ("CreateTableDelegate")
#define NameDeleteTableDelegate ("DeleteTableDelegate")

#endif