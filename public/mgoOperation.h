#ifndef INCLUDE_MGO_OPERATION_H
#define INCLUDE_MGO_OPERATION_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "public/errorCode.h"
#include "proto/HallServer.Message.pb.h"

namespace mgo {
	
	int64_t NewAutoId(
		document::view_or_value const& select,
		document::view_or_value const& update,
		document::view_or_value const& where);

	int64_t GetUserId(
		document::view_or_value const& select,
		document::view_or_value const& where);
	
	bool UpdateLogin(
		int64_t userId,
		std::string const& loginIp,
		STD::time_point const& lastLoginTime,
		STD::time_point const& now);

	bool UpdateLogout(
		int64_t userId);

	bool AddLoginLog(
		int64_t userId,
		std::string const& loginIp,
		std::string const& location,
		STD::time_point const& now,
		uint32_t status);

	bool AddLogoutLog(
		int64_t userId,
		STD::time_point const& loginTime,
		STD::time_point const& now);
	
	bool GetUserByAgent(
		document::view_or_value const& select,
		document::view_or_value const& where,
		agent_user_t& info);

	bool GetUserBaseInfo(
		document::view_or_value const& select,
		document::view_or_value const& where,
		UserBaseInfo& info);
	
	bool LoadUserClubs(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::vector<UserClubInfo>& infos);

	bool LoadUserClubs(
		int64_t userId, std::vector<UserClubInfo>& infos);
	
	bool LoadUserClub(
		document::view_or_value const& select,
		document::view_or_value const& where,
		UserClubInfo& info);
	
	bool LoadUserClub(
		int64_t userId, int64_t clubId, UserClubInfo& info);
	
	bool UserInClub(
		document::view_or_value const& select,
		document::view_or_value const& where);
	
	//创建俱乐部
	Msg const& CreateClub(
		int64_t userId,
		std::string const& clubName,
		int32_t ratio, int32_t autopartnerratio,
		UserClubInfo& info);

	//代理发起人邀请加入俱乐部
	Msg const& InviteJoinClub(
		int64_t clubId,
		int64_t promoterId,
		int64_t userId,
		int32_t status,
		int32_t ratio = 0, int32_t autopartnerratio = 0);
	
	//用户通过邀请码加入俱乐部
	Msg const& JoinClub(
		int64_t& clubId,
		int32_t invitationCode,
		int64_t userId);
	
	//用户退出俱乐部
	Msg const& ExitClub(int64_t userId, int64_t clubId);
	
	//代理发起人开除俱乐部成员
	Msg const& FireClubUser(int64_t clubId, int64_t promoterId, int64_t userId);
	
	//设置是否开启自动成为合伙人
	Msg const& SetAutoBecomePartner(int64_t userId, int64_t clubId, int32_t autopartnerratio);
	
	//成为合伙人
	Msg const& BecomePartner(int64_t userId, int64_t clubId, int64_t memberId, int32_t ratio);
	
	bool LoadGameClubInfos(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::vector<tagGameClubInfo>& infos);
	
	bool LoadGameClubInfos(std::vector<tagGameClubInfo>& infos);
	
	bool LoadGameRoomInfos(
		::HallServer::GetGameMessageResponse& gameinfos);
	
	bool LoadClubGamevisibility(
		std::map<uint32_t, std::set<int64_t>>& mapGamevisibility,
		std::map<int64_t, std::set<uint32_t>>& mapClubvisibility);
	
	bool LoadGameClubRoomInfos(
		::HallServer::GetGameMessageResponse& gameinfos);

	//金币场
	bool LoadGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_);
	bool LoadGameRoomInfos(
		uint32_t gameid,
		tagGameInfo& gameInfo_, std::vector<tagGameRoomInfo>& roomInfos_);
	//俱乐部
	bool LoadClubGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_);
	bool LoadClubGameRoomInfos(
		uint32_t gameid,
		tagGameInfo& gameInfo_, std::vector<tagGameRoomInfo>& roomInfos_);

	bool GetUserBaseInfo(int64_t userid, UserBaseInfo& info);

	std::string AddUser(document::view_or_value const& view);
	
	bool UpdateUser(
		document::view_or_value const& update,
		document::view_or_value const& where);
	
	bool UpdateOnline(int64_t userid, int32_t status);
	
	bool UpdateAgent(
		document::view_or_value const& update,
		document::view_or_value const& where);

	std::string AddOrder(document::view_or_value const& view);
	std::string SubOrder(document::view_or_value const& view);

	bool ExistAddOrder(document::view_or_value const& where);
	bool ExistSubOrder(document::view_or_value const& where);
	std::string AddOrderRecord(document::view_or_value const& view);

	bool LoadAgentInfos(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::map<int32_t, agent_info_t>& infos);

	bool LoadIpWhiteList(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::map<in_addr_t, eApiVisit> infos);
}

#endif