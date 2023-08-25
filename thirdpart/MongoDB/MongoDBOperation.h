#ifndef INCLUDE_MONGODB_OPERATION_H
#define INCLUDE_MONGODB_OPERATION_H

#include "Logger/src/Macro.h"
#include "MongoDB/MongoDBClient.h"

namespace mgo {

	using namespace mongocxx;
	using namespace bsoncxx;

	typedef std::function<bool(optional<mongocxx::result::insert_one> const&)> InsertOneCallback;
	typedef std::function<bool(optional<mongocxx::result::insert_many> const&)> InsertManyCallback;
	typedef std::function<bool(optional<mongocxx::result::delete_result> const&)> DeleteCallback;
	typedef std::function<bool(optional<mongocxx::result::update> const&)> UpdateCallback;
	typedef std::function<bool(optional<bsoncxx::document::value> const&)> ValueCallback;
	typedef std::function<void(mongocxx::cursor const&)> CursorCallback;

	namespace opt {

		int getErrCode(std::string const& errmsg);

		int64_t Count(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		int64_t Count(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int64_t limit,
			int timeout = 5000);

		optional<mongocxx::result::insert_one> InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& view,
			int timeout = 5000);

		optional<mongocxx::result::insert_many> Insert(
			std::string const& dbname,
			std::string const& tblname,
			std::vector<bsoncxx::document::view_or_value> const& views,
			int timeout = 5000);

		optional<mongocxx::result::delete_result> DeleteOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<mongocxx::result::delete_result> Delete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<mongocxx::result::update> UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<mongocxx::result::update> Update(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<bsoncxx::document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		mongocxx::cursor Find(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<bsoncxx::document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<bsoncxx::document::value> FindOneAndReplace(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& replace,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		optional<bsoncxx::document::value> FindOneAndDelete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout = 5000);

		//----------------------------------------------------------

		bool InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& view,
			InsertOneCallback const& cb,
			int timeout = 5000);

		bool Insert(
			std::string const& dbname,
			std::string const& tblname,
			std::vector<bsoncxx::document::view_or_value> const& views,
			InsertManyCallback const& cb,
			int timeout = 5000);

		bool DeleteOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			DeleteCallback const& cb,
			int timeout = 5000);

		bool Delete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			DeleteCallback const& cb,
			int timeout = 5000);

		bool UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			UpdateCallback const& cb,
			int timeout = 5000);

		bool Update(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			UpdateCallback const& cb,
			int timeout = 5000);

		bool FindOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout = 5000);

		void Find(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			CursorCallback const& cb,
			int timeout = 5000);

		bool FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout = 5000);

		bool FindOneAndReplace(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& replace,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout = 5000);

		bool FindOneAndDelete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout = 5000);

	} // namespace opt
} // namespace mgo

#endif