#ifndef INCLUDE_ROBOTMGR_H
#define INCLUDE_ROBOTMGR_H

#include "public/gameStruct.h"
#include "Packet.h"
#include "ITableContext.h"
#include "Robot.h"

class CRobotMgr : public boost::serialization::singleton<CRobotMgr> {
public:
	CRobotMgr();
	virtual ~CRobotMgr();
	void Init(ITableContext* tableContext);
	bool Empty();
	std::shared_ptr<CRobot> Pick();
	void Delete(int64_t userId);
	void OnTimerCheckIn();
	void Hourtimer(tagGameRoomInfo* roomInfo);
private:
	void load(tagGameRoomInfo* roomInfo, ITableContext* tableContext, RobotDelegateCreator creator);
	int64_t randScore(tagGameRoomInfo* roomInfo, int64_t minScore, int64_t maxScore);
	int randomOnce(int32_t need, int N = 3);
protected:
	std::map<int64_t, std::shared_ptr<CRobot>> items_;
	std::list<std::shared_ptr<CRobot>> freeItems_;
	mutable boost::shared_mutex mutex_;
	STD::Weight weight_;
	double_t percentage_;
};

#endif