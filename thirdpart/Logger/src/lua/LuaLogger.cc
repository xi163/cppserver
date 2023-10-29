#include "Logger/src/utils/utils.h"
#include "LuaLogger.h"

int LuaLogger::luaSetTimezone(lua_State* L) {
	int timezone = (int)lua_tonumber(L, 1);
	_LOG_SET_TIMEZONE(timezone);
	return 0;
}

int LuaLogger::luaSetLevel(lua_State* L) {
	int level = (int)lua_tonumber(L, 1);
	_LOG_SET_LEVEL(level);
	return 0;
}

int LuaLogger::luaSetMode(lua_State* L) {
	int logmode = (int)lua_tonumber(L, 1);
	_LOG_SET_MODE(logmode);
	return 0;
}

int LuaLogger::luaSetStyle(lua_State* L) {
	int logstyle = (int)lua_tonumber(L, 1);
	_LOG_SET_STYLE(logstyle);
	return 0;
}

int LuaLogger::luaInit(lua_State* L) {
	char const* logdir = lua_tostring(L, 1);
	char const* logname = lua_tostring(L, 2);
	long long logsize = (long long)lua_tonumber(L, 3);
	_LOG_INIT(logdir, logname, logsize);
	return 0;
}

// https://www.cnblogs.com/heyongqi/p/6816274.html
static inline std::string lua_FILE(lua_State* L) {
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "S", &ar);
	ASSERT(ar.source);
	std::string s(ar.source);
	return s.substr(s.find_last_of(SYS_G) + 1, -1);
}

static inline int lua_LINE(lua_State* L) {
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "l", &ar);
	return ar.currentline;
}

static inline char const* lua_FUNC(lua_State* L) {
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "n", &ar);
	return ar.name ? ar.name : "FUNC";
}

int LuaLogger::luaFatal(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Fatal_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaError(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Error_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaWarn(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Warn_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaCritical(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Critical_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaInfo(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Info_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaDebug(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Debug_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaTrace(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	std::string s;
	s.append(lua_FILE(L));
	s.append(":");
	s.append(std::to_string(lua_LINE(L)));
	s.append(" ");
	s.append(lua_FUNC(L));
	s.append(" ");
	s.append(msg);
	Trace_tmsp_thrd(s);
	return 0;
}

int LuaLogger::luaExcept(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	Error_tmsp_thrd(msg);
	return 0;
}

int LuaLogger::luaFatalExcept(lua_State* L) {
	char const* msg = lua_tostring(L, 1);
	Fatal_tmsp_thrd(msg);
	return 0;
}

static int luaL_openlib(lua_State* L) {
	static const struct luaL_Reg regFunc[] = {
		{ "setTimezone", LuaLogger::luaSetTimezone},
		{ "setLevel", LuaLogger::luaSetLevel},
		{ "setMode", LuaLogger::luaSetMode},
		{ "init", LuaLogger::luaInit},
		{ "fatal", LuaLogger::luaFatal },
		{ "error", LuaLogger::luaError },
		{ "warn", LuaLogger::luaWarn },
		{ "critical", LuaLogger::luaCritical },
		{ "info", LuaLogger::luaInfo },
		{ "debug", LuaLogger::luaDebug },
		{ "trace", LuaLogger::luaTrace },
		{ "except", LuaLogger::luaExcept },
		{ "fatalExcept", LuaLogger::luaFatalExcept },
		{NULL,NULL}
	};
	//Tracef("top=%d", lua_gettop(L)); // 1
	luaL_newlib(L, regFunc);
	//Tracef("top=%d", lua_gettop(L)); // 2
	return 1;
}

void LuaLogger::luaRegister(lua_State* L) {
 	ASSERT(L);
	//_LOG_SET_STYLE(F_THRD);
	//_LOG_SET_STYLE(F_TMSTMP_THRD);
	//_LOG_SET_STYLE(F_PURE);
	//Tracef("top=%d", lua_gettop(L)); // 0
	luaL_requiref(L, "logger", luaL_openlib, 1);
	//Tracef("top=%d", lua_gettop(L));// 1
	lua_pop(L, 1);
}