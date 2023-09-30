#ifndef INCLUDE_TABLE_THREAD_H
#define INCLUDE_TABLE_THREAD_H

#include "Logger/src/Macro.h"
#include "Packet/Packet.h"
#include "ITableContext.h"

class CTableThread : public muduo::noncopyable {
public:
	CTableThread(muduo::net::EventLoop* loop, ITableContext* tableContext);
	virtual ~CTableThread();
public:
	void append(uint16_t tableId);
	void startCheckUserIn();
private:
	bool enable();
	void checkUserIn();
	void hourtimer(tagGameRoomInfo* roomInfo);
private:
	int64_t randScore(tagGameRoomInfo* roomInfo, int64_t minScore, int64_t maxScore);
	int randomOnce(int32_t need, int N = 3);
protected:
	STD::Weight weight_;
	double_t percentage_;
	std::vector<uint16_t> tableId_;
	muduo::net::EventLoop* loop_;
	ITableContext* tableContext_;
};

typedef std::shared_ptr<CTableThread> LogicThreadPtr;

class CTableThreadMgr : public boost::serialization::singleton<CTableThreadMgr> {
public:
	CTableThreadMgr()/* = default*/;
	virtual ~CTableThreadMgr()/* = default*/;
public:
	void Init(muduo::net::EventLoop* loop, std::string const& name);

	muduo::net::EventLoop* getNextLoop();

	std::shared_ptr<muduo::net::EventLoopThreadPool> get();

	void setThreadNum(int numThreads);

	void start(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb, ITableContext* tableContext);

	void startCheckUserIn(ITableContext* tableContext);
	
	void quit();
private:
	void startInLoop(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb);
	void quitInLoop();
private:
	std::shared_ptr<muduo::net::EventLoopThreadPool> pool_;
	muduo::AtomicInt32 started_;
};

#endif