#ifndef INCLUDE_IREPLAYRECORD_H
#define INCLUDE_IREPLAYRECORD_H

struct tagGameRecPlayback;
struct tagGameReplay;

class IReplayRecord {
public:
	virtual bool SaveReplay(tagGameReplay& replay) = 0;
	virtual bool SaveReplayRecord(tagGameRecPlayback& replay) = 0;
};

#endif