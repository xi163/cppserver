#include "Gateway.h"

GateServ* gServer = NULL;
static void StopService(int signo) {
	if (gServer) {
		gServer->Quit();
	}
}

int main() {
	//检查配置文件
	if (!boost::filesystem::exists("./conf/game.conf")) {
		_LOG_ERROR("./conf/game.conf not exists");
		return -1;
	}
	utils::setrlimit();
	utils::setenv();
	//读取配置文件
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);

	//日志目录/文件/日志级别  logdir/logname
	std::string logdir = pt.get<std::string>("Gate.logdir", "./log/Gate/");
	std::string logname = pt.get<std::string>("Gate.logname", "Gate");
	int loglevel = pt.get<int>("Gate.loglevel", 1);
	if (!boost::filesystem::exists(logdir)) {
		boost::filesystem::create_directories(logdir);
	}
	_LOG_INFO("%s%s 日志级别 = %d", logdir.c_str(), logname.c_str(), loglevel);

	//获取指定网卡ipaddr
	std::string strIpAddr;
	std::string netcardName = pt.get<std::string>("Global.netcardName", "eth0");
	if (utils::getNetCardIp(netcardName, strIpAddr) < 0) {
		_LOG_FATAL("获取网卡 %s IP失败", netcardName.c_str());
		return -1;
	}
	_LOG_INFO("网卡名称 = %s 绑定IP = %s", netcardName.c_str(), strIpAddr.c_str());
	//////////////////////////////////////////////////////////////////////////
	//zookeeper服务器集群IP
	std::string strZookeeperIps = "";
	{
		auto const& childs = pt.get_child("Zookeeper");
		for (auto const& child : childs) {
			if (child.first.substr(0, 7) == "Server.") {
				if (!strZookeeperIps.empty()) {
					strZookeeperIps += ",";
				}
				strZookeeperIps += child.second.get_value<std::string>();
			}
		}
		_LOG_INFO("ZookeeperIP = %s", strZookeeperIps.c_str());
	}
	//////////////////////////////////////////////////////////////////////////
	//RedisCluster服务器集群IP
	std::map<std::string, std::string> mapRedisIps;
	std::string redisPasswd = pt.get<std::string>("RedisCluster.Password", "");
	std::string strRedisIps = "";
	{
		auto const& childs = pt.get_child("RedisCluster");
		for (auto const& child : childs) {
			if (child.first.substr(0, 9) == "Sentinel.") {
				if (!strRedisIps.empty()) {
					strRedisIps += ",";
				}
				strRedisIps += child.second.get_value<std::string>();
			}
			else if (child.first.substr(0, 12) == "SentinelMap.") {
				std::string const& ipport = child.second.get_value<std::string>();
				std::vector<std::string> vec;
				boost::algorithm::split(vec, ipport, boost::is_any_of(","));
				assert(vec.size() == 2);
				mapRedisIps[vec[0]] = vec[1];
			}
		}
		_LOG_INFO("RedisClusterIP = %s", strRedisIps.c_str());
	}
	//////////////////////////////////////////////////////////////////////////
	//redisLock分布式锁
	std::string strRedisLockIps = "";
	{
		auto const& childs = pt.get_child("RedisLock");
		for (auto const& child : childs) {
			if (child.first.substr(0, 9) == "Sentinel.") {
				if (!strRedisLockIps.empty()) {
					strRedisLockIps += ",";
				}
				strRedisLockIps += child.second.get_value<std::string>();
			}
		}
		_LOG_INFO("RedisLockIP = %s", strRedisLockIps.c_str());
	}
	//////////////////////////////////////////////////////////////////////////
	//MongoDB
	std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");

	//TCP监听客户端，websocket server_
	std::string tcpIp = pt.get<std::string>("Gate.ip", "");
	int16_t tcpPort = pt.get<int>("Gate.port", 8010);
	//TCP监听客户端，内部推送通知服务 innServer_
	int16_t innPort = pt.get<int>("Gate.innPort", 9010);
	//TCP监听客户端，HTTP httpServer_
	uint16_t httpPort = pt.get<int>("Gate.httpPort", 8120);
	//网络I/O线程数
	int16_t numThreads = pt.get<int>("Gate.numThreads", 10);
	//worker线程数
	int16_t numWorkerThreads = pt.get<int>("Gate.numWorkerThreads", 10);
	//最大连接数
	int kMaxConnections = pt.get<int>("Gate.kMaxConnections", 15000);
	//客户端连接超时时间(s)，心跳超时时间
	int kTimeoutSeconds = pt.get<int>("Gate.kTimeoutSeconds", 3);
	//Worker线程单任务队列大小
	int kMaxQueueSize = pt.get<int>("Gate.kMaxQueueSize", 1000);
	//管理员挂维护/恢复服务
	std::string strAdminList = pt.get<std::string>("Gate.adminList", "192.168.2.93,");
	//证书路径
	std::string cert_path = pt.get<std::string>("Gate.cert_path", "");
	//证书私钥
	std::string private_key = pt.get<std::string>("Gate.private_key", "");
	//是否调试
	bool isdebug = pt.get<int>("Gate.debug", 1);
	if (!tcpIp.empty() && boost::regex_match(tcpIp,
		boost::regex(
			"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
		strIpAddr = tcpIp;
	}
	//主线程EventLoop，I/O监听/连接读写 accept(read)/connect(write)
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(strIpAddr, tcpPort);
	muduo::net::InetAddress listenAddrRpc(strIpAddr, tcpPort+1);
	muduo::net::InetAddress listenAddrInn(strIpAddr, innPort);
	muduo::net::InetAddress listenAddrHttp(strIpAddr, httpPort);
	//server
	GateServ server(
		&loop,
		listenAddr, listenAddrRpc, listenAddrInn, listenAddrHttp,
		cert_path, private_key);
	server.isdebug_ = isdebug;
	server.strIpAddr_ = strIpAddr;
	server.kMaxConnections_ = kMaxConnections;
	server.kTimeoutSeconds_ = kTimeoutSeconds;
	//管理员ip地址列表
	{
		std::vector<std::string> vec;
		boost::algorithm::split(vec, strAdminList, boost::is_any_of(","));
		for (std::vector<std::string>::const_iterator it = vec.begin();
			it != vec.end(); ++it) {
			std::string const& ipaddr = *it;
			if (!ipaddr.empty() &&
				boost::regex_match(ipaddr,
					boost::regex(
						"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				server.adminList_[addr.ipv4NetEndian()] = IpVisitE::kEnable;
				_LOG_INFO("管理员IP[%s]", ipaddr.c_str());
			}
		}
	}
	boost::algorithm::split(server.redlockVec_, strRedisLockIps, boost::is_any_of(","));
	if (
		server.InitZookeeper(strZookeeperIps) &&
		server.InitMongoDB(strMongoDBUrl) &&
		server.InitRedisCluster(strRedisIps, redisPasswd)) {
		server.InitServer();
		//utils::registerSignalHandler(SIGINT, StopService);
		utils::registerSignalHandler(SIGTERM, StopService);
		server.Start(numThreads, numWorkerThreads, kMaxQueueSize);
		gServer = &server;
		loop.loop();
	}
}