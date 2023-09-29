#ifndef _MUDUO_NET_REACTOR_
#define _MUDUO_NET_REACTOR_

#include "muduo/base/noncopyable.h"
#include "muduo/base/Atomic.h"
#include "muduo/base/Types.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"

#include <map>

namespace muduo {
	namespace net {

		//class EventLoop;
		//class EventLoopThreadPool;

		class ReactorSingleton : muduo::noncopyable {
		public:
			ReactorSingleton() = delete;
			~ReactorSingleton() = delete;
			
			static void init(muduo::net::EventLoop* loop, std::string const& name);

			static muduo::net::EventLoop* getNextLoop();

			static std::shared_ptr<muduo::net::EventLoopThreadPool> get();

			static void setThreadNum(int numThreads);

			static void start(const EventLoopThreadPool::ThreadInitCallback& cb = EventLoopThreadPool::ThreadInitCallback());

			static void quit();
		};

	}// namespace net
} //namespace muduo

#endif
