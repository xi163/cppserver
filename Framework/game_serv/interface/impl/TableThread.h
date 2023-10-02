#ifndef INCLUDE_TABLE_THREAD_H
#define INCLUDE_TABLE_THREAD_H

#include "Logger/src/Macro.h"
#include "Packet/Packet.h"
#include "ITableContext.h"

/// <summary>
/// 桌子线程
/// </summary>
class CTableThread : public muduo::noncopyable {
public:
	CTableThread(muduo::net::EventLoop* loop, ITableContext* tableContext);
	virtual ~CTableThread();
public:
	/// <summary>
	/// 添加要管理的桌子
	/// </summary>
	/// <param name="tableId"></param>
	void append(uint16_t tableId);

	/// <summary>
	/// 开启定时器，机器人入场检查
	/// </summary>
	void startCheckUserIn();
private:

	/// <summary>
	/// 判断机器人是否开启
	/// </summary>
	/// <returns></returns>
	bool enable();

	/// <summary>
	/// 机器人入场检查
	/// </summary>
	void checkUserIn();

	/// <summary>
	/// 用于百人场人数随机波动
	/// </summary>
	/// <param name="roomInfo"></param>
	void hourtimer(tagGameRoomInfo* roomInfo);
private:
	/// <summary>
	/// 机器人入场随机积分
	/// </summary>
	/// <param name="roomInfo"></param>
	/// <param name="minScore"></param>
	/// <param name="maxScore"></param>
	/// <returns></returns>
	int64_t randScore(tagGameRoomInfo* roomInfo, int64_t minScore, int64_t maxScore);

	/// <summary>
	/// 一次进n个机器人
	/// </summary>
	/// <param name="need"></param>
	/// <param name="N"></param>
	/// <returns></returns>
	int randomOnce(int32_t need, int N = 3);
protected:
	STD::Weight weight_;
	double_t percentage_;
	std::vector<uint16_t> tableId_;
	muduo::net::EventLoop* loop_;
	ITableContext* tableContext_;
};

typedef std::shared_ptr<CTableThread> LogicThreadPtr;

/// <summary>
/// 桌子线程管理
/// </summary>
class CTableThreadMgr : public boost::serialization::singleton<CTableThreadMgr> {
public:
	CTableThreadMgr()/* = default*/;
	virtual ~CTableThreadMgr()/* = default*/;
public:
	/// <summary>
	/// 初始化
	/// </summary>
	/// <param name="loop"></param>
	/// <param name="name"></param>
	void Init(muduo::net::EventLoop* loop, std::string const& name);

	muduo::net::EventLoop* getNextLoop();

	void getAllLoops(std::vector<muduo::net::EventLoop*>& vec);

	void setThreadNum(int numThreads);

	void start(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb, ITableContext* tableContext);

	/// <summary>
	/// 开启定时器，机器人入场检查
	/// </summary>
	/// <param name="tableContext"></param>
	void startCheckUserIn(ITableContext* tableContext);
	
	void quit();
private:
	void getAllLoopsInLoop(std::vector<muduo::net::EventLoop*>& vec, bool& ok);
	void startInLoop(const muduo::net::EventLoopThreadPool::ThreadInitCallback& cb);
	void quitInLoop();
private:
	std::shared_ptr<muduo::net::EventLoopThreadPool> pool_;
	muduo::AtomicInt32 started_;
};

#endif