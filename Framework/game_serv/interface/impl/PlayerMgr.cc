
#include "PlayerMgr.h"
#include "Logger/src/log/Logger.h"

CPlayerMgr::CPlayerMgr() {
}

CPlayerMgr::~CPlayerMgr() {
	ASSERT(false);
	//WRITE_LOCK(mutex_); {
	//	items_.clear();
	//	freeItems_.clear();
	//}
}

std::shared_ptr<CPlayer> CPlayerMgr::New(int64_t userId) {
	std::shared_ptr<CPlayer> player;
#ifdef _USE_SHARED_MUTEX_
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
		if (player) {
			WRITE_LOCK(mutex_);
			items_[userId] = player;
		}
	}
	MY_CATCH();
#else
	{
#ifdef _USE_MUDUO_MUTEX_
		muduo::MutexLockGuard lock(mutex_);
#else
		std::unique_lock<std::mutex> lock(mutex_);
#endif
		if (!freeItems_.empty()) {
			player = freeItems_.front();
			freeItems_.pop_front();
			if (player) {
				ASSERT(std::find_if(items_.begin(), items_.end(), [&](Item const& kv) -> bool {
					return kv.second.get() == player.get();
					}) == items_.end());
				// FIXME crash bug!!!
				player->AssertReset();
				items_[userId] = player;
			}
		}
	}
	if (!player) {
		player = std::shared_ptr<CPlayer>(new CPlayer());
		if (player) {
#ifdef _USE_MUDUO_MUTEX_
			muduo::MutexLockGuard lock(mutex_);
#else
			std::unique_lock<std::mutex> lock(mutex_);
#endif
			items_[userId] = player;
		}
	}
#endif
	return player;
}

std::shared_ptr<CPlayer> CPlayerMgr::Get(int64_t userId) {
	{
#ifdef _USE_SHARED_MUTEX_
		READ_LOCK(mutex_);
#elif defined(_USE_MUDUO_MUTEX_)
		muduo::MutexLockGuard lock(mutex_);
#else
		std::unique_lock<std::mutex> lock(mutex_);
#endif
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator  it = items_.find(userId);
		if (it != items_.end()) {
			return it->second;
		}
	}
	return std::shared_ptr<CPlayer>();
}

//一个userId占用多个CPlayer对象?
void CPlayerMgr::Delete(int64_t userId) {
	ASSERT(userId != INVALID_USER);
	{
#ifdef _USE_SHARED_MUTEX_
		WRITE_LOCK(mutex_);
#elif defined(_USE_MUDUO_MUTEX_)
		muduo::MutexLockGuard lock(mutex_);
#else
		std::unique_lock<std::mutex> lock(mutex_);
#endif
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator it = items_.find(userId);
		ASSERT_V(it != items_.end(), "userId=%d", userId);
		if (it != items_.end()) {
			std::shared_ptr<CPlayer>& player = it->second;
			items_.erase(it);
			player->Reset();
			freeItems_.emplace_back(player);
		}
		//size_t n = items_.erase(userId);
		//(void)n;
		//ASSERT_V(n == 0, "n=%d", n);
	}
	Errorf("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}

void CPlayerMgr::Delete(std::shared_ptr<CPlayer> const& player) {
	int64_t userId = player->GetUserId();
	ASSERT(userId != INVALID_USER);
	{
#ifdef _USE_SHARED_MUTEX_
		WRITE_LOCK(mutex_);
#elif defined(_USE_MUDUO_MUTEX_)
		muduo::MutexLockGuard lock(mutex_);
#else
		std::unique_lock<std::mutex> lock(mutex_);
#endif
		std::map<int64_t, std::shared_ptr<CPlayer>>::iterator it = std::find_if(items_.begin(), items_.end(),
			[&](Item const& kv) -> bool {
				return kv.second.get() == player.get();
			});
		ASSERT_V(it != items_.end(), "userId=%d", userId);
		if (it != items_.end()) {
			std::shared_ptr<CPlayer>& player = it->second;
			ASSERT(player->GetUserId() == userId);
			items_.erase(it);
			player->Reset();
			freeItems_.emplace_back(player);
		}
		//size_t n = items_.erase(userId);
		//(void)n;
		//ASSERT_V(n == 0, "n=%d", n);
	}
	Errorf("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}