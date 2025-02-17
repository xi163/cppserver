// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoopThread.h"

#include "muduo/net/EventLoop.h"

#include "Logger/src/log/Assert.h"

using namespace muduo;
using namespace muduo::net;

std::shared_ptr<EventLoopThread> EventLoopThread::Singleton::thread_;
AtomicInt32 EventLoopThread::Singleton::started_;

void EventLoopThread::Singleton::init(const EventLoopThread::ThreadInitCallback& cb) {
	if (!thread_) {
		thread_.reset(new EventLoopThread(cb, "connThread"));
	}
}

EventLoop* EventLoopThread::Singleton::getLoop() {
	ASSERT_S(thread_, "connThread is nil");
	return thread_->getLoop();
}

void EventLoopThread::Singleton::start() {
    ASSERT_S(thread_, "connThread is nil");
    if (!thread_->getLoop() && started_.getAndSet(1) == 0) {
		thread_->startLoop();
	}
}

void EventLoopThread::Singleton::quit() {
    if (started_.getAndSet(0) == 1) {
        ASSERT_S(thread_, "connThread is nil");
        ASSERT_S(thread_->getLoop(), "connThread::getLoop is nil");
        thread_->getLoop()->quit();
    }
}

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
  : loop_(NULL),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  if (loop_ != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just now.
    // but when EventLoopThread destructs, usually programming is exiting anyway.
    loop_->quit();
    thread_.join();
  }
}

EventLoop* EventLoopThread::startLoop()
{
  ASSERT(!thread_.started());
  thread_.start();

  EventLoop* loop = NULL;
  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();
    }
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;

  if (callback_)
  {
    callback_(&loop);
  }

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  //ASSERT(exiting_);
  MutexLockGuard lock(mutex_);
  loop_ = NULL;
}

