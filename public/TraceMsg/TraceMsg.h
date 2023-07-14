#ifndef TRACEMSG_INCLUDE_H
#define TRACEMSG_INCLUDE_H

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MY_CMD_STR(n, s) { n, ""#n, s },

#define MY_TAB_MAP(var, MY_CMD_MAP_) \
	static struct { \
		int id_; \
		char const *name_; \
		char const* desc_; \
	}var[] = { \
		MY_CMD_MAP_(MY_CMD_STR) \
	}

#define MY_CMD_DESC(id, var, name, desc) \
for (int i = 0; i < ARRAYSIZE(var); ++i) { \
	if (var[i].id_ == id) { \
		name = var[i].name_; \
		desc = var[i].desc_; \
		break; \
	}\
}

//跟踪日志信息 mainId，subId
#define TraceMessageID(mainId, subId) { \
	std::string s = strMessageID(mainId, subId, false, false); \
	if(!s.empty()) { \
		LOG_DEBUG << "--- *** " << s; \
	} \
}

//格式化输入mainId，subId
extern std::string const strMessageID(
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat = false,
	bool trace_game_heartbeat = false);

#endif