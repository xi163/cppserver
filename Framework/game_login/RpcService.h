#ifndef INCLUDE_RPCSERVICE_H
#define INCLUDE_RPCSERVICE_H

#include "proto/ProxyServer.Message.pb.h"

#include "public/Inc.h"
#include "RpcClients.h"

namespace rpc {
	namespace client {
		class GameGate {
		public:
			GameGate(const ClientConn& conn, int timeout);
			virtual ::ProxyServer::Message::GameGateRspPtr GetGameGate(
				const ::ProxyServer::Message::GameGateReq& req);
		private:
			void doneGameGateRsp(const ::ProxyServer::Message::GameGateRspPtr& rsp);
			utils::SpinLock lock_;
			::ProxyServer::Message::GameGateRspPtr ptrGameGateRsp_;
			::ProxyServer::Message::RpcService_Stub stub_;
		};
	}
}

#endif