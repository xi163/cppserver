#ifndef INCLUDE_MONGODB_CLIENT_H
#define INCLUDE_MONGODB_CLIENT_H

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

using bsoncxx::types::b_int64;

namespace MongoDBClient {

	//@@ ThreadLocalSingleton 线程局部使用
	class ThreadLocalSingleton : boost::noncopyable {
	public:
		ThreadLocalSingleton() = delete;
		~ThreadLocalSingleton() = delete;
		//instance
		static mongocxx::client& instance() {
			if (!s_value_) {
				s_value_ = new mongocxx::client(mongocxx::uri{ s_mongoDBConntionString_ }, s_options_);
				s_deleter_.set(s_value_);
			}
			return *s_value_;
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
		static void setUri(std::string const& mongoDBStr) { s_mongoDBConntionString_ = mongoDBStr; }
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
		//mongoDB
		static std::string s_mongoDBConntionString_;
		static mongocxx::options::client s_options_;
	};

}//namespace MongoDBClient

#endif