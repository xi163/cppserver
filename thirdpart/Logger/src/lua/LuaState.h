#ifndef INCLUDE_LUASTATE_H
#define INCLUDE_LUASTATE_H

#include "Logger/src/Macro.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// 最大的LUA函数映射值集合
#define MAX_FUNCTION_MAP_VALUES 1024
// LUA函数映射字符串定义
#define LUASTATE_FUNCTION_MAPPING "G_LuaStateFuncMapping"

class LuaState {
public:
	LuaState();
	~LuaState();
private:
	bool init();
	int initfuncindex();
	void initfuncmapping();
public:
	static inline void setscriptpath(std::string const& path) { scriptPath_ = path; }
public:
	static LuaState *instance(lua_State *L);
	inline lua_State* getL() { return L_; }
	void addsearchpath(char const* path);
	
	void registerfunction(char const* funcName, lua_CFunction cb);
	int bindfunction(int lo);
	bool setfunction(int index);
	void removefunction(int index);
    
	inline void* getobject() { return obj_; }
	inline void setobject(void* obj) { obj_ = obj; }
	
	int loadfile(char const* filename);
	int callstring(char const* s);
	int dofile(char const* filename);
	int call(int argc, int retc = LUA_MULTRET, STD::variant* result = nullptr);
	
	void pushnil();
	void pushinteger(int val);
	void pushlong(long val);
	void pushfloat(float val);
	void pushboolean(int val);
	void pushnumber(double val);
	int loadstring(char const* s);
	void pushstring(char const* s);
	void pushstring(char const* s, size_t len);
	char const* pushfstring(char const* fmt, ...);
	void pushlightuserdata(void* val);
	void pushlightuserdata(char const* name, void* val);
	void pushvalue(int idx);
	void* newuserdata(size_t size);

	char const* gettypename(int idx);
	bool testfunction(int idx);
	lua_CFunction tofunction(int idx);
	int toboolean(int idx);
	double tonumber(int idx);
	long long tointeger(int idx);
	char const* tostring(int idx);
	void* touserdata(int idx);
	void const* topointer(int idx);
	int rawget(int idx);
	void rawset(int idx);
	void newtable();
	int gettable(int idx);
	void setglobal(char const* name);
	int getglobal(char const* name);
	void setfield(int idx, char const* key);
	int getfield(int idx, char const* key);
	void settop(int idx);
	int gettop();
	void clearstack();
	void pop(int n);
	void insert(int idx);
	void remove(int idx);
private:
	lua_State* L_;
	void* obj_;
	int funcId_; // 回调函数引用ID
	int funcValues_[MAX_FUNCTION_MAP_VALUES];// 函数映射的绑定值
	static std::string scriptPath_;// 程序脚本的主路径
};

extern void lua_test();

#endif