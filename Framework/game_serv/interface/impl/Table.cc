
#include "Table.h"

#include "RobotMgr.h"
#include "PlayerMgr.h"
#include "TableMgr.h"

#include "proto/GameServer.Message.pb.h"
#include "proto/Game.Common.pb.h"

atomic_llong  CTable::curStorage_(0);
double CTable::lowStorage_(0);
double CTable::highStorage_(0);
double CTable::secondLowStorage_(0);
double CTable::secondHighStorage_(0);

int CTable::sysKillAllRatio_(0);
int CTable::sysReduceRatio_(0);
int CTable::sysChangeCardRatio_(0);


CTable::CTable()
    : tableDelegate_(NULL)
    , gameInfo_(NULL)
    , roomInfo_(NULL)
    , tableContext_(NULL)
    , logicThread_(NULL)
    , status_(GAME_STATUS_INIT) {
    items_.clear();
    memset(&tableState_, 0, sizeof(tableState_));
}

CTable::~CTable() {
    items_.clear();
}

void CTable::Reset() {
    if (tableDelegate_) {
        tableDelegate_->Reposition();
    }
    status_ = GAME_STATUS_INIT;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        items_[i] = std::shared_ptr<CPlayer>();
    }
}

void CTable::Init(std::shared_ptr<ITableDelegate>& tableDelegate,
    TableState& tableState,
    tagGameInfo* gameInfo, tagGameRoomInfo* roomInfo,
    std::shared_ptr<muduo::net::EventLoopThread>& logicThread, ITableContext* tableContext) {
    gameInfo_ = gameInfo;
    roomInfo_ = roomInfo;
    logicThread_ = logicThread;
    tableContext_ = tableContext;
    tableState_ = tableState;
    items_.resize(roomInfo_->maxPlayerNum);
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        items_[i] = std::shared_ptr<CPlayer>();
    }
    tableDelegate_ = tableDelegate;
    tableDelegate_->SetTable(shared_from_this());
}

std::string CTable::NewRoundId() {
    //roomid-timestamp-pid-tableid-rand
    std::string strRoundId = std::to_string(roomInfo_->roomId) + "-";
    int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    strRoundId += std::to_string(seconds) + "-";
    strRoundId += std::to_string(::getpid()) + "-";
    strRoundId += std::to_string(GetTableId()) + "-";
    strRoundId += std::to_string(rand() % 10);
    return strRoundId;
}

std::string CTable::GetRoundId() {
	return tableDelegate_->GetRoundId();
}

bool CTable::send(
    std::shared_ptr<IPlayer> const& player,
    uint8_t const* msg, size_t len,
    uint8_t mainId,
    uint8_t subId, bool v, int flag) {
    if (player && player->Valid()) {
        if (!player->IsRobot()) {
            boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> tuple = tableContext_->GetContext(player->GetUserId());
            muduo::net::WeakTcpConnectionPtr weakConn = tuple.get<0>();
            std::shared_ptr<packet::internal_prev_header_t> pre_header = tuple.get<1>();
            std::shared_ptr<packet::header_t> header = tuple.get<2>();
            muduo::net::TcpConnectionPtr conn(weakConn.lock());
            if (conn) {
                switch (v) {
				case true: {
					int old = const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok;
					const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = flag;
					tableContext_->send(conn, msg, len, mainId, subId, pre_header.get(), header.get());
					const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = old;
                    break;
				}
                default:
                    tableContext_->send(conn, msg, len, mainId, subId, pre_header.get(), header.get());
                    break;
                }
                return true;
            }
        }
        else {
            return player->SendUserMessage(mainId, subId, msg, len);
        }
    }
    return false;
}

bool CTable::send(
    std::shared_ptr<IPlayer> const& player,
    uint8_t const* msg, size_t len,
    uint8_t subId, bool v, int flag) {
    if (player && player->Valid()) {
        if (!player->IsRobot()) {
            boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> tuple = tableContext_->GetContext(player->GetUserId());
            muduo::net::WeakTcpConnectionPtr weakConn = tuple.get<0>();
            std::shared_ptr<packet::internal_prev_header_t> pre_header = tuple.get<1>();
            std::shared_ptr<packet::header_t> header = tuple.get<2>();
            muduo::net::TcpConnectionPtr conn(weakConn.lock());
            if (conn) {
                switch (v) {
                case true: {
					int old = const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok;
					const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = flag;
					tableContext_->send(conn, msg, len, subId, pre_header.get(), header.get());
					const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = old;
                    break;
                }
                default:
                    tableContext_->send(conn, msg, len, subId, pre_header.get(), header.get());
                    break;   
                }
                return true;
            }
        }
        else {
            return player->SendUserMessage(Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC, subId, msg, len);
        }
    }
    return false;
}

bool CTable::send(
    std::shared_ptr<IPlayer> const& player,
    ::google::protobuf::Message* msg,
    uint8_t mainId,
    uint8_t subId, bool v, int flag) {
    if (player && player->Valid()) {
        if (!player->IsRobot()) {
            boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> tuple = tableContext_->GetContext(player->GetUserId());
            muduo::net::WeakTcpConnectionPtr weakConn = tuple.get<0>();
            std::shared_ptr<packet::internal_prev_header_t> pre_header = tuple.get<1>();
            std::shared_ptr<packet::header_t> header = tuple.get<2>();
            muduo::net::TcpConnectionPtr conn(weakConn.lock());
            if (conn) {
                switch (v) {
                case true: {
                    int old = const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok;
                    const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = flag;
                    tableContext_->send(conn, msg, mainId, subId, pre_header.get(), header.get());
                    const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = old;
                    break;
                }
                default:
                    tableContext_->send(conn, msg, mainId, subId, pre_header.get(), header.get());
                    break;
                }
                return true;
            }
        }
        else {
            std::string data = msg->SerializeAsString();
            return player->SendUserMessage(mainId, subId, (uint8_t const*)data.data(), data.size());
        }
    }
    return false;
}

bool CTable::send(
    std::shared_ptr<IPlayer> const& player,
    ::google::protobuf::Message* msg,
    uint8_t subId, bool v, int flag) {
    if (player && player->Valid()) {
        if (!player->IsRobot()) {
            boost::tuple<muduo::net::WeakTcpConnectionPtr, std::shared_ptr<packet::internal_prev_header_t>, std::shared_ptr<packet::header_t>> tuple = tableContext_->GetContext(player->GetUserId());
            muduo::net::WeakTcpConnectionPtr weakConn = tuple.get<0>();
            std::shared_ptr<packet::internal_prev_header_t> pre_header = tuple.get<1>();
            std::shared_ptr<packet::header_t> header = tuple.get<2>();
            muduo::net::TcpConnectionPtr conn(weakConn.lock());
            if (conn) {
                switch (v) {
                case true: {
                    int old = const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok;
                    const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = flag;
                    tableContext_->send(conn, msg, subId, pre_header.get(), header.get());
                    const_cast<packet::internal_prev_header_t*>(pre_header.get())->ok = old;
                    break;
                }
                default:
                    tableContext_->send(conn, msg, subId, pre_header.get(), header.get());
                    break;
                }
                return true;
            }
        }
        else {
            std::string data = msg->SerializeAsString();
            return player->SendUserMessage(Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC, subId, (uint8_t const*)data.data(), data.size());
        }
    }
    return false;
}

bool CTable::SendTableData(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len) {
    if (INVALID_CHAIR == chairId) {
        for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
            std::shared_ptr<IPlayer> player = items_[i];
            if (player && player->Valid()) {
                if (!SendUserData(player, subId, data, len)) {
                    return false;
                }
            }
        }
        return true;
    }
    else {
        if (chairId >= roomInfo_->maxPlayerNum) {
            return false;
        }
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            return SendUserData(player, subId, data, len);
        }
        return false;
    }
}

bool CTable::SendTableData(uint32_t chairId, uint8_t subId, ::google::protobuf::Message* msg) {
    if (INVALID_CHAIR == chairId) {
        for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
            std::shared_ptr<IPlayer> player = items_[i];
            if (player && player->Valid()) {
                std::string data = msg->SerializeAsString();
                if (!SendUserData(player, subId, (uint8_t const*)data.data(), data.size())) {
                    return false;
                }
            }
        }
        return true;
    }
    else {
        if (chairId >= roomInfo_->maxPlayerNum) {
            return false;
        }
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            std::string data = msg->SerializeAsString();
            return SendUserData(player, subId, (uint8_t const*)data.data(), data.size());
        }
        return false;
    }
}

