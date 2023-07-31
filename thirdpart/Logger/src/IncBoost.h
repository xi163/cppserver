#ifndef INCLUDE_INCBOOST_H
#define INCLUDE_INCBOOST_H

#include <boost/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> //boost::iequals
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/regex.hpp>
#include <boost/locale.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/get_pointer.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
//#include <boost/json.hpp>
//#include <boost/json/object.hpp>

using boost::serialization::singleton;
using namespace boost::posix_time;
using namespace boost::gregorian;

#if BOOST_VERSION < 104700
namespace boost {
	template <typename T>
	inline size_t hash_value(boost::shared_ptr<T> const& p) {
		return boost::hash_value(p.get());
	}
}
#endif

#ifdef _windows_
#pragma execution_character_set("utf-8")
#endif

#define read_lock(mutex) boost::shared_lock<boost::shared_mutex> guard(mutex)
#define write_lock(mutex) boost::lock_guard<boost::shared_mutex> guard(mutex)

#define READ_LOCK(mutex) read_lock(mutex)
#define WRITE_LOCK(mutex) write_lock(mutex)

namespace BOOST {
	void replace(std::string& json, const std::string& placeholder, const std::string& value);

	class Json;

	class Any {
	public:
		Any() {}
		virtual ~Any() {}
		virtual void bind(Json& obj) {}
		virtual void bind(Json& obj, int i) {}
	};

	class Json {
		friend std::string Result(int code, std::string const& msg, Any const& data);
	public:
		void put(std::string const& key, int val);
		void put(std::string const& key, int64_t val);
		void put(std::string const& key, float val);
		void put(std::string const& key, double val);

		void put(std::string const& key, int val, int i);
		void put(std::string const& key, int64_t val, int i);
		void put(std::string const& key, float val, int i);
		void put(std::string const& key, double val, int i);

		void put(std::string const& key, std::string const& val);
		void put(std::string const& key, Any const& val);

		void push_back(Any const& val);

		std::string to_json(bool v = true);
	private:
		void reset_();
		void replace_(std::string& json);
		static std::string& final_(std::string& json);
	private:
		std::map<std::string, int> int_;
		std::map<std::string, int64_t> i64_;
		std::map<std::string, float> float_;
		std::map<std::string, double> double_;
		std::vector<Json> objlist_;
	private:
		boost::property_tree::ptree pt_;
	};

	std::string Result(int code, std::string const& msg, Any const& data);
}

#endif