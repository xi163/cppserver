#include "RpcContainer.h"
#include "public/gameConst.h"
#include "Logger/src/log/Logger.h"
#include "RoomNodes.h"

namespace rpc {
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
		case containTy::kRpcGateTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 网关服 %s:%s tcp:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str(), vec[4].c_str());
			//try add & connect
			muduo::net::InetAddress serverAddr(vec[0], atoi(vec[3].c_str()));
			clients_->add(name, serverAddr);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcHallTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 大厅服 %s:%s rpc:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str());
			//try add & connect
			muduo::net::InetAddress serverAddr(vec[0], atoi(vec[2].c_str()));
			clients_->add(name, serverAddr);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcGameTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 游戏服 %s:%s rpc:%s 房间号[%s] %s", _gameip(vec), _game_tcp_port(vec), _game_rpc_port(vec), _roomid(vec), getModeStr(atoi(_modeid(vec))).c_str());
			//try add & connect
			muduo::net::InetAddress serverAddr(_gameip(vec), atoi(_game_rpc_port(vec)));
			clients_->add(name, serverAddr);
			//try remove from repair
			repair_.remove(name);
			room::nodes::add((GameMode)atoi(_modeid(vec)), name);
			break;
		}
		case containTy::kRpcLoginTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 登陆服 %s:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str());
			//try add & connect
			muduo::net::InetAddress serverAddr(vec[0], atoi(vec[2].c_str()));
			clients_->add(name, serverAddr);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcApiTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> API服 %s:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str());
			//try add & connect
			muduo::net::InetAddress serverAddr(vec[0], atoi(vec[2].c_str()));
			clients_->add(name, serverAddr);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		}
	}

	void Container::remove(std::string const& name) {
		switch (ty_) {
		case containTy::kRpcGateTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 网关服 %s:%s tcp:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str(), vec[4].c_str());
			//try remove
			clients_->remove(name, true);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcHallTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 大厅服 %s:%s rpc:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str());
			//try remove
			clients_->remove(name, true);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcGameTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 游戏服 %s:%s rpc:%s 房间号[%s] %s", _gameip(vec), _game_tcp_port(vec), _game_rpc_port(vec), _roomid(vec), getModeStr(atoi(_modeid(vec))).c_str());
			//try remove
			clients_->remove(name, true);
			//try remove from repair
			repair_.remove(name);
			room::nodes::remove((GameMode)atoi(_modeid(vec)), name);
			break;
		}
		case containTy::kRpcLoginTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> 登陆服 %s:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str());
			//try remove
			clients_->remove(name, true);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		case containTy::kRpcApiTy: {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			_LOG_WARN(">>> API服 %s:%s rpc:%s http:%s", vec[0].c_str(), vec[1].c_str(), vec[2].c_str(), vec[3].c_str());
			//try remove
			clients_->remove(name, true);
			//try remove from repair
			repair_.remove(name);
			break;
		}
		}
	}
}