bool CTable::SendUserData(std::shared_ptr<IPlayer> const& player, uint8_t subId, uint8_t const* data, size_t len) {
    uint8_t mainId = Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC;
    if (!player->IsRobot()) {
        ::GameServer::MSG_CSC_Passageway pass;
        pass.mutable_header()->set_sign(PROTO_BUF_SIGN);
        pass.set_passdata(data, len);
        return send(player, &pass, mainId, subId);
    }
    else {
        //IRobotDelegate消息回调
        return player->SendUserMessage(mainId, subId, data, len);
    }
}

bool CTable::SendGameMessage(uint32_t chairId, std::string const& msg, uint8_t msgType, int64_t score) {
    uint8_t mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC;
    uint8_t subId = ::GameServer::SUB_GF_SYSTEM_MESSAGE;
    if ((msgType & SMT_GLOBAL) && (msgType & SMT_SCROLL)) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            UserBaseInfo userBaseItem = std::dynamic_pointer_cast<CPlayer>(player)->GetUserBaseInfo();
            // Json::Value root;
            // root["gameId"] = roomInfo_->gameId;
            // root["nickName"] = player->GetNickName();
            // root["userId"] = player->GetUserId();
            // root["agentId"] = userBaseItem.agentId;
            // root["roomName"] = roomInfo_->roomName;
            // root["roomId"] = roomInfo_->roomId;
            // root["msg"]=szMessage;
            // root["msgId"] = rand()%7;//szMessage;
            // root["score"] = (uint32_t)score;
            // Json::FastWriter writer;
            // string msg = writer.write(root);
            // //REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_marquee,msg);
            // // REDISCLIENT.publishUserKillBossMessage(msg);
            return true;
        }
        return false;
    }
    ::GameServer::MSG_S2C_SystemMessage msgInfo;
    ::Game::Common::Header* header = msgInfo.mutable_header();
    header->set_sign(HEADER_SIGN);
    msgInfo.set_msgtype(msgType);
    msgInfo.set_msgcontent(msg);
    if (INVALID_CHAIR == chairId) {
        for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
            std::shared_ptr<IPlayer>  player = items_[i];
            if (player && player->Valid()) {
                return send(player, &msgInfo, mainId, subId);
            }
        }
    }
    else {
        if (chairId >= roomInfo_->maxPlayerNum) {
            return false;
        }
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            return send(player, &msgInfo, mainId, subId);
        }
    }
    return false;
}

void CTable::ClearTableUser(uint32_t chairId, bool sendState, bool sendToSelf, uint8_t sendErrCode) {
    if (INVALID_CHAIR == chairId) {
        for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
            std::shared_ptr<IPlayer> player = items_[i];
            if (player && player->Valid()) {
                int64_t userId = player->GetUserId();
                switch (sendErrCode) {
                case 0:
                default:
					::GameServer::MSG_S2C_UserEnterMessageResponse msg;
					msg.mutable_header()->set_sign(HEADER_SIGN);
					msg.set_retcode(sendErrCode);
					msg.set_errormsg("");
					send(player, &msg,
						Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
						GameServer::SUB_S2C_ENTER_ROOM_RES, true, -1);
                }
                if (!OnUserStandup(player, sendState, sendToSelf)) {
                    _LOG_ERROR("%s %d %d err, sPlaying!", (player->IsRobot() ? "<robot>" : "<real>"), chairId, userId);
                }
                else {
                    _LOG_ERROR("%s %d %d ok", (player->IsRobot() ? "<robot>" : "<real>"), chairId, userId);
                }
            }
        }
    }
    else {
        if (chairId >= roomInfo_->maxPlayerNum) {
            return;
        }
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            int64_t userId = player->GetUserId();
			switch (sendErrCode) {
			case 0:
			default:
				::GameServer::MSG_S2C_UserEnterMessageResponse msg;
				msg.mutable_header()->set_sign(HEADER_SIGN);
				msg.set_retcode(sendErrCode);
				msg.set_errormsg("");
				send(player, &msg,
					Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
					GameServer::SUB_S2C_ENTER_ROOM_RES, true, -1);
			}
            if (!OnUserStandup(player, sendState, sendToSelf)) {
                _LOG_ERROR("%s %d %d err, sPlaying!", (player->IsRobot() ? "<robot>" : "<real>"), chairId, userId);
            }
            else {
                _LOG_ERROR("%s %d %d ok", (player->IsRobot() ? "<robot>" : "<real>"), chairId, userId);
            }
        }
    }
    if (GetPlayerCount() == 0) {
        if (gameInfo_->gameType == GameType_Confrontation) {
            CTableMgr::get_mutable_instance().Delete(tableState_.tableId);
        }
    }
}

void CTable::GetTableInfo(TableState& tableState) {
    tableState = tableState_;
}

uint32_t CTable::GetTableId() {
    return tableState_.tableId;
}

std::shared_ptr<muduo::net::EventLoopThread> CTable::GetLoopThread() {
    return logicThread_;
}

void CTable::assertThisThread() {
    logicThread_->getLoop()->assertInLoopThread();
}

bool CTable::DismissGame() {
    return tableDelegate_->OnGameConclude(INVALID_CHAIR, GER_DISMISS);
}

bool CTable::ConcludeGame(uint8_t gameStatus) {
    status_ = gameStatus;
    tableDelegate_->Reposition();
    return true;
}

int64_t CTable::CalculateRevenue(int64_t score) {
    return score * gameInfo_->revenueRatio / 100;
}

std::shared_ptr<IPlayer> CTable::GetChairPlayer(uint32_t chairId) {
    if (chairId < roomInfo_->maxPlayerNum) {
        return items_[chairId];
    }
    return std::shared_ptr<IPlayer>();
}

std::shared_ptr<IPlayer> CTable::GetPlayer(int64_t userId) {
    //     for (std::vector<std::shared_ptr<IPlayer>>::iterator it = items_.begin();
    //         it != items_.end(); ++it) {
    //         if (*it && (*it)->GetUserId() == userId) {
    //             return *it;
    //         }
    //     }
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player && player->GetUserId() == userId) {
            return player;
        }
    }
    return  std::shared_ptr<IPlayer>();
}

bool CTable::ExistUser(uint32_t chairId) {
    if (chairId < 0 || chairId == INVALID_CHAIR || chairId >= roomInfo_->maxPlayerNum) {
        return false;
    }
    std::shared_ptr<IPlayer> player = items_[chairId];
    if (player && player->Valid()) {
        return true;
    }
    return false;
}

void CTable::SetGameStatus(uint8_t status) {
    status_ = status;
}

uint8_t CTable::GetGameStatus() {
    return status_;
}

std::string CTable::StrGameStatus() {
    switch (status_) {
    case GAME_STATUS_INIT:
        return "INIT";
    case GAME_STATUS_FREE:
        return "READY";
    case GAME_STATUS_END:
        return "END";
    }
    if (status_ >= GAME_STATUS_START && status_ < GAME_STATUS_END) {
		return "START";
	}
	return "";
}

bool CTable::CanJoinTable(std::shared_ptr<IPlayer> const& player) {
    if (roomInfo_->serverStatus == SERVER_STOPPED || roomInfo_->serverStatus == SERVER_REPAIRING) {
        return false;
    }
    return tableDelegate_->CanJoinTable(player);
}

bool CTable::CanLeftTable(int64_t userId) {
    return tableDelegate_->CanLeftTable(userId);
}

void CTable::SetUserTrustee(uint32_t chairId, bool trustee) {
    if (chairId < roomInfo_->maxPlayerNum) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            player->setTrustee(trustee);
        }
    }
}

bool CTable::GetUserTrustee(uint32_t chairId) {
    bool trustee = false;
    if (chairId < roomInfo_->maxPlayerNum) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            trustee = player->getTrustee();
        }
    }
    return trustee;
}

void CTable::SetUserReady(uint32_t chairId) {
    if (chairId < roomInfo_->maxPlayerNum) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player && player->Valid()) {
            player->SetUserStatus(sReady);
            BroadcastUserStatus(player, true);
        }
    }
}

//点击离开按钮
bool CTable::OnUserLeft(std::shared_ptr<IPlayer> const& player, bool sendToSelf) {
    if (!player->IsRobot()) {
        _LOG_INFO("%d 点击离开按钮", player->GetUserId());
    }
    //	if (status_ >= GAME_STATUS_START && status_ < GAME_STATUS_END) {
            //tableContext_->DelContext(player->GetUserId());
    player->setOffline();
    player->setTrustee(true);
    //BroadcastUserStatus(player, sendToSelf);
    return tableDelegate_->OnUserLeft(player->GetUserId(), false);
    //	}
    // 	if (tableDelegate_->OnUserLeft(player->GetUserId(), false)) {
    // 		OnUserStandup(player, true, sendToSelf);
    // 		return true;
    // 	}
    // 	return false;
}

