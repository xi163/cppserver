#ifndef INCLUDE_MGO_OPERATION_H
#define INCLUDE_MGO_OPERATION_H

#include "public/Inc.h"

namespace mgo {
	
	using namespace mongocxx;
	using namespace bsoncxx;
	
	namespace opt {
		
		int getErrCode(std::string const& errmsg);

		optional<result::insert_one> InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value doc);

		optional<document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value select,
			document::view_or_value where);

		optional<document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			document::view_or_value select,
			document::view_or_value update,
			document::view_or_value where);
	}
	
	int64_t NewUserId(
		document::view_or_value select,
		document::view_or_value update,
		document::view_or_value where);

	int64_t GetUserId(
		document::view_or_value select,
		document::view_or_value where);

	std::string InsertUser(document::view_or_value doc);
}

#endif