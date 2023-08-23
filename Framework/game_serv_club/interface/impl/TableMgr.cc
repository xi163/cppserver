#include "Logger/src/log/Logger.h"

#include "ITableDelegate.h"
#include "TableMgr.h"
#include "Table.h"
#include "Player.h"

#include "TableThread.h"

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

CTableMgr::CTableMgr()
	: tableContext_(NULL) {
}

CTableMgr::~CTableMgr() {
	items_.clear();
	usedItems_.clear();
	freeItems_.clear();
}

void CTableMgr::Init(ITableContext* tableContext) {
	if (!tableContext->GetGameInfo() || !tableContext->GetRoomInfo()) {
		return;
	}
	TableDelegateCreator creator = LoadLibrary(tableContext->GetGameInfo()->serviceName);
	if (!creator) {
		exit(0);
	}
	tableContext_ = tableContext;
	CTable::ReadStorageScore(tableContext->GetRoomInfo());
	for (uint32_t i = 0; i < tableContext->GetRoomInfo()->tableCount; ++i) {
		//桌子逻辑线程
		muduo::net::EventLoop* loop = CTableThreadMgr::get_mutable_instance().getNextLoop();
		//创建子游戏桌子代理
		std::shared_ptr<ITableDelegate> tableDelegate = creator();
		//创建桌子
		std::shared_ptr<CTable> table(new CTable(loop, tableContext));
		if (!table || !tableDelegate) {
			_LOG_ERROR("table = %d Failed", i);
			break;
		}
		TableState state = { 0 };
		state.tableId = i;
		state.locked = false;
		state.lookon = false;
		table->Init(tableDelegate, state, tableContext->GetRoomInfo());
		items_.emplace_back(table);
		freeItems_.emplace_back(table);
		//添加到桌子线程管理
		boost::any_cast<LogicThreadPtr>(loop->getContext())->append(state.tableId);
		//_LOG_DEBUG("%d:%s %d:%s tableId:%d stock:%ld",
		//	tableContext->GetGameInfo()->gameId,
		//	tableContext->GetGameInfo()->gameName.c_str(),
		//	tableContext->GetRoomInfo()->roomId,
		//	tableContext->GetRoomInfo()->roomName.c_str(),
		//	state.tableId, tableContext->GetRoomInfo()->totalStock);
	}
	_LOG_WARN("%d:%s %d:%s tableCount:%d stock:%ld",
		tableContext->GetGameInfo()->gameId,
		tableContext->GetGameInfo()->gameName.c_str(),
		tableContext->GetRoomInfo()->roomId,
		tableContext->GetRoomInfo()->roomName.c_str(),
		tableContext->GetRoomInfo()->tableCount, tableContext->GetRoomInfo()->totalStock);
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

/// <summary>
/// 返回桌子数量
/// </summary>
size_t CTableMgr::Count() {
	return items_.size();
}

/// <summary>
/// 返回空闲的桌子数量
/// </summary>
size_t CTableMgr::FreeCount() {
	{
		READ_LOCK(mutex_);
		return freeItems_.size();
	}
}

/// <summary>
/// 返回有人的桌子数量
/// </summary>
size_t CTableMgr::UsedCount() {
	{
		READ_LOCK(mutex_);
		return usedItems_.size();
	}
}

/// <summary>
/// 返回指定俱乐部桌子
/// </summary>
void CTableMgr::Get(int64_t clubId, std::set<uint32_t>& tableId) {
	if (clubId > INVALID_CLUB) {
		READ_LOCK(mutex_);
		for (std::map<uint32_t, std::shared_ptr<CTable>>::iterator it = usedItems_.begin(); it != usedItems_.end(); ++it) {
			if (clubId == it->second->GetClubId()) {
				tableId.insert(it->second->GetTableId());
			}
		}
	}
}

std::shared_ptr<CTable> CTableMgr::Get(uint32_t tableId) {
	{
		//READ_LOCK(mutex_);
		if (tableId < items_.size()) {
			return items_[tableId];
		}
	}
	return std::shared_ptr<CTable>();
}

std::shared_ptr<CTable> CTableMgr::GetSuit(std::shared_ptr<CPlayer> const& player, int64_t clubId, uint32_t tableId) {
	{
		//READ_LOCK(mutex_);
		if (tableId < items_.size()) {
			std::shared_ptr<CTable> table = items_[tableId];
			do {
				if (!table->CanJoinTable(player, clubId)) {
					break;
				}
				return table;
			} while (0);
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
			if (!it->second->Full()) {
				return it->second;
			}
		}
	}
	return std::shared_ptr<CTable>();
}

/// <summary>
/// 查找能进的桌子，没有则取空闲桌子
/// </summary>
std::shared_ptr<CTable> CTableMgr::FindSuit(std::shared_ptr<CPlayer> const& player, int64_t clubId, uint32_t ignoreTableId) {
	std::list<std::shared_ptr<CTable>> usedItems;
	{
		READ_LOCK(mutex_);
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint32_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
	}
	for (auto it : usedItems) {
		std::shared_ptr<CTable> table = it;
		if (table->CanJoinTable(player, clubId, ignoreTableId)) {
			return table;
		}
	}
	return New(clubId);
}

std::shared_ptr<CTable> CTableMgr::New(int64_t clubId) {
	{
		WRITE_LOCK(mutex_);
		if (!freeItems_.empty()) {
			std::shared_ptr<CTable> table = freeItems_.front();
			freeItems_.pop_front();
			table->SetClubId(clubId);
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

// bool CTableMgr::SetTableStockInfo(tagStockInfo& stockInfo) {
// 	stockInfo_ = stockInfo;
// 	return true;
// }

/// <summary>
/// 踢出所有桌子玩家
/// </summary>
void CTableMgr::KickAll() {
	if (tableContext_->GetRoomInfo()) {
		tagGameRoomInfo* roomInfo = tableContext_->GetRoomInfo();
		roomInfo->serverStatus = kStopped;
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
				RunInLoop(table->GetLoop(), CALLBACK_0([=](std::shared_ptr<CTable>& table) {
					table->assertThisThread();
					table->SetGameStatus(kStopped);
					table->DismissGame();
					for (int i = 0; i < table->roomInfo_->maxPlayerNum; ++i) {
						std::shared_ptr<CPlayer> player = table->items_[i];
						if (player) {
							table->OnUserOffline(player);
						}
					}
					//table->ClearTableUser(INVALID_TABLE, false, false);
				}, table));
			}
		}
	}
}