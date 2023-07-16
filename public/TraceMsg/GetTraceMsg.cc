#include "public/Inc.h"

#include "TraceMsg.h"

#define NameTraceMessageID "traceMessageID"

typedef std::string const (*fnTraceMessageID)(uint8_t, uint8_t, bool, bool);

static fnTraceMessageID fnStrMessageID;

static inline fnTraceMessageID loadTraceMessageID(std::string const& serviceName) {
	std::string dllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
	dllPath.append("/");
	//./libgame_tracemsg.so
	std::string dllName = clearDllPrefix(serviceName);
	dllName.insert(0, "./lib");
	dllName.append(".so");
	dllName.insert(0, dllPath);
	_LOG_WARN(dllName.c_str());
	//getchar();
	void* handle = dlopen(dllName.c_str(), RTLD_LAZY);
	if (!handle) {
		_LOG_ERROR("Can't Open %s, %s", dllName.c_str(), dlerror());
		exit(0);
	}
	fnTraceMessageID fn = (fnTraceMessageID)dlsym(handle, NameTraceMessageID);
	if (!fn) {
		dlclose(ha^ndle);
		_LOG_ERROR("Can't Find %s, %s", NameTraceMessageID, dlerror());
		exit(0);
	}
	return fn;
}

void initTraceMessageID() {
	fnStrMessageID = loadTraceMessageID("game_tracemsg");
}

std::string const strMessageID(
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	return fnStrMessageID ? fnStrMessageID(mainId, subId, trace_hall_heartbeat, trace_game_heartbeat) : "";
}