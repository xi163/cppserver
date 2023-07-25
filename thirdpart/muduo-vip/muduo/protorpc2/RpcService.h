// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_PROTORPC2_RPCSERVICE_H
#define MUDUO_PROTORPC2_RPCSERVICE_H

#include <muduo/protorpc2/rpcservice.pb.h>
#include <muduo/protorpc2/service.h>
#include <muduo/protorpc2/RpcChannel.h>

namespace muduo
{
namespace net
{
// the meta service
class RpcServiceImpl : public muduo::net::RpcService
{
 public:
  RpcServiceImpl(const RpcChannel::ServiceMap* services)
    : services_(services)
  {
  }


  virtual void listRpc(const ListRpcRequestPtr& request,
                       const ListRpcResponse* responsePrototype,
                       const RpcDoneCallback& done);

  virtual void getService(const GetServiceRequestPtr& request,
                          const GetServiceResponse* responsePrototype,
                          const RpcDoneCallback& done);

 private:

  const RpcChannel::ServiceMap* services_;
};

}
}

#endif  // MUDUO_PROTORPC2_RPCSERVICE_H
