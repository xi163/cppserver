#ifndef INCLUDE_TABLEMGR_H
#define INCLUDE_TABLEMGR_H

#include "public/Inc.h"
#include "gameDefine.h"
#include "Packet.h"

#include "Table.h"

template<typename T>
struct second_t {
	typename T::second_type operator()(T const& p) const {
		return p.second;
	}
};

template<typename T>
second_t<typename T::value_type> second(T const& m) {
	return second_t<typename T::value_type>();
}

class CTableMgr : public boost::serialization::singleton<CTableMgr> {
public:
	CTableMgr();
	virtual ~CTableMgr();
	void Clear();
	std::list<std::shared_ptr<CTable>> GetUsedTables();
	void Init(tagGameInfo* gameInfo, tagGameRoomInfo* roomInfo, std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext);
	std::shared_ptr<CTable> GetTable(uint32_t tableId);
	void KickAllTableUsers();
	/// <summary>
	/// 返回指定桌子，前提是桌子未满
	/// </summary>
	std::shared_ptr<CTable> FindNormalTable(uint32_t tableId);
	/// <summary>
	/// 查找能进的桌子，没有则取空闲卓子
	/// </summary>
	std::shared_ptr<CTable> FindSuitTable(std::shared_ptr<IPlayer> const& player, uint32_t exceptTableId = INVALID_TABLE);
	void FreeNormalTable(uint32_t tableId);
	bool SetTableStockInfo(tagStockInfo& stockInfo);
protected:
	tagGameInfo* gameInfo_;
	tagGameRoomInfo* roomInfo_;
	tagStockInfo stockInfo_;
	std::vector<std::shared_ptr<CTable>> items_;//items_[tableId]
	std::map<uint32_t, std::shared_ptr<CTable>> usedItems_;
	std::list<std::shared_ptr<CTable>> freeItems_;
	mutable boost::shared_mutex mutex_;
};

#endif