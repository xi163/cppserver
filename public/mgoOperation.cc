#include "mgoOperation.h"
#include "mgoKeys.h"

namespace mgo {
	
	namespace opt {

		//E11000 duplicate key
		int getErrCode(std::string const& errmsg) {
			if (!errmsg.empty() && errmsg[0] == 'E') {
				std::string::size_type npos = errmsg.find_first_of(' ');
				if (npos != std::string::npos && npos > 2) {
					std::string numStr = errmsg.substr(1, npos - 1);
					if (boost::regex_match(numStr, boost::regex("^[1-9]*[1-9][0-9]*$"))) {
						return atol(numStr.c_str());
					}
				}
			}
			return 0XFFFFFFFF;
		}

		//https://mongocxx.org/mongocxx-v3/tutorial/
		optional<result::insert_one> InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& view) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};
			return coll.insert_one(view, opts);
		}
		
		optional<result::update> UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& update,
			document::view_or_value const& where) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::update opts = mongocxx::options::update{};
			return coll.update_one(where, update, opts);
		}
		
		optional<document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& where) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);
			return coll.find_one(where, opts);
		}

		mongocxx::cursor Find(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& where) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);
			return coll.find(where, opts);
		}

		optional<document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value const& select,
			document::view_or_value const& update,
			document::view_or_value const& where) {
			//select = document{} << "seq" << 1 << finalize;
			//update = document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize;
			//where = document{} << "_id" << "userid" << finalize;
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_update opts = mongocxx::options::find_one_and_update{};
			opts.projection(select);
			return coll.find_one_and_update(where, update, opts);
		}
	}

	int64_t NewUserId(
		document::view_or_value const& select,
		document::view_or_value const& update,
		document::view_or_value const& where) {
		try {
			auto result = opt::FindOneAndUpdate(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::AUTO_INCREMENT,
				select, update, where);
			if (!result) {
			}
			else {
				document::view view = result->view();
				if (view["seq"]) {
					switch (view["seq"].type()) {
					case bsoncxx::type::k_int64:
						return view["seq"].get_int64();
					case bsoncxx::type::k_int32:
						return view["seq"].get_int32();
					}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return 0;
	}

	int64_t GetUserId(
		document::view_or_value const& select,
		document::view_or_value const& where) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				select, where);
			if (!result) {
			}
			else {
				document::view view = result->view();
				if (view["userid"]) {
					switch (view["userid"].type()) {
					case bsoncxx::type::k_int64:
						return view["userid"].get_int64();
					case bsoncxx::type::k_int32:
						return view["userid"].get_int32();
					}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return 0;
	}
	
	bool UpdateLogin(
		int64_t userId,
		std::string const& loginIp,
		STD::time_point const& lastLoginTime,
		STD::time_point const& now) {
		int64_t days = lastLoginTime.to_sec() / 3600 / 24;
		int64_t nowDays = now.to_sec() / 3600 / 24;
		try {
			if (nowDays == days) {
				auto result = opt::UpdateOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAMEUSER,
					builder::stream::document{}
					<< "$set" << open_document
					<< "lastlogintime" << bsoncxx::types::b_date(*now)
					<< "lastloginip" << loginIp
					<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
					<< close_document << finalize,
					builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
				if (!result || result->modified_count() == 0) {
					return false;
				}
				return true;
			}
			else if (nowDays == days + 1) {
				auto result = opt::UpdateOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAMEUSER,
					builder::stream::document{}
					<< "$set" << open_document
					<< "lastlogintime" << bsoncxx::types::b_date(*now)
					<< "lastloginip" << loginIp
					<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
					<< close_document
					<< "$inc" << open_document
					<< "activedays" << 1								//活跃天数+1
					<< "keeplogindays" << 1			//连续登陆天数+1
					<< close_document << finalize,
					builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
				if (!result || result->modified_count() == 0) {
					return false;
				}
				return true;
			}
			else {
				auto result = opt::UpdateOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAMEUSER,
					builder::stream::document{}
					<< "$set" << open_document
					<< "lastlogintime" << bsoncxx::types::b_date(*now)
					<< "lastloginip" << loginIp
					<< "keeplogindays" << bsoncxx::types::b_int32{ 1 }	//连续登陆天数1
					<< "onlinestatus" << bsoncxx::types::b_int32{ 1 }
					<< close_document
					<< "$inc" << open_document
					<< "activedays" << 1	//活跃天数+1
					<< close_document << finalize,
					builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
				if (!result || result->modified_count() == 0) {
					return false;
				}
				return true;
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool UpdateLogout(
		int64_t userId) {
		try {
			auto result = opt::UpdateOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				builder::stream::document{}
				<< "$set" << open_document
				<< "onlinestatus" << b_int32{ 0 }
				<< close_document << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
			if (!result || result->modified_count() == 0) {
				return false;
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	bool AddLoginLog(
		int64_t userId,
		std::string const& loginIp,
		std::string const& location,
		STD::time_point const& now,
		uint32_t status) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				builder::stream::document{} << "agentid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
			if (!result) {
			}
			else {
				int32_t agentId = 0;
				document::view view = result->view();
				if (view["agentid"]) {
					switch (view["agentid"].type()) {
					case bsoncxx::type::k_int64:
						agentId = view["agentid"].get_int64();
					case bsoncxx::type::k_int32:
						agentId = view["agentid"].get_int32();
					}
				}
				if (agentId > 0) {
					auto result = opt::InsertOne(
						mgoKeys::db::GAMEMAIN,
						mgoKeys::tbl::LOG_LOGIN,
						builder::stream::document{}
						<< "userid" << b_int64{ userId }
						<< "loginip" << loginIp
						<< "address" << location
						<< "status" << b_int32{ status }
						<< "agentid" << b_int32{ agentId }
						<< "logintime" << b_date(*now) << finalize);
					if (!result) {
					}
					else {
						auto docid = result->inserted_id();
						switch (docid.type()) {
						case bsoncxx::type::k_oid:
							bsoncxx::oid oid = docid.get_oid().value;
							std::string insert_id = oid.to_string();
							//_LOG_DEBUG(insert_id.c_str());
							return !insert_id.empty();

						}
					}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	bool AddLogoutLog(
		int64_t userId,
		STD::time_point const& loginTime,
		STD::time_point const& now) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				builder::stream::document{} << "agentid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
			if (!result) {
			}
			else {
				int32_t agentId = 0;
				document::view view = result->view();
				if (view["agentid"]) {
					switch (view["agentid"].type()) {
					case bsoncxx::type::k_int64:
						agentId = view["agentid"].get_int64();
					case bsoncxx::type::k_int32:
						agentId = view["agentid"].get_int32();
					}
				}
				if (agentId > 0) {
					auto result = opt::InsertOne(
						mgoKeys::db::GAMEMAIN,
						mgoKeys::tbl::LOG_LOGOUT,
						builder::stream::document{}
						<< "userid" << b_int64{ userId }
						<< "logintime" << bsoncxx::types::b_date(*loginTime)
						<< "logouttime" << bsoncxx::types::b_date(*now)
						<< "agentid" << b_int32{ agentId }
						<< "playseconds" << bsoncxx::types::b_int64{ (now - loginTime).to_sec() }
						<< finalize);
					if (!result) {
					}
					else {
						auto docid = result->inserted_id();
						switch (docid.type()) {
						case bsoncxx::type::k_oid:
							bsoncxx::oid oid = docid.get_oid().value;
							std::string insert_id = oid.to_string();
							//_LOG_DEBUG(insert_id.c_str());
							return !insert_id.empty();

						}
					}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool GetUserByAgent(
		document::view_or_value const& select,
		document::view_or_value const& where,
		agent_user_t& info) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				select, where);
			if (!result) {
				return false;
			}
			document::view view = result->view();
			if (view["userid"]) {
				switch (view["userid"].type()) {
				case bsoncxx::type::k_int64:
					info.userId = view["userid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.userId = view["userid"].get_int32();
					break;
				}
			}
			if (view["score"]) {
				switch (view["score"].type()) {
				case bsoncxx::type::k_int64:
					info.score = view["score"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.score = view["score"].get_int32();
					break;
				}
			}
			if (view["onlinestatus"]) {
				switch (view["onlinestatus"].type()) {
				case bsoncxx::type::k_int32:
					info.onlinestatus = view["onlinestatus"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					info.onlinestatus = view["onlinestatus"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					break;
				case bsoncxx::type::k_bool:
					info.onlinestatus = view["onlinestatus"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					info.onlinestatus = atol(view["onlinestatus"].get_utf8().value.to_string().c_str());
					break;
				}
			}
			if (view["linecode"]) {
				switch (view["linecode"].type()) {
				case bsoncxx::type::k_utf8:
					info.linecode = view["linecode"].get_utf8().value.to_string();
					break;
				}
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool LoadUserClubs(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::vector<UserClubInfo>& infos) {
		try {
			mongocxx::cursor cursor = opt::Find(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAME_CLUB_MEMBER,
				select, where);
			for (auto& view : cursor) {
				_LOG_WARN(to_json(view).c_str());
				UserClubInfo info;
				if (view["userid"]) {
					switch (view["userid"].type()) {
					case bsoncxx::type::k_int64:
						info.userId = view["userid"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.userId = view["userid"].get_int32();
						break;
					}
				}
				//俱乐部Id 当玩家userId与clubId相同时为盟主
				if (view["clubid"]) {
					switch (view["clubid"].type()) {
					case bsoncxx::type::k_int64:
						info.clubId = view["clubid"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.clubId = view["clubid"].get_int32();
						break;
					}
				}
				if (info.clubId <= 0) {
					return false;
				}
				//我的上级推广代理
				if (view["promoterid"]) {
					switch (view["promoterid"].type()) {
					case bsoncxx::type::k_int64:
						info.promoterId = view["promoterid"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.promoterId = view["promoterid"].get_int32();
						break;
					}
				}
				//1:会员 2:合伙人
				if (view["status"]) {
					switch (view["status"].type()) {
					case bsoncxx::type::k_int64:
						info.status = view["status"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.status = view["status"].get_int32();
						break;
					}
				}
				//邀请码 会员:0 合伙人或盟主 8位数邀请码
				if (view["invitationcode"]) {
					switch (view["invitationcode"].type()) {
					case bsoncxx::type::k_int64:
						info.invitationCode = view["invitationcode"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.invitationCode = view["invitationcode"].get_int32();
						break;
					}
				}
				//提成比例 会员:0 合伙人或盟主:75 表示75%
				if (view["ratio"]) {
					switch (view["ratio"].type()) {
					case bsoncxx::type::k_int64:
						info.ratio = view["ratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.ratio = view["ratio"].get_int32();
						break;
					}
				}
				//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
				if (view["autopartnerratio"]) {
					switch (view["autopartnerratio"].type()) {
					case bsoncxx::type::k_int64:
						info.autoPartnerRatio = view["autopartnerratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.autoPartnerRatio = view["autopartnerratio"].get_int32();
						break;
					}
				}
				//会员:"" 合伙人或盟主:url不为空 当玩家userId与clubId相同时为盟主
				if (view["url"]) {
					switch (view["url"].type()) {
					case bsoncxx::type::k_utf8:
						info.url = view["url"].get_utf8().value.to_string();
						break;
					}
				}
				//加入俱乐部时间
				if (view["createtime"]) {
					switch (view["createtime"].type()) {
					case bsoncxx::type::k_date:
						info.joinTime = ::time_point(view["createtime"].get_date());
						break;
					}
				}
				{
					auto result = opt::FindOne(
						mgoKeys::db::GAMEMAIN,
						mgoKeys::tbl::GAME_CLUB,
						{},
						builder::stream::document{} << "clubid" << b_int64{ info.clubId } << finalize);
					if (!result) {
						return false;
					}
					document::view view = result->view();
					//俱乐部名称
					if (view["clubname"]) {
						switch (view["clubname"].type()) {
						case bsoncxx::type::k_utf8:
							info.clubName = view["clubname"].get_utf8().value.to_string();
							break;
						}
					}
					//俱乐部图标
					if (view["iconid"]) {
						switch (view["iconid"].type()) {
						case bsoncxx::type::k_int64:
							info.iconId = view["iconid"].get_int64();
							break;
						case bsoncxx::type::k_int32:
							info.iconId = view["iconid"].get_int32();
							break;
						}
					}
					//0-未启用 1-活动 2-弃用 3-禁用
					if (view["status"]) {
						switch (view["status"].type()) {
						case bsoncxx::type::k_int64:
							info.clubStatus = view["status"].get_int64();
							break;
						case bsoncxx::type::k_int32:
							info.clubStatus = view["status"].get_int32();
							break;
						}
					}
					//俱乐部人数
					if (view["playernum"]) {
						switch (view["playernum"].type()) {
						case bsoncxx::type::k_int64:
							info.playerNum = view["playernum"].get_int64();
							break;
						case bsoncxx::type::k_int32:
							info.playerNum = view["playernum"].get_int32();
							break;
						}
					}
					//提成比例 会员:0 合伙人或盟主:75 表示75%
					if (view["ratio"]) {
						switch (view["ratio"].type()) {
						case bsoncxx::type::k_int64:
							info.ratio = view["ratio"].get_int64();
							break;
						case bsoncxx::type::k_int32:
							info.ratio = view["ratio"].get_int32();
							break;
						}
					}
					//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
					if (view["autopartnerratio"]) {
						switch (view["autopartnerratio"].type()) {
						case bsoncxx::type::k_int64:
							info.autoPartnerRatio = view["autopartnerratio"].get_int64();
							break;
						case bsoncxx::type::k_int32:
							info.autoPartnerRatio = view["autopartnerratio"].get_int32();
							break;
						}
					}
					//俱乐部创建时间
					if (view["createtime"]) {
						switch (view["createtime"].type()) {
						case bsoncxx::type::k_date:
							info.createTime = ::time_point(view["createtime"].get_date());
							break;
						}
					}
					//俱乐部更新时间
					if (view["updatetime"]) {
						switch (view["updatetime"].type()) {
						case bsoncxx::type::k_date:
							info.updateTime = ::time_point(view["updatetime"].get_date());
							break;
						}
					}
				}
				infos.emplace_back(info);
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool LoadUserClubs(int64_t userId, std::vector<UserClubInfo>& infos) {
		infos.clear();
		return LoadUserClubs(
			{},
			builder::stream::document{} << "userid" << b_int64{ userId } << finalize,
			infos);
	}
	
	bool LoadUserClub(
		document::view_or_value const& select,
		document::view_or_value const& where,
		UserClubInfo& info) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAME_CLUB_MEMBER,
				select, where);
			if (!result) {
				return false;
			}
			document::view view = result->view();
			_LOG_WARN(to_json(view).c_str());
			if (view["userid"]) {
				switch (view["userid"].type()) {
				case bsoncxx::type::k_int64:
					info.userId = view["userid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.userId = view["userid"].get_int32();
					break;
				}
			}
			//俱乐部Id 当玩家userId与clubId相同时为盟主
			if (view["clubid"]) {
				switch (view["clubid"].type()) {
				case bsoncxx::type::k_int64:
					info.clubId = view["clubid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.clubId = view["clubid"].get_int32();
					break;
				}
			}
			if (info.clubId <= 0) {
				return false;
			}
			//我的上级推广代理
			if (view["promoterid"]) {
				switch (view["promoterid"].type()) {
				case bsoncxx::type::k_int64:
					info.promoterId = view["promoterid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.promoterId = view["promoterid"].get_int32();
					break;
				}
			}
			//1:会员 2:合伙人
			if (view["status"]) {
				switch (view["status"].type()) {
				case bsoncxx::type::k_int64:
					info.status = view["status"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.status = view["status"].get_int32();
					break;
				}
			}
			//邀请码 会员:0 合伙人或盟主 8位数邀请码
			if (view["invitationcode"]) {
				switch (view["invitationcode"].type()) {
				case bsoncxx::type::k_int64:
					info.invitationCode = view["invitationcode"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.invitationCode = view["invitationcode"].get_int32();
					break;
				}
			}
			//提成比例 会员:0 合伙人或盟主:75 表示75%
			if (view["ratio"]) {
				switch (view["ratio"].type()) {
				case bsoncxx::type::k_int64:
					info.ratio = view["ratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.ratio = view["ratio"].get_int32();
					break;
				}
			}
			//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
			if (view["autopartnerratio"]) {
				switch (view["autopartnerratio"].type()) {
				case bsoncxx::type::k_int64:
					info.autoPartnerRatio = view["autopartnerratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.autoPartnerRatio = view["autopartnerratio"].get_int32();
					break;
				}
			}
			//会员:"" 合伙人或盟主:url不为空 当玩家userId与clubId相同时为盟主
			if (view["url"]) {
				switch (view["url"].type()) {
				case bsoncxx::type::k_utf8:
					info.url = view["url"].get_utf8().value.to_string();
					break;
				}
			}
			//加入俱乐部时间
			if (view["createtime"]) {
				switch (view["createtime"].type()) {
				case bsoncxx::type::k_date:
					info.joinTime = ::time_point(view["createtime"].get_date());
					break;
				}
			}
			{
				auto result = opt::FindOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB,
					{},
					builder::stream::document{} << "clubid" << b_int64{ info.clubId } << finalize);
				if (!result) {
					return false;
				}
				document::view view = result->view();
				//俱乐部名称
				if (view["clubname"]) {
					switch (view["clubname"].type()) {
					case bsoncxx::type::k_utf8:
						info.clubName = view["clubname"].get_utf8().value.to_string();
						break;
					}
				}
				//俱乐部图标
				if (view["iconid"]) {
					switch (view["iconid"].type()) {
					case bsoncxx::type::k_int64:
						info.iconId = view["iconid"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.iconId = view["iconid"].get_int32();
						break;
					}
				}
				//0-未启用 1-活动 2-弃用 3-禁用
				if (view["status"]) {
					switch (view["status"].type()) {
					case bsoncxx::type::k_int64:
						info.clubStatus = view["status"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.clubStatus = view["status"].get_int32();
						break;
					}
				}
				//俱乐部人数
				if (view["playernum"]) {
					switch (view["playernum"].type()) {
					case bsoncxx::type::k_int64:
						info.playerNum = view["playernum"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.playerNum = view["playernum"].get_int32();
						break;
					}
				}
				//提成比例 会员:0 合伙人或盟主:75 表示75%
				if (view["ratio"]) {
					switch (view["ratio"].type()) {
					case bsoncxx::type::k_int64:
						info.ratio = view["ratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.ratio = view["ratio"].get_int32();
						break;
					}
				}
				//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
				if (view["autopartnerratio"]) {
					switch (view["autopartnerratio"].type()) {
					case bsoncxx::type::k_int64:
						info.autoPartnerRatio = view["autopartnerratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.autoPartnerRatio = view["autopartnerratio"].get_int32();
						break;
					}
				}
				//俱乐部创建时间
				if (view["createtime"]) {
					switch (view["createtime"].type()) {
					case bsoncxx::type::k_date:
						info.createTime = ::time_point(view["createtime"].get_date());
						break;
					}
				}
				//俱乐部更新时间
				if (view["updatetime"]) {
					switch (view["updatetime"].type()) {
					case bsoncxx::type::k_date:
						info.updateTime = ::time_point(view["updatetime"].get_date());
						break;
					}
				}
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool LoadUserClub(
		int64_t userId, int64_t clubId, UserClubInfo& info) {
		return LoadUserClub(
			{},
			builder::stream::document{} << "userid" << b_int64{ userId } << "clubid" << b_int64{ clubId } << finalize,
			info);
	}
	
	bool UserInClub(
		document::view_or_value const& select,
		document::view_or_value const& where) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAME_CLUB_MEMBER,
				select, where);
			if (!result) {
				return false;
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	//代理发起人邀请加入
	Msg const& InviteJoinClub(
		int64_t clubId,
		int64_t promoterId,
		int64_t userId,
		int32_t status,
		int32_t ratio, int32_t autopartnerratio) {
		try {
			if (GetUserId(
				builder::stream::document{} << "userid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << finalize) <= 0) {
				//被邀请人不存在
				return ERR_JoinClub_InvitedUserNotExist;
			}
			if (UserInClub(
				builder::stream::document{} << "userid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << "clubid" << b_int64{ clubId } << finalize)) {
				//被邀请人已加入俱乐部
				return ERR_JoinClub_UserAlreadyInClub;
			}
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAME_CLUB_MEMBER,
				builder::stream::document{} << "ratio" << 1 << "autopartnerratio" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ promoterId }  << "clubid" << b_int64{ clubId } << finalize);
			if (!result) {
				//邀请人不是俱乐部成员
				return ERR_JoinClub_InvitorNotInClubOrClubNotExist;
			}
			document::view view = result->view();
			_LOG_WARN(to_json(view).c_str());
			//提成比例 会员:0 合伙人或盟主:75 表示75%
			if (ratio == 0 && view["ratio"]) {
				switch (view["ratio"].type()) {
				case bsoncxx::type::k_int64:
					ratio = view["ratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					ratio = view["ratio"].get_int32();
					break;
				}
			}
			//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
			if (autopartnerratio == 0 && view["autopartnerratio"]) {
				switch (view["autopartnerratio"].type()) {
				case bsoncxx::type::k_int64:
					autopartnerratio = view["autopartnerratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					autopartnerratio = view["autopartnerratio"].get_int32();
					break;
				}
			}
			{
#if 0
				auto result = opt::FindOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB,
					<< "ratio" << 1 << "autopartnerratio" << 1 << finalize,
					builder::stream::document{} << "clubid" << b_int64{ clubId } << finalize);
				if (!result) {
					return false;
				}
				//提成比例 会员:0 合伙人或盟主:75 表示75%
				if (view["ratio"]) {
					switch (view["ratio"].type()) {
					case bsoncxx::type::k_int64:
						ratio = view["ratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						ratio = view["ratio"].get_int32();
						break;
					}
				}
				//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
				if (view["autopartnerratio"]) {
					switch (view["autopartnerratio"].type()) {
					case bsoncxx::type::k_int64:
						autopartnerratio = view["autopartnerratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						autopartnerratio = view["autopartnerratio"].get_int32();
						break;
					}
				}
#endif
			}
			::time_point now = NOW();
			{
				auto result = opt::InsertOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB_MEMBER,
					builder::stream::document{}
					<< "userid" << b_int64{ userId }
					<< "clubid" << b_int64{ clubId }
					<< "promoterid" << b_int64{ promoterId }
					<< "status" << b_int32{ status }
					<< "invitationcode" << b_int32{ RANDOM().betweenInt64(10000000, 99999999).randInt64_mt() }//STD::variant(utils::random::charStr(8,rTy::Number)).as_ulong()
					<< "ratio" << ratio
					<< "autopartnerratio" << autopartnerratio
					<< "url" << ""
					<< "createtime" << b_date{ now }
					<< finalize);
				if (!result) {
					return ERR_JoinClub_InsideError;
				}
			}
			{
#if 0
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
				}
#endif
				auto result = opt::UpdateOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB,
					builder::stream::document{}
					<< "$inc" << open_document << "playernum" << b_int32{ 1 } << close_document
					<< "$set" << open_document << "updatetime" << b_date{ now } << close_document
					<< finalize,
					builder::stream::document{} << "clubid" << b_int64{ clubId } << finalize);
#if 0
				if (!result || result->modified_count() == 0) {
					return ERR_JoinClub_InsideError;
				}
#endif
			}
			return Ok;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return ERR_JoinClub_InsideError;
	}
	
	//用户通过邀请码加入
	Msg const& JoinClub(
		int64_t& clubId,
		int32_t invitationCode,
		int64_t userId,
		int32_t status,
		int32_t ratio, int32_t autopartnerratio) {
		try {
			if (GetUserId(
				builder::stream::document{} << "userid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << finalize) <= 0) {
				//被邀请人不存在
				return ERR_JoinClub_InvitedUserNotExist;
			}
			/*int64_t*/ clubId = 0;
			int64_t promoterId = 0;
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAME_CLUB_MEMBER,
				builder::stream::document{}
				<< "userid" << 1 << "clubid" << 1 << "ratio" << 1 << "autopartnerratio" << 1 << finalize,
				builder::stream::document{} << "invitationcode" << b_int32{ invitationCode } << finalize);
			if (!result) {
				//无效邀请码或已失效
				return ERR_JoinClub_InvalidInvitationcode;
			}
			document::view view = result->view();
			_LOG_WARN(to_json(view).c_str());
			if (view["userid"]) {
				switch (view["userid"].type()) {
				case bsoncxx::type::k_int64:
					promoterId = view["userid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					promoterId = view["userid"].get_int32();
					break;
				}
			}
			//俱乐部Id 当玩家userId与clubId相同时为盟主
			if (view["clubid"]) {
				switch (view["clubid"].type()) {
				case bsoncxx::type::k_int64:
					clubId = view["clubid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					clubId = view["clubid"].get_int32();
					break;
				}
			}
			//提成比例 会员:0 合伙人或盟主:75 表示75%
			if (ratio == 0 && view["ratio"]) {
				switch (view["ratio"].type()) {
				case bsoncxx::type::k_int64:
					ratio = view["ratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					ratio = view["ratio"].get_int32();
					break;
				}
			}
			//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
			if (autopartnerratio == 0 && view["autopartnerratio"]) {
				switch (view["autopartnerratio"].type()) {
				case bsoncxx::type::k_int64:
					autopartnerratio = view["autopartnerratio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					autopartnerratio = view["autopartnerratio"].get_int32();
					break;
				}
			}
			{
#if 0
				auto result = opt::FindOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB,
					<< "ratio" << 1 << "autopartnerratio" << 1 << finalize,
					builder::stream::document{} << "clubid" << b_int64{ clubId } << finalize);
				if (!result) {
					return false;
				}
				//提成比例 会员:0 合伙人或盟主:75 表示75%
				if (view["ratio"]) {
					switch (view["ratio"].type()) {
					case bsoncxx::type::k_int64:
						ratio = view["ratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						ratio = view["ratio"].get_int32();
						break;
					}
				}
				//-1:自动成为合伙人 无效 0:自动成为合伙人 未开启 1-100:自动成为合伙人 提成比例
				if (view["autopartnerratio"]) {
					switch (view["autopartnerratio"].type()) {
					case bsoncxx::type::k_int64:
						autopartnerratio = view["autopartnerratio"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						autopartnerratio = view["autopartnerratio"].get_int32();
						break;
					}
				}
#endif
			}
			if (UserInClub(
				builder::stream::document{} << "userid" << 1 << finalize,
				builder::stream::document{} << "userid" << b_int64{ userId } << "clubid" << b_int64{ clubId } << finalize)) {
				//被邀请人已加入俱乐部
				return ERR_JoinClub_UserAlreadyInClub;
			}
			::time_point now = NOW();
			{
				auto result = opt::InsertOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB_MEMBER,
					builder::stream::document{}
					<< "userid" << b_int64{ userId }
					<< "clubid" << b_int64{ clubId }
					<< "promoterid" << b_int64{ promoterId }
					<< "status" << b_int32{ status }
					<< "invitationcode" << b_int32{ RANDOM().betweenInt64(10000000, 99999999).randInt64_mt() }//STD::variant(utils::random::charStr(8,rTy::Number)).as_ulong()
					<< "ratio" << ratio
					<< "autopartnerratio" << autopartnerratio
					<< "url" << ""
					<< "createtime" << b_date{ now }
					<< finalize);
				if (!result) {
					return ERR_JoinClub_InsideError;
				}
			}
			{
#if 0
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
				}
#endif
				auto result = opt::UpdateOne(
					mgoKeys::db::GAMEMAIN,
					mgoKeys::tbl::GAME_CLUB,
					builder::stream::document{}
					<< "$inc" << open_document << "playernum" << b_int32{ 1 } << close_document
					<< "$set" << open_document << "updatetime" << b_date{ now } << close_document
					<< finalize,
					builder::stream::document{} << "clubid" << b_int64{ clubId } << finalize);
#if 0
				if (!result || result->modified_count() == 0) {
					return ERR_JoinClub_InsideError;
				}
#endif
			}
			return Ok;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return ERR_JoinClub_InsideError;
	}
	
	bool LoadGameRoomInfos(::HallServer::GetGameMessageResponse& gameinfos) {
		//gameinfos.clear_header();
		//gameinfos.clear_gamemessage();
		try {
			mongocxx::cursor cursor = opt::Find(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::GAME_KIND,
				{}, {});
			for (auto& view : cursor) {
				//_LOG_WARN(to_json(view).c_str());
				::HallServer::GameMessage* gameinfo_ = gameinfos.add_gamemessage();
				gameinfo_->set_gameid(view["gameid"].get_int32());
				gameinfo_->set_gamename(view["gamename"].get_utf8().value.to_string());
				gameinfo_->set_gamesortid(view["sort"].get_int32());//游戏排序0 1 2 3 4
				gameinfo_->set_gametype(view["type"].get_int32());//0-百人场 1-对战类
				gameinfo_->set_gameishot(view["ishot"].get_int32());//0-正常 1-火爆 2-新
				gameinfo_->set_gamestatus(view["status"].get_int32());//-1:关停 0:暂未开放 1：正常开启 2：敬请期待 3: 正在维护
				if (view["rooms"]) {
					switch (view["rooms"].type()) {
					case bsoncxx::type::k_array: {
						auto rooms = view["rooms"].get_array();
						for (auto& item : rooms.value) {
							::HallServer::GameRoomMessage* roomInfo_ = gameinfo_->add_gameroommsg();
							roomInfo_->set_roomid(item["roomid"].get_int32());//房间编号 初 中 高 房间
							roomInfo_->set_roomname(item["roomname"].get_utf8().value.to_string());
							roomInfo_->set_tablecount(item["tablecount"].get_int32());
							roomInfo_->set_ceilscore(item["ceilscore"].get_int64());
							roomInfo_->set_floorscore(item["floorscore"].get_int64());
							roomInfo_->set_enterminscore(item["enterminscore"].get_int64());//最小准入分
							roomInfo_->set_entermaxscore(item["entermaxscore"].get_int64());//最大准入分
							roomInfo_->set_minplayernum(item["minplayernum"].get_int32());//最少游戏人数
							roomInfo_->set_maxplayernum(item["maxplayernum"].get_int32());//最多游戏人数
							roomInfo_->set_maxjettonscore(item["maxjettonscore"].get_int64());//各区域最大下注(限红)
							roomInfo_->set_status(item["status"].get_int32());//-1:关停 0:暂未开放 1：正常开启 2：敬请期待 3: 正在维护
							document::element elem = item["jettons"];
							b_array jettons = elem.type() == bsoncxx::type::k_array ?
											  elem.get_array() :
											  elem["_v"].get_array();
							for (auto& val : jettons.value) {
								roomInfo_->add_jettons(val.get_int64());
							}
						}
						break;
					}
					}
				}
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool LoadGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::GAME_KIND,
				{},
				builder::stream::document{} << "gameid" << b_int32{ gameid } << finalize);
			if (!result) {
				return false;
			}
			document::view view = result->view();
			if (view["gameid"]) {
				switch (view["gameid"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.gameId = view["gameid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.gameId = view["gameid"].get_int32();
					break;
				}
			}
			gameInfo_.gameId = gameid;
			if (view["gamename"]) {
				switch (view["gamename"].type()) {
				case bsoncxx::type::k_utf8:
					gameInfo_.gameName = view["gamename"].get_utf8().value.to_string();
					break;
				}
			}
			//游戏排序0 1 2 3 4
			if (view["sort"]) {
				switch (view["sort"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.sortId = view["sort"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.sortId = view["sort"].get_int32();
					break;
				}
			}
			//*.so名称
			if (view["servicename"]) {
				switch (view["servicename"].type()) {
				case bsoncxx::type::k_utf8:
					gameInfo_.serviceName = view["servicename"].get_utf8().value.to_string();
					break;
				}
			}
			//抽水百分比
			if (view["revenueRatio"]) {
				switch (view["revenueRatio"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.revenueRatio = view["revenueRatio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.revenueRatio = view["revenueRatio"].get_int32();
					break;
				}
			}
			//0-百人场 1-对战类
			if (view["type"]) {
				switch (view["type"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.gameType = view["type"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.gameType = view["type"].get_int32();
					break;
				}
			}
			//0-正常 1-火爆 2-新
			if (view["ishot"]) {
				switch (view["ishot"].type()) {
				case bsoncxx::type::k_int64:
					view["ishot"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					view["ishot"].get_int32();
					break;
				}
			}
			//-1:关停 0:暂未开放 1：正常开启 2：敬请期待 3: 正在维护
			if (view["status"]) {
				switch (view["status"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.serverStatus = view["status"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.serverStatus = view["status"].get_int32();
					break;
				}
			}
// 			if (view["updatePlayerNum"]) {
// 				switch (view["updatePlayerNum"].type()) {
// 				case bsoncxx::type::k_int64:
// 					gameInfo_.updatePlayerNum = view["updatePlayerNum"].get_int64();
// 					break;
// 				case bsoncxx::type::k_int32:
// 					gameInfo_.updatePlayerNum = view["updatePlayerNum"].get_int32();
// 					break;
// 				}
// 			}
// 			//金币场(kGoldCoin)匹配规则
// 			if (view["matchmask"]) {
// 				switch (view["matchmask"].type()) {
// 				case bsoncxx::type::k_int64:
// 					for (int i = 0; i < MTH_MAX; ++i) {
// 						gameInfo_.matchforbids[i] = ((view["matchmask"].get_int64() & (1 << i)) != 0);
// 					}
// 					break;
// 				case bsoncxx::type::k_int32:
// 					for (int i = 0; i < MTH_MAX; ++i) {
// 						gameInfo_.matchforbids[i] = ((view["matchmask"].get_int32() & (1 << i)) != 0);
// 					}
// 					break;
// 				}
// 			}
			if (view["rooms"]) {
				switch (view["rooms"].type()) {
				case bsoncxx::type::k_array: {
					auto rooms = view["rooms"].get_array();
					for (auto& item : rooms.value) {
						if (item["roomid"].get_int32() != roomid) {
							continue;
						}
						roomInfo_.gameId = gameid;
						roomInfo_.roomId = roomid;//房间编号 初 中 高 房间
						roomInfo_.roomName = item["roomname"].get_utf8().value.to_string();
						roomInfo_.tableCount = item["tablecount"].get_int32();
						roomInfo_.floorScore = item["floorscore"].get_int64();
						roomInfo_.ceilScore = item["ceilscore"].get_int64();
						roomInfo_.enterMinScore = item["enterminscore"].get_int64();
						roomInfo_.enterMaxScore = item["entermaxscore"].get_int64();
						roomInfo_.minPlayerNum = item["minplayernum"].get_int32();
						roomInfo_.maxPlayerNum = item["maxplayernum"].get_int32();
						roomInfo_.broadcastScore = item["broadcastscore"].get_int64();
						roomInfo_.bEnableAndroid = item["enableandroid"].get_int32();
						roomInfo_.androidCount = item["androidcount"].get_int32();
						roomInfo_.maxAndroidCount = item["androidmaxusercount"].get_int32();
						roomInfo_.maxJettonScore = item["maxjettonscore"].get_int64();
						roomInfo_.totalStock = item["totalstock"].get_int64();
						roomInfo_.totalStockLowerLimit = item["totalstocklowerlimit"].get_int64();
						roomInfo_.totalStockHighLimit = item["totalstockhighlimit"].get_int64();
						roomInfo_.totalStockSecondLowerLimit = item["totalstocksecondlowerlimit"].get_int64();
						roomInfo_.totalStockSecondHighLimit = item["totalstocksecondhighlimit"].get_int64();
						roomInfo_.systemKillAllRatio = item["systemkillallratio"].get_int32();
						roomInfo_.systemReduceRatio = item["systemreduceratio"].get_int32();
						roomInfo_.changeCardRatio = item["changecardratio"].get_int32();
						roomInfo_.serverStatus = item["status"].get_int32();
						document::element elem = item["jettons"];
						b_array jettons = elem.type() == bsoncxx::type::k_array ?
										  elem.get_array() :
										  elem["_v"].get_array();
						for (auto& val : jettons.value) {
							roomInfo_.jettons.emplace_back(val.get_int64());
						}
						if (gameInfo_.gameType == GameType_BaiRen) {
							roomInfo_.realChangeAndroid = item["realChangeAndroid"].get_int32();
							b_array percentage = item["androidPercentage"]["_v"].get_array();
							for (auto& val : percentage.value) {
								roomInfo_.enterAndroidPercentage.emplace_back(val.get_double());
							}
						}
						return true;
					}
					break;
				}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	bool LoadClubGameRoomInfo(
		uint32_t gameid, uint32_t roomid,
		tagGameInfo& gameInfo_, tagGameRoomInfo& roomInfo_) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::GAME_KIND_CLUB,
				{},
				builder::stream::document{} << "gameid" << b_int32{ gameid } << finalize);
			if (!result) {
				return false;
			}
			document::view view = result->view();
			if (view["gameid"]) {
				switch (view["gameid"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.gameId = view["gameid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.gameId = view["gameid"].get_int32();
					break;
				}
			}
			gameInfo_.gameId = gameid;
			if (view["gamename"]) {
				switch (view["gamename"].type()) {
				case bsoncxx::type::k_utf8:
					gameInfo_.gameName = view["gamename"].get_utf8().value.to_string();
					break;
				}
			}
			//游戏排序0 1 2 3 4
			if (view["sort"]) {
				switch (view["sort"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.sortId = view["sort"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.sortId = view["sort"].get_int32();
					break;
				}
			}
			//*.so名称
			if (view["servicename"]) {
				switch (view["servicename"].type()) {
				case bsoncxx::type::k_utf8:
					gameInfo_.serviceName = view["servicename"].get_utf8().value.to_string();
					break;
				}
			}
			//抽水百分比
			if (view["revenueRatio"]) {
				switch (view["revenueRatio"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.revenueRatio = view["revenueRatio"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.revenueRatio = view["revenueRatio"].get_int32();
					break;
				}
			}
			//0-百人场 1-对战类
			if (view["type"]) {
				switch (view["type"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.gameType = view["type"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.gameType = view["type"].get_int32();
					break;
				}
			}
			//0-正常 1-火爆 2-新
			if (view["ishot"]) {
				switch (view["ishot"].type()) {
				case bsoncxx::type::k_int64:
					view["ishot"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					view["ishot"].get_int32();
					break;
				}
			}
			//-1:关停 0:暂未开放 1：正常开启 2：敬请期待 3: 正在维护
			if (view["status"]) {
				switch (view["status"].type()) {
				case bsoncxx::type::k_int64:
					gameInfo_.serverStatus = view["status"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					gameInfo_.serverStatus = view["status"].get_int32();
					break;
				}
			}
// 			if (view["updatePlayerNum"]) {
// 				switch (view["updatePlayerNum"].type()) {
// 				case bsoncxx::type::k_int64:
// 					gameInfo_.updatePlayerNum = view["updatePlayerNum"].get_int64();
// 					break;
// 				case bsoncxx::type::k_int32:
// 					gameInfo_.updatePlayerNum = view["updatePlayerNum"].get_int32();
// 					break;
// 				}
// 			}
// 			//金币场(kGoldCoin)匹配规则
// 			if (view["matchmask"]) {
// 				switch (view["matchmask"].type()) {
// 				case bsoncxx::type::k_int64:
// 					for (int i = 0; i < MTH_MAX; ++i) {
// 						gameInfo_.matchforbids[i] = ((view["matchmask"].get_int64() & (1 << i)) != 0);
// 					}
// 					break;
// 				case bsoncxx::type::k_int32:
// 					for (int i = 0; i < MTH_MAX; ++i) {
// 						gameInfo_.matchforbids[i] = ((view["matchmask"].get_int32() & (1 << i)) != 0);
// 					}
// 					break;
// 				}
// 			}
			if (view["rooms"]) {
				switch (view["rooms"].type()) {
				case bsoncxx::type::k_array: {
					auto rooms = view["rooms"].get_array();
					for (auto& item : rooms.value) {
						if (item["roomid"].get_int32() != roomid) {
							continue;
						}
						roomInfo_.gameId = gameid;
						roomInfo_.roomId = roomid;//房间编号
						roomInfo_.roomName = item["roomname"].get_utf8().value.to_string();
						roomInfo_.tableCount = item["tablecount"].get_int32();
						roomInfo_.floorScore = item["floorscore"].get_int64();
						roomInfo_.ceilScore = item["ceilscore"].get_int64();
						roomInfo_.enterMinScore = item["enterminscore"].get_int64();
						roomInfo_.enterMaxScore = item["entermaxscore"].get_int64();
						roomInfo_.minPlayerNum = item["minplayernum"].get_int32();
						roomInfo_.maxPlayerNum = item["maxplayernum"].get_int32();
						roomInfo_.broadcastScore = item["broadcastscore"].get_int64();
						roomInfo_.bEnableAndroid = item["enableandroid"].get_int32();
						roomInfo_.androidCount = item["androidcount"].get_int32();
						roomInfo_.maxAndroidCount = item["androidmaxusercount"].get_int32();
						roomInfo_.maxJettonScore = item["maxjettonscore"].get_int64();
						roomInfo_.totalStock = item["totalstock"].get_int64();
						roomInfo_.totalStockLowerLimit = item["totalstocklowerlimit"].get_int64();
						roomInfo_.totalStockHighLimit = item["totalstockhighlimit"].get_int64();
						roomInfo_.totalStockSecondLowerLimit = item["totalstocksecondlowerlimit"].get_int64();
						roomInfo_.totalStockSecondHighLimit = item["totalstocksecondhighlimit"].get_int64();
						roomInfo_.systemKillAllRatio = item["systemkillallratio"].get_int32();
						roomInfo_.systemReduceRatio = item["systemreduceratio"].get_int32();
						roomInfo_.changeCardRatio = item["changecardratio"].get_int32();
						roomInfo_.serverStatus = item["status"].get_int32();
						document::element elem = item["jettons"];
						b_array jettons = elem.type() == bsoncxx::type::k_array ?
										  elem.get_array() :
										  elem["_v"].get_array();
						for (auto& val : jettons.value) {
							roomInfo_.jettons.emplace_back(val.get_int64());
						}
						if (gameInfo_.gameType == GameType_BaiRen) {
							roomInfo_.realChangeAndroid = item["realChangeAndroid"].get_int32();
							b_array percentage = item["androidPercentage"]["_v"].get_array();
							for (auto& val : percentage.value) {
								roomInfo_.enterAndroidPercentage.emplace_back(val.get_double());
							}
						}
						return true;
					}
					break;
				}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	bool GetUserBaseInfo(
		document::view_or_value const& select,
		document::view_or_value const& where,
		UserBaseInfo& info) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				select, where);
			if (!result) {
				return false;
			}
			document::view view = result->view();
			//_LOG_WARN(to_json(view).c_str());
			if (view["userid"]) {
				switch (view["userid"].type()) {
				case bsoncxx::type::k_int64:
					info.userId = view["userid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.userId = view["userid"].get_int32();
					break;
				}
			}
			if (view["account"]) {
				switch (view["account"].type()) {
				case bsoncxx::type::k_utf8:
					info.account = view["account"].get_utf8().value.to_string();
					break;
				}
			}
			if (view["agentid"]) {
				switch (view["agentid"].type()) {
				case bsoncxx::type::k_int64:
					info.agentId = view["agentid"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.agentId = view["agentid"].get_int32();
					break;
				}
			}
			if (view["linecode"]) {
				switch (view["linecode"].type()) {
				case bsoncxx::type::k_utf8:
					info.lineCode = view["linecode"].get_utf8().value.to_string();
					break;
				}
			}
			if (view["nickname"]) {
				switch (view["nickname"].type()) {
				case bsoncxx::type::k_utf8:
					info.nickName = view["nickname"].get_utf8().value.to_string();
					break;
				}
			}
			if (view["headindex"]) {
				switch (view["headindex"].type()) {
				case bsoncxx::type::k_int64:
					info.headId = view["headindex"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.headId = view["headindex"].get_int32();
					break;
				}
			}
			if (view["registertime"]) {
				switch (view["registertime"].type()) {
				case bsoncxx::type::k_date:
					info.registertime = ::time_point(view["registertime"].get_date());
					break;
				}
			}
			if (view["regip"]) {
				switch (view["regip"].type()) {
				case bsoncxx::type::k_utf8:
					info.registerip = view["regip"].get_utf8().value.to_string();
					break;
				}
			}
			if (view["lastlogintime"]) {
				switch (view["lastlogintime"].type()) {
				case bsoncxx::type::k_date:
					info.lastlogintime = ::time_point(view["lastlogintime"].get_date());
					break;
				}
			}
			if (view["lastloginip"]) {
				switch (view["lastloginip"].type()) {
				case bsoncxx::type::k_utf8:
					info.lastloginip = view["lastloginip"].get_utf8().value.to_string();
					break;
				}
			}
			if (view["activedays"]) {
				switch (view["activedays"].type()) {
				case bsoncxx::type::k_int64:
					info.activedays = view["activedays"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.activedays = view["activedays"].get_int32();
					break;
				}
			}
			if (view["keeplogindays"]) {
				switch (view["keeplogindays"].type()) {
				case bsoncxx::type::k_int64:
					info.keeplogindays = view["keeplogindays"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.keeplogindays = view["keeplogindays"].get_int32();
					break;
				}
			}
			if (view["alladdscore"]) {
				switch (view["alladdscore"].type()) {
				case bsoncxx::type::k_int64:
					info.alladdscore = view["alladdscore"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.alladdscore = view["alladdscore"].get_int32();
					break;
				}
			}
			if (view["allsubscore"]) {
				switch (view["allsubscore"].type()) {
				case bsoncxx::type::k_int64:
					info.allsubscore = view["allsubscore"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.allsubscore = view["allsubscore"].get_int32();
					break;
				}
			}
			info.allbetscore = info.alladdscore - info.allsubscore;
			if (view["addscoretimes"]) {
				switch (view["addscoretimes"].type()) {
				case bsoncxx::type::k_int64:
					info.addscoretimes = view["addscoretimes"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.addscoretimes = view["addscoretimes"].get_int32();
					break;
				}
			}
			if (view["subscoretimes"]) {
				switch (view["subscoretimes"].type()) {
				case bsoncxx::type::k_int64:
					info.subscoretimes = view["subscoretimes"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.subscoretimes = view["subscoretimes"].get_int32();
					break;
				}
			}
			if (view["gamerevenue"]) {
				switch (view["gamerevenue"].type()) {
				case bsoncxx::type::k_int64:
					info.gamerevenue = view["gamerevenue"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.gamerevenue = view["gamerevenue"].get_int32();
					break;
				}
			}
			if (view["winorlosescore"]) {
				switch (view["winorlosescore"].type()) {
				case bsoncxx::type::k_int64:
					info.winlostscore = view["winorlosescore"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.winlostscore = view["winorlosescore"].get_int32();
					break;
				}
			}
			if (view["score"]) {
				switch (view["score"].type()) {
				case bsoncxx::type::k_int64:
					info.userScore = view["score"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.userScore = view["score"].get_int32();
					break;
				}
			}
			if (view["status"]) {
				switch (view["status"].type()) {
				case bsoncxx::type::k_int32:
					info.status = view["status"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					info.status = view["status"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					info.status = 0;
					break;
				case bsoncxx::type::k_bool:
					info.status = view["status"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					info.status = atol(view["status"].get_utf8().value.to_string().c_str());
					break;
				}
			}
			if (view["onlinestatus"]) {
				switch (view["onlinestatus"].type()) {
				case bsoncxx::type::k_int32:
					info.onlinestatus = view["onlinestatus"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					info.onlinestatus = view["onlinestatus"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					info.onlinestatus = 0;
					break;
				case bsoncxx::type::k_bool:
					info.onlinestatus = view["onlinestatus"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					info.onlinestatus = atol(view["onlinestatus"].get_utf8().value.to_string().c_str());
					break;
				}
			}
			if (view["gender"]) {
				switch (view["gender"].type()) {
				case bsoncxx::type::k_int64:
					info.gender = view["gender"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.gender = view["gender"].get_int32();
					break;
				}
			}
			if (view["integralvalue"]) {
				switch (view["integralvalue"].type()) {
				case bsoncxx::type::k_int64:
					info.integralvalue = view["integralvalue"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					info.integralvalue = view["integralvalue"].get_int32();
					break;
				}
			}
			return true;
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}

	bool GetUserBaseInfo(int64_t userId, UserBaseInfo& info) {
		return GetUserBaseInfo(
			{},
			builder::stream::document{} << "userid" << b_int64{ userId } << finalize,
			info);
	}
	
	std::string AddUser(document::view_or_value const& view) {
		try {
			auto result = opt::InsertOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				view);
			if (!result) {
			}
			else {
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
					//_LOG_DEBUG(insert_id.c_str());
					return insert_id;
					
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return "";
	}

	bool UpdateUser(
		document::view_or_value const& update,
		document::view_or_value const& where) {
		try {
			auto result = opt::UpdateOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				update, where);
			if (!result || result->modified_count() == 0) {
				return false;
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return true;
	}
	
	bool UpdateOnline(int64_t userId, int32_t status) {
		return UpdateUser(
			builder::stream::document{}
			<< "$set" << open_document
			<< "onlinestatus" << b_int32{ status }
			<< close_document << finalize,
			builder::stream::document{} << "userid" << b_int64{ userId } << finalize);
	}
	
	bool UpdateAgent(
		document::view_or_value const& update,
		document::view_or_value const& where) {
		try {
			auto result = opt::UpdateOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::AGENTINFO,
				update, where);
			if (!result || result->modified_count() == 0) {
				return false;
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return true;
	}

	std::string AddOrder(document::view_or_value const& view) {
		try {
			auto result = opt::InsertOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::ADDSCORE_ORDER,
				view);
			if (!result) {
			}
			else {
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
					//_LOG_DEBUG(insert_id.c_str());
					return insert_id;
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return "";
	}
	
	std::string SubOrder(document::view_or_value const& view) {
		try {
			auto result = opt::InsertOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::SUBSCORE_ORDER,
				view);
			if (!result) {
			}
			else {
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
					//_LOG_DEBUG(insert_id.c_str());
					return insert_id;
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return "";
	}
	
	bool ExistAddOrder(document::view_or_value const& where) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::ADDSCORE_ORDER,
				{}, where);
			if (result) {
				return true;
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	bool ExistSubOrder(document::view_or_value const& where) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::SUBSCORE_ORDER,
				{}, where);
			if (result) {
				return true;
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return false;
	}
	
	std::string AddOrderRecord(document::view_or_value const& view) {
		try {
			auto result = opt::InsertOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::USERSCORE_RECORD,
				view);
			if (!result) {
			}
			else {
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
					//_LOG_DEBUG(insert_id.c_str());
					return insert_id;
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return "";
	}
	
	bool LoadAgentInfos(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::map<int32_t, agent_info_t>& infos) {
		try {
			int32_t agentId = 0;
			mongocxx::cursor cursor = opt::Find(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::AGENTINFO,
				select, where);
			for (auto& view : cursor) {
				//_LOG_WARN(to_json(view).c_str());
				if (view["agentid"]) {
					switch (view["agentid"].type()) {
					case bsoncxx::type::k_int64:
						agentId = view["agentid"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						agentId = view["agentid"].get_int32();
						break;
					}
				}
				if (agentId <= 0) {
					continue;
				}
				agent_info_t& info = infos[agentId];
				info.agentId = agentId;
				if (view["score"]) {
					switch (view["score"].type()) {
					case bsoncxx::type::k_int64:
						info.score = view["score"].get_int64();
						break;
					case bsoncxx::type::k_int32:
						info.score = view["score"].get_int32();
						break;
					}
				}
				if (view["status"]) {
					switch (view["status"].type()) {
					case bsoncxx::type::k_int32:
						info.status = view["status"].get_int32();
						break;
					}
				}
				if (view["cooperationtype"]) {
					switch (view["cooperationtype"].type()) {
					case bsoncxx::type::k_int32:
						info.cooperationtype = view["cooperationtype"].get_int32();
						break;
					}
				}
				if (view["md5code"]) {
					switch (view["md5code"].type()) {
					case bsoncxx::type::k_utf8:
						info.md5code = view["md5code"].get_utf8().value.to_string();
						break;
					}
				}
				if (view["descode"]) {
					switch (view["descode"].type()) {
					case bsoncxx::type::k_utf8:
						info.descode = view["descode"].get_utf8().value.to_string();
						break;
					}
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return infos.size() > 0;
	}

	bool LoadIpWhiteList(
		document::view_or_value const& select,
		document::view_or_value const& where,
		std::map<in_addr_t, eApiVisit> infos) {
		try {
			int32_t agentId = 0;
			mongocxx::cursor cursor = opt::Find(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::IP_WHITE_LIST,
				select, where);
			for (auto& view : cursor) {
				//_LOG_WARN(to_json(view).c_str());
				std::string ipaddr;
				eApiVisit ipstatus = eApiVisit::kDisable;
				if (view["ipaddress"]) {
					switch (view["ipaddress"].type()) {
					case bsoncxx::type::k_utf8:
						ipaddr = view["ipaddress"].get_utf8().value.to_string();
						break;
					}
				}
				if (view["ipstatus"]) {
					switch (view["ipstatus"].type()) {
					case bsoncxx::type::k_int32:
						//0允许访问 1禁止访问
						ipstatus = (view["ipstatus"].get_int32() > 0) ?
							eApiVisit::kDisable : eApiVisit::kEnable;
						break;
					}
				}
				if (!ipaddr.empty() &&
					boost::regex_match(ipaddr,
						boost::regex(
							"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
							"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
					muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
					infos[addr.ipv4NetEndian()] = ipstatus;
				}
			}
		}
		catch (const bsoncxx::exception& e) {
			_LOG_ERROR(e.what());
			switch (opt::getErrCode(e.what())) {
			case 11000:
				break;
			default:
				break;
			}
		}
		catch (const std::exception& e) {
			_LOG_ERROR(e.what());
		}
		catch (...) {
		}
		return infos.size() > 0;
	}
}