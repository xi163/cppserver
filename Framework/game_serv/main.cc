#include "Game.h"

GameServ* gServer = NULL;
static void StopService(int signo) {
	if (gServer) {
		gServer->Quit();
	}
}

int main(int argc, char* argv[]) {
	//检查命令行参数
	if (argc < 3) {
		_LOG_ERROR("argc < 2, error gameid & roomid");
		exit(1);
	}
	uint32_t gameId = strtol(argv[1], NULL, 10);
	uint32_t roomId = strtol(argv[2], NULL, 10);


	//检查配置文件
	if (!boost::filesystem::exists("./conf/game.conf")) {
		_LOG_ERROR("./conf/game.conf not exists");
		return -1;
	}

	//读取配置文件
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);

	//日志目录/文件 logdir/logname
	std::string logdir = pt.get<std::string>("Game.logdir", "./log/Game/");
	std::string logname = pt.get<std::string>("Game.logname", "Game");
	int loglevel = pt.get<int>("Game.loglevel", 1);
	if (setEnv(logdir, logname, loglevel) < 0) {
		return -1;
	}
	_LOG_INFO("%s%s 日志级别 = %d", logdir.c_str(), logname.c_str(), loglevel);

	//获取指定网卡ipaddr
	std::string strIpAddr;
	std::string netcardName = pt.get<std::string>("Global.netcardName", "eth0");
	if (IpByNetCardName(netcardName, strIpAddr) < 0) {
		LOG_FATAL << __FUNCTION__ << " --- *** 获取网卡IP失败";
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

	std::string ip = pt.get<std::string>("GameServer.ip", "192.168.0.113");
	uint16_t port = pt.get<int>("GameServer.port", 8120);
	port = 30000 + roomId;
	int16_t numThreads = pt.get<int>("GameServer.numThreads", 10);
	int16_t numWorkerThreads = pt.get<int>("GameServer.numWorkerThreads", 10);
	int kMaxQueueSize = pt.get<int>("GameServer.kMaxQueueSize", 1000);
	bool isdebug = pt.get<int>("GameServer.debug", 1);
	if (!ip.empty() && boost::regex_match(ip,
		boost::regex(
			"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
			"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
		strIpAddr = ip;
	}
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(strIpAddr, port);
	GameServ server(&loop, listenAddr, gameId, roomId);
	//server.isdebug_ = isdebug;
	server.strIpAddr_ = strIpAddr;
	boost::algorithm::split(server.redlockVec_, strRedisLockIps, boost::is_any_of(","));
	if (
		server.InitZookeeper(strZookeeperIps) &&
		server.InitMongoDB(strMongoDBUrl) &&
		server.InitRedisCluster(strRedisIps, redisPasswd)) {
		if (server.InitServer()) {
			//registerSignalHandler(SIGINT, StopService);
			registerSignalHandler(SIGTERM, StopService);
			server.Start(numThreads, numWorkerThreads, kMaxQueueSize);
			gServer = &server;
			loop.loop();
		}
	}
	return 0;
}