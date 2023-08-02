#include "IncBoost.h"

namespace BOOST {
	void replace(std::string& json, const std::string& placeholder, const std::string& value) {
		boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
	}
	void Json::put(std::string const& key, int val) {
		pt_.put(key, ":" + key);
		int_[":" + key] = val;
	}
	void Json::put(std::string const& key, int64_t val) {
		pt_.put(key, ":" + key);
		i64_[":" + key] = val;
	}
	void Json::put(std::string const& key, float val) {
		pt_.put(key, ":" + key);
		float_[":" + key] = val;
	}
	void Json::put(std::string const& key, double val) {
		pt_.put(key, ":" + key);
		double_[":" + key] = val;
	}
	void Json::put(std::string const& key, int val, int i) {
		pt_.put(key, std::to_string(i) + ":" + key);
		int_[std::to_string(i) + ":" + key] = val;
	}
	void Json::put(std::string const& key, int64_t val, int i) {
		pt_.put(key, std::to_string(i) + ":" + key);
		i64_[std::to_string(i) + ":" + key] = val;
	}
	void Json::put(std::string const& key, float val, int i) {
		pt_.put(key, std::to_string(i) + ":" + key);
		float_[std::to_string(i) + ":" + key] = val;
	}
	void Json::put(std::string const& key, double val, int i) {
		pt_.put(key, std::to_string(i) + ":" + key);
		double_[std::to_string(i) + ":" + key] = val;
	}
	void Json::put(std::string const& key, std::string const& val) {
		pt_.put(key, val);
	}
	void Json::put(std::string const& key, Any const& val) {
		Json obj;
		const_cast<Any&>(val).bind(obj);
		pt_.add_child(key, obj.pt_);
		objlist_.emplace_back(obj);
	}
	void Json::push_back(Any const& val) {
		Json obj;
		const_cast<Any&>(val).bind(obj, pt_.size());
		pt_.push_back(std::make_pair("", obj.pt_));
		objlist_.emplace_back(obj);
	}
	std::string Json::to_json(bool v) {
		std::stringstream ss;
		boost::property_tree::write_json(ss, pt_, false);
		std::string json = ss.str();
		replace_(json);
		switch (v) {
		case true:
			return Json::final_(json);
		}
		return json;
	}
	std::string& Json::final_(std::string& json) {
		boost::replace_all<std::string>(json, "\\", "");
		return json;
	}
	void Json::replace_(std::string& json) {
		for (std::map<std::string, int>::const_iterator it = int_.begin();
			it != int_.end(); ++it) {
			replace(json, it->first, std::to_string(it->second));
		}
		for (std::map<std::string, int64_t>::const_iterator it = i64_.begin();
			it != i64_.end(); ++it) {
			replace(json, it->first, std::to_string(it->second));
		}
		for (std::map<std::string, float>::const_iterator it = float_.begin();
			it != float_.end(); ++it) {
			replace(json, it->first, std::to_string(it->second));
		}
		for (std::map<std::string, double>::const_iterator it = double_.begin();
			it != double_.end(); ++it) {
			replace(json, it->first, std::to_string(it->second));
		}
		for (std::vector<Json>::const_iterator it = objlist_.begin();
			it != objlist_.end(); ++it) {
			const_cast<Json&>(*it).replace_(json);
		}
		reset_();
	}
	void Json::reset_() {
		int_.clear();
		i64_.clear();
		float_.clear();
		double_.clear();
		objlist_.clear();
	}
	namespace json {
		std::string Result(int code, std::string const& msg, Any const& data) {
			Json obj;
			obj.put("code", code);
			obj.put("errmsg", msg);
			obj.put("data", data);
			std::string json = obj.to_json(false);
			return Json::final_(json);
		}
	}
}