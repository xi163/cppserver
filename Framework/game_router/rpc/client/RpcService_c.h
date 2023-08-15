#ifndef INCLUDE_RPCSERVICE_CLIENT_H
#define INCLUDE_RPCSERVICE_CLIENT_H

#include "proto/Game.Rpc.pb.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		class Service {
		public:
			Service(const ClientConn& conn, int timeout);
			virtual ::Game::Rpc::NodeInfoRspPtr GetNodeInfo(
				const ::Game::Rpc::NodeInfoReq& req);
			virtual ::Game::Rpc::UserScoreRspPtr NotifyUserScore(
				const ::Game::Rpc::UserScoreReq& req);
		private:
			void doneNodeInfoRsp(const ::Game::Rpc::NodeInfoRspPtr& rsp);
			void doneUserScoreReq(const ::Game::Rpc::UserScoreRspPtr& rsp);
		private:
			ClientConn const& conn_;
			utils::SpinLock lock_;
		private:
			::Game::Rpc::NodeInfoRspPtr ptrNodeInfoRsp_;
			::Game::Rpc::UserScoreRspPtr ptrUserScoreRsp_;
		};
	}
}

#endif