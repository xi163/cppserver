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
public:
	/// <summary>
	/// 初始化
	/// </summary>
	/// <param name="tableContext"></param>
	/// <returns></returns>
	bool Init(ITableContext* tableContext);
	
	/// <summary>
	/// 判空
	/// </summary>
	/// <returns></returns>
	bool Empty();
	
	/// <summary>
	/// 取一个空闲机器人
	/// </summary>
	/// <returns></returns>
	std::shared_ptr<CRobot> Pick();
	
	/// <summary>
	/// 回收
	/// </summary>
	/// <param name="userId"></param>
	void Delete(int64_t userId);
	
	/// <summary>
	/// 回收
	/// </summary>
	/// <param name="robot"></param>
	void Delete(std::shared_ptr<CRobot> const& robot);
private:
	/// <summary>
	/// 数据库加载
	/// </summary>
	/// <param name="roomInfo"></param>
	/// <param name="tableContext"></param>
	/// <param name="creator"></param>
	/// <returns></returns>
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