//关闭页面
bool CTable::OnUserOffline(std::shared_ptr<IPlayer> const& player) {
    if (!player->IsRobot()) {
        _LOG_INFO("%d 关闭页面", player->GetUserId());
    }
    //tableContext_->DelContext(player->GetUserId());
    player->setOffline();
    player->setTrustee(true);
    //BroadcastUserStatus(player, false);
    return tableDelegate_->OnUserLeft(player->GetUserId(), false);
}

uint32_t CTable::GetPlayerCount() {
    uint32_t count = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player) {
            ++count;
        }
    }
    return count;
}

uint32_t CTable::GetPlayerCount(bool includeRobot) {
    uint32_t count = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player) {
            if (includeRobot && player->IsRobot()) {
                ++count;
            }
            else {
                ++count;
            }
        }
    }
    return count;
}

uint32_t CTable::GetRobotPlayerCount() {
    uint32_t count = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player && player->IsRobot()) {
            ++count;
        }
    }
    return count;
}

uint32_t CTable::GetRealPlayerCount() {
    uint32_t count = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player && !player->IsRobot()) {
            ++count;
        }
    }
    return count;
}

void CTable::GetPlayerCount(uint32_t& realCount, uint32_t& robotCount) {
    realCount = 0;
    robotCount = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player) {
            if (player->IsRobot()) {
                ++robotCount;
            }
            else {
                ++realCount;
            }
        }
    }
}

uint32_t CTable::GetMaxPlayerCount() {
    return roomInfo_->maxPlayerNum;
}

tagGameRoomInfo* CTable::GetRoomInfo() {
    return roomInfo_;
}

bool CTable::IsRobot(uint32_t chairId) {
    if (chairId < roomInfo_->maxPlayerNum) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player) {
            return player->IsRobot();
        }
    }
    return false;
}

bool CTable::IsOfficial(uint32_t chairId) {
    if (chairId < roomInfo_->maxPlayerNum) {
        std::shared_ptr<IPlayer> player = items_[chairId];
        if (player) {
            return player->IsOfficial();
        }
    }
    return false;
}

bool CTable::OnGameEvent(uint32_t chairId, uint8_t subId, uint8_t const* data, size_t len) {
    return tableDelegate_->OnGameMessage(chairId, subId, data, len);
}

void CTable::OnStartGame() {
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (player && player->Valid()) {
            if (player->isSit() ||
                player->isReady() ||
                player->isPlaying()) {
                player->SetUserStatus(sPlaying);
                BroadcastUserStatus(player);
            }
        }
    }
    tableDelegate_->OnGameStart();
}

bool CTable::CheckGameStart() {
    int playerCount = 0;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> player = items_[i];
        if (!player) {
            continue;
        }
        ++playerCount;
        if (!player->isReady()) {
            return false;
        }
    }
    // 	if (playerCount >= gameInfo_->MIN_GAME_PLAYER) {
    // 		return true;
    // 	}
    return false;
}

bool CTable::OnUserEnterAction(std::shared_ptr<IPlayer> const& player, packet::internal_prev_header_t const* pre_header, packet::header_t const* header/*, bool bDistribute*/) {
    SendUserSitdownFinish(player, pre_header, header);
    //BroadcastUserInfoToOther(player);
    //SendAllOtherUserInfoToUser(player);
    //BroadcastUserStatus(player, true);
    return tableDelegate_->OnUserEnter(player->GetUserId(), player->lookon());
}

void CTable::SendUserSitdownFinish(std::shared_ptr<IPlayer> const& player, packet::internal_prev_header_t const* pre_header, packet::header_t const* header/*, bool bDistribute*/) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_ENTER_ROOM_RES;
    GameServer::MSG_S2C_UserEnterMessageResponse rspdata;
    rspdata.mutable_header()->set_sign(HEADER_SIGN);
    rspdata.set_retcode(0);
    rspdata.set_errormsg("USER ENTER ROOM OK.");
    send(player, &rspdata, mainId, subId);
}

bool CTable::OnUserStandup(std::shared_ptr<IPlayer> const& player, bool sendState, bool sendToSelf) {
    assert(player && player->Valid() && player->GetTableId() >= 0 && player->GetChairId() >= 0);
    //游戏中禁止起立
    if (!player->isPlaying()) {
        int64_t userId = player->GetUserId();
        uint32_t chairId = player->GetChairId();
        //assert(player->GetTableId() == GetTableId());
        //assert(player.get() == GetChairPlayer(chairId).get());
        if (player->IsRobot()) {
            _LOG_WARN("<robot> %d %d", chairId, userId);
            //清理机器人数据
            CRobotMgr::get_mutable_instance().Delete(userId);
        }
        else {
            if (roomInfo_->serverStatus != SERVER_RUNNING && gameInfo_->gameType == GameType_BaiRen) {
                uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
                uint8_t subId = GameServer::SUB_S2C_USER_LEFT_RES;
                ::GameServer::MSG_C2S_UserLeftMessageResponse response;
                ::Game::Common::Header* resp_header = response.mutable_header();
                resp_header->set_sign(HEADER_SIGN);
                response.set_gameid(gameInfo_->gameId);
                response.set_roomid(roomInfo_->roomId);
                response.set_type(0);
                response.set_retcode(ERROR_ENTERROOM_SERVERSTOP);
                response.set_errormsg("游戏维护请进入其他房间");
                send(player, &response, mainId, subId);
            }
            _LOG_WARN("<real> %d %d", chairId, userId);
            //清理真人数据
            tableContext_->DelContext(userId);
            DelUserOnlineInfo(userId);
            CPlayerMgr::get_mutable_instance().Delete(userId);
        }
        //清空重置座位
        if (chairId < items_.size()) {
            items_[chairId] = std::shared_ptr<IPlayer>();
        }
        return true;
    }
    return false;
}

void CTable::BroadcastUserInfoToOther(std::shared_ptr<IPlayer> const& player) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_ENTER_NOTIFY;
    UserBaseInfo& userInfo = std::dynamic_pointer_cast<CPlayer>(player)->GetUserBaseInfo();
    GameServer::MSG_S2C_UserBaseInfo msg;
    Game::Common::Header* header = msg.mutable_header();
    header->set_sign(HEADER_SIGN);
    msg.set_userid(userInfo.userId);
    msg.set_account(userInfo.account);
    msg.set_nickname(userInfo.nickName);
    msg.set_headindex(userInfo.headId);
    msg.set_score(userInfo.userScore);
    msg.set_location(userInfo.location);
    msg.set_tableid(player->GetTableId());
    msg.set_chairid(player->GetChairId());
    msg.set_userstatus(player->GetUserStatus());
    //std::string content = msg.SerializeAsString();
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer>  target = items_[i];
        if (target) {
            send(target, &msg, mainId, subId);
        }
    }
}

void CTable::SendAllOtherUserInfoToUser(std::shared_ptr<IPlayer> const& player) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_ENTER_NOTIFY;
    for (int i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> other = items_[i];
        if (other && other != player) {
            UserBaseInfo& userInfo = std::dynamic_pointer_cast<CPlayer>(other)->GetUserBaseInfo();
            GameServer::MSG_S2C_UserBaseInfo msg;
            Game::Common::Header* header = msg.mutable_header();
            header->set_sign(HEADER_SIGN);
            msg.set_userid(userInfo.userId);
            msg.set_nickname(userInfo.nickName);
            msg.set_location(userInfo.location);
            msg.set_headindex(userInfo.headId);
            msg.set_tableid(other->GetTableId());
            msg.set_chairid(other->GetChairId());
            msg.set_score(userInfo.userScore);
            msg.set_userstatus(other->GetUserStatus());
            send(player, &msg, mainId, subId);
        }
    }
}

void CTable::SendOtherUserInfoToUser(std::shared_ptr<IPlayer> const& player, tagUserInfo& userInfo) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_ENTER_NOTIFY;
    if (player) {
        GameServer::MSG_S2C_UserBaseInfo msg;
        Game::Common::Header* header = msg.mutable_header();
        header->set_sign(HEADER_SIGN);
        msg.set_userid(userInfo.userId);
        msg.set_nickname(userInfo.nickName);
        msg.set_location(userInfo.location);
        msg.set_headindex(userInfo.headId);
        msg.set_tableid(userInfo.tableId);
        msg.set_chairid(userInfo.chairId);
        msg.set_score(userInfo.score);
        msg.set_userstatus(userInfo.status);
        std::string content = msg.SerializeAsString();
        send(player, &msg, mainId, subId);
    }
}

void CTable::BroadcastUserScore(std::shared_ptr<IPlayer> const& player) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_SCORE_NOTIFY;
    GameServer::MSG_S2C_UserScoreInfo scoreinfo;
    Game::Common::Header* header = scoreinfo.mutable_header();
    header->set_sign(HEADER_SIGN);
    scoreinfo.set_userid(player->GetUserId());
    scoreinfo.set_tableid(player->GetTableId());
    scoreinfo.set_chairid(player->GetChairId());
    scoreinfo.set_userscore(player->GetUserScore());
    for (uint32_t i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> target = items_[i];
        if (target) {
            send(target, &scoreinfo, mainId, subId);
        }
    }
}

