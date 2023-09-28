#include "Logger/src/log/Logger.h"

#include "MongoDB/MongoDBOperation.h"

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

		int64_t Count(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::count opts = mongocxx::options::count{};

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));

			return coll.count_documents(where, opts);
		}

		int64_t Count(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int64_t limit,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::count opts = mongocxx::options::count{};

			opts.limit(limit);
			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));

			return coll.count_documents(where, opts);
		}

		optional<mongocxx::result::insert_one> InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& view,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.insert_one(view, opts);
		}

		optional<mongocxx::result::insert_many> Insert(
			std::string const& dbname,
			std::string const& tblname,
			std::vector<bsoncxx::document::view_or_value> const& views,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.insert_many(views, opts);
		}

		optional<mongocxx::result::delete_result> DeleteOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::delete_options opts = mongocxx::options::delete_options{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.delete_one(where, opts);
		}

		optional<mongocxx::result::delete_result> Delete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::delete_options opts = mongocxx::options::delete_options{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.delete_many(where, opts);
		}

		optional<mongocxx::result::update> UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::update opts = mongocxx::options::update{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.update_one(where, update, opts);
		}

		optional<mongocxx::result::update> Update(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::update opts = mongocxx::options::update{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.update_many(where, update, opts);
		}

		optional<bsoncxx::document::value> FindOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));

			return coll.find_one(where, opts);
		}

		mongocxx::cursor Find(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));
#if 0
			mongocxx::cursor cursor = coll.find(where, opts);
			for (auto const& view : cursor) {
				Warnf(to_json(view).c_str());
			}
#endif
			return coll.find(where, opts);
		}
		
		mongocxx::cursor Find(
			std::string const& dbname,
			std::string const& tblname,
			int64_t skip, int64_t limit,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			bsoncxx::document::view_or_value const& sort,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);
			
			opts.sort(sort);
			opts.skip(skip);
			opts.limit(limit);
			
			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));
#if 0
			mongocxx::cursor cursor = coll.find(where, opts);
			for (auto const& view : cursor) {
				Warnf(to_json(view).c_str());
			}
#endif
			return coll.find(where, opts);
		}
		
		optional<bsoncxx::document::value> FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			//select = document{} << "seq" << 1 << finalize;
			//update = document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize;
			//where = document{} << "_id" << "userid" << finalize;
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_update opts = mongocxx::options::find_one_and_update{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.find_one_and_update(where, update, opts);
		}

		optional<bsoncxx::document::value> FindOneAndReplace(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& replace,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_replace opts = mongocxx::options::find_one_and_replace{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.find_one_and_replace(where, replace, opts);
		}

		optional<bsoncxx::document::value> FindOneAndDelete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_delete opts = mongocxx::options::find_one_and_delete{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return coll.find_one_and_delete(where, opts);
		}

		//----------------------------------------------------------

		bool InsertOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& view,
			InsertOneCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.insert_one(view, opts));
		}

		bool Insert(
			std::string const& dbname,
			std::string const& tblname,
			std::vector<bsoncxx::document::view_or_value> const& views,
			InsertManyCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::insert opts = mongocxx::options::insert{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.insert_many(views, opts));
		}

		bool DeleteOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			DeleteCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::delete_options opts = mongocxx::options::delete_options{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.delete_one(where, opts));
		}

		bool Delete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& where,
			DeleteCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::delete_options opts = mongocxx::options::delete_options{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.delete_many(where, opts));
		}

		bool UpdateOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			UpdateCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::update opts = mongocxx::options::update{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.update_one(where, update, opts));
		}

		bool Update(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			UpdateCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::update opts = mongocxx::options::update{};

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.update_many(where, update, opts));
		}

		bool FindOne(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));

			return cb(coll.find_one(where, opts));
		}

		void Find(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			CursorCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));		
#if 0
			mongocxx::cursor cursor = coll.find(where, opts);
			for (auto const& view : cursor) {
				Warnf(to_json(view).c_str());
			}
#endif
			cb(coll.find(where, opts));
		}
		
		void Find(
			std::string const& dbname,
			std::string const& tblname,
			int64_t skip, int64_t limit,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			bsoncxx::document::view_or_value const& sort,
			CursorCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find opts = mongocxx::options::find{};
			opts.projection(select);
			
			opts.sort(sort);
			opts.skip(skip);
			opts.limit(limit);

			opts.max_time(std::chrono::milliseconds(timeout));
			//opts.max_await_time(std::chrono::milliseconds(timeout));
			//opts.no_cursor_timeout(true);//FIXME: true-让cursor永不超时

			mongocxx::read_preference rp;
			rp.mode(mongocxx::read_preference::read_mode::k_secondary);
			opts.read_preference(rp).max_time(std::chrono::milliseconds(timeout));
#if 0
			mongocxx::cursor cursor = coll.find(where, opts);
			for (auto const& view : cursor) {
				Warnf(to_json(view).c_str());
			}
#endif
			cb(coll.find(where, opts));
		}
		
		bool FindOneAndUpdate(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& update,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout) {
			//select = document{} << "seq" << 1 << finalize;
			//update = document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize;
			//where = document{} << "_id" << "userid" << finalize;
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_update opts = mongocxx::options::find_one_and_update{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.find_one_and_update(where, update, opts));
		}

		bool FindOneAndReplace(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& replace,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_replace opts = mongocxx::options::find_one_and_replace{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.find_one_and_replace(where, replace, opts));
		}

		bool FindOneAndDelete(
			std::string const& dbname,
			std::string const& tblname,
			bsoncxx::document::view_or_value const& select,
			bsoncxx::document::view_or_value const& where,
			ValueCallback const& cb,
			int timeout) {
			/*static*/ /*__thread*/ mongocxx::database db = MONGODBCLIENT[dbname];
			mongocxx::collection  coll = db[tblname];
			mongocxx::options::find_one_and_delete opts = mongocxx::options::find_one_and_delete{};
			opts.projection(select);

			opts.max_time(std::chrono::milliseconds(timeout));

			mongocxx::write_concern wc;
			wc.timeout(std::chrono::milliseconds(timeout));
			mongocxx::options::insert insert_options;
			opts.write_concern(wc);

			return cb(coll.find_one_and_delete(where, opts));
		}

	} // namespace opt
} // namespace mgo