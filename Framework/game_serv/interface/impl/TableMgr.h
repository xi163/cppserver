#ifndef INCLUDE_TABLEMGR_H
#define INCLUDE_TABLEMGR_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "Packet.h"

#include "Table.h"
#include "ITableContext.h"
#include "Player.h"

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
	std::list<std::shared_ptr<CTable>> UsedTables();
	void Init(std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext);
	/// <summary>
	/// 返回有人的桌子数量
	/// </summary>
	size_t UsedCount();
	std::shared_ptr<CTable> Get(uint32_t tableId);
	std::shared_ptr<CTable> GetSuit(std::shared_ptr<CPlayer> const& player, uint32_t tableId);
	/// <summary>
	/// 返回指定桌子，前提是桌子未满
	/// </summary>
	std::shared_ptr<CTable> Find(uint32_t tableId);
	/// <summary>
	/// 查找能进的桌子，没有则取空闲桌子
	/// </summary>
	std::shared_ptr<CTable> FindSuit(std::shared_ptr<CPlayer> const& player, uint32_t exceptTableId = INVALID_TABLE);
	std::shared_ptr<CTable> New();
	void Delete(uint32_t tableId);
	/// <summary>
	/// 踢出所有桌子玩家
	/// </summary>
	void KickAll();
protected:
	ITableContext* tableContext_;
	std::vector<std::shared_ptr<CTable>> items_;//items_[tableId]
	std::map<uint32_t, std::shared_ptr<CTable>> usedItems_;
	std::list<std::shared_ptr<CTable>> freeItems_;
	mutable boost::shared_mutex mutex_;
};

#endif