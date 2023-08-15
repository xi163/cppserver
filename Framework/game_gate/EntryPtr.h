#ifndef INCLUDE_ENTRYPTR_H
#define INCLUDE_ENTRYPTR_H

#include "public/gameConst.h"
#include "Logger/src/log/Logger.h"
#include "clients/Clients.h"
#include "clients/Container.h"

//处理空闲连接，避免恶意连接占用sockfd不请求处理也不关闭，耗费系统资源，空闲超时将其强行关闭!
struct Entry : public muduo::noncopyable {
public:
	enum TypeE { HttpTy, TcpTy };
	explicit Entry(TypeE ty,
		const muduo::net::WeakTcpConnectionPtr& weakConn,
		std::string const& peerName, std::string const& localName)
		: ty_(ty), locked_(false), weakConn_(weakConn)
		, peerName_(peerName), localName_(localName) {
	}
	explicit Entry(TypeE ty,
		const muduo::net::WeakTcpConnectionPtr& weakConn, const boost::any& context,
		std::string const& peerName, std::string const& localName)
		: ty_(ty), locked_(false), weakConn_(weakConn), context_(context)
		, peerName_(peerName), localName_(localName) {
	}
	~Entry();
	inline muduo::net::WeakTcpConnectionPtr const& getWeakConnPtr() {
		return weakConn_;
	}
	inline boost::any& getContext() {
		return context_;
	}
	inline const boost::any& getContext() const {
		return context_;
	}
	inline boost::any* getMutableContext() {
		return &context_;
	}
	//锁定同步业务操作
	inline void setLocked(bool locked = true) { locked_ = locked; }
	inline bool getLocked() { return locked_; }
	TypeE ty_;
	bool locked_;
	std::string peerName_, localName_;
	muduo::net::WeakTcpConnectionPtr weakConn_;
	boost::any context_;
};

typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> WeakEntryPtr;
//boost::unordered_set改用std::unordered_set
typedef std::unordered_set<EntryPtr> Bucket;

struct Buckets /*: public muduo::noncopyable*/ {
	explicit Buckets(muduo::net::EventLoop* loop, size_t size, int interval)
		: loop_(CHECK_NOTNULL(loop))
		, interval_(interval) {
		buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
		_LOG_WARN("timeout = %ds", size);
#endif
	}
	//定时器弹出操作，强行关闭空闲超时连接!
	void pop() {
		loop_->assertInLoopThread();
		//////////////////////////////////////////////////////////////////////////
		//shared_ptr/weak_ptr 引用计数lock持有/递减是读操作，线程安全!
		//shared_ptr/unique_ptr new创建与reset释放是写操作，非线程安全，操作必须在同一线程!
		//new时内部引用计数递增，reset时递减，递减为0时销毁对象释放资源
		//////////////////////////////////////////////////////////////////////////
		buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
		_LOG_WARN("timeout = %ds", buckets_.size());
#endif
		loop_->runAfter(interval_, std::bind(&Buckets::pop, this));
	}
	void push(EntryPtr const& entry) {
		loop_->assertInLoopThread();
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				assert(conn->getLoop() == loop_);
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				_LOG_WARN("timeout = %ds %s[%s] <- %s[%s]",
					buckets_.size(),
					entry->localName_.c_str(), conn->localAddress().toIpPort().c_str(),
					entry->peerName_.c_str(), conn->peerAddress().toIpPort().c_str());
#endif
			}
		}
		else {
			//assert(false);
		}
	}
	void update(EntryPtr const& entry) {
		loop_->assertInLoopThread();
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				assert(conn->getLoop() == loop_);
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				_LOG_WARN("timeout = %ds %s[%s] <- %s[%s]",
					buckets_.size(),
					entry->localName_.c_str(), conn->localAddress().toIpPort().c_str(),
					entry->peerName_.c_str(), conn->peerAddress().toIpPort().c_str());
#endif
			}
		}
		else {
		}
	}
private:
	int interval_;
	muduo::net::EventLoop* loop_;
	boost::circular_buffer<Bucket> buckets_;
};

