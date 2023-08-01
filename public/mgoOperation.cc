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
			document::view_or_value doc) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};
			return coll.insert_one(doc, opts);
		}

		optional<document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value select,
			document::view_or_value where) {
			static __thread mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);
			return coll.find_one(where, opts);
		}

		optional<document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value select,
			document::view_or_value update,
			document::view_or_value where) {
			document::view view;
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
		document::view_or_value select,
		document::view_or_value update,
		document::view_or_value where) {
		try {
			auto result = opt::FindOneAndUpdate(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::AUTO_INCREMENT,
				select, update, where);
			if (!result/* || result->modified_count() == 0*/) {
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
		document::view_or_value select,
		document::view_or_value where) {
		try {
			auto result = opt::FindOne(
				mgoKeys::db::GAMECONFIG,
				mgoKeys::tbl::AUTO_INCREMENT,
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

	std::string InsertUser(document::view_or_value doc) {
		try {
			auto result = opt::InsertOne(
				mgoKeys::db::GAMEMAIN,
				mgoKeys::tbl::GAMEUSER,
				doc);
			if (!result/* || result->modified_count() == 0*/) {
			}
			else {
				auto docid = result->inserted_id();
				switch (docid.type()) {
				case bsoncxx::type::k_oid:
					bsoncxx::oid oid = docid.get_oid().value;
					std::string insert_id = oid.to_string();
					return insert_id;
					_LOG_DEBUG(insert_id.c_str());
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
}