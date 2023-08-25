#include "Logger/src/log/Logger.h"

#include "MongoDBClient.h"

namespace MongoDBClient {

	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	void initialize(std::string const& url) {

		// This should be done only once.
		/*static*/ __thread mongocxx::instance instance{};

		ThreadLocalSingleton::setUri(url);

		_LOG_INFO("%s", url.c_str());
	}

	mongocxx::client_session start_session() {
		return MONGODBCLIENT.start_session();
	}

	__thread mongocxx::client* ThreadLocalSingleton::s_value_ = 0;
	typename ThreadLocalSingleton::Deleter ThreadLocalSingleton::s_deleter_;
	std::string ThreadLocalSingleton::s_uri_ = "";
	mongocxx::options::client ThreadLocalSingleton::s_options_;

}