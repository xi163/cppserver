#include "GateServList.h"

#include "RpcClients.h"
#include "RpcContainer.h"

#include "RpcService.h"
#include "../Login.h"

extern LoginServ* gServer;

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