// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Connector.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)
{
  //LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
  //LOG_DEBUG << "dtor[" << this << "]";
  ASSERT(!channel_);
}

void Connector::start()
{
  connect_ = true;
  RunInLoop(loop_, std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  ASSERT(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    //LOG_DEBUG << "do not connect";
  }
}

void Connector::stop()
{
  connect_ = false;
  RunInLoop/*QueueInLoop*/(loop_, std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::stopInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnecting)
  {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
  else if (state_ == kConnected) {
      setState(kDisconnected);
      int sockfd = removeAndResetChannel();
  }
}

void Connector::connect()
{
  int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
  int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
  int saveErrno = (ret == 0) ? 0 : errno;
  switch (saveErrno)
  {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      //LOG_SYSERR << "connect error in Connector::startInLoop " << saveErrno;
      Errorf("%d %s", saveErrno, strerror_tl(saveErrno));
      sockets::close(sockfd);
      break;

    default:
      //LOG_SYSERR << "Unexpected error in Connector::startInLoop " << saveErrno;
      Errorf("%d %s", saveErrno, strerror_tl(saveErrno));
      sockets::close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::restart()
{
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  ASSERT(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  QueueInLoop(loop_, std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel()
{
  channel_.reset();
}

void Connector::handleWrite()
{
  //LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    if (err)
    {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = "
               << err << " " << strerror_tl(err);
      retry(sockfd);
    }
    else if (sockets::isSelfConnect(sockfd))
    {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
#ifdef _MUDUO_ASYNC_CONN_POOL_
          EventLoop* loop = EventLoopThreadPool::Singleton::getNextLoop_safe();
		  RunInLoop(loop, std::bind([this](int sockfd, EventLoop* loop) {
			  newConnectionCallback_(sockfd, loop);
		  }, sockfd, loop));
#elif defined(_MUDUO_ASYNC_CONN_)
		  RunInLoop(EventLoopThread::Singleton::getLoop(), std::bind([this](int sockfd) {
              newConnectionCallback_(sockfd);
	      }, sockfd));
#else
        newConnectionCallback_(sockfd);
#endif
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    ASSERT(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  //LOG_ERROR << "Connector::handleError state=" << state_;
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    /*int err = */sockets::getSocketError(sockfd);
    //LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    errno = 0;//reset
    retry(sockfd);
  }
}

void Connector::retry(int sockfd)
{
  sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_)
  {
    //LOG_INFO << "Connector::retry connecting to " << serverAddr_.toIpPort()
    //         << " in " << retryDelayMs_ << " milliseconds. ";
    loop_->runAfter(retryDelayMs_/1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
    //LOG_DEBUG << "do not connect";
  }
}

