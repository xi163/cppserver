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

#include "ErrorCode.h"

typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;
typedef std::map<std::string, std::string> HttpParams;

static void replace(std::string& json, const std::string& placeholder, const std::string& value) {
	boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
}

//最近一次请求操作的elapsed detail
static void createLatestElapsed(
	boost::property_tree::ptree& latest,
	std::string const& op, std::string const& key, double elapsed) {
	//{"op":"mongo.collect", "dt":1000, "key":}
	//{"op":"mongo.insert", "dt":1000, "key":}
	//{"op":"mongo.update", "dt":1000, "key":}
	//{"op":"mongo.query", "dt":1000, "key":}
	//{"op":"redis.insert", "dt":1000, "key":}
	//{"op":"redis.update", "dt":1000, "key":}
	//{"op":"redis.query", "dt":1000, "key":}
#if 0
	boost::property_tree::ptree data, item;
	data.put("op", op);
	data.put("key", key);
	data.put("dt", ":dt");
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, data, false);
	std::string json = s.str();
	replace(json, ":dt", std::to_string(elapsed));
	std::stringstream ss(json);
	boost::property_tree::json_parser::read_json(ss, item);
	latest.push_back(std::make_pair("", item));
#else
	boost::property_tree::ptree item;
	item.put("op", op);
	item.put("key", key);
	item.put("dt", elapsed);
	latest.push_back(std::make_pair("", item));
#endif
}

//监控数据
static std::string createMonitorData(
	boost::property_tree::ptree const& latest, double totalTime, int timeout,
	int64_t requestNum, int64_t requestNumSucc, int64_t requestNumFailed, double ratio,
	int64_t requestNumTotal, int64_t requestNumTotalSucc, int64_t requestNumTotalFailed, double ratioTotal, int testTPS) {
	boost::property_tree::ptree root, stat, history;
	//最近一次请求 latest
	root.add_child("latest", latest);
	//统计间隔时间 totalTime
	root.put("stat_dt", ":stat_dt");
	//估算每秒请求处理数 testTPS
	root.put("test_TPS", ":test_TPS");
	//请求超时时间 idleTimeout
	root.put("req_timeout", ":req_timeout");
	{
		//统计请求次数 requestNum
		stat.put("stat_total", ":stat_total");
		//统计成功次数 requestNumSucc
		stat.put("stat_succ", ":stat_succ");
		//统计失败次数 requestNumFailed
		stat.put("stat_fail", ":stat_fail");
		//统计命中率 ratio
		stat.put("stat_ratio", ":stat_ratio");
		root.add_child("stat", stat);
	}
	{
		//历史请求次数 requestNumTotal
		history.put("total", ":total");
		//历史成功次数 requestNumTotalSucc
		history.put("succ", ":succ");
		//历史失败次数 requestNumTotalFailed
		history.put("fail", ":fail");
		//历史命中率 ratioTotal
		history.put("ratio", ":ratio");
		root.add_child("history", history);
	}
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	replace(json, ":stat_dt", std::to_string(totalTime));
	replace(json, ":test_TPS", std::to_string(testTPS));
	replace(json, ":req_timeout", std::to_string(timeout));
	replace(json, ":stat_total", std::to_string(requestNum));
	replace(json, ":stat_succ", std::to_string(requestNumSucc));
	replace(json, ":stat_fail", std::to_string(requestNumFailed));
	replace(json, ":stat_ratio", std::to_string(ratio));
	replace(json, ":total", std::to_string(requestNumTotal));
	replace(json, ":succ", std::to_string(requestNumTotalSucc));
	replace(json, ":fail", std::to_string(requestNumTotalFailed));
	replace(json, ":ratio", std::to_string(ratioTotal));
	return json;
}

/* 返回格式
{
	"maintype": "/GameHandle",
		"type": 2,
		"data":
		{
			"orderid":"",
			"agentid": 10000,
			"account": "999",
			"score": 10000,
			"code": 0,
			"errmsg":"",
		}
}
*/
static std::string createResponse(
	int32_t opType,
	std::string const& orderId,
	uint32_t agentId,
	std::string account, double score,
	int errcode, std::string const& errmsg, bool debug) {
	boost::property_tree::ptree root, data;
	if (debug) data.put("orderid", orderId);
	if (debug) data.put("agentid", ":agentid");
	data.put("account", account);
	data.put("score", ":score");
	data.put("code", ":code");
	if (debug) data.put("errmsg", errmsg);
	// 外层json
	root.put("maintype", "/GameHandle");
	root.put("type", ":type");
	root.add_child("data", data);
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	if (debug) replace(json, ":agentid", std::to_string(agentId));
	replace(json, ":score", std::to_string(score));
	replace(json, ":code", std::to_string(errcode));
	replace(json, ":type", std::to_string(opType));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
}

static std::string createResponse2(
	int opType,//status=1
	std::string const& servname,//type=HallSever
	std::string const& name,//name=192.168.2.93:10000
	int errcode, std::string const& errmsg) {
	boost::property_tree::ptree root, data;
	root.put("op", ":op");
	root.put("type", servname);
	root.put("name", name);
	root.put("code", ":code");
	root.put("errmsg", errmsg);
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	replace(json, ":op", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
}

static void parseQuery(std::string const& queryStr, HttpParams& params) {
	params.clear();
	_LOG_DEBUG(queryStr.c_str());
	utils::parseQuery(queryStr, params);
	std::string keyValues;
	for (auto param : params) {
		keyValues += "\n" + param.first + "=" + param.second;
	}
	_LOG_DEBUG(keyValues.c_str());
}

static std::string getRequestStr(muduo::net::HttpRequest const& req) {
	std::string headers;
	for (std::map<string, string>::const_iterator it = req.headers().begin();
		it != req.headers().end(); ++it) {
		headers += it->first + ": " + it->second + "\n";
	}
	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		<< "<xs:head>" << headers << "</xs:head>"
		<< "<xs:body>"
		<< "<xs:method>" << req.methodString() << "</xs:method>"
		<< "<xs:path>" << req.path() << "</xs:path>"
		<< "<xs:query>" << utils::HTML::Encode(req.query()) << "</xs:query>"
		<< "</xs:body>"
		<< "</xs:root>";
	return ss.str();
}

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
#elif 0
	rsp.setContentType("application/json;charset=utf-8");
	rsp.setBody(msg);
#else
	rsp.setContentType("text/plain;charset=utf-8");
	rsp.setBody(msg);
#endif
}

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
		const muduo::net::WeakTcpConnectionPtr& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void asyncOfflineHandler(Context& entryContext);
	void refreshBlackList();
	bool refreshBlackListSync();
	bool refreshBlackListInLoop();
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
	std::string onProcess(std::string const& reqStr, muduo::Timestamp receiveTime, int& code, std::string& errMsg, boost::property_tree::ptree& latest, int& testTPS);
	int execute(int32_t opType, std::string const& account, double score, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
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

	int interval_ = 1;
	int idleTimeout_;
	int maxConnections_;

	CmdCallbacks handlers_;

	muduo::net::TcpServer server_;
	muduo::net::TcpServer httpServer_;
	
	STD::Random randomGate_;
	rpc::Connector gateRpcClients_;
	rpc::Container rpcClients_[rpc::kMaxRpcTy];
	
	muduo::AtomicInt32 nextPool_;
	std::hash<std::string> hash_session_;
	std::vector<Buckets> bucketsPool_;
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