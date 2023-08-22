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
			virtual ::Game::Rpc::RoomInfoRspPtr GetRoomInfo(
				const ::Game::Rpc::RoomInfoReq& req);
			virtual ::Game::Rpc::TableInfoRspPtr GetTableInfo(
				const ::Game::Rpc::TableInfoReq& req);
		private:
			void doneNodeInfoRsp(const ::Game::Rpc::NodeInfoRspPtr& rsp);
			void doneRoomInfoRsp(const ::Game::Rpc::RoomInfoRspPtr& rsp);
			void doneTableInfoRsp(const ::Game::Rpc::TableInfoRspPtr& rsp);
			void doneUserScoreReq(const ::Game::Rpc::UserScoreRspPtr& rsp);
		private:
			ClientConn const& conn_;
			utils::SpinLock lock_;
		private:
			::Game::Rpc::NodeInfoRspPtr ptrNodeInfoRsp_;
			::Game::Rpc::RoomInfoRspPtr ptrRoomInfoRsp_;
			::Game::Rpc::TableInfoRspPtr ptrTableInfoRsp_;
			::Game::Rpc::UserScoreRspPtr ptrUserScoreRsp_;
		};
	}
}

#endif