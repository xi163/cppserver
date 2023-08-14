#include "GateServList.h"

#include "../rpc/client/RpcClients.h"
#include "../rpc/client/RpcContainer.h"
#include "../rpc/client/RpcService_c.h"

#include "../Api.h"

extern ApiServ* gServer;

void GetGateServList(GateServList& servList) {
	rpc::ClientConnList clients;
	gServer->rpcClients_[rpc::containTy::kRpcGateTy].clients_->getAll(clients);
	for (rpc::ClientConnList::iterator it = clients.begin();
		it != clients.end(); ++it) {
		rpc::client::GameGate client(*it, 3);
		::ProxyServer::Message::GameGateReq req;
		::ProxyServer::Message::GameGateRspPtr rsp = client.GetGameGate(req);
		if (rsp) {
			servList.emplace_back(GateServItem(rsp->host(), rsp->domain(), rsp->numofloads()));
		}
	}
}

void BroadcastGateUserScore(int64_t userId, int64_t score) {
#if 0
	rpc::ClientConnList clients;
	gServer->rpcClients_[rpc::containTy::kRpcGateTy].clients_->getAll(clients);
	for (rpc::ClientConnList::iterator it = clients.begin();
		it != clients.end(); ++it) {
		rpc::client::GameGate client(*it, 3);
		::ProxyServer::Message::UserScoreReq req;
		req.set_userid(userId);
		req.set_score(score);
		client.NotifyUserScore(req);
	}
#else
	std::string token;
	if (REDISCLIENT.GetUserToken(userId, token) && !token.empty()) {
		std::string gateIp;
		if (REDISCLIENT.GetTokenInfoIP(token, gateIp) && !gateIp.empty()) {
			rpc::ClientConn conn;
			gServer->rpcClients_[rpc::containTy::kRpcGateTy].clients_->get(gateIp, conn);
			rpc::client::GameGate client(conn, 3);
			::ProxyServer::Message::UserScoreReq req;
			req.set_userid(userId);
			req.set_score(score);
			client.NotifyUserScore(req);
		}
	}
#endif
}