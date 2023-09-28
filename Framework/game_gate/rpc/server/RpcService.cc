#include "RpcService.h"

#include "../../Gate.h"

extern GateServ* gServer;

namespace rpc {
	namespace server {
		void Service::GetNodeInfo(
			const ::Game::Rpc::NodeInfoReqPtr& req,
			const ::Game::Rpc::NodeInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::NodeInfoRsp rsp;
			rsp.set_numofloads(gServer->numConnected_[KWebsocketTy].get());
			rsp.set_host(gServer->proto_ + gServer->server_ipport_ + gServer->path_handshake_);
			rsp.set_domain(gServer->proto_ + gServer->server_ipport_ + gServer->path_handshake_);
			//Warnf("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}

		void Service::NotifyUserScore(const ::Game::Rpc::UserScoreReqPtr& req,
			const ::Game::Rpc::UserScoreRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::UserScoreRsp rsp;
			muduo::net::TcpConnectionPtr peer(gServer->sessions_.get(req->userid()).lock());
			if (peer) {
				BufferPtr buffer = GateServ::packOrderScoreMsg(req->userid(), req->score());
				muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
				Warnf("succ %lld.score: %lld", req->userid(), req->score());
			}
			else {
				Errorf("failed %lld.score: %lld", req->userid(), req->score());
			}
			done(&rsp);
		}
	}
}