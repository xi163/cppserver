#ifndef INCLUDE_MONGODB_CLIENT_H
#define INCLUDE_MONGODB_CLIENT_H

#include <mongocxx/exception/authentication_exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/gridfs_exception.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <mongocxx/exception/write_exception.hpp>
#include <mongocxx/exception/exception.hpp>

#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>

#include <mongocxx/uri.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/config/config.hpp>
#include <mongocxx/options/client.hpp>
#include <mongocxx/options/client_session.hpp>

//using namespace mongocxx;
//using namespace bsoncxx;
//using namespace bsoncxx::document;
//using namespace bsoncxx::types;
//using namespace bsoncxx::builder::stream;
//using namespace bsoncxx::builder::basic;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::to_json;

using bsoncxx::stdx::optional;
//using bsoncxx::document::view;
//using bsoncxx::document::value;
using bsoncxx::document::view_or_value;

using bsoncxx::types::b_oid;
using bsoncxx::types::b_date;
using bsoncxx::types::b_bool;
using bsoncxx::types::b_utf8;
using bsoncxx::types::b_int32;
using bsoncxx::types::b_int64;
using bsoncxx::types::b_double;
using bsoncxx::types::b_binary;
using bsoncxx::types::b_array;
using bsoncxx::types::b_timestamp;

#define MONGODBCLIENT MongoDBClient::ThreadLocalSingleton::instance()

#define MGO_TRY() \
	try { \

#define MGO_CATCH() } \
	catch (mongocxx::authentication_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::bulk_write_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::gridfs_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::operation_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::logic_error const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::query_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::write_exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (mongocxx::exception const& e) { \
		Errorf(e.what()); \
		switch (mgo::opt::getErrCode(e.what())) { \
		case 11000: \
			break; \
		default: \
			break; \
		} \
	} \
	catch (bsoncxx::exception const& e) { \
		Errorf(e.what()); \
		switch (mgo::opt::getErrCode(e.what())) { \
		case 11000: \
			break; \
		default: \
			break; \
		} \
	} \
	catch (std::exception const& e) { \
		Errorf(e.what()); \
	} \
	catch (...) { \
	} \

namespace MongoDBClient {
	
	void initialize(std::string const& url);
	
	mongocxx::client_session start_session();
	
	//@@ 线程局部使用
	class ThreadLocalSingleton : boost::noncopyable {
	public:
		ThreadLocalSingleton() = delete;
		~ThreadLocalSingleton() = delete;
		//instance
		static inline mongocxx::client& instance() {
			if (__builtin_expect(s_value_ == 0, 0)) {
				setvalue();
			}
			return *s_value_;
		}
		static inline void setvalue() {
			if (!s_value_) {
				s_value_ = new mongocxx::client(mongocxx::uri{ s_uri_ }, s_options_);
				s_deleter_.set(s_value_);
			}
		}
		//reset() 抛连接断开异常时调用
		static void reset() {
			s_deleter_.reset();
			s_value_ = 0;
		}
		//pointer
		static mongocxx::client* pointer() {
			return s_value_;
		}
		//setUri
		static void setUri(std::string const& url) { s_uri_ = url; }
		//setOption
		static void setOption(mongocxx::options::client options) { s_options_ = options; }
	private:
		//destructor
		static void destructor(void* obj) {
			assert(obj == s_value_);
			typedef char T_must_be_complete_type[sizeof(mongocxx::client) == 0 ? -1 : 1];
			T_must_be_complete_type dummy; (void)dummy;
			delete s_value_;
			s_value_ = 0;
		}
		//@@ Deleter
		class Deleter {
		public:
			Deleter() {
				pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
			}
			~Deleter() {
				pthread_key_delete(pkey_);
			}
			//reset
			void reset() {
				pthread_key_delete(pkey_);
			}
			//set
			void set(mongocxx::client* newObj) {
				assert(pthread_getspecific(pkey_) == NULL);
				pthread_setspecific(pkey_, newObj);
			}

			pthread_key_t pkey_;
		};
		//ThreadLocal
		static __thread mongocxx::client* s_value_;
		static Deleter s_deleter_;
		//MongoDB
		static std::string s_uri_;
		static mongocxx::options::client s_options_;
	};

}//namespace MongoDBClient

#endif