struct Context /*: public muduo::noncopyable*/ {
	explicit Context(WeakEntryPtr const& weakEntry)
		: weakEntry_(weakEntry) {
		reset();
	}
	explicit Context(WeakEntryPtr const& weakEntry, const boost::any& context)
		: weakEntry_(weakEntry), context_(context) {
		assert(!context_.empty());
		reset();
	}
	virtual ~Context() {
		reset();
	}
	inline void reset() {
		kicking_ = KICK_NULL;
		ipaddr_ = 0;
		userid_ = 0;
		session_.clear();
		aeskey_.clear();
		//此处调用无效
#if 0
		//引用计数加1
		EntryPtr entry(weakEntry_.lock());
		if (likely(entry)) {
			//引用计数减1，减为0时析构entry对象
			//因为bucket持有引用计数，所以entry直到超时才析构
			entry.reset();
		}
#endif
	}
	inline std::shared_ptr<muduo::ThreadPool> const& getWorker() {
		return thread_;
	}
	//给新conn指定worker线程，与之相关所有逻辑业务都在该线程中处理
	inline void setWorker(
		std::string const& session,
		std::hash<std::string> hash_session_,
		std::vector<std::shared_ptr<muduo::ThreadPool>>& threadPool_) {
		int index = hash_session_(session) % threadPool_.size();
		thread_ = threadPool_[index];
	}
	//对于HTTP请求来说，每一个conn都应该是独立的，指定一个独立线程处理即可，避免锁开销与多线程竞争抢占共享资源带来的性能损耗
	inline void setWorker(
		muduo::AtomicInt32& nextPool_,
		std::vector<std::shared_ptr<muduo::ThreadPool>>& threadPool_) {
		int index = nextPool_.getAndAdd(1) % threadPool_.size();
		if (index < 0) {
			nextPool_.getAndSet(-1);
			index = nextPool_.addAndGet(1);
		}
		thread_ = threadPool_[index];
	}
	inline void setContext(const boost::any& context) {
		context_ = context;
	}
	inline boost::any& getContext() {
		return context_;
	}
	inline const boost::any& getContext() const {
		return context_;
	}
	inline boost::any* getMutableContext() {
		return &context_;
	}
	inline WeakEntryPtr const& getWeakEntryPtr() {
		return weakEntry_;
	}
	inline void setkicking(int kicking) { kicking_ = kicking; }
	inline int getKicking() { return kicking_; }
	inline void setFromIp(in_addr_t inaddr) { ipaddr_ = inaddr; }
	inline in_addr_t getFromIp() { return ipaddr_; }
	inline void setSession(std::string const& session) { session_ = session; }
	inline std::string const& getSession() const { return session_; }
	inline void setUserId(int64_t userid) { userid_ = userid; }
	inline int64_t getUserId() const { return userid_; }
	inline void setAesKey(std::string key) { aeskey_ = key; }
	inline std::string const& getAesKey() const { return aeskey_; }
	inline void setClientConn(containTy ty,
		std::string const& name,
		muduo::net::WeakTcpConnectionPtr const& weakConn) {
		assert(!name.empty());
		client_[ty].first = name;
		client_[ty].second = weakConn;
	}
	inline void setClientConn(containTy ty, ClientConn const& client) {
		client_[ty] = client;
	}
	inline void resetClientConn(containTy ty) {
		muduo::net::WeakTcpConnectionPtr weakConn;
		client_[ty].second = weakConn;
		switch (ty) {
		case kHallTy:
			_LOG_WARN("[kHallTy] %d", userid_);
			break;
		case kGameTy:
			_LOG_TRACE("[kGameTy] %d", userid_);
			break;
		}
	}
	inline ClientConn const& getClientConn(containTy ty) { return client_[ty]; }
public:
	int kicking_;
	uint32_t ipaddr_;
	int64_t userid_;
	std::string session_;
	std::string aeskey_;
	ClientConn client_[kMaxContainTy];
	WeakEntryPtr weakEntry_;
	boost::any context_;
	std::shared_ptr<muduo::ThreadPool> thread_;
};

#endif