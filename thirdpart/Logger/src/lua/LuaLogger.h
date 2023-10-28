#ifndef INCLUDE_LUALOGGER_H
#define INCLUDE_LUALOGGER_H

#include "Logger/src/Macro.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaLogger {
public:
	static void luaRegister(lua_State* L);
public:
	static int luaSetTimezone(lua_State* L);
	static int luaSetLevel(lua_State* L);
	static int luaSetMode(lua_State* L);
	static int luaSetStyle(lua_State* L);
	static int luaInit(lua_State* L);
	static int luaFatal(lua_State* L);
	static int luaError(lua_State* L);
	static int luaWarn(lua_State* L);
	static int luaCritical(lua_State* L);
	static int luaInfo(lua_State* L);
	static int luaDebug(lua_State* L);
	static int luaTrace(lua_State* L);
	static int luaExcept(lua_State* L);
	static int luaFatalExcept(lua_State* L);
};

#endif