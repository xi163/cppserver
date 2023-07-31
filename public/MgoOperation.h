#ifndef INCLUDE_MGO_OPERATION_H
#define INCLUDE_MGO_OPERATION_H

#include "public/Inc.h"

namespace mgo {

	int getErrCode(std::string const& errmsg);

	optional<bsoncxx::document::value> FindOneAndUpdate(
		std::string const& dbname,
		std::string const& tblname,
		bsoncxx::document::view_or_value select,
		bsoncxx::document::view_or_value update,
		bsoncxx::document::view_or_value where);

	int64_t NewUserId(
		bsoncxx::document::view_or_value select,
		bsoncxx::document::view_or_value update,
		bsoncxx::document::view_or_value where);
}

#endif