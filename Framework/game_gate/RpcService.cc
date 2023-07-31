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
			rsp.set_numofloads(gServer->numConnectedC_.get());
			rsp.set_host(gServer->proto_ + gServer->server_.ipPort() + gServer->path_handshake_);
			rsp.set_domain(gServer->proto_ + gServer->server_.ipPort() + gServer->path_handshake_);
			_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}
	}
}