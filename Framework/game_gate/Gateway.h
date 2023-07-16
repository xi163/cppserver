#ifndef INCLUDE_GAME_GATE_H
#define INCLUDE_GAME_GATE_H

#include "public/Inc.h"

#include "Packet.h"

#include "Entities.h"
#include "Container.h"
#include "Clients.h"
#include "EntryPtr.h"

//#define NDEBUG
#define KICK_GS                 (0x01)
#define KICK_HALL               (0x02)
#define KICK_CLOSEONLY          (0x100)
#define KICK_LEAVEGS            (0x200)

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

// #define MY_TRY()	\
// 	try {
// 
// #define MY_CATCH() \
// 	} \
// catch (const muduo::Exception & e) { \
// 	_LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
// 	abort(); \
// 	} \
// catch (const std::exception & e) { \
// 	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
// 	abort(); \
// } \
// catch (...) { \
// 	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught "; \
// 	throw; \
// } \

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

class GateServ : public muduo::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	GateServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrInn,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	~GateServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
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
	void Start(int numThreads, int numWorkerThreads, int maxSize);
public:
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
public:
	void onHallConnection(const muduo::net::TcpConnectionPtr& conn);
	void onHallMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncHallHandler(
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void sendHallMessage(
		Context /*const*/& entryContext,
		BufferPtr const& buf, int64_t userId);
	void onUserLoginNotify(std::string const& msg);
	void onUserOfflineHall(Context /*const*/& entryContext);
public:
	void onGameConnection(const muduo::net::TcpConnectionPtr& conn);
	void onGameMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncGameHandler(
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void sendGameMessage(
		Context /*const*/& entryContext,
		BufferPtr const& buf, int64_t userId);
	void onUserOfflineGame(Context /*const*/& entryContext, bool leave = 0);
public:
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
public:
	void onInnConnection(const muduo::net::TcpConnectionPtr& conn);
	void onInnMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	void asyncInnHandler(
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
	muduo::net::TcpServer server_;
	muduo::net::TcpServer innServer_;
	muduo::net::TcpServer httpServer_;
	muduo::AtomicInt32 numConnected_;
	std::vector<ConnBucketPtr> bucketsPool_;
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
	//map[session] = weakConn
	STR::Entities entities_;

#ifdef MAP_USERID_SESSION
	//map[userid] = session
	INT::Sessions sessions_;
#else
	//map[userid] = weakConn
	INT::Entities sessions_;
#endif
	CmdCallbacks handlers_;
	//连接到所有大厅服
	Connector hallClients_;
	//连接到所有游戏服
	Connector gameClients_;
	//所有大厅服/游戏服节点
	Container clients_[kMaxServTy];
	std::hash<std::string> hash_session_;
	//大厅分配随机数
	STD::Random randomHall_;

	//服务状态
	volatile long serverState_;

	bool isdebug_;

	CIpFinder ipFinder_;

	std::string strIpAddr_;

	int kMaxConnections_;

	//指定时间轮盘大小(bucket桶大小)
	//即环形数组大小(size) >=
	//心跳超时清理时间(timeout) >
	//心跳间隔时间(interval)
	int kTimeoutSeconds_, kHttpTimeoutSeconds_;

	//管理员挂维护/恢复服务
	std::map<in_addr_t, IpVisitE> adminList_;

	IpVisitCtrlE whiteListControl_;

	std::map<in_addr_t, IpVisitE> whiteList_;
	mutable boost::shared_mutex whiteList_mutex_;

	IpVisitCtrlE blackListControl_;

	std::map<in_addr_t, IpVisitE> blackList_;
	mutable boost::shared_mutex blackList_mutex_;
};

#endif