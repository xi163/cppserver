#include "RpcService.h"

#include "Gate.h"

extern GateServ* gServer;

namespace rpc {
	namespace server {
		void GameGate::GetGameGate(
			const ::ProxyServer::Message::GameGateReqPtr& req,
			const ::ProxyServer::Message::GameGateRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::ProxyServer::Message::GameGateRsp rsp;
			rsp.set_numofloads(111);
			rsp.set_host(gServer->server_.ipPort());
			rsp.set_domain(gServer->server_.ipPort());
			_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}
	}
}