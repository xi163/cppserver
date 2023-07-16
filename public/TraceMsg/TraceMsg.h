#ifndef INCLUDE_TRACEMSG_H
#define INCLUDE_TRACEMSG_H

#define TraceMessageID(mainId, subId) { \
	std::string s = strMessageID(mainId, subId, false, false); \
	if(!s.empty()) { \
		_LOG_DEBUG(s.c_str()); \
	} \
}

extern void initTraceMessageID();

extern std::string const strMessageID(
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat);

#endif