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
			if (view["nickname"]) {
				switch (view["nickname"].type()) {
				case bsoncxx::type::k_utf8:
					info.nickName = view["nickname"].get_utf8().value.to_string();
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
			info.allbetscore = info.alladdscore - info.allsubscore;
			if (view["status"]) {
				switch (view["status"].type()) {
				case bsoncxx::type::k_int32:
					info.status = view["status"].get_int32();
					break;
				case bsoncxx::type::k_int64:
					info.status = view["status"].get_int64();
					break;
				case  bsoncxx::type::k_null:
					break;
				case bsoncxx::type::k_bool:
					info.status = view["status"].get_bool();
					break;
				case bsoncxx::type::k_utf8:
					info.status = atol(view["status"].get_utf8().value.to_string().c_str());
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

	bool GetUserBaseInfo(int64_t userid, UserBaseInfo& info) {
		return GetUserBaseInfo(
			{},
			builder::stream::document{} << "userid" << userid << finalize,
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
					_LOG_DEBUG(insert_id.c_str());
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
					_LOG_DEBUG(insert_id.c_str());
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
					_LOG_DEBUG(insert_id.c_str());
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
				_LOG_WARN(to_json(view).c_str());
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
				_LOG_WARN(to_json(view).c_str());
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
						//0ÔÊÐí·ÃÎÊ 1½ûÖ¹·ÃÎÊ
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