#ifndef INCLUDE_INCMUDUO_H
#define INCLUDE_INCMUDUO_H

//#include <muduo/base/Logging.h>
//#include <muduo/base/AsyncLogging.h>
#include <muduo/base/noncopyable.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TimerId.h>
//#include <muduo/net/protorpc/RpcServer.h>
//#include <muduo/net/protorpc/RpcChannel.h>
#include <muduo/protorpc2/service.h>
#include <muduo/protorpc2/RpcServer.h>
#include <muduo/protorpc2/RpcChannel.h>
#include <muduo/net/http/HttpContext.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/websocket/context.h>
#include <muduo/net/websocket/server.h>
#include <libwebsocket/ssl.h>

typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

#endif