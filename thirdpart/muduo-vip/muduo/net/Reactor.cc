
#include "Reactor.h"
#include "Logger/src/utils/Assert.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <stdio.h>  // snprintf

namespace muduo {
	namespace net {
		static muduo::net::EventLoop* baseLoop_;
		static std::shared_ptr<muduo::net::EventLoopThreadPool> pool_;
		static AtomicInt32 started_;

		void ReactorSingleton::init(muduo::net::EventLoop* loop, std::string const& name) {
			if (!pool_) {
				pool_.reset(new muduo::net::EventLoopThreadPool(CHECK_NOTNULL(loop), name));
				baseLoop_ = loop;
			}
		}

		std::shared_ptr<muduo::net::EventLoopThreadPool> ReactorSingleton::get() {
			ASSERT_S(pool_, "pool is nil");
			return pool_;
		}

		muduo::net::EventLoop* ReactorSingleton::getNextLoop() {
			ASSERT_S(pool_, "pool is nil");
			return pool_->getNextLoop();
		}
		
		void ReactorSingleton::setThreadNum(int numThreads) {
			ASSERT_S(pool_, "pool is nil");
			ASSERT_V(numThreads > 0, "numThreads:%d", numThreads);
			pool_->setThreadNum(numThreads);
		}

		void ReactorSingleton::start(const EventLoopThreadPool::ThreadInitCallback& cb) {
			if (started_.getAndSet(1) == 0) {
				assert(pool_);
				pool_->start(cb);
			}
		}

		static void quitInLoop() {
			std::vector<muduo::net::EventLoop*> loops = pool_->getAllLoops();
			for (std::vector<muduo::net::EventLoop*>::iterator it = loops.begin();
				it != loops.end(); ++it) {
				(*it)->quit();
			}
			pool_.reset();
		}

		void ReactorSingleton::quit() {
			ASSERT_S(pool_, "pool is nil");
			ASSERT_S(baseLoop_, "loop is nil");
			if (pool_->started()) {
				RunInLoop(baseLoop_, std::bind(&quitInLoop));
			}
		}

	}// namespace net
} //namespace muduo