#include "Logger/src/utils/utils.h"

#include "TraceMsg/TraceMsg.h"

#define NameStrMessageId "strMessageId"
#define NameFmtMessageId "fmtMessageId"

typedef int (*FnStrMessageId)(std::string&, std::string&, std::string&, std::string&, uint8_t, uint8_t, bool, bool);
typedef int (*FnFmtMessageId)(std::string&, uint8_t, uint8_t, bool, bool);

static FnStrMessageId fnStrMessageId;
static FnFmtMessageId fnFmtMessageId;

static inline void LoadLibrary(std::string const& serviceName) {
	std::string dllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
	dllPath.append("/");
	//./libgame_tracemsg.so
	std::string dllName = utils::clearDllPrefix(serviceName);
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
	fnStrMessageId = (FnStrMessageId)dlsym(handle, NameStrMessageId);
	if (!fnStrMessageId) {
		dlclose(handle);
		_LOG_ERROR("Can't Find %s, %s", NameStrMessageId, dlerror());
		exit(0);
	}
	fnFmtMessageId = (FnFmtMessageId)dlsym(handle, NameFmtMessageId);
	if (!fnFmtMessageId) {
		dlclose(handle);
		_LOG_ERROR("Can't Find %s, %s", NameFmtMessageId, dlerror());
		exit(0);
	}
}

void initTraceMessage() {
	LoadLibrary("game_tracemsg");
}

void strMessageId(
	std::string& strMainID,
	std::string& strSubID,
	uint8_t mainId, uint8_t subId) {
	if (fnStrMessageId) {
		std::string strMainDesc, strSubDesc;
		fnStrMessageId(strMainID, strMainDesc, strSubID, strSubDesc, mainId, subId, false, false);
	}
}

int fmtMessageId(
	std::string& str,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	return fnFmtMessageId ? fnFmtMessageId(str, mainId, subId, trace_hall_heartbeat, trace_game_heartbeat) : 0;
}

std::string const fmtMessageId(
	uint8_t mainId, uint8_t subId) {
	if (fnFmtMessageId) {
		std::string str;
		fnFmtMessageId(str, mainId, subId, false, false);
		return str;
	}
	return "";
}