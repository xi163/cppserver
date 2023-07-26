
#include "ITableDelegate.h"
#include "TableMgr.h"
#include "Table.h"

static TableDelegateCreator LoadLibrary(std::string const& serviceName) {
	std::string dllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
	dllPath.append("/");
	//./libGame_s13s.so
	std::string dllName = utils::clearDllPrefix(serviceName);
	dllName.insert(0, "./lib");
	dllName.append(".so");
	dllName.insert(0, dllPath);
	_LOG_WARN(dllName.c_str());
	//getchar();
	void* handle = dlopen(dllName.c_str(), RTLD_LAZY);
	if (!handle) {
		_LOG_ERROR("Can't Open %s, %s", dllName.c_str(), dlerror());
		exit(0);
	}
	TableDelegateCreator creator = (TableDelegateCreator)dlsym(handle, NameCreateTableDelegate);
	if (!creator) {
		dlclose(handle);
		_LOG_ERROR("Can't Find %s, %s", NameCreateTableDelegate, dlerror());
		exit(0);
	}
	return creator;
}

CTableMgr::CTableMgr() :roomInfo_(NULL)
, gameInfo_(NULL) {
}

CTableMgr::~CTableMgr() {
	items_.clear();
	usedItems_.clear();
	freeItems_.clear();
}

void CTableMgr::Init(tagGameInfo* gameInfo, tagGameRoomInfo* roomInfo, std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext) {
	if (!gameInfo || !roomInfo) {
		return;
	}
	TableDelegateCreator creator = LoadLibrary(gameInfo->gameServiceName);
	if (!creator) {
		exit(0);
	}
	gameInfo_ = gameInfo;
	roomInfo_ = roomInfo;
	//muduo::AtomicInt32 int32_;
	CTable::ReadStorageScore(roomInfo_);
	for (uint32_t i = 0; i < roomInfo->tableCount; ++i) {
		//创建子游戏桌子代理
		std::shared_ptr<ITableDelegate> tableDelegate = creator();
		//创建桌子
		std::shared_ptr<CTable> table(new CTable());
		if (!table || !tableDelegate) {
			_LOG_ERROR("table = %d Failed", i);
			break;
		}
		TableState state = { 0 };
		state.tableId = i;
		state.bisLock = false;
		state.bisLookOn = false;
		table->Init(tableDelegate, state, gameInfo, roomInfo, logicThread, tableContext);
#if 0
		static struct ___init {
			___init() {
				table->ReadStorageScore();
			}
		}_x;
#elif 0
		if (int32_.getAndSet(1) != 0) {
			table->ReadStorageScore();
		}
#endif
		items_.emplace_back(table);
		freeItems_.emplace_back(table);
		//_LOG_DEBUG("table:%d %d %d stock:%ld", roomInfo->tableCount, gameInfo_->gameId, roomInfo_->roomId, roomInfo_->totalStock);
	}
	_LOG_WARN("table count:%d %d %d stock:%ld", roomInfo->tableCount, gameInfo_->gameId, roomInfo_->roomId, roomInfo_->totalStock);
}

std::list<std::shared_ptr<CTable>> CTableMgr::UsedTables() {
	std::list<std::shared_ptr<CTable>> usedItems;
	{
		READ_LOCK(mutex_);
#if 0
		std::transform(usedItems_.begin(), usedItems_.end(),
			std::back_inserter(usedItems),
			std::bind(&std::map<uint32_t, std::shared_ptr<CTable>>::value_type::second, std::placeholders::_1));
#elif 0
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), second(usedItems_));
#else
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint32_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
#endif
	}
	return usedItems;
}

std::shared_ptr<CTable> CTableMgr::Get(uint32_t tableId) {
	{
		READ_LOCK(mutex_);
		if (tableId < items_.size()) {
			return items_[tableId];
		}
	}
	return std::shared_ptr<CTable>();
}

