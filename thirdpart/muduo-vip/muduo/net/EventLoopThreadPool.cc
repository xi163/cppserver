// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoopThreadPool.h"

#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"

#include <stdio.h>

#include "Logger/src/utils/Assert.h"

#include "muduo/net/Define.h"

using namespace muduo;
using namespace muduo::net;

void EventLoopThreadPool::Singleton::init(EventLoop* loop, std::string const& name) {
	if (!pool_) {
#ifdef _MUDUO_ASYNC_CONN_
		EventLoopThread::Singleton::init();
		EventLoopThread::Singleton::start();
		pool_.reset(new EventLoopThreadPool(ASSERT_NOTNULL(EventLoopThread::Singleton::getLoop()), name));
#else
		pool_.reset(new EventLoopThreadPool(ASSERT_NOTNULL(loop), name));
#endif
	}
}

std::shared_ptr<EventLoopThreadPool> EventLoopThreadPool::Singleton::get() {
	ASSERT_S(pool_, "pool is nil");
	return pool_;
}

EventLoop* EventLoopThreadPool::Singleton::getBaseLoop() {
	ASSERT_S(pool_, "pool is nil");
	return pool_->getBaseLoop();
}

EventLoop* EventLoopThreadPool::Singleton::getNextLoop() {
	ASSERT_S(pool_, "pool is nil");
	return pool_->getNextLoop();
}

EventLoop* EventLoopThreadPool::Singleton::getNextLoop_safe() {
	ASSERT_S(pool_, "pool is nil");
	return pool_->getNextLoop_safe();
}

void EventLoopThreadPool::Singleton::setThreadNum(int numThreads) {
	ASSERT_S(pool_, "pool is nil");
	ASSERT_V(numThreads > 0, "numThreads:%d", numThreads);
	pool_->setThreadNum(numThreads);
}

void EventLoopThreadPool::Singleton::start(const EventLoopThreadPool::ThreadInitCallback& cb) {
	if (started_.getAndSet(1) == 0) {
		assert(pool_);
		pool_->start(cb);
	}
}

void EventLoopThreadPool::Singleton::quitInLoop() {
	std::vector<EventLoop*> loops = pool_->getAllLoops();
	for (std::vector<EventLoop*>::iterator it = loops.begin();
		it != loops.end(); ++it) {
		(*it)->quit();
	}
	pool_.reset();
#ifdef _MUDUO_ASYNC_CONN_
	EventLoopThread::Singleton::quit();
#endif
}

void EventLoopThreadPool::Singleton::quit() {
	ASSERT_S(pool_, "pool is nil");
	if (pool_->started()) {
		RunInLoop(pool_->getBaseLoop(), std::bind(&EventLoopThreadPool::Singleton::quitInLoop));
	}
}

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
  : baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    EventLoopThread* t = new EventLoopThread(cb, buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }
  if (numThreads_ == 0 && cb)
  {
    cb(baseLoop_);
  }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop* loop = baseLoop_;

  if (!loops_.empty())
  {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop* EventLoopThreadPool::getNextLoop_safe()
{
	assert(started_);
	EventLoop* loop = baseLoop_;
	if (!loops_.empty())
	{
#if 0
		loop = loops_[atomic_next_.getAndAdd(1)];
		if (implicit_cast<size_t>(atomic_next_.get()) >= loops_.size()) {
            atomic_next_.getAndSet(0);
		}
#else
		int index = atomic_next_.getAndAdd(1) % loops_.size();
		if (index < 0) {
            atomic_next_.getAndSet(0);
			index = atomic_next_.getAndAdd(1);
		}
		loop = loops_[index];
#endif
	}
	return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  baseLoop_->assertInLoopThread();
  EventLoop* loop = baseLoop_;

  if (!loops_.empty())
  {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty())
  {
    return std::vector<EventLoop*>(1, baseLoop_);
  }
  else
  {
    return loops_;
  }
}
