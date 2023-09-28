#include "Container.h"
#include "public/gameConst.h"
#include "Logger/src/log/Logger.h"

void Container::add(std::vector<std::string> const& names, std::string const& exclude) {
	{
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
	{
		//READ_LOCK(mutex_);
		for (std::string const& name : names) {
			//添加新节点
			if (exclude == name) {
				continue;
			}
			Container::add(name);
		}
	}
}

void Container::process(std::vector<std::string> const& names, std::string const& exclude) {
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
		if (exclude == name) {
			continue;
		}
		Container::remove(name);
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
		if (exclude == name) {
			continue;
		}
		Container::add(name);
	}
	{
		//添加names到names_
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
}

void Container::add(std::string const& name) {
	switch (ty_) {
	case containTy::kGateTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 网关服 %s:%s http:%s:%s tcp:%s:%s rpc:%s:%s", _gate_internet_ip(vec), _gate_ws_port(vec), _gate_internet_ip(vec), _gate_http_port(vec), _gate_ip(vec), _gate_tcp_port(vec), _gate_ip(vec), _gate_rpc_port(vec));
		//try add & connect
		muduo::net::InetAddress serverAddr(_gate_ip(vec), atoi(_gate_tcp_port(vec)));
		clients_->add(name, serverAddr);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	case containTy::kHallTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 大厅服 %s:%s rpc:%s", _hall_ip(vec), _hall_tcp_port(vec), _hall_rpc_port(vec));
		//try add & connect
		muduo::net::InetAddress serverAddr(_hall_ip(vec), atoi(_hall_tcp_port(vec)));
		clients_->add(name, serverAddr);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	case containTy::kGameTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 游戏服 %s:%s rpc:%s 房间号[%s] %s", _serv_ip(vec), _serv_tcp_port(vec), _serv_rpc_port(vec), _serv_roomid(vec), getModeMsg(atoi(_serv_modeid(vec))).c_str());
		//try add & connect
		muduo::net::InetAddress serverAddr(_serv_ip(vec), atoi(_serv_tcp_port(vec)));
		clients_->add(name, serverAddr);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	}
}

void Container::remove(std::string const& name) {
	switch (ty_) {
	case containTy::kGateTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 网关服 %s:%s http:%s:%s tcp:%s:%s rpc:%s:%s", _gate_internet_ip(vec), _gate_ws_port(vec), _gate_internet_ip(vec), _gate_http_port(vec), _gate_ip(vec), _gate_tcp_port(vec), _gate_ip(vec), _gate_rpc_port(vec));
		//try remove
		clients_->remove(name, true);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	case containTy::kHallTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 大厅服 %s:%s rpc:%s", _hall_ip(vec), _hall_tcp_port(vec), _hall_rpc_port(vec));
		//try remove
		clients_->remove(name, true);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	case containTy::kGameTy: {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		Warnf(">>> 游戏服 %s:%s rpc:%s 房间号[%s] %s", _serv_ip(vec), _serv_tcp_port(vec), _serv_rpc_port(vec), _serv_roomid(vec), getModeMsg(atoi(_serv_modeid(vec))).c_str());
		//try remove
		clients_->remove(name, true);
		//try remove from repair
		repair_.remove(name);
		break;
	}
	}
}