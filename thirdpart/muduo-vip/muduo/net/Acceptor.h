// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>

#include "muduo/net/Define.h"

#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"

namespace muduo
{
namespace net
{

class EventLoop;
//class EventLoopThread;
//class EventLoopThreadPool;
class InetAddress;
class InetRegion;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
 public:
  typedef std::function<bool(const InetAddress&, InetRegion&)> ConditionCallback;
#ifdef _MUDUO_ASYNC_CONN_POOL_
  typedef std::function<void (int sockfd, const InetAddress&, const InetRegion&, EventLoop*)> NewConnectionCallback;
#else
  typedef std::function<void(int sockfd, const InetAddress&, const InetRegion&)> NewConnectionCallback;
#endif

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }
  
  void setConditionCallback(const ConditionCallback& cb)
  {  conditionCallback_ = cb; }
  
  void listen(bool et = false);
  
  // Deprecated, use the correct spelling one above.
  // Leave the wrong spelling here in case one needs to grep it for error messages.
  bool listening() const { return listening_; }

 private:
  void handleRead(int events);

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  ConditionCallback conditionCallback_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_, et_;
  int idleFd_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ACCEPTOR_H
