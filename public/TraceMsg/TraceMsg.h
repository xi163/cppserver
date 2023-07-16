#ifndef INCLUDE_TRACEMSG_H
#define INCLUDE_TRACEMSG_H

#define TraceMessageID(mainId, subId) { \
	std::string s; \
	int lvl = fmtMessageID(s, mainId, subId, false, false); \
	if(!s.empty()) { \
		switch(lvl) { \
			case LVL_DEBUG: \
				_LOG_DEBUG(s.c_str()); \
				break; \
			case LVL_TRACE: \
				_LOG_TRACE(s.c_str()); \
				break; \
			case LVL_INFO: \
				_LOG_INFO(s.c_str()); \
				break; \
			case LVL_WARN: \
				_LOG_WARN(s.c_str()); \
				break; \
			case LVL_ERROR: \
				_LOG_ERROR(s.c_str()); \
				break; \
		} \
	} \
}

extern void initTraceMessageID();

extern void strMessageID(
	std::string& strMainID,
	std::string& strSubID,
	uint8_t mainId, uint8_t subId);

extern int fmtMessageID(
	std::string& str,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat);

extern std::string const fmtMessageID(
	uint8_t mainId, uint8_t subId);

#endif