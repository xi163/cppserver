// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Types.h"
#include "muduo/base/Atomic.h"

#include <functional>
#include <memory>
#include <vector>

namespace muduo
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());
  
  EventLoop* getBaseLoop();
  
  // valid after calling start()
  /// round-robin
  EventLoop* getNextLoop();
  
  EventLoop* getNextLoop_safe();
  
  /// with the same hash code, it will always return the same EventLoop
  EventLoop* getLoopForHash(size_t hashCode);

  std::vector<EventLoop*> getAllLoops();

  bool started() const
  { return started_; }

  const string& name() const
  { return name_; }

 private:

  EventLoop* baseLoop_;
  string name_;
  bool started_;
  int numThreads_;
  int next_;
  AtomicInt32 atomic_next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;

public:
	class Singleton : noncopyable {
	public:
		Singleton() = delete;
		~Singleton() = delete;

		static void init(EventLoop* loop, std::string const& name);
		
		static EventLoop* getBaseLoop();
		
		static EventLoop* getNextLoop();

		static std::shared_ptr<EventLoopThreadPool> get();

		static void setThreadNum(int numThreads);

		static void start(const ThreadInitCallback& cb = ThreadInitCallback());

		static void quit();
	private:
		static void quitInLoop();
		static std::shared_ptr<EventLoopThreadPool> pool_;
		static AtomicInt32 started_;
	};
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
