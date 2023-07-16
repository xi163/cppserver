
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
/// IRobotDelegate��Ϣ�ص�
/// </summary>
bool CPlayer::SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}

/// <summary>
/// ITableDelegate��Ϣ�ص�
/// </summary>
bool CPlayer::SendTableMessage(uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}