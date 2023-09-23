#include "Router.h"

RouterServ* gServer = NULL;
static void StopService(int signo) {
	if (gServer) {
		gServer->Quit();
	}
}

int main(int argc, char* argv[]) {
	//检查配置文件
	if (!boost::filesystem::exists("./conf/game.conf")) {
		_LOG_ERROR("./conf/game.conf not exists");
		return -1;
	}
	std::string config = "game_router";
	if (argc == 2) {
		int id = std::stoi(argv[1]);
		config += "_" + std::to_string(id);
	}
	utils::setrlimit();
	utils::setenv();
	//读取配置文件
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);
	//日志目录/文件 logdir/logname
	std::string logdir = pt.get<std::string>(config + ".logdir", "./log/game_router/");
	std::string logname = pt.get<std::string>(config + ".logname", "game_router");
	int loglevel = pt.get<int>(config + ".loglevel", 1);
	if (!boost::filesystem::exists(logdir)) {
		boost::filesystem::create_directories(logdir);
	}
	//_LOG_INFO("%s%s 日志级别 = %d", logdir.c_str(), logname.c_str(), loglevel);
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
		//_LOG_INFO("ZookeeperIP = %s", strZookeeperIps.c_str());
	}
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
		//_LOG_INFO("RedisClusterIP = %s", strRedisIps.c_str());
	}
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
		//_LOG_INFO("RedisLockIP = %s", strRedisLockIps.c_str());
	}
	 //MongoDB
	std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");
	std::string internetIp = pt.get<std::string>(config + ".internetIp", "");
	std::string ip = pt.get<std::string>(config + ".ip", "");
	int16_t port = pt.get<int>(config + ".port", 0);
	if (0 == port) {
		port = RANDOM().betweenInt(5000, 10000).randInt_mt();
	}
	int16_t httpPort = pt.get<int>(config + ".httpPort", 0);
	if (0 == httpPort) {
		httpPort = RANDOM().betweenInt(5000, 10000).randInt_mt();
	}
	int16_t numThreads = pt.get<int>(config + ".numThreads", 10);
	int16_t numWorkerThreads = pt.get<int>(config + ".numWorkerThreads", 10);
	int kMaxQueueSize = pt.get<int>(config + ".kMaxQueueSize", 1000);
	int kMaxConnections = pt.get<int>(config + ".kMaxConnections", 15000);
	int kTimeoutSeconds = pt.get<int>(config + ".kTimeoutSeconds", 3);
	//管理员挂维护/恢复服务
	std::string strAdminList = pt.get<std::string>(config + ".adminList", "192.168.2.93,");
	//证书路径
	std::string cert_path = pt.get<std::string>(config + ".cert_path", "");
	//证书私钥
	std::string private_key = pt.get<std::string>(config + ".private_key", "");
	if (!ip.empty() && boost::regex_match(ip,
		boost::regex(
			"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
	}
	else {
		std::string netcardName = pt.get<std::string>("Global.netcardName", "eth0");
		if (utils::getNetCardIp(netcardName, ip) < 0) {
			_LOG_FATAL("获取网卡 %s IP失败", netcardName.c_str());
			return -1;
		}
		_LOG_INFO("网卡名称 = %s 绑定IP = %s", netcardName.c_str(), ip.c_str());
	}
	if (internetIp.empty()) {
		internetIp = ip;
	}
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(/*internetIp, */port);//websocket
	muduo::net::InetAddress listenAddrHttp(/*internetIp, */httpPort);//http
	RouterServ server(&loop, listenAddr, listenAddrHttp, cert_path, private_key);
	server.internetIp_ = internetIp;
	server.maxConnections_ = kMaxConnections;
	server.idleTimeout_ = kTimeoutSeconds;
	server.tracemsg_ = pt.get<int>(config + ".tracemsg", 0);
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
				server.admin_list_[addr.ipv4NetEndian()] = eApiVisit::kEnable;
				//_LOG_INFO("管理员IP[%s]", ipaddr.c_str());
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
	return 0;
}