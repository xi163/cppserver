
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
					player->Reset();
					items_[userId] = player;
				}
#endif
			}
		}
#if 1
		if (player) {
			player->Reset();
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

//一个userId占用多个CPlayer对象?
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

void CPlayerMgr::Delete(std::shared_ptr<CPlayer> const& player) {
	{
		WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator it = std::find_if(std::begin(items_), std::end(items_),
			[&](Item const& kv) -> bool {
				return kv.second.get() == player.get();
			});
		if (it != items_.end()) {
			std::shared_ptr<CPlayer>& player = it->second;
			items_.erase(it);
			player->Reset();
			freeItems_.emplace_back(player);
		}
	}
	_LOG_ERROR("%d used = %d free = %d", player->GetUserId(), items_.size(), freeItems_.size());
}