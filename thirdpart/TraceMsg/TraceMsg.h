#ifndef INCLUDE_GETTRACEMSG_H
#define INCLUDE_GETTRACEMSG_H

#include "Logger/src/log/Logger.h"

#define TraceMessageId(mainId, subId) { \
	std::string s; \
	int lvl = fmtMessageId(s, mainId, subId, false, false); \
	if(!s.empty()) { \
		switch(lvl) { \
			case LVL_DEBUG: \
				Debugf(s.c_str()); \
				break; \
			case LVL_TRACE: \
				Tracef(s.c_str()); \
				break; \
			case LVL_INFO: \
				Infof(s.c_str()); \
				break; \
			case LVL_WARN: \
				Warnf(s.c_str()); \
				break; \
			case LVL_ERROR: \
				Errorf(s.c_str()); \
				break; \
		} \
	} \
}

extern void initTraceMessage();

extern void strMessageId(
	std::string& strMainID,
	std::string& strSubID,
	uint8_t mainId, uint8_t subId);

extern int fmtMessageId(
	std::string& str,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat);

extern std::string const fmtMessageId(
	uint8_t mainId, uint8_t subId);

#endif