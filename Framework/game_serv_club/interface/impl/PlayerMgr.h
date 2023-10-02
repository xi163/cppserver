#ifndef INCLUDE_PLAYERMGR_H
#define INCLUDE_PLAYERMGR_H

#include "Logger/src/Macro.h"

#include "Player.h"

class CPlayerMgr : public boost::serialization::singleton<CPlayerMgr> {
public:
	CPlayerMgr();
	virtual ~CPlayerMgr();
public:
	/// <summary>
	/// 取一个空闲对象，没有则new
	/// </summary>
	/// <param name="userId"></param>
	/// <returns></returns>
	std::shared_ptr<CPlayer> New(int64_t userId);
	
	/// <summary>
	/// 查找
	/// </summary>
	/// <param name="userId"></param>
	/// <returns></returns>
	std::shared_ptr<CPlayer> Get(int64_t userId);
	
	/// <summary>
	/// 回收
	/// </summary>
	/// <param name="userId"></param>
	void Delete(int64_t userId);

	/// <summary>
	/// 回收
	/// </summary>
	/// <param name="player"></param>
	void Delete(std::shared_ptr<CPlayer> const& player);
protected:
	typedef std::pair<int64_t, std::shared_ptr<CPlayer>> Item;
	std::map<int64_t, std::shared_ptr<CPlayer>> items_;
	std::list<std::shared_ptr<CPlayer>> freeItems_;
	mutable boost::shared_mutex mutex_;
};

#endif