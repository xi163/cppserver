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
		Errorf("argc < 3, error gameid & roomid");
		exit(1);
	}
	std::string config = "game_serv_club";
	if (argc == 4) {
		int id = std::stoi(argv[3]);
		config += "_" + std::to_string(id);
	}
	utils::setrlimit();
	utils::setenv();
	uint32_t gameId = strtol(argv[1], NULL, 10);
	uint32_t roomId = strtol(argv[2], NULL, 10);
	if (gameId <= 0 || roomId <= 0) {
		Errorf("error gameid & roomid");
		exit(1);
	}
	//检查配置文件
	if (!boost::filesystem::exists("./conf/game.conf")) {
		Errorf("./conf/game.conf not exists");
		return -1;
	}
	//读取配置文件
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);
	//日志相关
	std::string logname = pt.get<std::string>(config + ".logname", "game_serv_club");
	std::string logdir = pt.get<std::string>(config + ".logdir", "./log/game_serv_club");
	int logtimezone = pt.get<int>(config + ".logtimezone", MY_CST);
	int loglevel = pt.get<int>(config + ".loglevel", LVL_DEBUG);
	int logmode = pt.get<int>(config + ".logmode", M_STDOUT_FILE);
	int logstyle = pt.get<int>(config + ".logstyle", F_DETAIL);
	_LOG_SET_TIMEZONE(logtimezone);
	_LOG_SET_LEVEL(loglevel);
	_LOG_SET_MODE(logmode);
	_LOG_SET_STYLE(logstyle);
	_LOG_INIT(logdir.c_str(), logname.append(".").append(std::to_string(roomId)).c_str(), 100000000);
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
		//Infof("ZookeeperIP = %s", strZookeeperIps.c_str());
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
		//Infof("RedisClusterIP = %s", strRedisIps.c_str());
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
		//Infof("RedisLockIP = %s", strRedisLockIps.c_str());
	}
	 //MongoDB
	std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");
	std::string ip = pt.get<std::string>(config + ".ip", "");
	uint16_t port = pt.get<int>(config + ".port", 0);
	if (0 == port) {
		port = RANDOM().betweenInt(15000, 30000).randInt_mt();
	}
	int16_t rpcPort = pt.get<int>(config + ".rpcPort", 0);
	if (0 == rpcPort) {
		rpcPort = RANDOM().betweenInt(5000, 10000).randInt_mt();
	}
	int16_t numThreads = pt.get<int>(config + ".numThreads", 0);
	if (numThreads == 0) {
		numThreads = utils::numCPU();
	}
	int16_t numWorkerThreads = pt.get<int>(config + ".numWorkerThreads", 0);
	if (numWorkerThreads == 0) {
		numWorkerThreads = utils::numCPU();
	}
	int kMaxQueueSize = pt.get<int>(config + ".kMaxQueueSize", 1000);
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
			Fatalf("获取网卡 %s IP失败", netcardName.c_str());
			return -1;
		}
		Infof("网卡名称 = %s 绑定IP = %s", netcardName.c_str(), ip.c_str());
	}
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(ip, port);//tcp
	muduo::net::InetAddress listenAddrRpc(ip, rpcPort);//rpc
	GameServ server(&loop, listenAddr, listenAddrRpc, gameId, roomId);
	server.tracemsg_ = pt.get<int>(config + ".tracemsg", 0);
	server.et_ = pt.get<int>(config + ".et", 0);
	boost::algorithm::split(server.redlockVec_, strRedisLockIps, boost::is_any_of(","));
	if (
		server.InitZookeeper(strZookeeperIps) &&
		server.InitMongoDB(strMongoDBUrl) &&
		server.InitRedisCluster(strRedisIps, redisPasswd)) {
		if (server.InitServer()) {
			//utils::registerSignalHandler(SIGINT, StopService);
			utils::registerSignalHandler(SIGTERM, StopService);
			server.Start(numThreads, std::min<int>(numWorkerThreads, utils::numCPU()), kMaxQueueSize);
			gServer = &server;
			loop.loop();
		}
	}
	return 0;
}