/// <summary>
/// 返回指定桌子，前提是桌子未满
/// </summary>
std::shared_ptr<CTable> CTableMgr::Find(uint32_t tableId) {
	{
		READ_LOCK(mutex_);
		std::map<uint32_t, std::shared_ptr<CTable>>::iterator it = usedItems_.find(tableId);
		if (it != usedItems_.end()) {
			if (it->second->GetPlayerCount() < it->second->GetMaxPlayerCount()) {
				return it->second;
			}
		}
	}
	return std::shared_ptr<CTable>();
}


/// <summary>
/// 查找能进的桌子，没有则取空闲桌子
/// </summary>
std::shared_ptr<CTable> CTableMgr::FindSuit(std::shared_ptr<IPlayer> const& player, uint32_t ignoreTableId) {
	std::list<std::shared_ptr<CTable>> usedItems;
	{
		READ_LOCK(mutex_);
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint32_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
	}
	for (auto it : usedItems) {
		std::shared_ptr<CTable> table = it;
		if (INVALID_TABLE == ignoreTableId || ignoreTableId == table->GetTableId()) {
			continue;
		}
		if (table->GetPlayerCount() >= table->GetMaxPlayerCount()) {
			continue;
		}
		if (table->CanJoinTable(player)) {
			return table;
		}
	}
	{
		WRITE_LOCK(mutex_);
		if (!freeItems_.empty()) {
			std::shared_ptr<CTable> table = freeItems_.front();
			freeItems_.pop_front();
			table->Reset();
			usedItems_[table->GetTableId()] = table;
			return table;
		}
	}
	return std::shared_ptr<CTable>();
}

void CTableMgr::Delete(uint32_t tableId) {
	{
		WRITE_LOCK(mutex_);
		std::map<uint32_t, std::shared_ptr<CTable>>::iterator it = usedItems_.find(tableId);
		if (it != usedItems_.end()) {
			std::shared_ptr<CTable>& table = it->second;
			usedItems_.erase(it);
			table->Reset();
			freeItems_.emplace_back(table);
		}
	}
	_LOG_ERROR("%d used = %d free = %d", tableId, usedItems_.size(), freeItems_.size());
}

// struct CountsStat {
// 	uint64_t totalRobotCount;
// 	uint64_t totalRealCount;
// 	uint32_t robotCount;
// 	uint32_t realCount;
// };

//void CTableMgr::UpdataPlayerCount2Redis(std::string &strNodeValue)
//{
//    WRITE_LOCK(mutex_);
//    uint64_t totalRobotCount=0;
//    uint64_t totalRealCount=0;
//    uint32_t robotCount=0;
//    uint32_t realCount=0;
//    for(auto it = usedItems_.begin(); it != usedItems_.end(); ++it) {
//        (*it)->GetPlayerCount(realCount, robotCount);
//		totalRobotCount += robotCount;
//        totalRealCount+=realCount;
//    }
//	if (totalRobotCount > 0 || totalRealCount > 0) {
//		std::string str = std::to_string(totalRealCount) + "+" + std::to_string(totalRobotCount);
//		REDISCLIENT.set(strNodeValue, str, 120);
//	}
//}

bool CTableMgr::SetTableStockInfo(tagStockInfo& stockInfo) {
	stockInfo_ = stockInfo;
	return true;
}

void CTableMgr::KickAllTableUsers() {
	roomInfo_->serverStatus = SERVER_STOPPED;
	std::list<std::shared_ptr<CTable>> usedItems;
	{
		READ_LOCK(mutex_);
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint32_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
	}
	for (auto it : usedItems) {
		std::shared_ptr<CTable> table = std::dynamic_pointer_cast<CTable>(it);
		if (table) {
			table->GetRoomInfo()->serverStatus = SERVER_STOPPED;
			table->SetGameStatus(SERVER_STOPPED);
			table->DismissGame();
			for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
				std::shared_ptr<IPlayer> player = table->items_[i];
				if (player) {
					table->OnUserLeft(player);
				}
			}
			table->ClearTableUser(INVALID_TABLE, false, false);
		}
	}
}