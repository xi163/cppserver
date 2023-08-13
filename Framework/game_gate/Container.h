#ifndef INCLUDE_CONTAINER_H
#define INCLUDE_CONTAINER_H

#include "public/Inc.h"

#include "Clients.h"
#include "EntryPtr.h"

struct Repair {

	inline void add(std::string const& name) {
		WRITE_LOCK(mutex_);
#ifndef NDEBUG
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		assert(it == names_.end());
#endif
		names_[name] = true;
	}
	inline void remove(std::string const& name) {
		WRITE_LOCK(mutex_);
#if 0
		names_.erase(name);
#else
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		if (it != names_.end()) {
			names_.erase(it);
		}
#endif
	}
	inline bool exist(std::string const& name) /*const*/ {
		READ_LOCK(mutex_);
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		return it != names_.end();
	}
	inline size_t count() /*const*/ {
		READ_LOCK(mutex_);
		return names_.size();
	}
public:
	std::map<std::string, bool> names_;
	mutable boost::shared_mutex mutex_;
};

struct Container {

	void add(std::vector<std::string> const& names);

	void process(std::vector<std::string> const& names);

	//判断是否在指定类型服务中
	inline bool exist(std::string const& name) /*const*/ {
		return clients_->exists(name);
	}

	//活动节点数
	inline size_t count() /*const*/ {
		return clients_->count();
	}

	//维护节点
	inline void repair(std::string const& name) {
		repair_.add(name);
	}

	//恢复服务
	inline void recover(std::string const& name) {
		repair_.remove(name);
	}

	//判断是否在维护节点中
	inline bool isRepairing(std::string const& name) /*const*/ {
		return repair_.exist(name);
	}

	//服务中节点数 = 活动节点数 - 维护中节点数
	inline ssize_t remaining() /*const*/ {
		return clients_->count() - repair_.count();
	}
private:
	void add(std::string const& name);
	void remove(std::string const& name);
public:
	Repair repair_;
	containTy ty_;
	Connector* clients_;
	std::vector<std::string> names_;
	mutable boost::shared_mutex mutex_;
};

#endif