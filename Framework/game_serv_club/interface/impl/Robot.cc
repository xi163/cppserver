
#include "public/gameConst.h"
#include "Robot.h"

#include "TableMgr.h"

#include "../../proto/Game.Common.pb.h"


CRobot::CRobot() {
	official_ = false;
}

CRobot::~CRobot() {
	official_ = false;
}

void CRobot::Init(std::shared_ptr<IRobotDelegate> const& robotDelegate) {
	robotDelegate_ = robotDelegate;
	robotDelegate_->SetPlayer(shared_from_this());
}

void CRobot::Reset() {
	if (robotDelegate_) {
		robotDelegate_->Reposition();
	}
	tableId_ = INVALID_TABLE;
	chairId_ = INVALID_CHAIR;
	status_ = sFree;
	trustee_ = false;
	official_ = false;
}

/// <summary>
/// IRobotDelegate消息回调
/// </summary>
bool CRobot::SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len) {
	if ((mainId == 0) || (mainId == Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC)) {
		if (robotDelegate_) {
			return robotDelegate_->OnGameMessage(subId, data, len);
		}
	}
	else {
	}
	return false;
}

/// <summary>
/// ITableDelegate消息回调
/// </summary>
bool CRobot::SendTableMessage(uint8_t subId, uint8_t const* data, size_t len) {
	if (INVALID_TABLE != tableId_) {
		std::shared_ptr<ITable> table = CTableMgr::get_mutable_instance().Get(tableId_);
		if (table) {
			return table->OnGameEvent(chairId_, subId, data, len);
		}
		else {
		}
	}
	else {
	}
	return false;
}

void CRobot::setReady() {
	if (INVALID_TABLE != tableId_) {
		std::shared_ptr<ITable> table = CTableMgr::get_mutable_instance().Get(tableId_);
		if (table) {
			uint16_t chairId = GetChairId();
			table->SetUserReady(chairId);
		}
		else {
		}
	}
	else {
	}
}