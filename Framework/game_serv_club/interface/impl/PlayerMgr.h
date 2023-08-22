#ifndef INCLUDE_PLAYERMGR_H
#define INCLUDE_PLAYERMGR_H

#include "Logger/src/Macro.h"

#include "Player.h"

class CPlayerMgr : public boost::serialization::singleton<CPlayerMgr> {
public:
	CPlayerMgr();
	virtual ~CPlayerMgr();
	std::shared_ptr<CPlayer> New(int64_t userId);
	std::shared_ptr<CPlayer> Get(int64_t userId);
	void Delete(int64_t userId);
protected:
	std::map<int64_t, std::shared_ptr<CPlayer>> items_;
	std::list<std::shared_ptr<CPlayer>> freeItems_;
	mutable boost::shared_mutex mutex_;
};

#endif