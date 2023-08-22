#ifndef INCLUDE_GAME_SERV_H
#define INCLUDE_GAME_SERV_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "Packet.h"
#include "ITableContext.h"

#include "public/gameConst.h"
#include "public/gameStruct.h"

#include "rpc/server/RpcService.h"

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
	GameServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrRpc,
		uint32_t gameId, uint32_t roomId);
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
	void asyncOfflineHandler(std::string const& ipPort);
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
	tagGameInfo* GetGameInfo() { return &gameInfo_; }
	tagGameRoomInfo* GetRoomInfo() { return &roomInfo_; }
	std::string const& ServId() { return nodeValue_; }
	void KickUser(int64_t userId, int32_t kickType);
	TableContext GetContext(int64_t userId);
	void GetContextInLoop(int64_t userId, TableContext& context, bool& bok);
	void AddContext(
		const muduo::net::TcpConnectionPtr& conn,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
	void AddContextInLoop(
		const muduo::net::TcpConnectionPtr& conn,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
	void DelContext(int64_t userId);
	void DelContextInLoop(int64_t userId);
	bool IsStopped() { return false; }
public:
	bool SendGameErrorCode(
		const muduo::net::TcpConnectionPtr& conn,
		uint8_t mainid, uint8_t subid,
		uint32_t errcode, std::string errmsg,
		packet::internal_prev_header_t const* pre_header_,
		packet::header_t const* header_);
public:
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
public:
	tagGameInfo gameInfo_;
	tagGameRoomInfo roomInfo_;
	tagGameReplay gameReplay_;
	int maxConnections_;
	CmdCallbacks handlers_;
	rpc::server::Service rpcservice_;
	muduo::net::TcpServer server_;
	muduo::net::RpcServer rpcserver_;
	//muduo::net::TcpServer httpserver_;
	muduo::AtomicInt32 numConnected_;
	muduo::AtomicInt32 numUsers_;
	std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
	
	//muduo::net::EventLoop* tableThread_;
	
	std::map<std::string, muduo::net::WeakTcpConnectionPtr> mapGateConns_;
	mutable boost::shared_mutex mutexGateConns_;
	
	std::map<int64_t, std::shared_ptr<gate_t>> mapUserGates_;
	//mutable boost::shared_mutex mutexUserGates_;
	
	std::map<std::string, std::set<int64_t>> mapGateUsers_;
	//mutable boost::shared_mutex mutexGateUsers_;
	
	CIpFinder ipFinder_;
	bool tracemsg_ = 0;
};

#endif