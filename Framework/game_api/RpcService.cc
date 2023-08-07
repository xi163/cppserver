#include "RpcService.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		GameGate::GameGate(const ClientConn& conn, int timeout)
			: stub_(muduo::get_pointer(conn.get<2>()))
			, lock_(timeout) {
			muduo::net::TcpConnectionPtr c(conn.get<1>().lock());
			conn.get<2>()->setConnection(c);
		}

		::ProxyServer::Message::GameGateRspPtr GameGate::GetGameGate(
			const ::ProxyServer::Message::GameGateReq& req) {
			_LOG_WARN(req.DebugString().c_str());
			stub_.GetGameGate(req, std::bind(&GameGate::doneGameGateRsp, this, std::placeholders::_1));
			lock_.wait();
			return ptrGameGateRsp_;
		}

		void GameGate::doneGameGateRsp(const ::ProxyServer::Message::GameGateRspPtr& rsp) {
			ptrGameGateRsp_ = rsp;
			lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
			//stub_.channel()->connection()->conn->shutdown();
			//stub_.channel()->connection()->conn->forceClose();
		}

		::ProxyServer::Message::UserScoreRspPtr GameGate::NotifyUserScore(
			const ::ProxyServer::Message::UserScoreReq& req) {
			_LOG_WARN("userId: %lld score: %lld", req.userid(), req.score());
			//_LOG_WARN(req.DebugString().c_str());
			stub_.NotifyUserScore(req, std::bind(&GameGate::doneUserScoreReq, this, std::placeholders::_1));
			return ptrUserScoreRsp_;
		}
		
		void GameGate::doneUserScoreReq(const ::ProxyServer::Message::UserScoreRspPtr& rsp) {
			//ptrUserScoreRsp_ = rsp;//引发崩溃BUG???
		}
	}
}