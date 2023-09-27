
#include "PlayerMgr.h"
#include "Logger/src/log/Logger.h"

CPlayerMgr::CPlayerMgr() {
}

CPlayerMgr::~CPlayerMgr() {
	items_.clear();
	freeItems_.clear();
}

std::shared_ptr<CPlayer> CPlayerMgr::New(int64_t userId) {
	{
		READ_LOCK(mutex_);
		assert(items_.find(userId) == items_.end());
	}
	std::shared_ptr<CPlayer> player;
	bool empty = false; {
		READ_LOCK(mutex_);
		empty = freeItems_.empty();
	}
	if (!empty) {
		{
			WRITE_LOCK(mutex_);
			if (!freeItems_.empty()) {
				player = freeItems_.back();
				freeItems_.pop_back();
#if 0
				if (player) {
					//player->Reset();
					items_[userId] = player;
				}
#endif
			}
		}
#if 1
		if (player) {
			//player->Reset();
			{
				WRITE_LOCK(mutex_);
				items_[userId] = player;
			}
		}
#endif
	}
	else {
		player = std::shared_ptr<CPlayer>(new CPlayer());
		{
			WRITE_LOCK(mutex_);
			items_[userId] = player;
		}
	}
	return player;
}

std::shared_ptr<CPlayer> CPlayerMgr::Get(int64_t userId) {
	{
		READ_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator  it = items_.find(userId);
		if (it != items_.end()) {
			return it->second;
		}
	}
	return std::shared_ptr<CPlayer>();
}

void CPlayerMgr::Delete(int64_t userId) {
	{
		WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator it = items_.find(userId);
		if (it != items_.end()) {
			std::shared_ptr<CPlayer>& player = it->second;
			items_.erase(it);
			player->Reset();
			freeItems_.emplace_back(player);
		}
	}
	_LOG_ERROR("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}