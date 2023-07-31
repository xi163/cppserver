#ifndef INCLUDE_MGO_OPERATION_H
#define INCLUDE_MGO_OPERATION_H

#include "public/Inc.h"

namespace mgo {

	using namespace bsoncxx;
	//using namespace bsoncxx::document;

	int getErrCode(std::string const& errmsg);

	optional<document::value> FindOneAndUpdate(
		std::string const& dbname,
		std::string const& tblname,
		document::view_or_value select,
		document::view_or_value update,
		document::view_or_value where);

	int64_t NewUserId(
		document::view_or_value select,
		document::view_or_value update,
		document::view_or_value where);
}

#endif