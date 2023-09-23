#include "RpcService.h"

#include "../../Api.h"

extern ApiServ* gServer;

namespace rpc {
	namespace server {
		void Service::GetNodeInfo(
			const ::Game::Rpc::NodeInfoReqPtr& req,
			const ::Game::Rpc::NodeInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::NodeInfoRsp rsp;
			rsp.set_numofloads(gServer->numConnected_[KWebsocketTy].get() + gServer->numConnected_[KHttpTy].get());
			//rsp.set_nodevalue(gServer->nodeValue_);
			switch (req->flags()) {
			case 0:
				rsp.set_host(gServer->proto_ + gServer->server_ipport_ + gServer->path_handshake_);
				rsp.set_domain(gServer->proto_ + gServer->server_ipport_ + gServer->path_handshake_);
				break;
			case 1:
				rsp.set_host(gServer->http_proto_ + gServer->httpserver_ipport_/* + gServer->path_http_order_*/);
				rsp.set_domain(gServer->http_proto_ + gServer->httpserver_ipport_ + gServer->path_http_order_);
			}
			//_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}

		void Service::NotifyUserScore(const ::Game::Rpc::UserScoreReqPtr& req,
			const ::Game::Rpc::UserScoreRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::UserScoreRsp rsp;
			muduo::net::TcpConnectionPtr peer(gServer->sessions_.get(req->userid()).lock());
			if (peer) {
				//BufferPtr buffer = GateServ::packOrderScoreMsg(req->userid(), req->score());
				//muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
				//_LOG_WARN("succ %lld.score: %lld", req->userid(), req->score());
			}
			else {
				//_LOG_ERROR("failed %lld.score: %lld", req->userid(), req->score());
			}
			done(&rsp);
		}
	}
}