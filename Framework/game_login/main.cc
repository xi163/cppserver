#include "Hall.h"

HallServ* gServer = NULL;
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
	utils::setrlimit();
	utils::setenv();
	//读取配置文件
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);

	//日志目录/文件 logdir/logname
	std::string logdir = pt.get<std::string>("Hall.logdir", "./log/Hall/");
	std::string logname = pt.get<std::string>("Hall.logname", "Hall");
	int loglevel = pt.get<int>("Hall.loglevel", 1);
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
	std::string tcpIp = pt.get<std::string>("Hall.ip", "");
	int16_t tcpPort = pt.get<int>("Hall.port", 8120);
	int16_t numThreads = pt.get<int>("Hall.numThreads", 10);
	int16_t numWorkerThreads = pt.get<int>("Hall.numWorkerThreads", 10);
	int kMaxQueueSize = pt.get<int>("Hall.kMaxQueueSize", 1000);
	bool isdebug = pt.get<int>("Hall.debug", 1);
	if (!tcpIp.empty() && boost::regex_match(tcpIp,
		boost::regex(
			"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
		strIpAddr = tcpIp;
	}
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(strIpAddr, tcpPort);
	HallServ server(&loop, listenAddr);
	server.isdebug_ = isdebug;
	server.strIpAddr_ = strIpAddr;
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