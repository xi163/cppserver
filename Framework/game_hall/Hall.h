#ifndef INCLUDE_GAME_HALL_H
#define INCLUDE_GAME_HALL_H

#include "public/Inc.h"
#include "GameDefine.h"
#include "Packet.h"
#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"

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

class HallServ : public boost::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	HallServ(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr);
	~HallServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
	void onGateWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onGameWatcher(int type, int state,
		const shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void registerZookeeper();
	bool InitRedisCluster(std::string const& ipaddr, std::string const& passwd);
	bool InitMongoDB(std::string const& url);
	void threadInit();
	bool InitServer();
	void Start(int numThreads, int numWorkerThreads, int maxSize);
private:
	void onConnection(const muduo::net::TcpConnectionPtr& conn);
	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf,
		muduo::Timestamp receiveTime);
	void asyncLogicHandler(
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr const& buf,
		muduo::Timestamp receiveTime);
	void send(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t const* msg, size_t len,
		uint8_t mainId,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
	void send(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t const* msg, size_t len,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
	void send(
		const muduo::net::TcpConnectionPtr& conn,
		::google::protobuf::Message* msg,
		uint8_t mainId,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
	void send(
		const muduo::net::TcpConnectionPtr& conn,
		::google::protobuf::Message* msg,
		uint8_t subId,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
private:
	void cmd_keep_alive_ping(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_on_user_login(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_on_user_offline(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	/// <summary>
	/// 查询游戏房间列表
	/// </summary>
	void cmd_get_game_info(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	/// <summary>
	/// 查询正在玩的游戏
	/// </summary>
	void cmd_get_playing_game_info(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	/// <summary>
	/// 查询指定游戏节点
	/// </summary>
	void cmd_get_game_server_message(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_room_player_nums(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_set_headid(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_set_nickname(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_userscore(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_play_record(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_play_record_detail(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_repair_hallserver(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_task_list(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_get_task_award(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
public:
	void random_game_server_ipport(uint32_t roomid, std::string& ipport);
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
	//db刷新所有游戏房间信息
	void db_refresh_game_room_info();
	void db_update_game_room_info();
	//redis刷新所有房间游戏人数
	void redis_refresh_room_player_nums();
	void redis_update_room_player_nums();
	//redis通知刷新游戏房间配置
	void on_refresh_game_config(std::string msg);
public:
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_, invalidNodePath_;
	//redis订阅/发布
	std::shared_ptr<RedisClient> redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;
	std::vector<std::string> redlockVec_;
	std::string mongoDBUrl_;
private:
	//所有游戏房间信息
	::HallServer::GetGameMessageResponse gameinfo_;
	mutable boost::shared_mutex gameinfo_mutex_;
	::HallServer::GetServerPlayerNumResponse room_playernums_;
	mutable boost::shared_mutex room_playernums_mutex_;
public:
	int maxConnections_;
	CmdCallbacks handlers_;
	muduo::net::TcpServer server_;
	muduo::AtomicInt32 numConnected_;
	std::hash<std::string> hash_session_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
	std::map<int, std::vector<std::string>> room_servers_;
	mutable boost::shared_mutex room_servers_mutex_;
	CIpFinder ipFinder_;
};

#endif