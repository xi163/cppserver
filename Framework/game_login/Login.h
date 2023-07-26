#ifndef INCLUDE_GAME_LOGIN_H
#define INCLUDE_GAME_LOGIN_H

#include "public/Inc.h"
#include "GameDefine.h"
#include "Packet.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"

#include "Entities.h"
#include "Container.h"
#include "Clients.h"
#include "EntryPtr.h"

#include "RpcClients.h"
#include "RpcContainer.h"

enum ServiceStateE {
	kRepairing = 0,//维护中
	kRunning = 1,//服务中
};

enum IpVisitCtrlE {
	kClose = 0,
	kOpen = 1,//应用层IP截断
	kOpenAccept = 2,//网络底层IP截断
};

enum IpVisitE {
	kEnable = 0,//IP允许访问
	kDisable = 1,//IP禁止访问
};

/*
	HTTP/1.1 400 Bad Request\r\n\r\n
	HTTP/1.1 404 Not Found\r\n\r\n
	HTTP/1.1 405 服务维护中\r\n\r\n
	HTTP/1.1 500 IP访问限制\r\n\r\n
	HTTP/1.1 504 权限不够\r\n\r\n
	HTTP/1.1 505 timeout\r\n\r\n
	HTTP/1.1 600 访问量限制(1500)\r\n\r\n
*/

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

typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;
typedef std::map<std::string, std::string> HttpParams;

#if BOOST_VERSION < 104700
namespace boost
{
	template <typename T>
	inline size_t hash_value(const boost::shared_ptr<T>& x)
	{
		return boost::hash_value(x.get());
	}
} // namespace boost
#endif

class LoginServ : public boost::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	LoginServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	~LoginServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
	void onGateRpcWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void registerZookeeper();
	bool InitRedisCluster(std::string const& ipaddr, std::string const& passwd);
	bool InitMongoDB(std::string const& url);
	void threadInit();
	bool InitServer();
	void Start(int numThreads, int numWorkerThreads, int maxSize);
private:
	bool onCondition(const muduo::net::InetAddress& peerAddr);
	void onConnection(const muduo::net::TcpConnectionPtr& conn);
	void onConnected(
		const muduo::net::TcpConnectionPtr& conn,
		std::string const& ipaddr);
	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, int msgType,
		muduo::Timestamp receiveTime);
	void asyncClientHandler(
		WeakEntryPtr const& weakEntry,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void asyncOfflineHandler(ContextPtr /*const*/& entryContext);
	void refreshBlackList();
	bool refreshBlackListSync();
	bool refreshBlackListInLoop();
private:
	bool onHttpCondition(const muduo::net::InetAddress& peerAddr);
	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);
	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncHttpHandler(WeakEntryPtr const& weakEntry, muduo::Timestamp receiveTime);
	void onHttpWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	void processHttpRequest(
		const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
		muduo::net::InetAddress const& peerAddr,
		muduo::Timestamp receiveTime);
	void refreshWhiteList();
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();
	bool repairServer(servTyE servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata);
	bool repairServer(std::string const& queryStr, std::string& rspdata);
	void repairServerNotify(std::string const& msg, std::string& rspdata);
private:
	void cmd_on_user_login(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
public:
	void rangeGateRpcTest();
	void random_game_gate_ipport(uint32_t roomid, std::string& ipport);
	
	//db更新用户登陆信息(登陆IP，时间)
	bool db_update_login_info(
		int64_t userid,
		std::string const& loginIp,
		std::chrono::system_clock::time_point& lastLoginTime,
		std::chrono::system_clock::time_point& now);
	//db更新用户在线状态
	bool db_update_online_status(int64_t userid, int32_t status);
	//db添加用户登陆日志
	bool db_add_login_logger(
		int64_t userid,
		std::string const& loginIp,
		std::string const& location,
		std::chrono::system_clock::time_point& now,
		uint32_t status, uint32_t agentid);
	//db添加用户登出日志
	bool db_add_logout_logger(
		int64_t userid,
		std::chrono::system_clock::time_point& loginTime,
		std::chrono::system_clock::time_point& now);
public:
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_, invalidNodePath_;
	//redis订阅/发布
	std::shared_ptr<RedisClient> redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;

	std::vector<std::string> redlockVec_;

	std::string mongoDBUrl_;
public:
	muduo::AtomicInt32 numConnected_;
	muduo::AtomicInt64 numTotalReq_;
	muduo::AtomicInt64 numTotalBadReq_;

	//map[session] = weakConn
	STR::Entities entities_;
	//map[userid] = weakConn
	INT::Entities sessions_;

	int idleTimeout_;
	int maxConnections_;

	CmdCallbacks handlers_;

	muduo::net::TcpServer server_;
	muduo::net::TcpServer httpServer_;
	
	STD::Random randomGate_;
	rpc::Connector gateRpcClients_;
	rpc::Container rpcClients_[rpc::kMaxRpcTy];

	std::hash<std::string> hash_session_;
	std::vector<ConnBucketPtr> bucketsPool_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
	
	//管理员挂维护/恢复服务
	volatile long serverState_;
	std::map<in_addr_t, IpVisitE> adminList_;
	IpVisitCtrlE whiteListControl_;
	std::map<in_addr_t, IpVisitE> whiteList_;
	mutable boost::shared_mutex whiteList_mutex_;
	IpVisitCtrlE blackListControl_;
	std::map<in_addr_t, IpVisitE> blackList_;
	mutable boost::shared_mutex blackList_mutex_;
	CIpFinder ipFinder_;
};

#endif