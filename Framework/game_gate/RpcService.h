#ifndef INCLUDE_RPCSERVICE_H
#define INCLUDE_RPCSERVICE_H

#include "proto/ProxyServer.Message.pb.h"

#include "public/Inc.h"

namespace Rpc {
	class GameGateService : public ::ProxyServer::Message::RpcService {
	public:
		virtual void GetGameGate(
			const ::ProxyServer::Message::GameGateReqPtr& req,
			const ::ProxyServer::Message::GameGateRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done);
	};
}
#endif