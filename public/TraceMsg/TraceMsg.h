#ifndef INCLUDE_TRACEMSG_H
#define INCLUDE_TRACEMSG_H

#define TraceMessageID(mainId, subId) { \
	int lvl = LVL_DEBUG; \
	std::string s = strMessageID(lvl, mainId, subId, false, false); \
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

extern std::string const strMessageID(
	int& lvl,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat);

#endif