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
	Warnf(dllName.c_str());
	//getchar();
	void* handle = dlopen(dllName.c_str(), RTLD_LAZY);
	if (!handle) {
		Errorf("Can't Open %s, %s", dllName.c_str(), dlerror());
		exit(0);
	}
	TableDelegateCreator creator = (TableDelegateCreator)dlsym(handle, NameCreateTableDelegate);
	if (!creator) {
		dlclose(handle);
		Errorf("Can't Find %s, %s", NameCreateTableDelegate, dlerror());
		exit(0);
	}
	return creator;
}

CTableMgr::CTableMgr()
	: tableContext_(NULL) {
}

CTableMgr::~CTableMgr() {
	ASSERT(false);
	//READ_LOCK(mutex_); {
	//	items_.clear();
	//	usedItems_.clear();
	//	freeItems_.clear();
	//}
}

/// <summary>
/// 初始化
/// </summary>
/// <param name="tableContext"></param>
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
	for (uint16_t i = 0; i < tableContext->GetRoomInfo()->tableCount; ++i) {
		//桌子逻辑线程
		muduo::net::EventLoop* loop = CTableThreadMgr::get_mutable_instance().getNextLoop();
		//创建子游戏桌子代理
		std::shared_ptr<ITableDelegate> tableDelegate = creator();
		//创建桌子
		std::shared_ptr<CTable> table(new CTable(loop, tableContext));
		if (!table || !tableDelegate) {
			Errorf("table = %d Failed", i);
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
		//Debugf("%d:%s %d:%s tableId:%d stock:%ld",
		//	tableContext->GetGameInfo()->gameId,
		//	tableContext->GetGameInfo()->gameName.c_str(),
		//	tableContext->GetRoomInfo()->roomId,
		//	tableContext->GetRoomInfo()->roomName.c_str(),
		//	state.tableId, tableContext->GetRoomInfo()->totalStock);
	}
	Warnf("%d:%s %d:%s tableCount:%d stock:%ld",
		tableContext->GetGameInfo()->gameId,
		tableContext->GetGameInfo()->gameName.c_str(),
		tableContext->GetRoomInfo()->roomId,
		tableContext->GetRoomInfo()->roomName.c_str(),
		tableContext->GetRoomInfo()->tableCount, tableContext->GetRoomInfo()->totalStock);
}

/// <summary>
/// 返回使用中桌子
/// </summary>
/// <param name="usedItems"></param>
/// <returns></returns>
bool CTableMgr::UsedTables(std::list<std::shared_ptr<CTable>>& usedItems) {
	{
		READ_LOCK(mutex_);
#if 0
		std::transform(usedItems_.begin(), usedItems_.end(),
			std::back_inserter(usedItems),
			std::bind(&std::map<uint16_t, std::shared_ptr<CTable>>::value_type::second, std::placeholders::_1));
#elif 0
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), second(usedItems_));
#else
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint16_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
#endif
	}
	return !usedItems.empty();
}

/// <summary>
/// 返回使用中桌子
/// </summary>
/// <param name="tableId"></param>
/// <param name="usedItems"></param>
/// <returns></returns>
bool CTableMgr::UsedTables(std::vector<uint16_t> const& tableId, std::list<std::shared_ptr<CTable>>& usedItems) {
	{
		READ_LOCK(mutex_);
		for (int i = 0; i < tableId.size(); ++i) {
			std::map<uint16_t, std::shared_ptr<CTable>>::const_iterator it = usedItems_.find(tableId[i]);
			if (it != usedItems_.end()) {
				usedItems.emplace_back(it->second);
			}
		}
	}
	return !usedItems.empty();
}

/// <summary>
/// 返回桌子数量
/// </summary>
/// <returns></returns>
size_t CTableMgr::Count() {
	return items_.size();
}

/// <summary>
/// 返回空闲的桌子数量
/// </summary>
/// <returns></returns>
size_t CTableMgr::FreeCount() {
	{
		READ_LOCK(mutex_);
		return freeItems_.size();
	}
}

/// <summary>
/// 返回有人的桌子数量
/// </summary>
/// <returns></returns>
size_t CTableMgr::UsedCount() {
	{
		READ_LOCK(mutex_);
		return usedItems_.size();
	}
}

/// <summary>
/// 直接返回指定桌子
/// </summary>
/// <param name="tableId"></param>
/// <returns></returns>
std::shared_ptr<CTable> CTableMgr::Get(uint16_t tableId) {
	switch (tableId)
	{
	case INVALID_TABLE:
		break;
	default: {
		//READ_LOCK(mutex_);
		if (tableId < items_.size()) {
			return items_[tableId];
		}
		break;
	}
	}
	return std::shared_ptr<CTable>();
}

