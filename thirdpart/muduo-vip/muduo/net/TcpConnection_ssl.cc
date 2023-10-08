// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/TcpConnection.h"

#include "muduo/base/Logging.h"
#include "muduo/base/WeakCallback.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>

#include <muduo/net/websocket/context.h>
#include <libwebsocket/ssl.h>

using namespace muduo;
using namespace muduo::net;

void TcpConnection::sendInLoop_ssl(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
    //LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
	int saveErrno = 0;
    nwrote = Buffer::SSL_write(ssl_, data, len, &saveErrno);//SSL_write
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        QueueInLoop(loop_, std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }

  ASSERT(remaining <= len);
  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      QueueInLoop(loop_, std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting(et_);
    }
  }
}

void TcpConnection::handleRead_ssl(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  if (ssl_ctx_ && !sslConnected_) {
	  int saveErrno = 0;
	  sslConnected_ = ssl::SSL_handshake(ssl_ctx_, ssl_, socket_->fd(), saveErrno);//SSL_handshake
	  switch (saveErrno)
	  {
	  case SSL_ERROR_WANT_READ:
		  channel_->enableReading(et_);
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting(et_);
		  break;
	  case SSL_ERROR_SSL:
		  Tracef("close fd=%d rc=%d", socket_->fd(), saveErrno);
		  handleClose();
		  break;
	  case 0:
		  //succ
		  break;
	  default:
          Tracef("close fd=%d rc=%d", socket_->fd(), saveErrno);
		  handleClose();
		  break;
	  }
	  return;
  }
  ASSERT(channel_->isReading());
  ASSERT(!channel_->isET());
  int saveErrno = 0;
  ssize_t n = inputBuffer_.SSL_read(ssl_, &saveErrno);//SSL_read
  if (n > 0)
  {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = saveErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite_ssl()
{
  loop_->assertInLoopThread();
  if (ssl_ctx_ && !sslConnected_) {
	  int saveErrno = 0;
	  sslConnected_ = ssl::SSL_handshake(ssl_ctx_, ssl_, socket_->fd(), saveErrno);//SSL_handshake
	  switch (saveErrno)
	  {
	  case SSL_ERROR_WANT_READ:
		  channel_->enableReading(et_);
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting(et_);
		  break;
	  case SSL_ERROR_SSL:
          Tracef("close fd=%d rc=%d", socket_->fd(), saveErrno);
		  handleClose();
		  break;
	  case 0:
		  //succ
		  break;
	  default:
          Tracef("close fd=%d rc=%d", socket_->fd(), saveErrno);
		  handleClose();
		  break;
	  }
	  return;
  }
  if (channel_->isWriting())
  {
    ASSERT(!channel_->isET());
    int saveErrno = 0;
    ssize_t n = Buffer::SSL_write(ssl_,
                    outputBuffer_.peek(),
                    outputBuffer_.readableBytes(), &saveErrno);//SSL_write
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting(et_);
        if (writeCompleteCallback_)
        {
            QueueInLoop(loop_, std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_SYSERR << "TcpConnection::handleWrite";
      // if (state_ == kDisconnecting)
      // {
      //   shutdownInLoop();
      // }
    }
  }
  else
  {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}