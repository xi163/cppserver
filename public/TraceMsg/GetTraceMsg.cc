#include "public/Inc.h"

#include "TraceMsg.h"

#define NameStrMessageID "strMessageID"
#define NameFmtMessageID "fmtMessageID"

typedef int (*FnStrMessageID)(std::string&, std::string&, std::string&, std::string&, uint8_t, uint8_t, bool, bool);
typedef int (*FnFmtMessageID)(std::string&, uint8_t, uint8_t, bool, bool);

static FnStrMessageID fnStrMessageID;
static FnFmtMessageID fnFmtMessageID;

static inline void LoadLibrary(std::string const& serviceName) {
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
	fnStrMessageID = (FnStrMessageID)dlsym(handle, NameStrMessageID);
	if (!fnStrMessageID) {
		dlclose(handle);
		_LOG_ERROR("Can't Find %s, %s", NameStrMessageID, dlerror());
		exit(0);
	}
	fnFmtMessageID = (FnFmtMessageID)dlsym(handle, NameFmtMessageID);
	if (!fnFmtMessageID) {
		dlclose(handle);
		_LOG_ERROR("Can't Find %s, %s", NameFmtMessageID, dlerror());
		exit(0);
	}
}

void initTraceMessageID() {
	LoadLibrary("game_tracemsg");
}

void strMessageID(
	std::string& strMainID,
	std::string& strSubID,
	uint8_t mainId, uint8_t subId) {
	if (fnStrMessageID) {
		std::string strMainDesc, strSubDesc;
		fnStrMessageID(strMainID, strMainDesc, strSubID, strSubDesc, mainId, subId, false, false);
	}
}

int fmtMessageID(
	std::string& str,
	uint8_t mainId, uint8_t subId,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	return fnFmtMessageID ? fnFmtMessageID(str, mainId, subId, trace_hall_heartbeat, trace_game_heartbeat) : 0;
}

std::string const fmtMessageID(
	uint8_t mainId, uint8_t subId) {
	if (fnFmtMessageID) {
		std::string str;
		fnFmtMessageID(str, mainId, subId, false, false);
		return str;
	}
	return "";
}