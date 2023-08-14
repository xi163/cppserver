#ifndef INCLUDE_RPCSERVICE_SERVER_H
#define INCLUDE_RPCSERVICE_SERVER_H

#include "proto/GameServer.Message.pb.h"

namespace rpc {
	namespace server {
		class GameServ : public ::GameServer::RpcService {
		public:
			virtual void GetGameServ(
				const ::GameServer::GameServReqPtr& req,
				const ::GameServer::GameServRsp* responsePrototype,
				const muduo::net::RpcDoneCallback& done);

		};
	}
}

#endif