void CTable::BroadcastUserStatus(std::shared_ptr<IPlayer> const& player, bool sendToSelf) {
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_STATUS_NOTIFY;
    GameServer::MSG_S2C_GameUserStatus UserStatus;
    Game::Common::Header* header = UserStatus.mutable_header();
    header->set_sign(HEADER_SIGN);
    UserStatus.set_tableid(player->GetTableId());
    UserStatus.set_chairid(player->GetChairId());
    UserStatus.set_userid(player->GetUserId());
    UserStatus.set_usstatus(player->GetUserStatus());
    for (uint32_t i = 0; i < roomInfo_->maxPlayerNum; ++i) {
        std::shared_ptr<IPlayer> target = items_[i];
        if (!target) {
            continue;
        }
        if (!sendToSelf && target == player) {
            continue;
        }
        send(target, &UserStatus, mainId, subId);
    }
}

//获取水果机免费游戏剩余次数
//bool CTable::GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId)
//{
//    //LOG_INFO<<"+++++++++GetSgjFreeGameRecord+++++++: "<<userId;
//    //操作数据
//    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["sgj_freegame_record"];
//    auto query_value = document{} << "userId" << (int64_t)userId << finalize;
//    LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
//    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
//    if(maybe_result)
//    {
//        LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(maybe_result->view());
//
//        bsoncxx::document::view view = maybe_result->view();
//        pSgjFreeGameRecord->userId      = view["userId"].get_int64().value;
//        pSgjFreeGameRecord->betInfo     = view["betInfo"].get_int64().value;
//        pSgjFreeGameRecord->allMarry    = view["allMarry"].get_int32().value;
//        pSgjFreeGameRecord->freeLeft    = view["freeLeft"].get_int32().value;
//        pSgjFreeGameRecord->marryLeft   = view["marryLeft"].get_int32().value;
//        LOG_INFO<<"+++++++++ find_one +++++++:true "<<userId;
//        return true;
//    }
//    
//   // LOG_INFO<<"+++++++++ not find one +++++++:false "<<userId;
//    return false;
//}
//记录水果机免费游戏剩余次数
//bool CTable::WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord)
//{
//    // LOG_INFO"+++++++++ WriteSgjFreeGameRecord +++++++:"<<pSgjFreeGameRecord->userId
//    // <<pSgjFreeGameRecord->allMarry<< " " <<pSgjFreeGameRecord->betInfo<< " " 
//    // <<pSgjFreeGameRecord->freeLeft<< " " <<pSgjFreeGameRecord->marryLeft;
//
//    //操作数据
//    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["sgj_freegame_record"];
//
//    int leftCount = pSgjFreeGameRecord->freeLeft + pSgjFreeGameRecord->marryLeft;
//    //没有时清楚记录
//    if(leftCount <= 0){
//        coll.delete_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize);
//    }
//    else
//    {
//        auto find_result = coll.find_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize);
//        if(find_result) {
//            coll.update_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize,
//                    document{} << "$set" << open_document
//                    //<<"userId" << pSgjFreeGameRecord->userId
//                    <<"betInfo" << pSgjFreeGameRecord->betInfo
//                    <<"allMarry" << pSgjFreeGameRecord->allMarry
//                    <<"freeLeft" << pSgjFreeGameRecord->freeLeft
//                    <<"marryLeft" << pSgjFreeGameRecord->marryLeft << close_document
//                    << finalize);
//             LOG_INFO<<"+++++++++ update_one +++++++:";
//        }
//        else
//        {
//            auto insert_value = bsoncxx::builder::stream::document{}
//                << "userId" << pSgjFreeGameRecord->userId
//                << "betInfo" << pSgjFreeGameRecord->betInfo
//                << "allMarry" << pSgjFreeGameRecord->allMarry
//                << "freeLeft" << pSgjFreeGameRecord->freeLeft
//                << "marryLeft" <<  pSgjFreeGameRecord->marryLeft
//                << bsoncxx::builder::stream::finalize;
//
//            // LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);
//            bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
//            if(result){
//                // LOG_INFO<<"+++++++++ insert_one +++++++:"<<pSgjFreeGameRecord->userId;
//            }     
//        }
//        
//    }
//
//    return true;
//}

/*
写玩家分
@param pScoreInfo 玩家分数结构体数组
@param nCount  写玩家个数
@param strRound
*/
bool CTable::WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, std::string& strRound) {
    if (pScoreInfo && nCount > 0)
    {
        //   TaskService::get_mutable_instance().loadTaskStatus();
        uint32_t chairId = 0;
        // std::vector<tagUserTaskInfo> tasks;

 //        mongocxx::client_session dbSession = MONGODBCLIENT.start_session();
 //        dbSession.start_transaction();
        try
        {
            for (uint32_t i = 0; i < nCount; ++i)
            {
                tagScoreInfo* scoreInfo = (pScoreInfo + i);
                chairId = scoreInfo->chairId;

                std::shared_ptr<IPlayer> player;
                {
                    //                READ_LOCK(m_list_mutex);
                    player = items_[chairId];
                }

                if (player)
                {
                    UserBaseInfo& userBaseInfo = std::dynamic_pointer_cast<CPlayer>(player)->GetUserBaseInfo();
                    int64_t sourceScore = userBaseInfo.userScore;
                    int64_t targetScore = sourceScore + scoreInfo->addScore;

                    if (!player->IsRobot())
                    {
                        UpdateUserScoreToDB(userBaseInfo.userId, scoreInfo);
                        AddUserGameInfoToDB(userBaseInfo, scoreInfo, strRound, nCount, false);
                        AddScoreChangeRecordToDB(userBaseInfo, sourceScore, scoreInfo->addScore, targetScore);
                        AddUserGameLogToDB(userBaseInfo, scoreInfo, strRound);
                        player->SetUserScore(targetScore);
                        //LOG_DEBUG << "AddUserScore userId:" << userBaseInfo.userId << ", Source Score:" << sourceScore << ", AddScore:" << scoreInfo->addScore << ", lNewScore:" << targetScore;
                        /*if( gameInfo_->matchforbids[MTH_BLACKLIST] && player->GetBlacklistInfo().status == 1 )
                        {
                            long current;
                            std::string blacklistkey = REDIS_BLACKLIST+to_string(player->GetUserId());
                            bool ret = REDISCLIENT.hincrby(blacklistkey,REDIS_FIELD_CURRENT,-scoreInfo->addScore,&current);
                            if(ret)
                            {
                                std::string fields[2] = {REDIS_FIELD_TOTAL,REDIS_FIELD_STATUS};
                                redis::RedisValue values;
                                ret = REDISCLIENT.hmget(blacklistkey,fields,2,values);
                                if  (ret && !values.empty())
                                {
                                    player->GetBlacklistInfo().current = current;
                                    if(values[REDIS_FIELD_STATUS].asInt() == 1)
                                    {
                                        if(values[REDIS_FIELD_TOTAL].asInt() <= current)
                                        {
                                            redis::RedisValue redisValue;
                                            redisValue[REDIS_FIELD_STATUS] = (int32_t)0;
                                            REDISCLIENT.hmset(blacklistkey,redisValue);
                                            player->GetBlacklistInfo().status = 0;
                                            WriteBlacklistLog(player,0);
                                        }
                                        else
                                        {
                                            WriteBlacklistLog(player,1);
                                        }
                                    }else
                                    {
                                        player->GetBlacklistInfo().status = 0;
                                    }
                                }
                            }

                        }*/
                        //                         tasks.clear();
                        //                         bool res = TaskService::get_mutable_instance().checkTask(userBaseInfo.userId,userBaseInfo.agentId,roomInfo_->gameId, roomInfo_->roomId, tasks);
                        //                         if( res )
                        //                         {
                        //                             for(auto task : tasks)
                        //                             {
                        //                                 switch(task.taskType)
                        //                                 {
                        //                                 case TASK_FIRST_WIN:
                        //                                     if(scoreInfo->addScore > 0)
                        //                                     {
                        //                                         TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                        //                                     }
                        //                                     break;
                        //                                 case TASK_PARTI_MTH:
                        //                                 case TASK_TOTAL_GAME_TIMES:
                        //                                     TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                        //                                     break;
                        //                                 case TASK_TOTAL_BET:
                        //                                     TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,scoreInfo->rWinScore);
                        //                                     break;
                        //                                 case TASK_TOTAL_WIN_TIMES:
                        //                                     if(scoreInfo->addScore > 0)
                        //                                         TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                        //                                     break;
                        //                                 default:
                        //                                     LOG_ERROR << "unknown task_type : " <<(int)task.taskType << ", task_name:" << task.taskName;
                        //                                     break;
                        //                                 }
                        //                             }
                        //                         }

                    }
                    else //is android
                    {
                        //                        UpdateUserScoreToDB(userBaseInfo.userId, scoreInfo);
                        if (gameInfo_->gameType == GameType_Confrontation)
                        {
                            //                            AddUserGameInfoToDB(userBaseInfo, scoreInfo, strRound, true);
                        }
                        player->SetUserScore(targetScore);
                        //                        AddScoreChangeRecordToDB(userBaseInfo, sourceScore, scoreInfo->addScore, targetScore);
                        //                        AddUserGameLogToDB(userBaseInfo, scoreInfo, strRound);
                    }
                    //                if(scoreInfo.bisBroadcast)
                    //                {
                    //                    BroadcastUserScore(player);
                    //                }
                }
                else
                {
                    //LOG_WARN << " >>> WriteUserScore player is NULL! <<<";
                    continue;
                }
            }
            //            dbSession.commit_transaction();
        }
        catch (std::exception& e)
        {
            //            dbSession.abort_transaction();
                        //LOG_ERROR << "MongoDB WriteData Error:"<<e.what();
        }
    }
    return true;
}

