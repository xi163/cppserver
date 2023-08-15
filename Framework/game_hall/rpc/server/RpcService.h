#ifndef INCLUDE_RPCSERVICE_SERVER_H
#define INCLUDE_RPCSERVICE_SERVER_H

#include "proto/Game.Rpc.pb.h"

namespace rpc {
	namespace server {
		class Service : public ::Game::Rpc::RpcService {
		public:
			virtual void GetNodeInfo(
				const ::Game::Rpc::NodeInfoReqPtr& req,
				const ::Game::Rpc::NodeInfoRsp* responsePrototype,
				const muduo::net::RpcDoneCallback& done);

			virtual void NotifyUserScore(const ::Game::Rpc::UserScoreReqPtr& req,
				const ::Game::Rpc::UserScoreRsp* responsePrototype,
				const muduo::net::RpcDoneCallback& done);
		};
	}
}

#endif