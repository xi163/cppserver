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
			void random_server(uint32_t gameId, uint32_t roomId, std::string& ipport, GameMode mode);
			void balance_server(uint32_t gameId, uint32_t roomId, std::string& ipport, GameMode mode);
			bool validate_server(uint32_t gameId, uint32_t roomId, std::string& ipport, std::string const& servId, uint16_t tableId, int64_t clubId);
		public:
			void get_club_room_info(int64_t clubId, ::club::info& info, std::set<uint32_t> const& exclude);
			void get_club_room_info(int64_t clubId, uint32_t gameId, ::club::game::info& info, std::set<uint32_t> const& exclude);
			void get_club_room_info(int64_t clubId, uint32_t gameId, uint32_t roomId, ::club::game::room::info& info, std::set<uint32_t> const& exclude);
		private:
			GameRoomNodes nodes_;
			mutable boost::shared_mutex mutex_;
		}game_serv_[kClub + 1];

		void RoomList::add(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			RoomNodes& roomNodes = nodes_[std::stoi(_gameid(vec))];
			Nodes& nodes = roomNodes[std::stoi(_roomid(vec))];
			nodes.insert(name);
		}

		void RoomList::remove(std::string const& name) {
			std::vector<std::string> vec;
			boost::algorithm::split(vec, name, boost::is_any_of(":"));
			WRITE_LOCK(mutex_);
			GameRoomNodes::iterator it = nodes_.find(std::stoi(_gameid(vec)));
			if (it != nodes_.end()) {
				RoomNodes::iterator ir = it->second.find(std::stoi(_roomid(vec)));
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

		void RoomList::random_server(uint32_t gameId, uint32_t roomId, std::string& ipport, GameMode mode) {
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

		void RoomList::balance_server(uint32_t gameId, uint32_t roomId, std::string& ipport, GameMode mode) {
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
								switch (mode) {
								case kMatch:
								case kFriend:
									rooms.emplace_back(std::make_pair(rsp->nodevalue(), rsp->numofloads()));
									break;
								case kClub:
									if (rsp->tablecount() > 0) {
										rooms.emplace_back(std::make_pair(rsp->nodevalue(), rsp->numofloads()));
									}
									break;
								}
							}
							else {
								_LOG_ERROR("%s", it->c_str());
							}
						}
						int i = rooms.size() > 0 ? RANDOM().betweenInt(0, rooms.size() - 1).randInt_mt() : 0;
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

		bool RoomList::validate_server(uint32_t gameId, uint32_t roomId, std::string& ipport, std::string const& servId, uint16_t tableId, int64_t clubId) {
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				RoomNodes::const_iterator ir = it->second.find(roomId);
				if (ir != it->second.end()) {
#if 0
					Nodes::const_iterator ix = ir->second.find(servId);
#else
					Nodes::const_iterator ix = std::find_if(ir->second.begin(), ir->second.end(), [&](std::string const& val) -> bool {
						return strncasecmp(val.c_str(), servId.c_str(), std::min(val.size(), servId.size())) == 0;
						});
#endif
					if (ix != ir->second.end()) {
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::TableInfoReq req;
						req.set_tableid(tableId);
						::Game::Rpc::TableInfoRspPtr rsp = client.GetTableInfo(req);
						if (rsp) {
							if (rsp->clubid() > 0 && rsp->clubid() == clubId) {
								ipport = *ix;
								return true;
							}
						}
					}
				}
			}
			return false;
		}

		void RoomList::get_club_room_info(int64_t clubId, ::club::info& info, std::set<uint32_t> const& exclude) {
			for (GameRoomNodes::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
				if (!exclude.empty() && exclude.find(it->first) != exclude.end()) {
					continue;
				}
				bool new_gameid = true;
				for (RoomNodes::const_iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
					bool new_roomid = true;
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						//_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, it->first, ir->first, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							//gameid
							if (new_gameid) {
								new_gameid = false;
								info.add_games()->set_gameid(it->first);
							}
							::club::game::info* gameinfo = info.mutable_games(info.games_size() - 1);
							//roomid
							if (new_roomid) {
								new_roomid = false;
								gameinfo->add_rooms()->set_roomid(ir->first);
							}
							::club::game::room::info* roominfo = gameinfo->mutable_rooms(gameinfo->rooms_size() - 1);
							roominfo->set_tablecount(roominfo->tablecount() + rsp->tablecount());//tablecount
							//tableinfos
							std::vector<std::string> vec;
							for (int i = 0; i < rsp->tables_size(); ++i) {
								::club::game::room::table::info* tableinfo = roominfo->add_tables();
								tableinfo->CopyFrom(*rsp->mutable_tables(i));
								if (tableinfo->users_size() > 0) {
#if 0
									tableinfo->set_nodeid(*ix);//nodevalue
#else
									if (vec.empty()) {
										boost::algorithm::split(vec, *ix, boost::is_any_of(":"));
									}
									tableinfo->set_nodeid(_game_servid(vec));
#endif
								}
							}
							//_LOG_DEBUG("--- %d %d %d %s ---\n%s", clubId, it->first, ir->first, ix->c_str(), info.DebugString().c_str());
						}
					}
				}
			}
			if (info.games_size() > 0) {
				info.set_clubid(clubId);
			}
			_LOG_DEBUG("\n%s", info.DebugString().c_str());
		}

		void RoomList::get_club_room_info(int64_t clubId, uint32_t gameId, ::club::game::info& info, std::set<uint32_t> const& exclude) {
			if (!exclude.empty() && exclude.find(gameId) != exclude.end()) {
				return;
			}
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				for (RoomNodes::const_iterator ir = it->second.begin(); ir != it->second.end(); ++ir) {
					bool new_roomid = true;
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						//_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, gameId, ir->first, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							//gameid
							//info.set_gameid(gameId);
							//roomid
							if (new_roomid) {
								new_roomid = false;
								info.add_rooms()->set_roomid(ir->first);
							}
							::club::game::room::info* roominfo = info.mutable_rooms(info.rooms_size() - 1);
							roominfo->set_tablecount(roominfo->tablecount() + rsp->tablecount());//tablecount
							//tableinfos
							std::vector<std::string> vec;
							for (int i = 0; i < rsp->tables_size(); ++i) {
								::club::game::room::table::info* tableinfo = roominfo->add_tables();
								tableinfo->CopyFrom(*rsp->mutable_tables(i));
								if (tableinfo->users_size() > 0) {
#if 0
									tableinfo->set_nodeid(*ix);//nodevalue
#else
									if (vec.empty()) {
										boost::algorithm::split(vec, *ix, boost::is_any_of(":"));
									}
									tableinfo->set_nodeid(_game_servid(vec));
#endif
								}
							}
							//_LOG_DEBUG("--- %d %d %d %s ---\n%s", clubId, it->first, ir->first, ix->c_str(), info.DebugString().c_str());
						}
					}
				}
			}
			if (info.rooms_size() > 0) {
				info.set_gameid(gameId);
			}
			_LOG_DEBUG("\n%s", info.DebugString().c_str());
		}

		void RoomList::get_club_room_info(int64_t clubId, uint32_t gameId, uint32_t roomId, ::club::game::room::info& info, std::set<uint32_t> const& exclude) {
			if (!exclude.empty() && exclude.find(gameId) != exclude.end()) {
				return;
			}
			GameRoomNodes::const_iterator it = nodes_.find(gameId);
			if (it != nodes_.end()) {
				RoomNodes::const_iterator ir = it->second.find(roomId);
				if (ir != it->second.end()) {
					for (Nodes::const_iterator ix = ir->second.begin(); ix != ir->second.end(); ++ix) {
						//_LOG_WARN("clubId:%d gameId:%d roomId:%d %s", clubId, gameId, roomId, ix->c_str());
						rpc::ClientConn conn;
						gServer->rpcClients_[rpc::containTy::kRpcGameTy].clients_->get(*ix, conn);
						rpc::client::Service client(conn, 3);
						::Game::Rpc::RoomInfoReq req;
						req.set_clubid(clubId);
						::Game::Rpc::RoomInfoRspPtr rsp = client.GetRoomInfo(req);
						if (rsp) {
							//roomid
							info.set_roomid(roomId);
							info.set_tablecount(info.tablecount() + rsp->tablecount());//tablecount
							//tableinfos
							std::vector<std::string> vec;
							for (int i = 0; i < rsp->tables_size(); ++i) {
								::club::game::room::table::info* tableinfo = info.add_tables();
								tableinfo->CopyFrom(*rsp->mutable_tables(i));
								if (tableinfo->users_size() > 0) {
#if 0
									tableinfo->set_nodeid(*ix);//nodevalue
#else
									if (vec.empty()) {
										boost::algorithm::split(vec, *ix, boost::is_any_of(":"));
									}
									tableinfo->set_nodeid(_game_servid(vec));
#endif
								}
							}
							//_LOG_DEBUG("--- %d %d %d %s ---\n%s", clubId, it->first, ir->first, ix->c_str(), info.DebugString().c_str());
						}
					}
				}
			}
			_LOG_DEBUG("\n%s", info.DebugString().c_str());
		}

		void add(GameMode mode, std::string const& name) {
			game_serv_[mode].add(name);
		}

		void remove(GameMode mode, std::string const& name) {
			game_serv_[mode].remove(name);
		}

		void random_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport) {
			game_serv_[mode].random_server(gameId, roomId, ipport, mode);
		}

		void balance_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport) {
			game_serv_[mode].balance_server(gameId, roomId, ipport, mode);
		}

		bool validate_server(GameMode mode, uint32_t gameId, uint32_t roomId, std::string& ipport, std::string const& servId, uint16_t tableId, int64_t clubId) {
			return game_serv_[mode].validate_server(gameId, roomId, ipport, servId, tableId, clubId);
		}

		void get_club_room_info(GameMode mode, int64_t clubId, ::club::info& info, std::set<uint32_t> const& exclude) {
			game_serv_[mode].get_club_room_info(clubId, info, exclude);
		}

		void get_club_room_info(GameMode mode, int64_t clubId, uint32_t gameId, ::club::game::info& info, std::set<uint32_t> const& exclude) {
			game_serv_[mode].get_club_room_info(clubId, gameId, info, exclude);
		}

		void get_club_room_info(GameMode mode, int64_t clubId, uint32_t gameId, uint32_t roomId, ::club::game::room::info& info, std::set<uint32_t> const& exclude) {
			game_serv_[mode].get_club_room_info(clubId, gameId, roomId, info, exclude);
		}
	}
}