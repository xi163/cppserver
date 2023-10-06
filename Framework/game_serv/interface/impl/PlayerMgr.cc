
#include "PlayerMgr.h"
#include "Logger/src/log/Logger.h"

CPlayerMgr::CPlayerMgr() {
}

CPlayerMgr::~CPlayerMgr() {
	//WRITE_LOCK(mutex_); {
	//	items_.clear();
	//	freeItems_.clear();
	//}
}

std::shared_ptr<CPlayer> CPlayerMgr::New(int64_t userId) {
	std::shared_ptr<CPlayer> player;
	MY_TRY(); {
		READ_LOCK(mutex_);
		ASSERT(items_.find(userId) == items_.end());
	}
	bool empty = false; {
		READ_LOCK(mutex_);
		empty = freeItems_.empty();
	}
	if (!empty) {
		{
			WRITE_LOCK(mutex_);
			if (!freeItems_.empty()) {
				player = freeItems_.front();
				freeItems_.pop_front();
#if 1
				if (player) {
					ASSERT(std::find_if(items_.begin(), items_.end(), [&](Item const& kv) -> bool {
						return kv.second.get() == player.get();
						}) == items_.end());
					player->AssertReset();
					items_[userId] = player;
				}
#endif
			}
		}
#if 0
		if (player) {
			{
				READ_LOCK(mutex_);
				ASSERT(std::find_if(items_.begin(), items_.end(), [&](Item const& kv) -> bool {
					return kv.second.get() == player.get();
				}) == items_.end());
			}
			player->AssertReset();
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
	MY_CATCH();
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
	Errorf("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}

void CPlayerMgr::Delete(std::shared_ptr<CPlayer> const& player) {
	int64_t userId = player->GetUserId();
	{
		WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator it = std::find_if(items_.begin(), items_.end(),
			[&](Item const& kv) -> bool {
				return kv.second.get() == player.get();
			});
		if (it != items_.end()) {
			std::shared_ptr<CPlayer>& player = it->second;
			ASSERT(player->GetUserId() == userId);
			items_.erase(it);
			player->Reset();
			freeItems_.emplace_back(player);
		}
	}
	Errorf("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}