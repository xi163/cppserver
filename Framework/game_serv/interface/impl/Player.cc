#include "public/Inc.h"
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
/// <param name="mainId"></param>
/// <param name="subId"></param>
/// <param name="data"></param>
/// <param name="len"></param>
/// <returns></returns>
bool CPlayer::SendUserMessage(uint8_t mainId, uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}

/// <summary>
/// ITableDelegate消息回调
/// </summary>
/// <param name="subId"></param>
/// <param name="data"></param>
/// <param name="len"></param>
/// <returns></returns>
bool CPlayer::SendTableMessage(uint8_t subId, uint8_t const* data, size_t len) {
	return false;
}

bool CPlayer::ExistOnlineInfo() {
	ASSERT(baseInfo_.userId > 0);
	return REDISCLIENT.ExistOnlineInfo(baseInfo_.userId);
}