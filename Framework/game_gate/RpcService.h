#ifndef INCLUDE_RPCSERVICE_H
#define INCLUDE_RPCSERVICE_H

#include "proto/ProxyServer.Message.pb.h"

#include "public/Inc.h"

namespace rpc {
	namespace server {
		class GameGate : public ::ProxyServer::Message::RpcService {
		public:
			virtual void GetGameGate(
				const ::ProxyServer::Message::GameGateReqPtr& req,
				const ::ProxyServer::Message::GameGateRsp* responsePrototype,
				const muduo::net::RpcDoneCallback& done);

			virtual void NotifyUserScore(const ::ProxyServer::Message::UserScoreReqPtr& req,
				const ::ProxyServer::Message::UserScoreRsp* responsePrototype,
				const muduo::net::RpcDoneCallback& done);
		};
	}
}
#endif