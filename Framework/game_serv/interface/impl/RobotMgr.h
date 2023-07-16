#ifndef INCLUDE_ROBOTMGR_H
#define INCLUDE_ROBOTMGR_H

#include "public/Inc.h"
#include "Packet.h"
#include "ITableContext.h"
#include "Robot.h"

class CRobotMgr : public boost::serialization::singleton<CRobotMgr> {
public:
	CRobotMgr();
	virtual ~CRobotMgr();
	void Init(tagGameInfo* gameInfo, tagGameRoomInfo* roomInfo, std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext);
	void Load();
	std::shared_ptr<CRobot> Pick();
	void Delete(int64_t userId);
	void OnTimerCheckIn();
	void Hourtimer();
private:
	int64_t randScore(int64_t minScore, int64_t maxScore);
	int randomOnce(int32_t need, int N = 3);
protected:
	RobotDelegateCreator creator_;
	tagGameInfo* gameInfo_;
	tagGameRoomInfo* roomInfo_;
	tagAndroidStrategyParam robotStrategy_;
	ITableContext* tableContext_;
	//std::shared_ptr<muduo::net::EventLoopThread> logicThread_;//桌子逻辑线程/定时器
	std::map<int64_t, std::shared_ptr<CRobot>> items_;
	std::list<std::shared_ptr<CRobot>> freeItems_;
	mutable boost::shared_mutex mutex_;
	STD::Weight weight_;
	double_t percentage_;
};

#endif