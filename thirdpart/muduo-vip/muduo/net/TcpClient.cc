// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/TcpClient.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Connector.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace muduo
{
namespace net
{
namespace detail
{

void removeConnection(const TcpConnectionPtr& conn)
{
	EventLoop* ioLoop = conn->getLoop();
    QueueInLoop(ioLoop, std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

void removeAllInLoop(EventLoop* loop_, TcpConnectionPtr connection_, ConnectorPtr connector_) {
    loop_->assertInLoopThread();
	TcpConnectionPtr conn;
	bool unique = false;
	{
		//MutexLockGuard lock(mutex_);
		unique = connection_.unique();
		conn = connection_;
	}
	if (conn) {
		//ASSERT(loop_ == conn->getLoop());
		// FIXME: not 100% safe, if we are in different thread
		//CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
		//loop_->runInLoop(
		//	std::bind(&TcpConnection::setCloseCallback, conn, cb));

        //maybe that connector_->restart after TcpClient::dtor
        //connector_->stop();
		conn->setCloseCallback(
			std::bind(&detail::removeConnection, _1));
		if (unique)
		{
			conn->forceClose();
		}
	}
	else {
		connector_->stop();
		// FIXME: HACK
		loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
	}
}

}  // namespace detail
}  // namespace net
}  // namespace muduo

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& nameArg)
  : loop_(CHECK_NOTNULL(loop)),
    connector_(new Connector(loop, serverAddr)),
    name_(nameArg),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
#ifdef _MUDUO_ASYNC_CONN_POOL_
  connector_->setNewConnectionCallback(
	  std::bind(&TcpClient::newConnection, this, _1, _2));
#else
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));
#endif
  // FIXME setConnectFailedCallback
  //LOG_INFO << "TcpClient::TcpClient[" << name_
  //         << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
  //LOG_INFO << "TcpClient::~TcpClient[" << name_
  //         << "] - connector " << get_pointer(connector_);
  RunInLoop(loop_,
      std::bind(&detail::removeAllInLoop, loop_, connection_, connector_));
//  TcpConnectionPtr conn;
//   bool unique = false;
//   {
//     //MutexLockGuard lock(mutex_);
//     unique = connection_.unique();
//     conn = connection_;
//   }
//   if (conn)
//   {
//     ASSERT(loop_ == conn->getLoop());
//     // FIXME: not 100% safe, if we are in different thread
//     CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
//     loop_->runInLoop(
//         std::bind(&TcpConnection::setCloseCallback, conn, cb));
//     if (unique)
//     {
//       conn->forceClose();
//     }
//   }
//   else
//   {
//     connector_->stop();
//     // FIXME: HACK
//     loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
//   }
}

TcpConnectionPtr TcpClient::connection() const
{
  //MutexLockGuard lock(mutex_);
  loop_->assertInLoopThread();
  return connection_;
}

void TcpClient::connect()
{
  // FIXME: check state
  //LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
  //         << connector_->serverAddress().toIpPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::reconnect() {
	connect_ = true;
	RunInLoop(loop_,
		std::bind(&Connector::restart, get_pointer(connector_)));
}

void TcpClient::disconnect() {
	RunInLoop(loop_,
		std::bind(&TcpClient::disconnectInLoop, this));
}

void TcpClient::disconnectInLoop() {
  loop_->assertInLoopThread();
  connect_ = false;

  {
    //MutexLockGuard lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

#ifdef _MUDUO_ASYNC_CONN_POOL_

void TcpClient::newConnection(int sockfd, EventLoop* ioLoop) {
	ioLoop->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(ioLoop/*loop_*/,
											connName,
											sockfd,
											localAddr,
											peerAddr));
	{
		MutexLockGuard lock(mutex_);
		connection_ = conn;
	}
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
	//conn->connectEstablished();
	RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	// FIXME: unsafe
	RunInLoop(conn->getLoop(), std::bind(&TcpClient::removeConnectionInLoop, this, conn));
}

void TcpClient::removeConnectionInLoop(const TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	{
		MutexLockGuard lock(mutex_);
		ASSERT(connection_ == conn);
		connection_.reset();
	}
	EventLoop* ioLoop = conn->getLoop();
	QueueInLoop(/*loop_*/ioLoop,
		std::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connect_)
	{
		//LOG_WARN << "TcpClient::connect[" << name_ << "] - Reconnecting to "
		//         << connector_->serverAddress().toIpPort();
		Tracef("Reconnecting to %s", connector_->serverAddress().toIpPort().c_str());
		//maybe that connector_->restart after TcpClient::dtor
		RunInLoop(loop_, std::bind(&Connector::restart, connector_));
	}
}

#elif defined(_MUDUO_ASYNC_CONN_)

void TcpClient::newConnection(int sockfd) {
	EventLoopThread::Singleton::getLoop()->assertInLoopThread();
	EventLoop* ioLoop = EventLoopThreadPool::Singleton::getNextLoop();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	TcpConnectionPtr conn(new TcpConnection(ioLoop/*loop_*/,
											connName,
											sockfd,
											localAddr,
											peerAddr));
	{
		//MutexLockGuard lock(mutex_);
		connection_ = conn;
	}
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
		std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
	//conn->connectEstablished();
	RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	conn->getLoop()->assertInLoopThread();
	// FIXME: unsafe
	RunInLoop(EventLoopThread::Singleton::getLoop(), std::bind(&TcpClient::removeConnectionInLoop, this, conn));
}

void TcpClient::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	EventLoopThread::Singleton::getLoop()->assertInLoopThread();
	{
		//MutexLockGuard lock(mutex_);
		ASSERT(connection_ == conn);
		connection_.reset();
	}
	EventLoop* ioLoop = conn->getLoop();
	QueueInLoop(/*loop_*/ioLoop,
		std::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connect_)
	{
		//LOG_WARN << "TcpClient::connect[" << name_ << "] - Reconnecting to "
		//         << connector_->serverAddress().toIpPort();
		Tracef("Reconnecting to %s", connector_->serverAddress().toIpPort().c_str());
		//maybe that connector_->restart after TcpClient::dtor
		RunInLoop(loop_, std::bind(&Connector::restart, connector_));
	}
}

#else

void TcpClient::newConnection(int sockfd)
{
  loop_->assertInLoopThread();
  EventLoop* ioLoop = EventLoopThreadPool::Singleton::getNextLoop();
  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(ioLoop/*loop_*/,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
  {
	  //MutexLockGuard lock(mutex_);
	  connection_ = conn;
  }
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  //conn->connectEstablished();
  RunInLoop(ioLoop, std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  conn->getLoop()->assertInLoopThread();
  // FIXME: unsafe
  RunInLoop(loop_, std::bind(&TcpClient::removeConnectionInLoop, this, conn));
}

void TcpClient::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  //ASSERT(loop_ == conn->getLoop());

  {
    //MutexLockGuard lock(mutex_);
    ASSERT(connection_ == conn);
    connection_.reset();
  }
  EventLoop* ioLoop = conn->getLoop();
  QueueInLoop(/*loop_*/ioLoop,
      std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    //LOG_WARN << "TcpClient::connect[" << name_ << "] - Reconnecting to "
    //         << connector_->serverAddress().toIpPort();
	Tracef("Reconnecting to %s", connector_->serverAddress().toIpPort().c_str());
    //maybe that connector_->restart after TcpClient::dtor
    connector_->restart();
  }
}

#endif