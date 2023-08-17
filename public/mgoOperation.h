#ifndef INCLUDE_MGO_OPERATION_H
#define INCLUDE_MGO_OPERATION_H

#include "public/Inc.h"
#include "public/gameConst.h"
#include "public/gameStruct.h"
#include "public/errorCode.h"
#include "proto/HallServer.Message.pb.h"

namespace mgo {
	
	using namespace mongocxx;
	using namespace bsoncxx;
	
	namespace opt {
		
		int getErrCode(std::string const& errmsg);
		
		optional<result::insert_one> InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& view);

		optional<result::insert_many> Insert(
			std::string const& dbname,
			std::string const& tblname,
			std::vector<document::view_or_value> const& views);
		
		optional<result::delete_result> DeleteOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& where);

		optional<result::delete_result> Delete(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& where);
		
		optional<result::update> UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& update,
			document::view_or_value const& where);
		
		optional<result::update> Update(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& update,
			document::view_or_value const& where);
		
		optional<document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& where);
		
		mongocxx::cursor Find(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& where);
		
		optional<document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& update,
			document::view_or_value const& where);
		
		optional<document::value> FindOneAndReplace(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& replace,
			document::view_or_value const& where);
		
		optional<document::value> FindOneAndDelete(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& where);

	
	} // namespace opt
	
	int64_t NewUserId(
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
	
	//代理发起人邀请加入
	Msg const& InviteJoinClub(
		int64_t clubId,
		int64_t promoterId,
		int64_t userId,
		int32_t status,
		int32_t ratio = 0, int32_t autopartnerratio = 0);
	
	//用户通过邀请码加入
	Msg const& JoinClub(
		int64_t& clubId,
		int32_t invitationCode,
		int64_t userId,
		int32_t status,
		int32_t ratio = 0, int32_t autopartnerratio = 0);
	
	//用户退出俱乐部
	Msg const& ExitClub(int64_t userId, int64_t clubId);
	
	//代理发起人开除俱乐部成员
	Msg const& FireClubUser(int64_t clubId, int64_t promoterId, int64_t userId);
	
	bool LoadGameRoomInfos(
		::HallServer::GetGameMessageResponse& gameinfos);
	
	bool LoadGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_);

	bool LoadClubGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_);

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