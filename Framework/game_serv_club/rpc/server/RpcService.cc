#include "RpcService.h"

#include "TableMgr.h"
#include "Player.h"

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
			//_LOG_WARN("\nreq:%s\nrsp:%s", req->DebugString().c_str(), rsp.DebugString().c_str());
			done(&rsp);
		}
		void Service::GetRoomInfo(const ::Game::Rpc::RoomInfoReqPtr& req,
			const ::Game::Rpc::RoomInfoRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::RoomInfoRsp rsp;
			rsp.set_clubid(req->clubid());
			rsp.set_gameid(gServer->gameInfo_.gameId);
			rsp.set_roomid(gServer->roomInfo_.roomId);
			rsp.set_tablecount(gServer->roomInfo_.tableCount - (uint16_t)CTableMgr::get_mutable_instance().UsedCount());
			//tableInfos
			std::set<uint32_t> vec;
			CTableMgr::get_mutable_instance().Get(req->clubid(), vec);
			for (std::set<uint32_t>::iterator it = vec.begin(); it != vec.end(); ++it) {
				std::shared_ptr<CTable> table = CTableMgr::get_mutable_instance().Get(*it);
				if (table) {
					::Game::Rpc::TableInfo* tableInfo = rsp.add_tableinfos();
					tableInfo->set_servid(gServer->nodeValue_);
					tableInfo->set_tableid(table->GetTableId());
					tableInfo->set_gamestatus(table->GetGameStatus());
					//userInfos
					std::vector<std::shared_ptr<CPlayer>> items;
					table->GetPlayers(items);
					for (std::vector<std::shared_ptr<CPlayer>>::iterator it = items.begin(); it != items.end(); ++it) {
						std::shared_ptr<CPlayer> player = *it;
						::Game::Rpc::UserInfo* userInfo = tableInfo->add_userinfo();
						userInfo->set_userid(player->GetUserId());
						userInfo->set_nickname(player->GetNickName());
						userInfo->set_chairid(player->GetChairId());
						userInfo->set_headerid(player->GetHeaderId());
						userInfo->set_headboxid(player->GetHeadboxId());
						userInfo->set_headimgurl(player->GetHeadImgUrl());
					}
				}
			}
		}
		void Service::NotifyUserScore(const ::Game::Rpc::UserScoreReqPtr& req,
			const ::Game::Rpc::UserScoreRsp* responsePrototype,
			const muduo::net::RpcDoneCallback& done) {
			::Game::Rpc::UserScoreRsp rsp;
			//muduo::net::TcpConnectionPtr peer(gServer->sessions_.get(req->userid()).lock());
			//if (peer) {
			//	//BufferPtr buffer = GateServ::packOrderScoreMsg(req->userid(), req->score());
			//	//muduo::net::websocket::send(peer, buffer->peek(), buffer->readableBytes());
			//	//_LOG_WARN("succ %lld.score: %lld", req->userid(), req->score());
			//}
			//else {
			//	//_LOG_ERROR("failed %lld.score: %lld", req->userid(), req->score());
			//}
			done(&rsp);
		}
	}
}