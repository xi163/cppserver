#ifndef INCLUDE_GAME_SERV_H
#define INCLUDE_GAME_SERV_H

#include "public/Inc.h"
#include "gameDefine.h"
#include "Packet.h"
#include "ITableContext.h"

#if BOOST_VERSION < 104700
namespace boost
{
template <typename T>
inline size_t hash_value(const boost::shared_ptr<T> &x)
{
    return boost::hash_value(x.get());
}
} // namespace boost
#endif

struct gate_t {
	std::string IpPort;
	std::shared_ptr<packet::internal_prev_header_t> pre_header;
	std::shared_ptr<packet::header_t> header;
};

class GameServ : public boost::noncopyable, public ITableContext {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
    GameServ(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, uint32_t gameId, uint32_t roomId);
    ~GameServ();
	void Quit();
	void registerHandlers();
	bool InitZookeeper(std::string const& ipaddr);
	void onZookeeperConnected();
	void onGateWatcher(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	void onHallWatcher(int type, int state,
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
	void cmd_on_user_enter_room(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_on_user_ready(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	//点击离开按钮
	void cmd_on_user_left_room(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	//关闭页面
	void cmd_on_user_offline(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_on_game_message(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	void cmd_notifyRepairServerResp(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
public:
	void KickOffLine(int64_t userId, int32_t kickType);
	boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> GetContext(int64_t userId);
	void DelContext(int64_t userId);
	bool IsStopped() { return false; }
public:
	bool GetUserBaseInfo(int64_t userid,
		UserBaseInfo& baseInfo);
	bool SendGameErrorCode(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t mainid, uint8_t subid,
		uint32_t errcode, std::string errmsg,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
public:
	bool LoadGameRoomKindInfo(uint32_t gameid, uint32_t roomid);
	bool redis_get_token_info(
		std::string const& token,
		int64_t& userid, std::string& account, uint32_t& agentid);
	bool db_update_online_status(int64_t userid, int32_t status);
	void db_refresh_game_room_info();
	//void db_update_game_room_info();
	void redis_refresh_room_player_nums();
	void redis_update_room_player_nums();
	void on_refresh_game_config(std::string msg);
public:
	uint32_t gameId_;
	uint32_t roomId_;
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_, invalidNodePath_;
	//redis订阅/发布
	std::shared_ptr<RedisClient> redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;

	std::vector<std::string> redlockVec_;

	std::string mongoDBUrl_;

	//std::map<int, std::vector<std::string>> room_servers_;
	//mutable boost::shared_mutex room_servers_mutex_;
	
	tagGameInfo gameInfo_;
	tagGameRoomInfo roomInfo_;
	tagGameReplay gameReplay_;

	CmdCallbacks handlers_;

	CIpFinder ipFinder_;
    
	std::string strIpAddr_;

	muduo::net::TcpServer server_;

	muduo::AtomicInt32 numConnected_;
	//桌子逻辑线程/定时器
	std::shared_ptr<muduo::net::EventLoopThread> logicThread_;
	//std::shared_ptr<muduo::net::EventLoopThreadPool> logicThread_;

	std::map<std::string, muduo::net::WeakTcpConnectionPtr> mapGateConns_;
	mutable boost::shared_mutex mutexGateConns_;
	std::map<int64_t, std::shared_ptr<gate_t>> mapUserGates_;
	mutable boost::shared_mutex mutexUserGates_;
};

#endif 
