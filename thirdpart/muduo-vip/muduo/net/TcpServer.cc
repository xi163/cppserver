// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/TcpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
  : loop_(CHECK_NOTNULL(loop)),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
	acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    //threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1),
    ssl_ctx_(NULL),
	et_(false)
{
  EventLoopThreadPool::Singleton::init(loop, name_);
#ifdef _MUDUO_ASYNC_CONN_POOL_
  acceptor_->setNewConnectionCallback(
	  std::bind(&TcpServer::newConnection, this, _1, _2, _3, _4));
#else
  acceptor_->setNewConnectionCallback(
	  std::bind(&TcpServer::newConnection, this, _1, _2, _3));
#endif
}

TcpServer::~TcpServer()
{
  //loop_->assertInLoopThread();
  //LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto& item : connections_)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    RunInLoop(conn->getLoop(),
      std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
  ASSERT(0 <= numThreads);
  EventLoopThreadPool::Singleton::setThreadNum(numThreads);//threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(bool et)
{
  if (started_.getAndSet(1) == 0)
  {
    EventLoopThreadPool::Singleton::start(threadInitCallback_);//threadPool_->start(threadInitCallback_);

    ASSERT(!acceptor_->listening());
    if (conditionCallback_)
    {
      acceptor_->setConditionCallback(conditionCallback_);
    }
    et_ = et;
    RunInLoop(loop_,
        std::bind(&Acceptor::listen, get_pointer(acceptor_), et_));
  }
}

#ifdef _MUDUO_ASYNC_CONN_POOL_

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr, const InetRegion& peerRegion, EventLoop* ioLoop) {
	ioLoop->assertInLoopThread();
	char buf[64];
	snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	//LOG_INFO << "TcpServer::newConnection [" << name_
	//         << "] - new connection [" << connName
	//         << "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(ioLoop,
											connName,
											sockfd,
											localAddr,
											peerAddr,
											peerRegion,
											ssl_ctx_,
											et_));
	{
		MutexLockGuard lock(mutex_);
		connections_[connName] = conn;
	}
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
	RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	// FIXME: unsafe
	RunInLoop(conn->getLoop(), std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	//LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
	//         << "] - connection " << conn->name();
	//释放conn连接对象sockfd资源流程
	//TcpConnection::handleClose ->
	//TcpConnection::closeCallback_ ->
	//TcpServer::removeConnection ->
	//TcpServer::removeConnectionInLoop ->
	//TcpConnection::dtor ->
	//Socket::dtor -> sockets::close(sockfd_)
	{
		MutexLockGuard lock(mutex_);
		size_t n = connections_.erase(conn->name());
		(void)n;
		ASSERT_V(n == 1, "n=%d", n);
		Tracef("%d", connections_.size());
	}
	EventLoop* ioLoop = conn->getLoop();
	QueueInLoop(ioLoop,
		std::bind(&TcpConnection::connectDestroyed, conn));
}

#elif defined(_MUDUO_ASYNC_CONN_)

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr, const InetRegion& peerRegion) {
	EventLoopThread::Singleton::getLoop()->assertInLoopThread();
	EventLoop* ioLoop = EventLoopThreadPool::Singleton::getNextLoop();//threadPool_->getNextLoop();
	char buf[64];
	snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	//LOG_INFO << "TcpServer::newConnection [" << name_
	//         << "] - new connection [" << connName
	//         << "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(ioLoop,
											connName,
											sockfd,
											localAddr,
											peerAddr,
											peerRegion,
											ssl_ctx_,
											et_));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
	RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	// FIXME: unsafe
	RunInLoop(EventLoopThread::Singleton::getLoop(), std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
	EventLoopThread::Singleton::getLoop()->assertInLoopThread();
	//LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
	//         << "] - connection " << conn->name();
	//释放conn连接对象sockfd资源流程
	//TcpConnection::handleClose ->
	//TcpConnection::closeCallback_ ->
	//TcpServer::removeConnection ->
	//TcpServer::removeConnectionInLoop ->
	//TcpConnection::dtor ->
	//Socket::dtor -> sockets::close(sockfd_)
	size_t n = connections_.erase(conn->name());
	(void)n;
	ASSERT_V(n == 1, "n=%d", n);
	Tracef("%d", connections_.size());
	EventLoop* ioLoop = conn->getLoop();
	QueueInLoop(ioLoop,
		std::bind(&TcpConnection::connectDestroyed, conn));
}

#else

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr, const InetRegion& peerRegion)
{
  loop_->assertInLoopThread();
  EventLoop* ioLoop = EventLoopThreadPool::Singleton::getNextLoop();//threadPool_->getNextLoop();
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  //LOG_INFO << "TcpServer::newConnection [" << name_
  //         << "] - new connection [" << connName
  //         << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr,
										  peerRegion,
                                          ssl_ctx_,
                                          et_));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
  RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  conn->getLoop()->assertInLoopThread();
  // FIXME: unsafe
  RunInLoop(loop_, std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  //LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
  //         << "] - connection " << conn->name();
  //释放conn连接对象sockfd资源流程
  //TcpConnection::handleClose ->
  //TcpConnection::closeCallback_ ->
  //TcpServer::removeConnection ->
  //TcpServer::removeConnectionInLoop ->
  //TcpConnection::dtor ->
  //Socket::dtor -> sockets::close(sockfd_)
  size_t n = connections_.erase(conn->name());
  (void)n;
  ASSERT_V(n == 1, "n=%d", n);
  Tracef("%d", connections_.size());
  EventLoop* ioLoop = conn->getLoop();
  QueueInLoop(ioLoop,
      std::bind(&TcpConnection::connectDestroyed, conn));
}

#endif