#include "IncBoost.h"

namespace BOOST {
	void replace(std::string& json, const std::string& placeholder, const std::string& value) {
		boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
	}
	static inline std::string& final_(std::string& json) {
		boost::replace_all<std::string>(json, "\\", "");
		return json;
	}
	void Json::clear() {
		int_.clear();
		i64_.clear();
		float_.clear();
		double_.clear();
		jsonlist_.clear();
		pt_.clear();
	}
	void Json::parse(std::string const& s) {
		std::stringstream ss(s);
		boost::property_tree::read_json(ss, pt_);
	}
	void Json::parse(char const* data, size_t len) {
		std::string s(data, len);
		parse(s);
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
	void Json::put(std::string const& key, Json const& val) {
		pt_.add_child(key, val.pt_);
		jsonlist_.emplace_back(val);
	}
	void Json::put(std::string const& key, Any const& val) {
		Json json;
		const_cast<Any&>(val).bind(json);
		put(key, json);
	}
	void Json::push_back(Json const& val) {
		pt_.push_back(std::make_pair("", val.pt_));
		jsonlist_.emplace_back(val);
	}
	void Json::push_back(Any const& val) {
		Json json;
		const_cast<Any&>(val).bind(json, (int)pt_.size());
		push_back(json);
	}
	std::string Json::to_string(bool v) {
		std::stringstream ss;
		boost::property_tree::write_json(ss, pt_, false);
		std::string json = ss.str();
		replace_(json);
		switch (v) {
		case true:
			return final_(json);
		}
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
		for (std::vector<Json>::const_iterator it = jsonlist_.begin();
			it != jsonlist_.end(); ++it) {
			const_cast<Json&>(*it).replace_(json);
		}
		clear();
	}
	namespace json {
		std::string Result(int code, std::string const& msg, Json const& data) {
			Json json;
			json.put("code", code);
			json.put("errmsg", msg);
			json.put("data", data);
			std::string s = json.to_string(false);
			return final_(s);
		}
		std::string Result(int code, std::string const& msg, Any const& data) {
			Json json;
			json.put("code", code);
			json.put("errmsg", msg);
			switch (typeid(data) == typeid(Any)) {
			case false:
				json.put("data", data);
			}
			std::string s = json.to_string(false);
			return final_(s);
		}
		std::string Result(int code, std::string const& msg, std::string const& extra, Json const& data) {
			Json json;
			json.put("code", code);
			json.put("errmsg", msg);
			json.put("data", data);
			switch (extra.empty()) {
			case false:
				json.put("extra", extra);
			}
			std::string s = json.to_string(false);
			return final_(s);
		}
		std::string Result(int code, std::string const& msg, std::string const& extra, Any const& data) {
			Json json;
			json.put("code", code);
			json.put("errmsg", msg);
			switch (typeid(data) == typeid(Any)) {
			case false:
				json.put("data", data);
			}
			switch (extra.empty()) {
			case false:
				json.put("extra", extra);
			}
			std::string s = json.to_string(false);
			return final_(s);
		}
	}
}