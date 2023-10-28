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
private:
	LuaState();
	~LuaState();
	bool init();
	int createBindFuncIndex();
	void openFuncMapping();
public:
	static inline void setWorkPath(std::string const& path) { workPath_ = path; }
public:
	static LuaState * create();
	static LuaState *instance(lua_State *L);
	inline lua_State* getL() { return L_; }
	void addSearchPath(char const* path);
	void registerFunc(char const* funcName, lua_CFunction cb);

	int bindFunc(int lo);
	bool setFuncByMapValues(int index);
	void removeFuncByMapValues(int index);
    
	inline void* getObject() { return obj_; }
	inline void setObject(void* obj) { obj_ = obj; }
	
	int loadFile(char const* filename);
	int execString(char const* s);
	int execFile(char const* filename);
	int call(int argc, int retc = LUA_MULTRET, STD::variant* result = nullptr);

	void pushNull();
	void pushInt(int val);
	void pushLong(long val);
	void pushFloat(float val);
	void pushBoolean(int val);
	void pushNumber(double val);
	void pushString(char const* val);
	void pushString(char const* val, size_t len);
	void pushUserData(void* val);
	void pushUserData(char const* name, void* val);
	void pushValue(int idx);
	
	bool testFunc(int idx);
	lua_CFunction toFunc(int idx);
	int toBoolean(int idx);
	double toNumber(int idx);
	long long toInteger(int idx);
	char const* toString(int idx);
	void* toUserdata(int idx);
	void const* toPointer(int idx);
	int getTable(int idx);
	int getGlobal(char const* name);
private:
	lua_State* L_;
	void* obj_;
	int funcId_; // 回调函数引用ID
	int funcValues_[MAX_FUNCTION_MAP_VALUES];// 函数映射的绑定值
	static std::string workPath_;// 程序脚本的主路径
};

#endif