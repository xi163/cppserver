#ifndef INCLUDE_IROBOTDELEGATE_H
#define INCLUDE_IROBOTDELEGATE_H

#include "public/Inc.h"

class ITable;
class IPlayer;

struct tagAndroidStrategyParam;

class IRobotDelegate {
public:
	IRobotDelegate() = default;
	virtual ~IRobotDelegate() = default;
	virtual bool Reposition() = 0;
	virtual bool Init(std::shared_ptr<ITable> const& table, std::shared_ptr<IPlayer> const& player) = 0;
	virtual void SetTable(std::shared_ptr<ITable> const& table) = 0;
	virtual bool SetPlayer(std::shared_ptr<IPlayer> const& player) = 0;
	//virtual bool OnTimerMessage(uint32_t timerId, uint32_t dt) = 0;
	virtual bool OnGameMessage(uint8_t subId, uint8_t const* data, size_t len) = 0;
	virtual void SetStrategy(tagAndroidStrategyParam* strategy) = 0;
	virtual tagAndroidStrategyParam* GetStrategy() = 0;
};

typedef std::shared_ptr<IRobotDelegate> (*RobotDelegateCreator)(void);
typedef void* (*RobotDelegateDeleter)(std::shared_ptr<IRobotDelegate>& robotDelegate);

#define NameCreateRobotDelegate	("CreateRobotDelegate")
#define NameDeleteRobotDelegate	("DeleteRobotDelegate")

#endif