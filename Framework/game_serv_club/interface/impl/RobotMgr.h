#ifndef INCLUDE_ROBOTMGR_H
#define INCLUDE_ROBOTMGR_H

#include "public/gameStruct.h"
#include "Packet/Packet.h"
#include "ITableContext.h"
#include "Robot.h"

class CRobotMgr : public boost::serialization::singleton<CRobotMgr> {
public:
	CRobotMgr();
	virtual ~CRobotMgr();
	bool Init(ITableContext* tableContext);
	bool Empty();
	std::shared_ptr<CRobot> Pick();
	void Delete(int64_t userId);
	void Delete(std::shared_ptr<CRobot> const& robot);
private:
	bool load(tagGameRoomInfo* roomInfo, ITableContext* tableContext, RobotDelegateCreator creator);
protected:
	typedef std::pair<int64_t, std::shared_ptr<CRobot>> Item;
	std::map<int64_t, std::shared_ptr<CRobot>> items_;
	std::list<std::shared_ptr<CRobot>> freeItems_;
	mutable boost::shared_mutex mutex_;
	STD::Weight weight_;
	double_t percentage_;
};

#endif