// bool CTable::WriteBlacklistLog(std::shared_ptr<IPlayer> const& player, int status)
// {
//     tagBlacklistInfo info = player->GetBlacklistInfo();
//     auto insert_value = bsoncxx::builder::stream::document{}
//             << "userid" << player->GetUserId()
//             << "account" << player->GetAccount()
//             << "optime" << bsoncxx::types::b_date(std::chrono::system_clock::now())
//             << "opaccount" << "GameServer"
//             << "score" << (int32_t)player->GetUserScore()
//             << "rate" << info.weight
//             << "amount" << info.total
//             << "current" << info.current
//             << "status" << status
//             << bsoncxx::builder::stream::finalize;
// 
//     LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);
// 
//     mongocxx::collection coll = MONGODBCLIENT["dblog"]["blacklistlog"];
//     bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
//     if(!result)
//     {
//         return false;
//     }
// 
//     coll = MONGODBCLIENT["gamemain"]["game_blacklist"];
//     coll.update_one(document{} << "userid" << player->GetUserId() << finalize,
//                     document{} << "$set" << open_document
//                     <<"status" << status
//                     <<"current" << info.current << close_document
//                     << finalize);
// 
//     return true;
// }

bool CTable::WriteSpecialUserScore(tagSpecialScoreInfo* pSpecialScoreInfo, uint32_t nCount, std::string& strRound) {
    if (pSpecialScoreInfo && nCount > 0)
    {
        //  TaskService::get_mutable_instance().loadTaskStatus();
          //std::vector<tagUserTaskInfo> tasks;
        int64_t userId = 0;
        uint32_t chairId = 0;
        mongocxx::client_session dbSession = MONGODBCLIENT.start_session();
        dbSession.start_transaction();
        try
        {
            for (uint32_t i = 0; i < nCount; ++i)
            {
                // try to get the special user score info.
                tagSpecialScoreInfo* scoreInfo = (pSpecialScoreInfo + i);

                userId = scoreInfo->userId;
                chairId = scoreInfo->chairId;
                if (scoreInfo->bWriteScore)
                {
                    std::shared_ptr<IPlayer> player;
                    {
                        //                        if(userId > 0)
                        //        //                READ_LOCK(m_list_mutex);
                        //                            player = GetPlayer(userId); //items_[chairId];
                        //                        else
                        //                        {
                        player = items_[chairId];
                        //                        }
                    }
                    if (player)
                    {
                        UserBaseInfo& userBaseInfo = std::dynamic_pointer_cast<CPlayer>(player)->GetUserBaseInfo();
                        int64_t sourceScore = userBaseInfo.userScore;
                        int64_t targetScore = sourceScore + scoreInfo->addScore;
                        if (!player->IsRobot())
                        {
                            UpdateUserScoreToDB(userBaseInfo.userId, scoreInfo);
                            AddUserGameLogToDB(scoreInfo, strRound);
                            // 							if (gameInfo_->matchforbids[MTH_BLACKLIST] && player->GetBlacklistInfo().status == 1)
                            // 							{
                            // 								long current;
                            //                                 std::string blacklistkey = REDIS_BLACKLIST + std::to_string(player->GetUserId());
                            // 								bool ret = REDISCLIENT.hincrby(blacklistkey, REDIS_FIELD_CURRENT, -scoreInfo->addScore, &current);
                            // 								if (ret)
                            // 								{
                            //                                     std::string fields[2] = { REDIS_FIELD_TOTAL,REDIS_FIELD_STATUS };
                            // 									redis::RedisValue values;
                            // 									ret = REDISCLIENT.hmget(blacklistkey, fields, 2, values);
                            // 									if (ret && !values.empty())
                            // 									{
                            // 										player->GetBlacklistInfo().current = current;
                            // 										if (values[REDIS_FIELD_STATUS].asInt() == 1)
                            // 										{
                            // 											if (values[REDIS_FIELD_TOTAL].asInt() <= current)
                            // 											{
                            // 												redis::RedisValue redisValue;
                            // 												redisValue[REDIS_FIELD_STATUS] = (int32_t)0;
                            // 												REDISCLIENT.hmset(blacklistkey, redisValue);
                            // 												player->GetBlacklistInfo().status = 0;
                            // 												WriteBlacklistLog(player, 0);
                            // 											}
                            //                                             else
                            //                                             {
                            //                                                 WriteBlacklistLog(player,1);
                            //                                             }
                            // 										}
                            // 										else
                            // 										{
                            // 											player->GetBlacklistInfo().status = 0;
                            // 										}
                            // 									}
                            // 								}
                            // 							}
                                                     //   tasks.clear();
                            //                             bool res = TaskService::get_mutable_instance().checkTask(userBaseInfo.userId,userBaseInfo.agentId,roomInfo_->gameId, roomInfo_->roomId, tasks);
                            //                             if(res)
                            //                             {
                            //                                 for(auto task : tasks)
                            //                                 {
                            //                                     switch(task.taskType)
                            //                                     {
                            //                                     case TASK_FIRST_WIN:
                            //                                         if(scoreInfo->addScore > 0)
                            //                                         {
                            //                                             TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                            //                                         }
                            //                                         break;
                            //                                     case TASK_PARTI_MTH:
                            //                                     case TASK_TOTAL_GAME_TIMES:
                            //                                         TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                            //                                         break;
                            //                                     case TASK_TOTAL_BET:
                            //                                         TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,scoreInfo->betScore);
                            //                                         break;
                            //                                     case TASK_TOTAL_WIN_TIMES:
                            //                                         if(scoreInfo->addScore > 0)
                            //                                             TaskService::get_mutable_instance().updateTaskProgress(userBaseInfo.userId, task.taskId,1);
                            //                                         break;
                            //                                     default:
                            //                                         LOG_ERROR << "unknown task_type : " <<(int)task.taskType << ", task_name:" << task.taskName;
                            //                                         break;
                            //                                     }
                            //                                 }
                            //                             }else
                            //                             {
                            //                                 LOG_ERROR << "can't get user tasks";
                            //                             }
                        }
                        else//is android
                        {
                            //                        UpdateUserScoreToDB(userBaseInfo.userId, scoreInfo);
                            if (gameInfo_->gameType == GameType_Confrontation)
                            {
                                //                            AddUserGameInfoToDB(userBaseInfo, scoreInfo, strRound, true);
                            }
                        }
                        player->SetUserScore(targetScore);
                    }
                }

                if (scoreInfo->bWriteRecord)
                {
                    std::shared_ptr<IPlayer> player;
                    player = items_[chairId];
                    if (player) scoreInfo->lineCode = std::dynamic_pointer_cast<CPlayer>(player)->GetUserBaseInfo().lineCode;
                    AddUserGameInfoToDB(scoreInfo, strRound, nCount, false);
                    AddScoreChangeRecordToDB(scoreInfo);
                }
            }
            dbSession.commit_transaction();
        }
        catch (std::exception& e)
        {
            dbSession.abort_transaction();
            //LOG_ERROR << "MongoDB WriteData Error:"<<e.what();
        }
    }
    return true;
}

