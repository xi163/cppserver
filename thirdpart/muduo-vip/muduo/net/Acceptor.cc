// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/Acceptor.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
    acceptChannel_(loop, acceptSocket_.fd()),
    listening_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(
      std::bind(&Acceptor::handleRead, this, std::placeholders::_2));
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();
  ::close(idleFd_);
}

void Acceptor::listen(bool et)
{
  loop_->assertInLoopThread();
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading(et);
}

void Acceptor::handleRead(int events)
{
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  bool et = acceptChannel_.isETReading();/* EPOLLET */
  do {
      //FIXME loop until no more
      int connfd = acceptSocket_.accept(&peerAddr);
      if (connfd >= 0)
      {
          // string hostport = peerAddr.toIpPort();
          // LOG_TRACE << "Accepts of " << hostport;
#ifdef _MUDUO_ACCEPT_CONNPOOL_
          //The IO threads is shared with the accept threads
		  EventLoop* loop = EventLoopThreadPool::Singleton::getNextLoop_safe();
		  RunInLoop(loop, std::bind([this](int connfd, InetAddress const& peerAddr, EventLoop* loop) {
			  if (conditionCallback_ && !conditionCallback_(peerAddr))
			  {
				  sockets::close(connfd);
			  }
			  else if (newConnectionCallback_)
			  {
				  newConnectionCallback_(connfd, peerAddr, loop);
			  }
			  else
			  {
				  sockets::close(connfd);
			  }
		  }, connfd, peerAddr, loop));
#elif defined(_MUDUO_ACCEPT_CONNASYN_)
          //Asynchronous accept thread
		  RunInLoop(EventLoopThread::Singleton::getAcceptLoop(), std::bind([this](int connfd, InetAddress const& peerAddr) {
			  if (conditionCallback_ && !conditionCallback_(peerAddr))
			  {
				  sockets::close(connfd);
			  }
			  else if (newConnectionCallback_)
			  {
				  newConnectionCallback_(connfd, peerAddr);
			  }
			  else
			  {
				  sockets::close(connfd);
			  }
	      }, connfd, peerAddr));
#else
          if (conditionCallback_ && !conditionCallback_(peerAddr))
          {
              sockets::close(connfd);
          }
          else if (newConnectionCallback_)
          {
              newConnectionCallback_(connfd, peerAddr);
          }
          else
          {
              sockets::close(connfd);
          }
#endif
      }
      else {
		  if (errno != EAGAIN &&
			  errno != EWOULDBLOCK &&
			  errno != ECONNABORTED &&
			  errno != EPROTO &&
			  errno != EINTR) {
			  //break;
		  }
          //ERROR Too many open files (errno=24) in Acceptor::handleRead - Acceptor.cc:80
          //LOG_SYSERR << "in Acceptor::handleRead";
          // Read the section named "The special problem of
          // accept()ing when you can't" in libev's doc.
          // By Marc Lehmann, author of libev.
          if (errno == EMFILE)
          {
              ::close(idleFd_);
              idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
              ::close(idleFd_);
              idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
          }
          break;
      }
  } while (et);
}