/// <summary>
/// 直接返回指定桌子，如果能加入的话
/// </summary>
/// <param name="player"></param>
/// <param name="tableId"></param>
/// <returns></returns>
std::shared_ptr<CTable> CTableMgr::GetSuit(std::shared_ptr<CPlayer> const& player, uint16_t tableId) {
	switch (tableId)
	{
	case INVALID_TABLE:
		break;
	default: {
		//READ_LOCK(mutex_);
		if (tableId < items_.size()) {
			std::shared_ptr<CTable> table = items_[tableId];
			do {
				if (!table->CanJoinTable(player, INVALID_TABLE)) {
					break;
				}
				return table;
			} while (0);
		}
		break;
	}
	}
	return std::shared_ptr<CTable>();
}

/// <summary>
/// 返回指定桌子，前提是桌子未满
/// </summary>
/// <param name="tableId"></param>
/// <returns></returns>
std::shared_ptr<CTable> CTableMgr::Find(uint16_t tableId) {
	{
		READ_LOCK(mutex_);
		std::map<uint16_t, std::shared_ptr<CTable>>::iterator it = usedItems_.find(tableId);
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
/// <param name="player"></param>
/// <param name="excludeId"></param>
/// <returns></returns>
std::shared_ptr<CTable> CTableMgr::FindSuit(std::shared_ptr<CPlayer> const& player, uint16_t excludeId) {
	std::list<std::shared_ptr<CTable>> usedItems;
	{
		READ_LOCK(mutex_);
		std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint16_t, std::shared_ptr<CTable>> const& p) {
			return p.second;
			});
	}
	for (auto it : usedItems) {
		std::shared_ptr<CTable> table = it;
		if (table->CanJoinTable(player, excludeId)) {
			return table;
		}
	}
	return New();
}

/// <summary>
/// 取一个空闲桌子
/// </summary>
/// <returns></returns>
std::shared_ptr<CTable> CTableMgr::New() {
	{
		WRITE_LOCK(mutex_);
		if (!freeItems_.empty()) {
			std::shared_ptr<CTable> table = freeItems_.front();
			freeItems_.pop_front();
			ASSERT(table);
			ASSERT(table->GetTableId() >= 0);
			ASSERT(table->GetTableId() < items_.size());
			usedItems_[table->GetTableId()] = table;
			return table;
		}
	}
	return std::shared_ptr<CTable>();
}

/// <summary>
/// 回收
/// </summary>
/// <param name="tableId"></param>
void CTableMgr::Delete(uint16_t tableId) {
	ASSERT(tableId != INVALID_TABLE);
	{
		WRITE_LOCK(mutex_);
		std::map<uint16_t, std::shared_ptr<CTable>>::iterator it = usedItems_.find(tableId);
		//ASSERT_V(it != usedItems_.end(), "tableId=%d", tableId);
		if (it != usedItems_.end()) {
			std::shared_ptr<CTable>/*&*/ table = it->second;
			ASSERT(table);
			ASSERT(table->GetTableId() >= 0);
			ASSERT(table->GetTableId() < items_.size());
			usedItems_.erase(it);
			table->Reset();
			freeItems_.emplace_back(table);
		}
		else {
			ASSERT((usedItems_.size() + freeItems_.size()) == items_.size());
		}
		//size_t n = usedItems_.erase(tableId);
		//(void)n;
		//ASSERT_V(n == 0, "n=%d", n);
	}
	Errorf("%d used = %d free = %d", tableId, usedItems_.size(), freeItems_.size());
}

/// <summary>
/// 回收
/// </summary>
/// <param name="table"></param>
void CTableMgr::Delete(std::shared_ptr<CTable> const& table) {
	uint16_t tableId = table->GetTableId();
	ASSERT(tableId != INVALID_TABLE);
	{
		WRITE_LOCK(mutex_);
		std::map<uint16_t, std::shared_ptr<CTable>>::iterator it = std::find_if(usedItems_.begin(), usedItems_.end(),
			[&](Item const& kv) -> bool {
				return kv.second.get() == table.get();
			});
		//ASSERT_V(it != usedItems_.end(), "tableId=%d", tableId);
		if (it != usedItems_.end()) {
			std::shared_ptr<CTable>/*&*/ table = it->second;
			ASSERT(table);
			ASSERT(table->GetTableId() >= 0);
			ASSERT(table->GetTableId() < items_.size());
			ASSERT(table->GetTableId() == tableId);
			usedItems_.erase(it);
			table->Reset();
			freeItems_.emplace_back(table);
		}
		else {
			ASSERT((usedItems_.size() + freeItems_.size()) == items_.size());
		}
		//size_t n = usedItems_.erase(tableId);
		//(void)n;
		//ASSERT_V(n == 0, "n=%d", n);
	}
	Errorf("%d used = %d free = %d", tableId, usedItems_.size(), freeItems_.size());
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
			std::transform(usedItems_.begin(), usedItems_.end(), std::back_inserter(usedItems), [](std::pair<uint16_t, std::shared_ptr<CTable>> const& p) {
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