bool CTable::UpdateUserScoreToDB(int64_t userId, tagScoreInfo* pScoreInfo) {
    bool ok = false;
    //    try
    //    {
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
    coll.update_one(document{} << "userid" << userId << finalize,
        document{} << "$inc" << open_document
        << "gamerevenue" << pScoreInfo->revenue
        << "winorlosescore" << pScoreInfo->addScore
        << "integralvalue" << pScoreInfo->rWinScore
        << "score" << pScoreInfo->addScore << close_document
        << finalize);
    int64_t result = 0;
    std::string key = REDIS_SCORE_PREFIX + to_string(userId);
    bool res = REDISCLIENT.hincrby(key, REDIS_WINSCORE, pScoreInfo->addScore, &result);
    if (res)
    {
        while (true) {
            std::string addscorestr;
            res = REDISCLIENT.hget(key, REDIS_ADDSCORE, addscorestr);
            if (!res)
            {
                //LOG_ERROR << "read addscore from redis faild";
                break;
            }
            int64_t addscore = stod(addscorestr);
            res = REDISCLIENT.hget(key, REDIS_SUBSCORE, addscorestr);
            if (!res)
            {
                //LOG_ERROR << "read subscore from redis faild";
                break;
            }
            int64_t subscore = stod(addscorestr);
            int64_t totalBet = addscore - subscore;
            res = REDISCLIENT.hget(key, REDIS_WINSCORE, addscorestr);
            if (!res)
            {
                //LOG_ERROR << "read subscore from redis faild";
                break;
            }
            int64_t totalWinScore = stod(addscorestr);
            if (totalBet == 0)
            {
                totalBet = 1;
            }
            int64_t per = result * 100 / totalBet;
            if (totalWinScore > 0)
            {
                if (per > 200 || per < 0)
                {
                    REDISCLIENT.AddQuarantine(userId);
                }
                else
                {
                    REDISCLIENT.RemoveQuarantine(userId);
                }

            }
            break;
        }
    }
    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo) {
    bool ok = false;

    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
    coll.update_one(document{} << "userid" << userId << finalize,
        document{} << "$inc" << open_document
        << "gamerevenue" << pScoreInfo->revenue
        << "winorlosescore" << pScoreInfo->addScore
        << "integralvalue" << pScoreInfo->rWinScore
        << "score" << pScoreInfo->addScore << close_document
        << finalize);
    ok = true;

    return ok;
}

