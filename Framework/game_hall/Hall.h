#ifndef INCLUDE_GAME_HALL_H
#define INCLUDE_GAME_HALL_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "Packet/Packet.h"

#include "rpc/client/RpcClients.h"
#include "rpc/client/RpcContainer.h"

#include "IPLocator/IPLocator.h"

#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/HallClubServer.Message.pb.h"

class HallServ : public boost::noncopyable {
public:
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, BufferPtr const&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	HallServ(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrRpc);
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
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
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
	/// 获取所有游戏列表
	/// </summary>
	void cmd_get_game_info(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	/// <summary>
	/// 查询正在玩的游戏/游戏服务器IP
	/// </summary>
	void cmd_get_playing_game_info(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	/// <summary>
	/// 查询指定游戏节点/游戏服务器IP
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


	//===================俱乐部==================
	
	//CLIENT_TO_HALL_CLUB_GET_ROOM_INFO_MESSAGE_REQ                 = 77;     // 获取俱乐部房间信息 ClubHallServer.GetRoomInfoMessage
	//CLIENT_TO_HALL_CLUB_GET_ROOM_INFO_MESSAGE_RES                 = 78;     // 获取俱乐部房间信息 ClubHallServer.GetRoomInfoMessageResponse
	
	//CLIENT_TO_HALL_CLUB_GET_GAME_SERVER_MESSAGE_REQ               = 3;      // 获取游戏服务器IP ClubHallServer.GetGameServerMessage
	//CLIENT_TO_HALL_CLUB_GET_GAME_SERVER_MESSAGE_RES               = 4;      // 返回游戏服务器IP ClubHallServer.GetGameServerMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_GAME_MESSAGE_REQ              = 5;      // 获取我的俱乐部 游戏部分  ClubHallServer.GetMyClubGameMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_GAME_MESSAGE_RES              = 6;      // 获取我的俱乐部 游戏部分  ClubHallServer.GetMyClubGameMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_HALL_MESSAGE_REQ              = 11;     // 获取我的俱乐部  ClubHallServer.GetMyClubHallMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_HALL_MESSAGE_RES              = 12;     // 获取我的俱乐部  ClubHallServer.GetMyClubHallMessageResponse

	//CLIENT_TO_HALL_CLUB_JOIN_THE_CLUB_MESSAGE_REQ                 = 13;     // 加入俱乐部  ClubHallServer.JoinTheClubMessage
	//CLIENT_TO_HALL_CLUB_JOIN_THE_CLUB_MESSAGE_RES                 = 14;     // 加入俱乐部  ClubHallServer.JoinTheClubMessageResponse

	//CLIENT_TO_HALL_CLUB_EXIT_THE_CLUB_MESSAGE_REQ                 = 15;     // 退出俱乐部  ClubHallServer.ExitTheClubMessage
	//CLIENT_TO_HALL_CLUB_EXIT_THE_CLUB_MESSAGE_RES                 = 16;     // 退出俱乐部  ClubHallServer.ExitTheClubMessageResponse


	//CLIENT_TO_HALL_CLUB_SET_AUTO_BECOME_PARTNER_MESSAGE_REQ       = 21;     // 设置是否开启自动成为合伙人  ClubHallServer.SetAutoBecomePartnerMessage
	//CLIENT_TO_HALL_CLUB_SET_AUTO_BECOME_PARTNER_MESSAGE_RES       = 22;     // 设置是否开启自动成为合伙人  ClubHallServer.SetAutoBecomePartnerMessageResponse

	//CLIENT_TO_HALL_CLUB_BECOME_PARTNER_MESSAGE_REQ                = 23;     // 成为合伙人  ClubHallServer.BecomePartnerMessage
	//CLIENT_TO_HALL_CLUB_BECOME_PARTNER_MESSAGE_RES                = 24;     // 成为合伙人  ClubHallServer.BecomePartnerMessageResponse

	//CLIENT_TO_HALL_CLUB_EXCHANGE_MY_REVENUE_REQ                   = 25;     // 佣金兑换金币 ClubHallServer.ExchangeRevenueMessage
	//CLIENT_TO_HALL_CLUB_EXCHANGE_MY_REVENUE_RES                   = 26;     // 佣金兑换金币 ClubHallServer.ExchangeRevenueMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_EXCHANGE_MY_REVENUE_RECORD_REQ        = 31;     // 获取佣金提取记录     ClubHallServer.GetExchangeRevenueRecordMessage
	//CLIENT_TO_HALL_CLUB_GET_EXCHANGE_MY_REVENUE_RECORD_RES        = 32;     // 返回佣金提取记录结果  ClubHallServer.GetExchangeRevenueRecordMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_REQ                    = 33;     // 获取我的业绩 ClubHallServer.GetMyAchievementMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_RES                    = 34;     // 返回我的业绩 ClubHallServer.GetMyAchievementMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_MEMBER_REQ      = 35;     // 获取会员业绩详情 ClubHallServer.GetAchievementDetailMemberMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_MEMBER_RES      = 36;     // 返回会员业绩详情结果 ClubHallServer.GetAchievementDetailMemberMessage

	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_PARTNER_REQ     = 41;     // 获取合伙人业绩详情 ClubHallServer.GetAchievementDetailPartnerMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_ACHIEVEMENT_DETAIL_PARTNER_RES     = 42;     // 返回合伙人业绩详情结果 ClubHallServer.GetAchievementDetailPartnerMessage

	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_REQ                           = 43;     // 获取俱乐部内我的团队我的俱乐部 ClubHallServer.GetMyClubMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_CLUB_RES                           = 44;     // 返回俱乐部内我的团队我的俱乐部 ClubHallServer.GetMyClubMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_MY_TEAM_REQ                           = 51;     // 获取我的团队成员 ClubHallServer.GetMyTeamMessage
	//CLIENT_TO_HALL_CLUB_GET_MY_TEAM_RES                           = 52;     // 返回我的团队成员 ClubHallServer.GetMyTeamMessageResponse

	//CLIENT_TO_HALL_CLUB_SET_SUBORDINATE_RATE_REQ                  = 53;     // 设置下一级合伙人提成比例     ClubHallServer.SetSubordinateRateMessage
	//CLIENT_TO_HALL_CLUB_SET_SUBORDINATE_RATE_RES                  = 54;     // 返回设置下一级合伙人提成比例  ClubHallServer.SetSubordinateRateMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_PLAY_RECORD_REQ                       = 55;     // 获取游戏明细记录  ClubHallServer.GetPlayRecordMessage
	//CLIENT_TO_HALL_CLUB_GET_PLAY_RECORD_RES                       = 56;     // 获取游戏明细记录  ClubHallServer.GetPlayRecordMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_ALL_PLAY_RECORD_REQ                   = 61;     // 获取玩家俱乐部所有游戏记录列表   ClubHallServer.GetAllPlayRecordMessage
	//CLIENT_TO_HALL_CLUB_GET_ALL_PLAY_RECORD_RES                   = 62;     // 返回玩家俱乐部所有游戏记录列表   ClubHallServer.GetAllPlayRecordMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_USER_SCORE_CHANGE_RECORD_REQ          = 63;     // 获取玩家账户明细列表   ClubHallServer.GetUserScoreChangeRecordMessage
	//CLIENT_TO_HALL_CLUB_GET_USER_SCORE_CHANGE_RECORD_RES          = 64;     // 返回玩家账户明细列表   ClubHallServer.GetUserScoreChangeRecordMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_CLUB_PROMOTER_REQ                     = 65;     // 我的上级信息  ClubHallServer.GetClubPromoterInfoMessage
	//CLIENT_TO_HALL_CLUB_GET_CLUB_PROMOTER_RES                     = 66;     // 我的上级信息  ClubHallServer.GetClubPromoterInfoMessageResponse

	//CLIENT_TO_HALL_CLUB_FIRE_MEMBER_REQ                           = 71;     // 开除此用户  ClubHallServer.FireMemberMessage
	//CLIENT_TO_HALL_CLUB_FIRE_MEMBER_RES                           = 72;     // 开除此用户  ClubHallServer.FireMemberMessageResponse

	//CLIENT_TO_HALL_CLUB_GET_APPLY_CLUB_QQ_REQ                     = 73;     // 获取俱乐部申请QQ  ClubHallServer.GetApplyClubQQMessage
	//CLIENT_TO_HALL_CLUB_GET_APPLY_CLUB_QQ_RES                     = 74;     // 获取俱乐部申请QQ  ClubHallServer.GetApplyClubQQMessage
	
	//CLIENT_TO_HALL_CLUB_CREATE_CLUB_MESSAGE_REQ                   = 75;     // 创建俱乐部  ClubHallServer.CreateClubMessage
	//CLIENT_TO_HALL_CLUB_CREATE_CLUB_MESSAGE_RES                   = 76;     // 创建俱乐部  ClubHallServer.CreateClubMessageResponse

	// 获取俱乐部房间信息
	void GetRoomInfoMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取我的俱乐部
	void GetMyClubHallMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 创建俱乐部
	void CreateClubMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 加入俱乐部
	void JoinTheClubMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 退出俱乐部
	void ExitTheClubMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 设置是否开启自动成为合伙人
	void SetAutoBecomePartnerMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 成为合伙人
	void BecomePartnerMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 佣金兑换金币
	void ExchangeRevenueMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取佣金提取记录 
	void GetExchangeRevenueRecordMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取我的业绩
	void GetMyAchievementMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取会员业绩详情
	void GetAchievementDetailMemberMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取合伙人业绩详情
	void GetAchievementDetailPartnerMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取俱乐部内我的团队我的俱乐部
	void GetMyClubMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取我的团队成员
	void GetMyTeamMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 设置下一级合伙人提成比例
	void SetSubordinateRateMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取游戏明细记录
	void GetPlayRecordMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取玩家俱乐部所有游戏记录列表
	void GetAllPlayRecordMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取玩家账户明细列表
	void GetUserScoreChangeRecordMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 我的上级信息
	void GetClubPromoterInfoMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 开除此用户
	void FireMemberMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);
	// 获取俱乐部申请QQ
	void GetApplyClubQQMessage_club(
		const muduo::net::TcpConnectionPtr& conn, BufferPtr const& buf);

	//===================好友房==================
	/*
	CLIENT_TO_HALL_FRIEND_GET_GAMES_MESSAGE_REQ                   = 3;     //好友房游戏列表       HallFriendServer.GameListMessage
	CLIENT_TO_HALL_FRIEND_GET_GAMES_MESSAGE_RES                   = 4;     //好友房游戏列表返回    HallFriendServer.GameListMessageResponse

	CLIENT_TO_HALL_FRIEND_CREATE_ROOM_MESSAGE_REQ                 = 5;     //创建房间            HallFriendServer.CreateRoomMessage
	CLIENT_TO_HALL_FRIEND_CREATE_ROOM_MESSAGE_RES                 = 6;     //创建房间返回         HallFriendServer.CreateRoomMessageResponse

	CLIENT_TO_HALL_FRIEND_GET_GAME_SERVER_MESSAGE_REQ             = 11;    // 获取游戏服务器IP    HallFriendServer.GetGameServerMessage
	CLIENT_TO_HALL_FRIEND_GET_GAME_SERVER_MESSAGE_RES             = 12;    // 返回游戏服务器IP    HallFriendServer.GetGameServerMessageResponse

	CLIENT_TO_HALL_FRIEND_GET_REAL_TIME_GAME_RECORD_MESSAGE_REQ   = 13;    // 获取好友房 实时战绩  HallFriendServer.GetFriendRoomGameRecordMessage
	CLIENT_TO_HALL_FRIEND_GET_REAL_TIME_GAME_RECORD_MESSAGE_RES   = 14;    // 获取好友房 实时战绩  HallFriendServer.GetFriendRoomGameRecordMessageS

	CLIENT_TO_HALL_FRIEND_GET_MAIN_GAME_RECORD_MESSAGE_REQ        = 15;    // 获取约牌记录  HallFriendServer.GetFriendRoomMainGameRecordMessage
	CLIENT_TO_HALL_FRIEND_GET_MAIN_GAME_RECORD_MESSAGE_RES        = 16;    // 获取约牌记录  HallFriendServer.GetFriendRoomMainGameRecordMessageResponse

	CLIENT_TO_HALL_FRIEND_GET_DETAIL_GAME_RECORD_MESSAGE_REQ      = 21;    // 获取约牌详细记录  HallFriendServer.GetFriendRoomDetailGameRecordMessage
	CLIENT_TO_HALL_FRIEND_GET_DETAIL_GAME_RECORD_MESSAGE_RES      = 22;    // 获取约牌详细记录  HallFriendServer.GetFriendRoomDetailGameRecordMessageResponse

	CLIENT_TO_HALL_FRIEND_GET_CREATE_ROOM_MESSAGE_REQ             = 23;    // 获取创建房间记录  HallFriendServer.GetFriendRoomCreateRoomMessage
	CLIENT_TO_HALL_FRIEND_GET_CREATE_ROOM_MESSAGE_RES             = 24;    // 获取创建房间记录  HallFriendServer.GetFriendRoomCreateRoomMessageResponse

	//=================房卡================
	//购买房卡列表
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_LIST_MESSAGE_REQ          = 25;    // 获取购买房卡列表  HallFriendServer.GetFriendRechargeRoomCardListMessage
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_LIST_MESSAGE_RES          = 26;    // 获取购买房卡列表  HallFriendServer.GetFriendRechargeRoomCardListResponse

	//购买房卡
	CLIENT_TO_HALL_FRIEND_RECHARGE_ROOM_CARD_MESSAGE_REQ          = 31;    // 购买房卡  HallFriendServer.RechargeRoomCardMessage
	CLIENT_TO_HALL_FRIEND_RECHARGE_ROOM_CARD_MESSAGE_RES          = 32;    // 购买房卡  HallFriendServer.RechargeRoomCardResponse

	//获取玩家信息
	CLIENT_TO_HALL_FRIEND_GET_USER_INFO_MESSAGE_REQ               = 33;    // 获取玩家信息  HallFriendServer.GetUserInfoMessage
	CLIENT_TO_HALL_FRIEND_GET_USER_INFO_MESSAGE_RES               = 34;    // 获取玩家信息  HallFriendServer.GetUserInfoResponse

	//转房卡给玩家
	CLIENT_TO_HALL_FRIEND_TRANSFER_ROOM_CARD_MESSAGE_REQ          = 35;    // 转房卡给玩家  HallFriendServer.TransferRoomCardMessage
	CLIENT_TO_HALL_FRIEND_TRANSFER_ROOM_CARD_MESSAGE_RES          = 36;    // 转房卡给玩家  HallFriendServer.TransferRoomCardResponse
	//最近转帐
	CLIENT_TO_HALL_FRIEND_RECENT_TRANSFER_ROOM_CARD_OUT_MESSAGE_REQ = 41;  // 最近转帐  HallFriendServer.RecentTransferRoomCardOutMessage
	CLIENT_TO_HALL_FRIEND_RECENT_TRANSFER_ROOM_CARD_OUT_MESSAGE_RES = 42;  // 最近转帐  HallFriendServer.RecentTransferRoomCardOutResponse
	//常用转帐
	CLIENT_TO_HALL_FRIEND_COMMON_TRANSFER_ROOM_CARD_OUT_MESSAGE_REQ = 43;  // 常用转帐  HallFriendServer.CommonTransferRoomCardOutMessage
	CLIENT_TO_HALL_FRIEND_COMMON_TRANSFER_ROOM_CARD_OUT_MESSAGE_RES = 44;  // 常用转帐  HallFriendServer.CommonTransferRoomCardOutResponse

	//房卡转帐记录
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_IN_OUT_MESSAGE_REQ        = 45;    // 房卡转帐记录  HallFriendServer.GetRoomCardInOutRecordMessage
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_IN_OUT_MESSAGE_RES        = 46;    // 房卡转帐记录  HallFriendServer.GetRoomCardInOutRecordResponse

	//个人中心 房卡明细
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_CHANGE_MESSAGE_REQ        = 51;    // 房卡明细记录  HallFriendServer.GetRoomCardChangeRecordMessage
	CLIENT_TO_HALL_FRIEND_GET_ROOM_CARD_CHANGE_MESSAGE_RES        = 52;    // 房卡明细记录  HallFriendServer.GetRoomCardChangeRecordResponse
	*/

public:
	void loadGameRoomInfos();
	//redis刷新所有房间游戏人数
	void redis_refresh_room_player_nums();
	void redis_update_room_player_nums();
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
	std::map<int64_t, std::set<uint32_t>> mapClubvisibility_;
	std::map<uint32_t, std::set<int64_t>> mapGamevisibility_;
	mutable boost::shared_mutex mutexGamevisibility_;
	//所有游戏房间信息
	::HallServer::GetGameMessageResponse gameinfo_[kClub + 1];
	mutable boost::shared_mutex gameinfo_mutex_[kClub + 1];
	::HallServer::GetServerPlayerNumResponse room_playernums_;
	mutable boost::shared_mutex room_playernums_mutex_;
public:
	int maxConnections_;
	CmdCallbacks handlers_;
	muduo::net::TcpServer server_;
	muduo::net::RpcServer rpcserver_;
	//muduo::net::TcpServer httpserver_;
	muduo::AtomicInt32 numConnected_;
	std::hash<std::string> hash_session_;
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	std::shared_ptr<muduo::net::EventLoopThread> thisTimer_;
	rpc::Connector gameRpcClients_;
	rpc::Container rpcClients_[rpc::kMaxRpcTy];
	CIPLocator ipLocator_;
	bool tracemsg_ = 0;
};

#endif