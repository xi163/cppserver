#include "RoomNodes.h"
#include "public/gameConst.h"
#include "Logger/src/log/Logger.h"
#include "RpcService_c.h"

#include "../../Hall.h"

extern HallServ* gServer;

namespace room {
	namespace nodes {

		static class RoomList {
		public:
			void add(std::string const& name);
			void remove(std::string const& name);
			void random_server(uint32_t roomid, std::string& ipport);
			void balance_server(uint32_t roomid, std::string& ipport);
		private:
			std::map<int, std::set<std::string>> nodes_;
			mutable boost::shared_mutex mutex_;
		}game_serv_[kClub + 1];

		void RoomList::add(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			std::set<std::string>& room = nodes_[stoi(vec[0])];
			room.insert(name);
		}

		void RoomList::remove(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			std::map<int, std::set<std::string>>::iterator it = nodes_.find(stoi(vec[0]));
			if (it != nodes_.end()) {
				std::set<std::string>::iterator ir = it->second.find(name);
				if (ir != it->second.end()) {
					it->second.erase(ir);
					if (it->second.empty()) {
						nodes_.erase(it);
					}
				}
			}
		}

		void RoomList::random_server(uint32_t roomid, std::string& ipport) {
			ipport.clear();
			READ_LOCK(mutex_);
			std::map<int, std::set<std::string>>::iterator it = nodes_.find(roomid);
			if (it != nodes_.end()) {
				if (!it->second.empty()) {
					std::vector<std::string> rooms;
					std::copy(it->second.begin(), it->second.end(), std::back_inserter(rooms));
					int index = RANDOM().betweenInt(0, rooms.size() - 1).randInt_mt();
					ipport = rooms[index];
				}
			}
		}

		void RoomList::balance_server(uint32_t roomid, std::string& ipport) {
			std::map<int, std::set<std::string>>::iterator it = nodes_.find(roomid);
			if (it != nodes_.end()) {
				if (!it->second.empty()) {
					std::vector<std::pair<std::string, int>> rooms;
					std::set<std::string>& ref = it->second;
					for (std::set<std::string>::iterator it = ref.begin(); it != ref.end(); ++it) {
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*it, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::NodeInfoReq req;
						::Game::Rpc::NodeInfoRspPtr rsp = client.GetNodeInfo(req);
						if (rsp) {
							//_LOG_ERROR("%s %s", it->c_str(), rsp->nodevalue().c_str());
							rooms.emplace_back(std::make_pair(rsp->nodevalue(), rsp->numofloads()));
						}
					}
					int i = 0;
					int minLoads = 0;
					for (int k = 0; k < rooms.size(); k++) {
						_LOG_WARN("[%d] %s numOfLoads:%d", k, rooms[k].first.c_str(), rooms[k].second);
						switch (k) {
						case 0:
							i = k;
							minLoads = rooms[k].second;
							break;
						default:
							if (minLoads > rooms[k].second) {
								i = k;
								minLoads = rooms[k].second;
							}
							break;
						}
					}
					if (rooms.size() > 0) {
						_LOG_DEBUG(">>> [%d] %s numOfLoads:%d", i, rooms[i].first.c_str(), rooms[i].second);
						ipport = rooms[i].first;
					}
				}
			}
		}

		void add(GameMode mode, std::string const& name) {
			game_serv_[mode].add(name);
		}

		void remove(GameMode mode, std::string const& name) {
			game_serv_[mode].remove(name);
		}

		void random_server(GameMode mode, uint32_t roomid, std::string& ipport) {
			game_serv_[mode].random_server(roomid, ipport);
		}

		void balance_server(GameMode mode, uint32_t roomid, std::string& ipport) {
			game_serv_[mode].balance_server(roomid, ipport);
		}
	}
}