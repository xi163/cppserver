#include "RpcService.h"

#include "RpcClients.h"

namespace Rpc {
	
	GameGateService::GameGateService(const ClientConn& conn, int timeout)
		: stub_(muduo::get_pointer(conn.get<2>()))
		, lock_(timeout) {
		muduo::net::TcpConnectionPtr c(conn.get<1>().lock());
		conn.get<2>()->setConnection(c);
	}
	
	::ProxyServer::Message::GameGateRspPtr GameGateService::GetGameGate(
		const ::ProxyServer::Message::GameGateReq& req) {
		_LOG_WARN(req.DebugString().c_str());
		stub_.GetGameGate(req, std::bind(&GameGateService::doneGameGateRsp, this, std::placeholders::_1));
		lock_.wait();
		return ptrGameGateRsp_;
	}

	void GameGateService::doneGameGateRsp(const ::ProxyServer::Message::GameGateRspPtr& rsp) {
		ptrGameGateRsp_ = rsp;
		lock_.notify();
		//_LOG_WARN(rsp->DebugString().c_str());
		//stub_.channel()->connection()->conn->shutdown();
		//stub_.channel()->connection()->conn->forceClose();
	}
}