#include "RoomNodes.h"
#include "public/gameConst.h"
#include "Logger/src/log/Logger.h"
#include "RpcService_c.h"

#include "../../Hall.h"

extern HallServ* gServer;

namespace room {
	namespace nodes {

		static class RoomList {
			typedef std::set<std::string> Nodes;
			typedef std::map<int, Nodes> RoomNodes;
			typedef std::map<int, RoomNodes> GameRoomNodes;
		public:
			void add(std::string const& name);
			void remove(std::string const& name);
		public:
			void random_server(uint32_t gameId, uint32_t roomId, std::string& ipport);
			void balance_server(uint32_t gameId, uint32_t roomId, std::string& ipport);
		public:
			void get_room_info(int64_t clubId);
			void get_room_info(int64_t clubId, uint32_t gameId);
			void get_room_info(int64_t clubId, uint32_t gameId, uint32_t roomId);
		private:
			GameRoomNodes nodes_;
			mutable boost::shared_mutex mutex_;
		}game_serv_[kClub + 1];

		void RoomList::add(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			RoomNodes& roomNodes = nodes_[stoi(vec[0])];
			Nodes& nodes = roomNodes[stoi(vec[1])];
			nodes.insert(name);
		}

		void RoomList::remove(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			GameRoomNodes::iterator it = nodes_.find(stoi(vec[0]));
			if (it != nodes_.end()) {
				RoomNodes::iterator ir = it->second.find(stoi(vec[1]));
				if (ir != it->second.end()) {
					Nodes::iterator ix = ir->second.find(name);
					if (ix != ir->second.end()) {
						ir->second.erase(ix);
						if (ir->second.empty()) {
							it->second.erase(ir);
							if (it->second.empty()) {
								nodes_.erase(it);
							}
						}
					}
				}
			}
		}

		void RoomList::random_server(uint32_t gameId, uint32_t roomId, std::string& ipport) {
			ipport.clear();
			READ_LOCK(mutex_);
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				RoomNodes::const_iterator ir = it->second.find(roomId);
				if (ir != it->second.end()) {
					if (!ir->second.empty()) {
						std::vector<std::string> rooms;
						std::copy(ir->second.begin(), ir->second.end(), std::back_inserter(rooms));
						int index = RANDOM().betweenInt(0, rooms.size() - 1).randInt_mt();
						ipport = rooms[index];
					}
				}
			}
		}
		
		void RoomList::balance_server(uint32_t gameId, uint32_t roomId, std::string& ipport) {
			ipport.clear();
			READ_LOCK(mutex_);
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				RoomNodes::const_iterator ir = it->second.find(roomId);
				if (ir != it->second.end()) {
					if (!ir->second.empty()) {
						std::vector<std::pair<std::string, int>> rooms;
						Nodes const& ref = ir->second;
						for (Nodes::const_iterator it = ref.begin(); it != ref.end(); ++it) {
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
						int i = RANDOM().betweenInt(0, rooms.size() - 1).randInt_mt();
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
		}
		
		void RoomList::get_room_info(int64_t clubId) {
			for (GameRoomNodes::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
				for (RoomNodes::const_iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, it->first, ir->first, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							_LOG_DEBUG("%d --- %s ---\n%s", clubId, ix->c_str(), rsp->DebugString().c_str());
						}
					}
				}
			}
		}
		
		void RoomList::get_room_info(int64_t clubId, uint32_t gameId) {
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				for (RoomNodes::const_iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, gameId, ir->first, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							_LOG_DEBUG("%d:%d --- %s ---\n%s", clubId, gameId, ix->c_str(), rsp->DebugString().c_str());
						}
					}
				}
			}
		}
		
		void RoomList::get_room_info(int64_t clubId, uint32_t gameId, uint32_t roomId) {
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				RoomNodes::const_iterator ir = it->second.find(roomId);
				if (ir != it->second.end()) {
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, gameId, roomId, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							_LOG_DEBUG("%d:%d:%d --- %s ---\n%s", clubId, gameId, roomId, ix->c_str(), rsp->DebugString().c_str());
						}
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

		void random_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport) {
			game_serv_[mode].random_server(gameId, roomId, ipport);
		}

		void balance_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport) {
			game_serv_[mode].balance_server(gameId, roomId, ipport);
		}
		
		void get_room_info(GameMode mode, int64_t clubId) {
			game_serv_[mode].get_room_info(clubId);
		}
		
		void get_room_info(GameMode mode, int64_t clubId, uint32_t gameId) {
			game_serv_[mode].get_room_info(clubId, gameId);
		}
		
		void get_room_info(GameMode mode, int64_t clubId, uint32_t gameId, uint32_t roomId) {
			game_serv_[mode].get_room_info(clubId, gameId, roomId);
		}
	}
}