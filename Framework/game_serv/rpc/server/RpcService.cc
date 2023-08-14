#include "RpcService.h"

#include "../../Game.h"

extern GameServ* gServer;

namespace rpc {
	namespace server {
		void GameServ::GetGameServ(
			const ::GameServer::GameServReqPtr& req,
			const ::GameServer::GameServRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::GameServer::GameServRsp rsp;
			rsp.set_numofloads(gServer->numUsers_.get());
			rsp.set_nodevalue(gServer->nodeValue_);
			_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}
	}
}