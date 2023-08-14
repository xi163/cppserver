#include "RpcService_c.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		GameGate::GameGate(const ClientConn& conn, int timeout)
			: conn_(conn), lock_(timeout) {
		}
		
		::ProxyServer::Message::GameGateRspPtr GameGate::GetGameGate(
			const ::ProxyServer::Message::GameGateReq& req) {
			_LOG_WARN(req.DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::ProxyServer::Message::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						stub.GetGameGate(req, std::bind(&GameGate::doneGameGateRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			}
			return ptrGameGateRsp_;
		}

		void GameGate::doneGameGateRsp(const ::ProxyServer::Message::GameGateRspPtr& rsp) {
			ptrGameGateRsp_ = rsp;
			lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::ProxyServer::Message::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						//stub.channel()->connection()->conn->shutdown();
						//stub.channel()->connection()->conn->forceClose();
					}
				}
			}
		}

		::ProxyServer::Message::UserScoreRspPtr GameGate::NotifyUserScore(
			const ::ProxyServer::Message::UserScoreReq& req) {
			_LOG_WARN("userId: %lld score: %lld", req.userid(), req.score());
			//_LOG_WARN(req.DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::ProxyServer::Message::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						stub.NotifyUserScore(req, std::bind(&GameGate::doneUserScoreReq, this, std::placeholders::_1));
						//lock_.wait();
					}
				}
			}
			return ptrUserScoreRsp_;
		}
		
		void GameGate::doneUserScoreReq(const ::ProxyServer::Message::UserScoreRspPtr& rsp) {
			//ptrUserScoreRsp_ = rsp;//引发崩溃BUG???
			//lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::ProxyServer::Message::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						//stub.channel()->connection()->conn->shutdown();
						//stub.channel()->connection()->conn->forceClose();
					}
				}
			}
		}
	}
}