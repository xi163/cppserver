
#include "Player.h"

CPlayer::CPlayer() {
	tableId_ = INVALID_TABLE;
	chairId_ = INVALID_CHAIR;
	status_ = sFree;
	trustee_ = false;
	official_ = false;
}

void CPlayer::Reset() {
	tableId_ = INVALID_TABLE;
	chairId_ = INVALID_CHAIR;
	status_ = sFree;
	trustee_ = false;
	official_ = false;
	baseInfo_ = UserBaseInfo();
}

/// <summary>
/// IRobotDelegate消息回调
/// </summary>
bool CPlayer::SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}

/// <summary>
/// ITableDelegate消息回调
/// </summary>
bool CPlayer::SendTableMessage(uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}