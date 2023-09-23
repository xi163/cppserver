#ifndef INCLUDE_GAME_ROUTER_H
#define INCLUDE_GAME_ROUTER_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "Packet/Packet.h"

#include "Entities.h"

#include "rpc/client/RpcClients.h"
#include "rpc/client/RpcContainer.h"

#include "IPLocator/IPLocator.h"

#include "public/errorCode.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"

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
	BOOST::replace(json, ":dt", std::to_string(elapsed));
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
	BOOST::replace(json, ":stat_dt", std::to_string(totalTime));
	BOOST::replace(json, ":test_TPS", std::to_string(testTPS));
	BOOST::replace(json, ":req_timeout", std::to_string(timeout));
	BOOST::replace(json, ":stat_total", std::to_string(requestNum));
	BOOST::replace(json, ":stat_succ", std::to_string(requestNumSucc));
	BOOST::replace(json, ":stat_fail", std::to_string(requestNumFailed));
	BOOST::replace(json, ":stat_ratio", std::to_string(ratio));
	BOOST::replace(json, ":total", std::to_string(requestNumTotal));
	BOOST::replace(json, ":succ", std::to_string(requestNumTotalSucc));
	BOOST::replace(json, ":fail", std::to_string(requestNumTotalFailed));
	BOOST::replace(json, ":ratio", std::to_string(ratioTotal));
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
	if (debug) BOOST::replace(json, ":agentid", std::to_string(agentId));
	BOOST::replace(json, ":score", std::to_string(score));
	BOOST::replace(json, ":code", std::to_string(errcode));
	BOOST::replace(json, ":type", std::to_string(opType));
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
	BOOST::replace(json, ":op", std::to_string(opType));
	BOOST::replace(json, ":code", std::to_string(errcode));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
}

class RouterServ : public boost::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	RouterServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	~RouterServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
	void onLoginWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onApiWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onGateWatcher(
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
	void asyncHttpHandler(
		const muduo::net::WeakTcpConnectionPtr& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void onHttpWriteComplete(const muduo::net::TcpConnectionPtr& conn);
	void processHttpRequest(
		const muduo::net::HttpRequest& req,
		muduo::net::HttpResponse& rsp,
		const muduo::net::TcpConnectionPtr& conn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	//std::string onProcess(std::string const& reqStr, muduo::Timestamp receiveTime, int& code, std::string& errMsg, boost::property_tree::ptree& latest, int& testTPS);
	//int execute(int opType, std::string const& account, double score, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	void refreshWhiteList();
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();
	bool repairServer(containTy servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata);
	bool repairServer(std::string const& queryStr, std::string& rspdata);
	void repairServerNotify(std::string const& msg, std::string& rspdata);
private:
	void cmd_on_user_login(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
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
	std::string internetIp_;
	std::string proto_ = "ws://";
	std::string http_proto_ = "http://";
	std::string path_http_router_ = "/opt/router";
	std::string path_handshake_;
	muduo::net::TcpServer server_;
	//muduo::net::TcpServer tcpserver_;
	//muduo::net::RpcServer rpcserver_;
	muduo::net::TcpServer httpserver_;
	
	STD::Random randomGate_;
	rpc::Connector loginRpcClients_;
	rpc::Connector apiRpcClients_;
	rpc::Connector gateRpcClients_;
	rpc::Container rpcClients_[rpc::kMaxRpcTy];
	
	muduo::AtomicInt32 nextPool_;
	std::hash<std::string> hash_session_;
	std::vector<Buckets> bucketsPool_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> thisTimer_;
	
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