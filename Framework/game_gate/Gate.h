#ifndef INCLUDE_GAME_GATE_H
#define INCLUDE_GAME_GATE_H

#include "public/Inc.h"
#include "public/gameConst.h"

#include "Packet/Packet.h"

#include "Entities.h"

#include "rpc/client/RpcClients.h"
#include "rpc/client/RpcContainer.h"

#include "rpc/server/RpcService.h"

#include "IPLocator/IPLocator.h"

static void setFailedResponse(muduo::net::HttpResponse& rsp,
	muduo::net::HttpResponse::HttpStatusCode code = muduo::net::HttpResponse::k200Ok,
	std::string const& msg = "") {
	rsp.setStatusCode(code);
	rsp.setStatusMessage("OK");
	rsp.addHeader("Server", "MUDUO");
#if 0
	rsp.setContentType("text/html;charset=utf-8");
	rsp.setBody("<html><body>" + msg + "</body></html>");
#elif 0
	rsp.setContentType("application/xml;charset=utf-8");
	rsp.setBody(msg);
#else
	rsp.setContentType("text/plain;charset=utf-8");
	rsp.setBody(msg);
#endif
}

class GateServ : public muduo::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	GateServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrTcp,
		const muduo::net::InetAddress& listenAddrRpc,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	~GateServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
	void onGateWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onHallWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onGameWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void registerZookeeper();
	bool InitRedisCluster(std::string const& ipaddr, std::string const& passwd);
	bool InitMongoDB(std::string const& url);
	void threadInit();
	bool InitServer();
	void Start(int numThreads, int numWorkerThreads, int maxSize);
public:
	bool onCondition(const muduo::net::InetAddress& peerAddr);
	void onConnection(const muduo::net::TcpConnectionPtr& conn);
	bool onValidate(muduo::net::http::IRequest const* request);
	void onConnected(
		const muduo::net::TcpConnectionPtr& conn,
		std::string const& ipaddr);
	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, int msgType,
		muduo::Timestamp receiveTime);
	void asyncClientHandler(
		const muduo::net::WeakTcpConnectionPtr& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void asyncOfflineHandler(Context& entryContext);
	static BufferPtr packOrderScoreMsg(int16_t userid, int64_t score);
	static BufferPtr packKickGameUserMsg();
	static BufferPtr packNotifyFailedMsg(uint8_t mainId, uint8_t subId);
	static BufferPtr packClientShutdownMsg(int64_t userid, int status = 0);
	static BufferPtr packNoticeMsg(
		int32_t agentid, std::string const& title,
		std::string const& content, int msgtype);
	void broadcastNoticeMsg(
		std::string const& title,
		std::string const& msg,
		int32_t agentid, int msgType);
	void broadcastMessage(int mainId, int subId, ::google::protobuf::Message* msg);
	void refreshBlackList();
	bool refreshBlackListSync();
	bool refreshBlackListInLoop();
private:
	void cmd_getAesKey(const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
private:
	void onGateConnection(const muduo::net::TcpConnectionPtr& conn);
	void onGateMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncGateHandler(
		muduo::net::WeakTcpConnectionPtr const& weakGateConn,
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void sendGateMessage(
		Context& entryContext,
		BufferPtr const& buf, int64_t userId);
private:
	void onHallConnection(const muduo::net::TcpConnectionPtr& conn);
	void onHallMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncHallHandler(
		muduo::net::WeakTcpConnectionPtr const& weakHallConn,
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void sendHallMessage(
		Context& entryContext,
		BufferPtr const& buf, int64_t userId);
	void onUserLoginNotify(std::string const& msg);
	void onUserOfflineHall(Context& entryContext);
private:
	void onGameConnection(const muduo::net::TcpConnectionPtr& conn);
	void onGameMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncGameHandler(
		muduo::net::WeakTcpConnectionPtr const& weakGameConn,
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void asyncGameOfflineHandler(std::string const& ipPort);
	void sendGameMessage(
		Context& entryContext,
		BufferPtr const& buf, int64_t userId);
	void onUserOfflineGame(Context& entryContext);
private:
	bool onHttpCondition(const muduo::net::InetAddress& peerAddr);
	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);
	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncHttpHandler(const muduo::net::WeakTcpConnectionPtr& weakConn, muduo::Timestamp receiveTime);
	void onHttpWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	void processHttpRequest(
		const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
		muduo::net::InetAddress const& peerAddr,
		muduo::Timestamp receiveTime);
	void refreshWhiteList();
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();
	bool repairServer(containTy servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata);
	bool repairServer(std::string const& queryStr, std::string& rspdata);
	void repairServerNotify(std::string const& msg, std::string& rspdata);
public:
	void onTcpConnection(const muduo::net::TcpConnectionPtr& conn);
	void onTcpMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncTcpHandler(
		muduo::net::WeakTcpConnectionPtr const& weakTcpConn,
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr& buf, muduo::Timestamp receiveTime);
	void onMarqueeNotify(std::string const& msg);
public:
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_, invalidNodePath_;
	//redis订阅/发布
	std::shared_ptr<RedisClient>  redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;
	std::vector<std::string> redlockVec_;
	std::string mongoDBUrl_;
public:
	muduo::AtomicInt32 numConnected_[kMaxNodeTy];
	muduo::AtomicInt64 numTotalReq_;
	muduo::AtomicInt64 numTotalBadReq_;
	//map[session] = weakConn
	STR::Entities entities_;
	//map[userid] = weakConn
	INT::Entities sessions_;
	
	int interval_ = 1;
	int idleTimeout_;
	int maxConnections_;
	
	CmdCallbacks handlers_;
	std::string internetIp_, server_ipport_, httpserver_ipport_;;
	std::string proto_ = "ws://";
	std::string path_handshake_;
	rpc::server::Service rpcservice_;
	muduo::net::TcpServer server_;
	muduo::net::TcpServer tcpserver_;
	muduo::net::RpcServer rpcserver_;
	muduo::net::TcpServer httpserver_;

	rpc::Connector gateRpcClients_;
	rpc::Container rpcClients_[rpc::kMaxRpcTy];

	STD::Random randomHall_;
	Connector gateClients_;
	Connector hallClients_;
	Connector gameClients_;
	Container clients_[kMaxContainTy];
	muduo::AtomicInt32 nextPool_;
	std::hash<std::string> hash_session_;
	std::vector<Buckets> bucketsPool_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> thisTimer_;
	
	std::map<std::string, std::set<int64_t>> mapHallUsers_;
	mutable boost::shared_mutex mutexHallUsers_;

	std::map<std::string, std::set<int64_t>> mapGameUsers_;
	mutable boost::shared_mutex mutexGameUsers_;

	//管理员挂维护/恢复服务
	volatile long server_state_;
	std::map<in_addr_t, eApiVisit> admin_list_;
	eApiCtrl whiteListControl_;
	std::map<in_addr_t, eApiVisit> white_list_;
	mutable boost::shared_mutex white_list_mutex_;
	eApiCtrl blackListControl_;
	std::map<in_addr_t, eApiVisit> black_list_;
	mutable boost::shared_mutex black_list_mutex_;
	CIPLocator ipLocator_;
	bool tracemsg_ = 0;
};

#endif