bool CTable::AddUserGameInfoToDB(UserBaseInfo& userBaseInfo, tagScoreInfo* scoreInfo, std::string& strRoundId, int32_t userCount, bool bAndroidUser) {
    bool ok = false;
    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
//     int32_t rankId = 0;
//     if (gameInfo_->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
//         //解析-97
//         if (scoreInfo->cardValue.length() >= 5){
//             std::string str1= scoreInfo->cardValue.substr(0,5);
//             std::string str2= scoreInfo->cardValue.substr(5);
//             if(str1.compare("---97") == 0){
//                 rankId = -97;
//                 // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
//                 scoreInfo->cardValue = str2;
//                 // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
//                 // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
//             }
//         }
//     }
    //    try
    //    {
    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
        << "gameinfoid" << strRoundId
        << "userid" << userBaseInfo.userId
        << "account" << userBaseInfo.account
        << "agentid" << (int32_t)userBaseInfo.agentId
        << "linecode" << userBaseInfo.lineCode
        << "gameid" << (int32_t)gameInfo_->gameId
        << "roomid" << (int32_t)roomInfo_->roomId
        << "tableid" << (int32_t)tableState_.tableId
        << "chairid" << (int32_t)scoreInfo->chairId
        << "isBanker" << (int32_t)scoreInfo->isBanker
        << "cardvalue" << scoreInfo->cardValue
        << "usercount" << userCount
        //<< "rank" << rankId
        << "beforescore" << userBaseInfo.userScore
        << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
        << "cellscore" << open_array;
    for (int64_t& betscore : scoreInfo->cellScore)
        insert_value = insert_value << betscore;
    auto after_array = insert_value << close_array;
    after_array
        << "allbet" << scoreInfo->betScore
        << "winscore" << scoreInfo->addScore
        << "score" << userBaseInfo.userScore + scoreInfo->addScore
        << "revenue" << scoreInfo->revenue
        << "isandroid" << bAndroidUser
        << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
        << "gameendtime" << bsoncxx::types::b_date(chrono::system_clock::now());

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    //        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(doc);

#if 1
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
    if (result)
    {
    }
#else
    tableContext_->RunWriteDataToMongoDB("gamemain", "play_record", INSERT_ONE, doc.view(), bsoncxx::document::view());
#endif

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::AddUserGameInfoToDB(tagSpecialScoreInfo* scoreInfo, std::string& strRoundId, int32_t userCount, bool bAndroidUser) {
    bool ok = false;

    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
//     int32_t rankId = 0;
//     if (gameInfo_->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
//         //解析-97
//         if (scoreInfo->cardValue.length() >= 5){
//             std::string str1= scoreInfo->cardValue.substr(0,5);
//             std::string str2= scoreInfo->cardValue.substr(5);
//             if(str1.compare("---97") == 0){
//                 rankId = -97;
//                 // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
//                 scoreInfo->cardValue = str2;
//                 // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
//                 // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
//             }
//         }
//     }
//    try
//    {
    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
        << "gameinfoid" << strRoundId
        << "userid" << scoreInfo->userId
        << "account" << scoreInfo->account
        << "agentid" << (int32_t)scoreInfo->agentId
        << "linecode" << scoreInfo->lineCode
        << "gameid" << (int32_t)gameInfo_->gameId
        << "roomid" << (int32_t)roomInfo_->roomId
        << "tableid" << (int32_t)tableState_.tableId
        << "chairid" << (int32_t)scoreInfo->chairId
        << "isBanker" << (int32_t)scoreInfo->isBanker
        << "cardvalue" << scoreInfo->cardValue
        << "usercount" << userCount
        // << "rank" << rankId
        << "beforescore" << scoreInfo->beforeScore
        << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
        << "cellscore" << open_array;
    for (int64_t& betscore : scoreInfo->cellScore)
        insert_value = insert_value << betscore;
    auto after_array = insert_value << close_array;
    after_array
        << "allbet" << scoreInfo->betScore
        << "winscore" << scoreInfo->addScore
        << "score" << scoreInfo->beforeScore + scoreInfo->addScore
        << "revenue" << scoreInfo->revenue
        << "isandroid" << bAndroidUser
        << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
        << "gameendtime" << bsoncxx::types::b_date(chrono::system_clock::now());

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    //_LOG_DEBUG("Insert Document: %s", bsoncxx::to_json(doc).c_str());
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
    if (result)
    {
    }

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::AddScoreChangeRecordToDB(UserBaseInfo& userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore) {
    bool ok = false;
    //    try
    //    {
    auto insert_value = bsoncxx::builder::stream::document{}
        << "userid" << userBaseInfo.userId
        << "account" << userBaseInfo.account
        << "agentid" << (int32_t)userBaseInfo.agentId
        << "linecode" << userBaseInfo.lineCode
        << "changetype" << (int32_t)roomInfo_->roomId
        << "changemoney" << addScore
        << "beforechange" << sourceScore
        << "afterchange" << targetScore
        << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
        << bsoncxx::builder::stream::finalize;

    //LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

#if 1
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
    if (result)
    {
    }
#else
    tableContext_->RunWriteDataToMongoDB("gamemain", "user_score_record", INSERT_ONE, insert_value.view(), bsoncxx::document::view());
#endif

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::AddScoreChangeRecordToDB(tagSpecialScoreInfo* scoreInfo) {
    bool ok = false;
    //    try
    //    {
    auto insert_value = bsoncxx::builder::stream::document{}
        << "userid" << scoreInfo->userId
        << "account" << scoreInfo->account
        << "agentid" << (int32_t)scoreInfo->agentId
        << "linecode" << scoreInfo->lineCode
        << "changetype" << (int32_t)roomInfo_->roomId
        << "changemoney" << scoreInfo->addScore
        << "beforechange" << scoreInfo->beforeScore
        << "afterchange" << scoreInfo->beforeScore + scoreInfo->addScore
        << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
        << bsoncxx::builder::stream::finalize;

    //LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
    if (result)
    {
    }

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::AddUserGameLogToDB(UserBaseInfo& userBaseInfo, tagScoreInfo* scoreInfo, std::string& strRoundId) {
    bool ok = false;
    //    try
    //    {
    auto insert_value = bsoncxx::builder::stream::document{}
        << "gamelogid" << strRoundId
        << "userid" << userBaseInfo.userId
        << "account" << userBaseInfo.account
        << "agentid" << (int32_t)userBaseInfo.agentId
        << "linecode" << userBaseInfo.lineCode
        << "gameid" << (int32_t)gameInfo_->gameId
        << "roomid" << (int32_t)roomInfo_->roomId
        << "winscore" << scoreInfo->addScore
        << "revenue" << scoreInfo->revenue
        << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
        << bsoncxx::builder::stream::finalize;

    //LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

#if 1
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_log"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
    if (result)
    {
    }
#else
    tableContext_->RunWriteDataToMongoDB(std::string("gamemain"), std::string("game_log"), INSERT_ONE, insert_value.view(), bsoncxx::document::view());
#endif

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

bool CTable::AddUserGameLogToDB(tagSpecialScoreInfo* scoreInfo, std::string& strRoundId) {
    bool ok = false;
    //    try
    //    {
    auto insert_value = bsoncxx::builder::stream::document{}
        << "gamelogid" << strRoundId
        << "userid" << scoreInfo->userId
        << "account" << scoreInfo->account
        << "agentid" << (int32_t)scoreInfo->agentId
        << "linecode" << scoreInfo->lineCode
        << "gameid" << (int32_t)gameInfo_->gameId
        << "roomid" << (int32_t)roomInfo_->roomId
        << "winscore" << scoreInfo->addScore
        << "revenue" << scoreInfo->revenue
        << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
        << bsoncxx::builder::stream::finalize;

    //LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_log"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
    if (result)
    {
    }

    ok = true;
    //    }catch(exception &e)
    //    {
    //        LOG_ERROR << "exception: " << e.what();
    //        throw;
    //    }
    return ok;
}

// 奖池消息 JackpotBroadCast
//@ optype:0 为累加，1为设置
//bool CTable::UpdateJackpot(int32_t optype,int64_t incScore,int32_t JackpotId,int64_t *newJackPot)
//{
//    int maxSize = sizeof(roomInfo_->totalJackPot)/sizeof(int64_t);
//    // 
//    // LOG_WARN << __FUNCTION__ << " " << optype << " "<< incScore <<" + "<< m_TotalJackPot[JackpotId] << " "<< maxSize;
//
//    if(JackpotId >= maxSize){
//        LOG_ERROR << " JackpotId Error : "<< incScore <<" to " << JackpotId;
//        return false;
//    }
//    // 组合
//    std::string fieldName = std::to_string(roomInfo_->gameId);
//    std::string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_lkJackpot);
// 
//    // 累加
//    bool res = REDISCLIENT.hincrby(keyName,fieldName,incScore,newJackPot);
//    if(res){
//        LOG_WARN << "Jackpot Changed From : "<< incScore << " + "<< m_TotalJackPot[JackpotId] <<" to " << (int64_t)(*newJackPot);
//        m_TotalJackPot[JackpotId] = *newJackPot;
//        roomInfo_->totalJackPot[JackpotId] = *newJackPot;
//        // LOG_WARN <<" "<<roomInfo_->totalJackPot[0]<<" "<<roomInfo_->totalJackPot[1]<<" "<<roomInfo_->totalJackPot[2]<<" "<<roomInfo_->totalJackPot[3]<<" "<<roomInfo_->totalJackPot[4];
//    }
//
//    // LOG_ERROR << "hincrby Jackpot end ";
//    return res;
//}
// 读取彩金值
//int64_t CTable::ReadJackpot( int32_t JackpotId )
//{
//    int maxSize = sizeof(roomInfo_->totalJackPot)/sizeof(int64_t);
//    if( JackpotId >= maxSize ){ return -1; }
//    // 
//    if (roomInfo_->totalJackPot[JackpotId] == 0){
//        // 组装字段
//        std::string fieldName = std::to_string(roomInfo_->gameId);
//        std::string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_lkJackpot);
//
//        int64_t newJackPot = 0;
//        // 累加
//        bool res = REDISCLIENT.hincrby(keyName,fieldName,0,&newJackPot);
//        if(res){
//            m_TotalJackPot[JackpotId] = newJackPot;
//            LOG_WARN << "ReadJackpot: "<< roomInfo_->totalJackPot[JackpotId] << " + "<< m_TotalJackPot[JackpotId] <<" to " << newJackPot;
//            roomInfo_->totalJackPot[JackpotId] = newJackPot;
//            // LOG_WARN <<" "<<roomInfo_->totalJackPot[0]<<" "<<roomInfo_->totalJackPot[1]<<" "<<roomInfo_->totalJackPot[2]<<" "<<roomInfo_->totalJackPot[3]<<" "<<roomInfo_->totalJackPot[4];
//        }
//        else{
//            LOG_ERROR <<keyName <<" "<<fieldName << " 读取彩金失败: "<<JackpotId <<" "<< (int64_t)m_TotalJackPot[JackpotId] <<" - " << (int64_t)roomInfo_->totalJackPot[JackpotId];
//        }
//    }
//    return roomInfo_->totalJackPot[JackpotId];
//}

// 公共方法
//  return -1,调用失败；返回0代表成功
// int32_t CTable::CommonFunc(eCommFuncId funcId, std::string &jsonInStr, std::string &jsonOutStr )
// {  
//     //Json::Reader reader;
//     //Json::Value root;
// 
//     //LOG_INFO << __FUNCTION__<<" "<<(int)funcId;
// 
//     //// 写水果机奖池记录
//     //if(funcId == eCommFuncId::fn_sgj_jackpot_rec){
//     //    if(reader.parse(jsonInStr, root))
//     //    {
//     //        int32_t msgCount = root["msgCount"].asInt64();
//     //        int32_t msgType = root["msgType"].asInt();
//     //        if (msgCount > 20)  msgCount = 20; //防止误传
// 
//     //        // 写
//     //        if(msgType == 0){
//     //            string msgstr = root["msgstr"].asString();
//     //            if(!SaveJackpotRecordMsg(msgstr,msgCount)) return -1;  
//     //        }
//     //        else if(msgType == 1){// 读
//     //            vector<string> msgLists;
//     //            REDISCLIENT.lrangeCmd(eRedisKey::lst_sgj_JackpotMsg, msgLists, msgCount);
//     //            // 返回
//     //            int32_t idx = 0;
//     //            Json::Value root;
//     //            for(string msg : msgLists){
//     //                Json::Value value;
//     //                value["Idx"] = idx++;
//     //                value["msg"] = msg;
//     //                root.append(value);
//     //            }
//     //            Json::FastWriter write;
//     //            jsonOutStr = write.write(root);
//     //        }
//     //    }
//     //    return 0;
//     //}
//  
//     return 0;
// }


// 存储奖池信息
// @msgStr 消息体
// @msgCount 消息数量
//bool CTable::SaveJackpotRecordMsg(std::string &msgStr,int32_t msgCount)
//{
//    bool result = false;
//   // int repeatCount = 10;//重试10次后放弃存储
//   // std::string lockStr = to_string(::getpid());
//   // do
//   // {
//   //     int ret = REDISCLIENT.setnxCmd(eRedisKey::str_lockId_jp_2, lockStr, 2);
//   //     if (ret == -1)
//   //     {
//   //         LOG_ERROR << "redis连接失败" << __FUNCTION__;
//   //         break;
//   //     }
//   //     if (ret == 1)
//   //     {
//   //         long long msgListLen = 0;
//   //       //  REDISCLIENT.lpushCmd(eRedisKey::lst_sgj_JackpotMsg, msgStr, msgListLen);
//
//   //         //7.3移除超出的数量
//   //         if (msgListLen > msgCount)
//   //         {
//   //             for (int idx = 0; idx < (msgListLen - msgCount); idx++)
//   //             {
//   //                 std::string lastElement;
//   //              //   REDISCLIENT.rpopCmd(eRedisKey::lst_sgj_JackpotMsg, lastElement);
//   //                 LOG_WARN << "存储和移除消息[" << msgStr << " " << lastElement << "]";
//   //             }
//   //         }
//
//   //         // 解锁(不是很有效)
//		 // /*  if (!REDISCLIENT.delnxCmd(eRedisKey::str_lockId_jp_2, lockStr)){
//			//	LOG_ERROR << "解锁失败[" << lockStr << "]" << __FUNCTION__;;
//			//}*/
//
//   //         result = true;
//   //         break;
//   //     }
//   //     else{
//   //         usleep(10 * 1000); // 10毫秒
//   //         LOG_WARN << "10毫秒延时 " << repeatCount;
//   //     }
//   // } 
//   // while (--repeatCount > 0);
//
//    return result;
//}

int CTable::UpdateStorageScore(int64_t changeStockScore) {
    static int count = 0;
    curStorage_ += changeStockScore;
    if (++count > 0)
    {
        WriteGameChangeStorage(changeStockScore);
        count = 0;
    }
    return count;
}

bool CTable::GetStorageScore(tagStorageInfo& storageInfo) {
    /*
    storageInfo.lLowlimit = lowStorage_;
    storageInfo.lUplimit = highStorage_;
    storageInfo.lEndStorage = curStorage_;
    storageInfo.lSysAllKillRatio = sysKillAllRatio_;
    storageInfo.lReduceUnit = sysReduceRatio_;
    storageInfo.lSysChangeCardRatio = sysChangeCardRatio_;
    */

    storageInfo.lLowlimit = roomInfo_->totalStockLowerLimit;;
    storageInfo.lUplimit = roomInfo_->totalStockHighLimit;
    storageInfo.lEndStorage = roomInfo_->totalStock;
    storageInfo.lSysAllKillRatio = roomInfo_->systemKillAllRatio;
    storageInfo.lReduceUnit = roomInfo_->systemReduceRatio;
    storageInfo.lSysChangeCardRatio = roomInfo_->changeCardRatio;

    return true;
}

bool CTable::ReadStorageScore(tagGameRoomInfo* roomInfo) {
    curStorage_ = roomInfo->totalStock;
    lowStorage_ = roomInfo->totalStockLowerLimit;
    highStorage_ = roomInfo->totalStockHighLimit;
    secondLowStorage_ = roomInfo->totalStockSecondLowerLimit;
    secondHighStorage_ = roomInfo->totalStockSecondHighLimit;

    sysKillAllRatio_ = roomInfo->systemKillAllRatio;
    sysReduceRatio_ = roomInfo->systemReduceRatio;
    sysChangeCardRatio_ = roomInfo->changeCardRatio;
    return true;
}

bool CTable::WriteGameChangeStorage(int64_t changeStockScore) {
    mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["game_kind"];
    coll.update_one(document{} << "rooms.roomid" << (int32_t)roomInfo_->roomId << finalize,
        document{} << "$inc" << open_document
        << "rooms.$.totalstock" << changeStockScore << close_document
        << finalize);
    int64_t newStock;
    bool res = REDISCLIENT.hincrby(REDIS_CUR_STOCKS, to_string(roomInfo_->roomId), changeStockScore, &newStock);
    if (res)
    {
        //LOG_INFO << "Stock Changed From : "<< changeStockScore <<" + " << (int64_t)roomInfo_->totalStock << "=" << (int64_t)newStock;
        roomInfo_->totalStock = newStock;
        curStorage_ = newStock;
    }
    return true;
}

/*
//对局记录详情
bool CTable::GetReplayRecordDetail(std::string const& gameinfoid, std::string& jsondata) {

    mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];

    mongocxx::options::find opts = mongocxx::options::find{};
    opts.projection(bsoncxx::builder::stream::document{}
        << "detail" << 1 << bsoncxx::builder::stream::finalize);

    bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
        bsoncxx::builder::stream::document{}
        << "gameinfoid" << gameinfoid
        << bsoncxx::builder::stream::finalize, opts);
    if (!result) {
        return false;
    }
    bsoncxx::document::view view = result->view();
    if (view["detail"]) {
        switch (view["detail"].type()) {
        case bsoncxx::type::k_null:
            break;
        case bsoncxx::type::k_utf8:
            jsondata = view["detail"].get_utf8().value.to_string();
            return true;
        case bsoncxx::type::k_binary:
             view["detail"].get_binary().bytes, view["detail"].get_binary().size;
            return true;
        }
    }
    return false;
}*/

bool CTable::SaveReplay(tagGameReplay& replay) {
    return replay.saveAsStream ?
        SaveReplayDetailBlob(replay) :
        SaveReplayDetailJson(replay);
}

bool CTable::SaveReplayDetailBlob(tagGameReplay& replay) {
    if (replay.players.size() == 0 || !replay.players[0].flag)
    {
        return  false;
    }
    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
        //<< "gameid" << replay.gameid
        << "gameinfoid" << replay.gameinfoid
        << "timestamp" << bsoncxx::types::b_date{ std::chrono::system_clock::now() }
        << "roomname" << replay.roomname
        << "cellscore" << replay.cellscore
        << "detail" << bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, uint32_t(replay.detailsData.size()), reinterpret_cast<uint8_t const*>(replay.detailsData.data()) }
    << "players" << open_array;

    for (tagReplayPlayer& player : replay.players)
    {
        if (player.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "userid" << player.userid
                << "account" << player.accout
                << "score" << player.score
                << "chairid" << player.chairid
                << bsoncxx::builder::stream::close_document;
        }
    }
    insert_value << close_array << "steps" << open_array;
    for (tagReplayStep& step : replay.steps)
    {
        if (step.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "bet" << step.bet
                << "time" << step.time
                << "ty" << step.ty
                << "round" << step.round
                << "chairid" << step.chairId
                << "pos" << step.pos
                << bsoncxx::builder::stream::close_document;
        }
    }

    insert_value << close_array << "results" << open_array;

    for (tagReplayResult& result : replay.results)
    {
        if (result.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "chairid" << result.chairId
                << "bet" << result.bet
                << "pos" << result.pos
                << "win" << result.win
                << "cardtype" << result.cardtype
                << "isbanker" << result.isbanker
                << bsoncxx::builder::stream::close_document;
        }
    }
    auto after_array = insert_value << close_array;

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
    return true;
}

bool CTable::SaveReplayDetailJson(tagGameReplay& replay) {
    if (replay.players.size() == 0 || !replay.players[0].flag)
    {
        return  false;
    }
    bsoncxx::builder::stream::document builder{};
    auto insert_value = builder
        //<< "gameid" << replay.gameid
        << "gameinfoid" << replay.gameinfoid
        << "timestamp" << bsoncxx::types::b_date{ std::chrono::system_clock::now() }
        << "roomname" << replay.roomname
        << "cellscore" << replay.cellscore
        << "detail" << std::move(replay.detailsData)
        << "players" << open_array;

    for (tagReplayPlayer& player : replay.players)
    {
        if (player.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "userid" << player.userid
                << "account" << player.accout
                << "score" << player.score
                << "chairid" << player.chairid
                << bsoncxx::builder::stream::close_document;
        }
    }
    insert_value << close_array << "steps" << open_array;
    for (tagReplayStep& step : replay.steps)
    {
        if (step.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "bet" << step.bet
                << "time" << step.time
                << "ty" << step.ty
                << "round" << step.round
                << "chairid" << step.chairId
                << "pos" << step.pos
                << bsoncxx::builder::stream::close_document;
        }
    }

    insert_value << close_array << "results" << open_array;

    for (tagReplayResult& result : replay.results)
    {
        if (result.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                << "chairid" << result.chairId
                << "bet" << result.bet
                << "pos" << result.pos
                << "win" << result.win
                << "cardtype" << result.cardtype
                << "isbanker" << result.isbanker
                << bsoncxx::builder::stream::close_document;
        }
    }
    auto after_array = insert_value << close_array;

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
    return true;
}

void CTable::KickOffLine(std::shared_ptr<IPlayer> const& player, int32_t kickType) {
    if (tableContext_) {
        tableContext_->KickOffLine(player->GetUserId(), kickType);
    }
}

bool CTable::DelUserOnlineInfo(int64_t userId) {
    _LOG_ERROR("%d", userId);
    if (false) {
        REDISCLIENT.ResetExpiredUserOnlineInfo(userId);
    }
    else {
        //if(REDISCLIENT.ExistsUserLoginInfo(userId)){
        //}
        REDISCLIENT.DelUserOnlineInfo(userId);
    }
    return true;
}

bool CTable::SetUserOnlineInfo(int64_t userId) {
    _LOG_ERROR("%d %d %d", userId, gameInfo_->gameId, roomInfo_->roomId);
    REDISCLIENT.SetUserOnlineInfo(userId, gameInfo_->gameId, roomInfo_->roomId);
    return true;
}

bool CTable::RoomSitChair(std::shared_ptr<IPlayer> const& player, packet::internal_prev_header_t const* pre_header, packet::header_t const* header/*, bool bDistribute*/) {
    if (!player || player->GetUserId() <= 0) {
        return false;
    }
    uint32_t chairId = 0;
    uint32_t maxPlayerNum = roomInfo_->maxPlayerNum;
    uint32_t startIndex = 0;
    if (gameInfo_->gameType == GameType_Confrontation) {
        startIndex = weight_.rand().betweenInt64(0, maxPlayerNum - 1).randInt_mt();
    }
    for (uint32_t i = 0; i < maxPlayerNum; ++i) {
        chairId = (startIndex + i) % maxPlayerNum;
        if (items_[chairId]) {
            continue;
        }
        player->SetTableId(tableState_.tableId);
        player->SetChairId(chairId);
        items_[chairId] = player;
        player->SetUserStatus(sSit);
        OnUserEnterAction(player, pre_header, header);
        if (!player->IsRobot()) {
            SetUserOnlineInfo(player->GetUserId());
        }
        return true;
    }
    return false;
}

bool CTable::SaveReplayRecord(tagGameRecPlayback& replay) {

    return false;
}

void CTable::RefreshRechargeScore(std::shared_ptr<IPlayer> const& player) {
}

int64_t CTable::CalculateAgentRevenue(uint32_t chairId, int64_t revenue) {
    return 0;
}