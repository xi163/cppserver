#include "RpcService.h"

#include "../../Gate.h"

extern GateServ* gServer;

namespace rpc {
	namespace server {
		void GameGate::GetGameGate(
			const ::ProxyServer::Message::GameGateReqPtr& req,
			const ::ProxyServer::Message::GameGateRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::ProxyServer::Message::GameGateRsp rsp;
			rsp.set_numofloads(gServer->numConnected_[KWebsocketTy].get());
			rsp.set_host(gServer->proto_ + gServer->server_.ipPort() + gServer->path_handshake_);
			rsp.set_domain(gServer->proto_ + gServer->server_.ipPort() + gServer->path_handshake_);
			//_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}

		void GameGate::NotifyUserScore(const ::ProxyServer::Message::UserScoreReqPtr& req,
			const ::ProxyServer::Message::UserScoreRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::ProxyServer::Message::UserScoreRsp rsp;
			muduo::net::TcpConnectionPtr peer(gServer->sessions_.get(req->userid()).lock());
			if (peer) {
				BufferPtr buffer = GateServ::packOrderScoreMsg(req->userid(), req->score());
				muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
				_LOG_WARN("succ %lld.score: %lld", req->userid(), req->score());
			}
			else {
				_LOG_ERROR("failed %lld.score: %lld", req->userid(), req->score());
			}
			done(&rsp);
		}
	}
}