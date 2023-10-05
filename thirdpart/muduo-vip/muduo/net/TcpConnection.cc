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

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  //LOG_TRACE << conn->localAddress().toIpPort() << " -> "
  //          << conn->peerAddress().toIpPort() << " is "
  //          << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr,
                             SSL_CTX* ctx,
                             bool et)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024),
    ssl_ctx_(ctx),
    ssl_(NULL),
    sslConnected_(false),
    et_(et)
{
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
  //LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
  //          << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  //释放conn连接对象sockfd资源流程
  //TcpConnection::handleClose ->
  //TcpConnection::closeCallback_ ->
  //TcpServer::removeConnection ->
  //TcpServer::removeConnectionInLoop ->
  //TcpConnection::dtor ->
  //Socket::dtor -> sockets::close(sockfd_)
  //LOG_WARN << "TcpConnection::dtor[" <<  name_ << "] at " << this
  //          << " fd=" << channel_->fd()
  //          << " state=" << stateToString();
  ASSERT(socket_->fd() == channel_->fd());
  Debugf("fd=%d", channel_->fd());
  //ssl::SSL_free(ssl_);
  //ASSERT(state_ == kDisconnected);
  ASSERT(state_ == kDisconnected ||
         state_ == kDisconnecting);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}

int TcpConnection::getFd() const {
    return socket_->fd();
}

void TcpConnection::setWsContext(WsContextPtr& context) {
    wsContext_ = std::move(context);
}

WsContextPtr& TcpConnection::getWsContext() {
    return wsContext_;
}

void TcpConnection::send(const void* data, int len)
{
  send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message.data(), message.size()/*message*/);
    }
    else
    {
#if 1
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      QueueInLoop(loop_,
          std::bind(fp,
                    this,     // FIXME
                    message.as_string()));
                    //std::forward<string>(message)));
#else
        std::string msg(message.data(), message.size());
        QueueInLoop(loop_,
			std::bind(&TcpConnection::sendInLoop_,
				this, msg));
#endif
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
#if 1
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      QueueInLoop(loop_,
          std::bind(fp,
                    this,     // FIXME
                    buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
#else
      std::string msg(buf->peek(), buf->readableBytes());
      QueueInLoop(loop_,
         std::bind(&TcpConnection::sendInLoop_,
                    this, msg));
#endif
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop_(const std::string& message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
    bool no_ssl = !ssl_;
	switch (no_ssl) {
	case true: {
		switch (et_) {
		case true:
            sendInLoop_et(data, len);
			break;
		default:
            sendInLoop_(data, len);
			break;
		}
		break;
	}
	default: {
		switch (et_) {
		case true:
            sendInLoop_ssl_et(data, len);
			break;
		default:
            sendInLoop_ssl(data, len);
			break;
		}
		break;
	}
	}
}

void TcpConnection::sendInLoop_(const void* data, size_t len)
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
    nwrote = sockets::write(channel_->fd(), data, len);
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

void TcpConnection::shutdown()
{
  Debugf("fd=%d", socket_->fd());
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    RunInLoop(loop_, std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    //ssl::SSL_free(ssl_);
    // we are not writing
    socket_->shutdownWrite();
  }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(std::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose()
{
  Debugf("fd=%d", socket_->fd());
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    QueueInLoop(loop_, std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
  Debugf("fd=%d", socket_->fd());
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->runAfter(
        seconds,
        makeWeakCallback(shared_from_this(),
                         &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
  }
}

void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    RunInLoop(loop_, std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading(et_);
    reading_ = true;
  }
}

void TcpConnection::stopRead()
{
    RunInLoop(loop_, std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading(et_);
    reading_ = false;
  }
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  ASSERT_V(state_ == kConnecting, "state_=%s", stateToString());
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading(et_);

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  //ASSERT_V(state_ == kConnected || state_ == kDisconnecting, "state_=%s", stateToString());//kDisconnected
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
	bool no_ssl = !ssl_;
	switch (no_ssl) {
	case true: {
		switch (et_) {
		case true:
            handleRead_et(receiveTime);
			break;
		default:
            handleRead_(receiveTime);
			break;
		}
		break;
	}
    default: {
		switch (et_) {
		case true:
            handleRead_ssl_et(receiveTime);
			break;
		default:
            handleRead_ssl(receiveTime);
			break;
		}
        break;
    }
    }
}

void TcpConnection::handleRead_(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  ASSERT(!channel_->isETReading());
  int saveErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
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

void TcpConnection::handleWrite() {
	bool no_ssl = !ssl_;
	switch (no_ssl) {
	case true: {
		switch (et_) {
		case true:
            handleWrite_et();
			break;
		default:
            handleWrite_();
			break;
		}
		break;
	}
	default: {
		switch (et_) {
		case true:
            handleWrite_ssl_et();
			break;
		default:
            handleWrite_ssl();
			break;
		}
		break;
	}
	}
}

void TcpConnection::handleWrite_()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())
  {
    ASSERT(!channel_->isETWriting());
    ssize_t n = sockets::write(channel_->fd(),
                               outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
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

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  ssl::SSL_free(ssl_);
  //LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  ASSERT_V(state_ == kConnected || state_ == kDisconnecting, "state_=%s", stateToString());
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}