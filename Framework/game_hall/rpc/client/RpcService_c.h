#ifndef INCLUDE_RPCSERVICE_CLIENT_H
#define INCLUDE_RPCSERVICE_CLIENT_H

#include "proto/ProxyServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		class GameGate {
		public:
			GameGate(const ClientConn& conn, int timeout);
			virtual ::ProxyServer::Message::GameGateRspPtr GetGameGate(
				const ::ProxyServer::Message::GameGateReq& req);
			virtual ::ProxyServer::Message::UserScoreRspPtr NotifyUserScore(
				const ::ProxyServer::Message::UserScoreReq& req);
		private:
			void doneGameGateRsp(const ::ProxyServer::Message::GameGateRspPtr& rsp);
			void doneUserScoreReq(const ::ProxyServer::Message::UserScoreRspPtr& rsp);
		private:
			ClientConn const& conn_;
			utils::SpinLock lock_;
		private:
			::ProxyServer::Message::GameGateRspPtr ptrGameGateRsp_;
			::ProxyServer::Message::UserScoreRspPtr ptrUserScoreRsp_;
		};
		
		class GameServ {
		public:
			GameServ(const ClientConn& conn, int timeout);
			virtual ::GameServer::GameServRspPtr GetGameServ(
				const ::GameServer::GameServReq& req);
		private:
			void doneGameServRsp(const ::GameServer::GameServRspPtr& rsp);
		private:
			ClientConn const& conn_;
			utils::SpinLock lock_;
		private:
			::GameServer::GameServRspPtr ptrGameServRsp_;
		};
	}
}

#endif