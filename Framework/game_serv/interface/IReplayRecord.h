#ifndef INCLUDE_IREPLAYRECORD_H
#define INCLUDE_IREPLAYRECORD_H

struct tagGameRecPlayback;
struct tagGameReplay;

class IReplayRecord {
public:
	/// <summary>
	/// 保存游戏记录回放
	/// </summary>
	/// <param name="replay"></param>
	/// <returns></returns>
	virtual bool SaveReplay(tagGameReplay& replay) = 0;

	/// <summary>
	/// 保存游戏记录回放
	/// </summary>
	/// <param name="replay"></param>
	/// <returns></returns>
	virtual bool SaveReplayRecord(tagGameRecPlayback& replay) = 0;
};

#endif