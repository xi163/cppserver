#include "RpcService_c.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		Service::Service(const ClientConn& conn, int timeout)
			: conn_(conn), lock_(timeout) {
		}
		
		::Game::Rpc::NodeInfoRspPtr Service::GetNodeInfo(
			const ::Game::Rpc::NodeInfoReq& req) {
			//_LOG_WARN(req.DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						stub.GetNodeInfo(req, std::bind(&Service::doneNodeInfoRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			}
			return ptrNodeInfoRsp_;
		}

		void Service::doneNodeInfoRsp(const ::Game::Rpc::NodeInfoRspPtr& rsp) {
			ptrNodeInfoRsp_ = rsp;
			lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						//stub.channel()->connection()->conn->shutdown();
						//stub.channel()->connection()->conn->forceClose();
					}
				}
			}
		}

		::Game::Rpc::UserScoreRspPtr Service::NotifyUserScore(
			const ::Game::Rpc::UserScoreReq& req) {
			_LOG_WARN("userId: %lld score: %lld", req.userid(), req.score());
			//_LOG_WARN(req.DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						stub.NotifyUserScore(req, std::bind(&Service::doneUserScoreReq, this, std::placeholders::_1));
						//lock_.wait();
					}
				}
			}
			return ptrUserScoreRsp_;
		}
		
		void Service::doneUserScoreReq(const ::Game::Rpc::UserScoreRspPtr& rsp) {
			//ptrUserScoreRsp_ = rsp;//引发崩溃BUG???
			//lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
// 			if (!conn_.get<1>().expired()) {
// 				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
// 				if (c) {
// 					if (conn_.get<2>()) {
// 						conn_.get<2>()->setConnection(c);
// 						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
// 						//stub.channel()->connection()->conn->shutdown();
// 						//stub.channel()->connection()->conn->forceClose();
// 					}
// 				}
// 			}
		}
		
		::Game::Rpc::RoomInfoRspPtr Service::GetRoomInfo(
			const ::Game::Rpc::RoomInfoReq& req) {
			//_LOG_WARN(req.DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						stub.GetRoomInfo(req, std::bind(&Service::doneRoomInfoRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			}
			return ptrRoomInfoRsp_;
		}

		void Service::doneRoomInfoRsp(const ::Game::Rpc::RoomInfoRspPtr& rsp) {
			ptrRoomInfoRsp_ = rsp;
			lock_.notify();
			//_LOG_WARN(rsp->DebugString().c_str());
			if (!conn_.get<1>().expired()) {
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					if (conn_.get<2>()) {
						conn_.get<2>()->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(conn_.get<2>()));
						//stub.channel()->connection()->conn->shutdown();
						//stub.channel()->connection()->conn->forceClose();
					}
				}
			}
		}
	}
}