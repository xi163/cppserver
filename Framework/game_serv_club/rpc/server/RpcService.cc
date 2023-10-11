#include "RpcService.h"

#include "../../interface/impl/TableMgr.h"
#include "../../interface/impl/Player.h"

#include "table.pb.h"

#include "../../Game.h"

extern GameServ* gServer;

namespace rpc {
	namespace server {
		void Service::GetNodeInfo(
			const ::Game::Rpc::NodeInfoReqPtr& req,
			const ::Game::Rpc::NodeInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::NodeInfoRsp rsp;
			rsp.set_numofloads(gServer->numUsers_.get());
			rsp.set_nodevalue(gServer->nodeValue_);
			rsp.set_tablecount(CTableMgr::get_mutable_instance().FreeCount());
			done(&rsp);
			//Warnf("\nrsp:%s", rsp.DebugString().c_str());
		}
		void Service::GetRoomInfo(const ::Game::Rpc::RoomInfoReqPtr& req,
			const ::Game::Rpc::RoomInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::RoomInfoRsp rsp;
			rsp.set_tablecount(gServer->roomInfo_.tableCount - (uint16_t)CTableMgr::get_mutable_instance().UsedCount());
			//tableInfos
			std::set<uint16_t> vec;
			CTableMgr::get_mutable_instance().Get(req->clubid(), vec);
			for (std::set<uint16_t>::iterator it = vec.begin(); it != vec.end(); ++it) {
				std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(*it);
				if (table) {
					::club::game::room::table::info* tableInfo = rsp.add_tables();
					tableInfo->set_tableid(table->GetTableId());
					tableInfo->set_gamestatus(table->GetGameStatus());
					tableInfo->set_gamestatusstr(table->GetGameStatusStr());
					//userInfos
					std::vector<std::shared_ptr<CPlayer>> items;
					table->GetPlayers(items);
					for (std::vector<std::shared_ptr<CPlayer>>::iterator it = items.begin(); it != items.end(); ++it) {
						std::shared_ptr<CPlayer> player = *it;
						::club::game::room::table::user::info* userInfo = tableInfo->add_users();
						userInfo->set_userid(player->GetUserId());
						userInfo->set_nickname(player->GetNickName());
						userInfo->set_chairid(player->GetChairId());
						userInfo->set_headerid(player->GetHeaderId());
						userInfo->set_headboxid(player->GetHeadboxId());
						userInfo->set_headimgurl(player->GetHeadImgUrl());
					}
				}
			}
			done(&rsp);
			//Warnf("\nrsp:%s", rsp.DebugString().c_str());
		}
		void Service::NotifyUserScore(const ::Game::Rpc::UserScoreReqPtr& req,
			const ::Game::Rpc::UserScoreRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::UserScoreRsp rsp;
			//muduo::net::TcpConnectionPtr peer(gServer->sessions_.get(req->userid()).lock());
			//if (peer) {
			//	//BufferPtr buffer = GateServ::packOrderScoreMsg(req->userid(), req->score());
			//	//muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
			//	//Warnf("succ %lld.score: %lld", req->userid(), req->score());
			//}
			//else {
			//	//Errorf("failed %lld.score: %lld", req->userid(), req->score());
			//}
			done(&rsp);
		}
		void Service::GetTableInfo(
			const ::Game::Rpc::TableInfoReqPtr& req,
			const ::Game::Rpc::TableInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::TableInfoRsp rsp;
			std::shared_ptr<CTable> table =  CTableMgr::get_mutable_instance().Get(req->tableid());
			if (table) {
				rsp.set_clubid(table->GetClubId());
			}
			done(&rsp);
			//Warnf("\nrsp:%s", rsp.DebugString().c_str());
		}
	}
}