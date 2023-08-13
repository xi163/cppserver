#include "RoomContainer.h"
#include "public/gameConst.h"

void RoomContainer::random_game_server_ipport(uint32_t roomid, std::string& ipport) {
	ipport.clear();
	READ_LOCK(room_servers_mutex_);
	std::map<int, std::set<std::string>>::iterator it = room_servers_.find(roomid);
	if (it != room_servers_.end()) {
		if (!it->second.empty()) {
			std::vector<std::string> rooms;
			std::copy(it->second.begin(), it->second.end(), std::back_inserter(rooms));
			int index = RANDOM().betweenInt(0, rooms.size() - 1).randInt_mt();
			ipport = rooms[index];//roomid:ip:port:mode
		}
	}
}

void RoomContainer::add(std::vector<std::string> const& names) {
	{
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
	{
		//READ_LOCK(mutex_);
		for (std::string const& name : names) {
			//添加新节点
			RoomContainer::add(name);
		}
	}
}

void RoomContainer::process(std::vector<std::string> const& names) {
	std::set<std::string> oldset, newset(names.begin(), names.end());
	{
		READ_LOCK(mutex_);
		for (std::string const& name : names_) {
			oldset.emplace(name);
		}
	}
	//失效节点：names_中有，而names中没有
	std::vector<std::string> diff(oldset.size());
	std::vector<std::string>::iterator it;
	it = std::set_difference(oldset.begin(), oldset.end(), newset.begin(), newset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& name : diff) {
		//names_中有
		assert(std::find(std::begin(names_), std::end(names_), name) != names_.end());
		//names中没有
		assert(std::find(std::begin(names), std::end(names), name) == names.end());
		//失效则移除
		RoomContainer::remove(name);
	}
	//活动节点：names中有，而names_中没有
	diff.clear();
	diff.resize(newset.size());
	it = std::set_difference(newset.begin(), newset.end(), oldset.begin(), oldset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& name : diff) {
		//names_中没有
		assert(std::find(std::begin(names_), std::end(names_), name) == names_.end());
		//names中有
		assert(std::find(std::begin(names), std::end(names), name) != names.end());
		//添加新节点
		RoomContainer::add(name);
	}
	{
		//添加names到names_
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
}

void RoomContainer::add(std::string const& name) {
	//name：roomid:ip:port:mode
	std::vector<std::string> vec;
	boost::algorithm::split(vec, name, boost::is_any_of(":"));
	_LOG_WARN(">>> 游戏服[%s:%s] 房间号[%s] %s", vec[1].c_str(), vec[2].c_str(), vec[0].c_str(), getModeStr(atoi(vec[3].c_str())).c_str());
	WRITE_LOCK(room_servers_mutex_);
	std::set<std::string>& room = room_servers_[stoi(vec[0])];
	room.insert(name);
}

void RoomContainer::remove(std::string const& name) {
	//name：roomid:ip:port:mode
	std::vector<std::string> vec;
	boost::algorithm::split(vec, name, boost::is_any_of(":"));
	_LOG_WARN(">>> 游戏服[%s:%s] 房间号[%s] %s", vec[1].c_str(), vec[2].c_str(), vec[0].c_str(), getModeStr(atoi(vec[3].c_str())).c_str());
	WRITE_LOCK(room_servers_mutex_);
	std::map<int, std::set<std::string>>::iterator it = room_servers_.find(stoi(vec[0]));
	if (it != room_servers_.end()) {
		std::set<std::string>::iterator ir = it->second.find(name);
		if (ir != it->second.end()) {
			it->second.erase(ir);
			if (it->second.empty()) {
				room_servers_.erase(it);
			}
		}
	}
}