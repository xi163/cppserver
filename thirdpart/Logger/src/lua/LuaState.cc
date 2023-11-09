#include "Logger/src/utils/utils.h"
//#include "lua_libs.h"
#include "LuaState.h"

#define _LUA_TEST_

std::string LuaState::scriptPath_ = utils::getModulePath();

static inline char const* gettypename(lua_State* L_, int idx) {
	return lua_typename(L_, lua_type(L_, idx));
}

static inline int getglobal(lua_State* L_, char const* name) {
#ifdef _LUA_TEST_
	int v = lua_getglobal(L_, name);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), name, gettypename(L_, lua_gettop(L_)));
	return v;
#else
	return lua_getglobal(L_, name);
#endif
}

static inline void const* topointer(lua_State* L_, int idx) {
	if (lua_islightuserdata(L_, idx)) {
#ifdef _LUA_TEST_
		void const* v = lua_topointer(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_islightuserdata(L_, lua_gettop(L_)));
		ASSERT(lua_isuserdata(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(L_, lua_gettop(L_)));
		return v;
#else
		return lua_topointer(L_, idx);
#endif
	}
	return NULL;
}

static inline void pop(lua_State* L_, int n) {
	ASSERT_V(n > 0, "n=%d", n);
	//ASSERT_V(n <= lua_gettop(L_), "n=%d top=%d", n, lua_gettop(L_));
	if (n <= lua_gettop(L_)) {
		Tracef("{S} %d top=%d %s", n, lua_gettop(L_), gettypename(L_, lua_gettop(L_)));
		lua_pop(L_, n);
		Tracef("{E} %d top=%d %s", n, lua_gettop(L_), gettypename(L_, lua_gettop(L_)));
	}
	else if (lua_gettop(L_) > 0) {
		Tracef("{S} %d top=%d %s", n, lua_gettop(L_), gettypename(L_, lua_gettop(L_)));
		lua_pop(L_, lua_gettop(L_));
		Tracef("{E} %d top=%d %s", n, lua_gettop(L_), gettypename(L_, lua_gettop(L_)));
	}
}

LuaState::LuaState()
	: L_(NULL)
	, funcId_(-1)
	, obj_(NULL) {
	memset(funcValues_, 0, sizeof funcValues_);
	init();
}

LuaState::~LuaState() {
	Tracef("...");
	if (L_ != NULL) {
		lua_close(L_);
	}
}

LuaState* LuaState::instance(lua_State* L) {
	::getglobal(L, "G_LuaStateInst");
	if (lua_isnil(L, lua_gettop(L))) {
		return NULL;
	}
	LuaState* ptr = (LuaState*)(::topointer(L, lua_gettop(L)));
	ASSERT(ptr);
	::pop(L, 1);
	return ptr;
}

bool LuaState::init() {
	L_ = luaL_newstate();
	ASSERT(L_);
	luaL_openlibs(L_);
	//luaopen_cjson(L_);
	//luaopen_luasql_mysql(L_);

	initfuncmapping();

	int sumMapValues = MAX_FUNCTION_MAP_VALUES;
	while (--sumMapValues >= 0) {
		funcValues_[sumMapValues] = LUA_REFNIL;
	}

	funcId_ = -1;
	obj_ = NULL;
	pushlightuserdata("G_LuaStateInst", this);
	std::string searchPath;
	// 添加LUA搜索路径
	std::string modulePath = utils::getModulePath();
	searchPath.append(modulePath)
		.append(SYS_G_STR)
		.append("..")
		.append(SYS_G_STR)
		.append("scripts")
		.append(SYS_G_STR)
		.append("public")
		.append(SYS_G_STR)
		.append("?.lua;");
	searchPath.append(modulePath)
		.append(SYS_G_STR)
		.append("config")
		.append(SYS_G_STR)
		.append("?.lua;");
	searchPath.append(modulePath)
		.append(SYS_G_STR)
		.append("?.lua;");
	if (!scriptPath_.empty() && scriptPath_ != modulePath) {
		searchPath.append(scriptPath_)
			.append(SYS_G_STR)
			.append("config")
			.append(SYS_G_STR)
			.append("?.lua;");
		searchPath.append(scriptPath_)
			.append(SYS_G_STR)
			.append("?.lua;");
	}
	this->addsearchpath(searchPath.c_str());
	ASSERT(lua_gettop(L_) == 0);
	return true;
}

void LuaState::addsearchpath(char const* path) {
	Tracef("%s", path);
	getglobal("package");
	getfield(-1, "path");
	// 获取LUA环境字段的值(package.path)
	char const* curPath = tostring(-1);
	// 设置新的搜索路径到LUA环境字段(package.path)
	pushfstring("%s;%s", curPath, path);
	setfield(-3, "path");
	pop(2);
	ASSERT(lua_gettop(L_) == 0);
}

void LuaState::initfuncmapping() {
	pushstring(LUASTATE_FUNCTION_MAPPING);
	newtable();
	rawset(LUA_REGISTRYINDEX);
	ASSERT(lua_gettop(L_) == 0);
}

void LuaState::registerfunction(char const* funcName, lua_CFunction cb) {
	lua_register(L_, funcName, cb);
}

int LuaState::initfuncindex() {
	int count = 0;
	while (count < MAX_FUNCTION_MAP_VALUES) {
		funcId_ = (funcId_ + 1) % MAX_FUNCTION_MAP_VALUES;
		if (funcValues_[funcId_] == LUA_REFNIL)
			return funcId_;
		++count;
	}
	return LUA_REFNIL;
}

int LuaState::bindfunction(int lo) {
	// 获取新的函数ID
	int val = initfuncindex();
	if (val == LUA_REFNIL) {
		return LUA_REFNIL;
	}
	// 检查当前是否为有效的函数类型
	if (lua_isfunction(L_, lo)) {
		pushstring(LUASTATE_FUNCTION_MAPPING);
		rawget(LUA_REGISTRYINDEX);
		pushinteger(val);
		pushvalue(lo);
		rawset(-3);
		pop(1);
	}

	// 添加函数句柄设置到函数映射值内
	if (val >= 0 && val < MAX_FUNCTION_MAP_VALUES) {
		funcValues_[val] = val;
	}
	return val;
}

void LuaState::removefunction(int index) {
	int val = LUA_REFNIL;
	// 获取函数的句柄ID
	if (index >= 0 && index < MAX_FUNCTION_MAP_VALUES) {
		val = funcValues_[index];
		funcValues_[index] = LUA_REFNIL;
	}
	// 从函数映射表内移除指定的函数
	pushstring(LUASTATE_FUNCTION_MAPPING);
	rawget(LUA_REGISTRYINDEX);
	pushinteger(val);
	pushnil();
	rawset(-3);
	pop(1);
}

int LuaState::callstring(char const* s) {
	loadstring(s);
	return call(0);
}

int LuaState::loadfile(char const* filename) {
	std::string scriptFile = utils::combineFilePath(scriptPath_, filename);
	int rc = luaL_loadfile(L_, scriptFile.c_str());
	Tracef("top=%d %s %s", lua_gettop(L_), scriptFile.c_str(), gettypename(lua_gettop(L_)));
	if (rc != 0) {
		Errorf("%s", lua_tostring(L_, -1));
		pop(1);
	}
	return rc;
}

int LuaState::dofile(char const* filename) {
	std::string scriptFile = utils::combineFilePath(scriptPath_, filename);
	int rc = luaL_dofile(L_, scriptFile.c_str());
	Tracef("top=%d %s %s", lua_gettop(L_), scriptFile.c_str(), gettypename(lua_gettop(L_)));
	if (rc != 0) {
		Errorf("%s", lua_tostring(L_, -1));
		pop(1);
	}
	return rc;
}

char const* LuaState::gettypename(int idx) {
	return lua_typename(L_, lua_type(L_, idx));
}

bool LuaState::setfunction(int index) {
	int val = LUA_REFNIL;
	if (index >= 0 && index < MAX_FUNCTION_MAP_VALUES) {
		val = funcValues_[index];
	}
	if (val == LUA_REFNIL) {
		return false;
	}

	// 设置LUA回调函数
	pushstring(LUASTATE_FUNCTION_MAPPING);
	rawget(LUA_REGISTRYINDEX);
	pushinteger(val);
	rawget(-2);
	remove(-2);
	return true;
}

bool LuaState::testfunction(int idx) {
	if (lua_isfunction(L_, idx)) {
		return true;
	}
	pop(1);
	return false;
}

lua_CFunction LuaState::tofunction(int idx) {
	if (lua_isfunction(L_, idx)) {
#ifdef _LUA_TEST_
		lua_CFunction p = lua_tocfunction(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isfunction(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return p;
#else
		return lua_tocfunction(L_, idx);
#endif
	}
	return NULL;
}

int LuaState::toboolean(int idx) {
	if (lua_isboolean(L_, idx)) {
#ifdef _LUA_TEST_
		int v = lua_toboolean(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isboolean(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_toboolean(L_, idx);
#endif
	}
	return 0;
}

double LuaState::tonumber(int idx) {
	if (lua_isnumber(L_, idx)) {
#ifdef _LUA_TEST_
		int v = lua_tonumber(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isnumber(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_tonumber(L_, idx);
#endif
	}
	return 0;
}

long long LuaState::tointeger(int idx) {
	if (lua_isinteger(L_, idx)) {
#ifdef _LUA_TEST_
		long long v = lua_tointeger(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isinteger(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_tointeger(L_, idx);
#endif
	}
	return 0;
}

char const* LuaState::tostring(int idx) {
	if (lua_isstring(L_, idx)) {
#ifdef _LUA_TEST_
		char const* v = lua_tostring(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isstring(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_tostring(L_, idx);
#endif
	}
	return "(null)";
}

void* LuaState::touserdata(int idx) {
	if (lua_isuserdata(L_, idx)) {
#ifdef _LUA_TEST_
		void* v = lua_touserdata(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_isuserdata(L_, lua_gettop(L_)));
		ASSERT(lua_islightuserdata(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_touserdata(L_, idx);
#endif
	}
	return NULL;
}

void const* LuaState::topointer(int idx) {
	if (lua_islightuserdata(L_, idx)) {
#ifdef _LUA_TEST_
		void const* v = lua_topointer(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		ASSERT(lua_islightuserdata(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_topointer(L_, idx);
#endif
	}
	return NULL;
}

int LuaState::rawget(int idx) {
#ifdef _LUA_TEST_
	int v = lua_rawget(L_, idx);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
	return v;
#else
	return lua_rawget(L_, idx);
#endif
}

void LuaState::rawset(int idx) {
#ifdef _LUA_TEST_
	lua_rawset(L_, idx);
	//ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
#else
	lua_rawset(L_, idx);
#endif
}

void LuaState::newtable() {
#ifdef _LUA_TEST_
	lua_newtable(L_);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
#else
	lua_newtable(L_);
#endif
}

int LuaState::gettable(int idx) {
	if (lua_istable(L_, idx)) {
#ifdef _LUA_TEST_
		int v = lua_gettable(L_, idx);
		ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
		//ASSERT(lua_istable(L_, lua_gettop(L_)));
		Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
		return v;
#else
		return lua_gettable(L_, idx);
#endif
	}
	return -1;
}

void LuaState::setglobal(char const* name) {
#ifdef _LUA_TEST_
	lua_setglobal(L_, name);
	//-1:nil 0:userdata
	//ASSERT_V(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)),
	//	"-1:%s %d:%s",
	//	gettypename(-1), lua_gettop(L_), gettypename(lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), name, gettypename(lua_gettop(L_)));
#else
	lua_setglobal(L_, name);
#endif
}

int LuaState::getglobal(char const* name) {
#ifdef _LUA_TEST_
	int v = lua_getglobal(L_, name);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	//ASSERT(lua_isstring(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), name, gettypename(lua_gettop(L_)));
	return v;
#else
	return lua_getglobal(L_, name);
#endif
}

int LuaState::getfield(int idx, char const* key) {
#ifdef _LUA_TEST_
	int v = lua_getfield(L_, idx, key);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), key, gettypename(lua_gettop(L_)));
	return v;
#else
	return lua_getfield(L_, idx, key);
#endif
}

void LuaState::setfield(int idx, char const* key) {
#ifdef _LUA_TEST_
	lua_setfield(L_, idx, key);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), key, gettypename(lua_gettop(L_)));
#else
	lua_setfield(L_, idx, key);
#endif
}

//idx <  -1 可能异常
//idx = 0-1 原样 lua_pop(L_, 0);
//idx =   0 清空
//idx >   0 栈高
void LuaState::settop(int idx) {
	if (idx < -1) {
		int n = -(idx + 1);
		ASSERT_V(n <= lua_gettop(L_), "n=%d top=%d", n, lua_gettop(L_));
		if (n <= lua_gettop(L_)) {
			lua_settop(L_, idx);
		}
	}
	else {
		lua_settop(L_, idx);
	}
}

//top >= 0
int LuaState::gettop() {
	return lua_gettop(L_);
}

void LuaState::clearstack() {
	//lua_pop(L_, lua_gettop(L_));
	lua_settop(L_, 0);
}

void LuaState::pop(int n) {
	ASSERT_V(n > 0, "n=%d", n);
	//ASSERT_V(n <= lua_gettop(L_), "n=%d top=%d", n, lua_gettop(L_));
	if (n <= lua_gettop(L_)) {
		Tracef("{S} %d top=%d %s", n, lua_gettop(L_), gettypename(lua_gettop(L_)));
		lua_pop(L_, n);
		Tracef("{E} %d top=%d %s", n, lua_gettop(L_), gettypename(lua_gettop(L_)));
	}
	else if (lua_gettop(L_) > 0) {
		Tracef("{S} %d top=%d %s", n, lua_gettop(L_), gettypename(lua_gettop(L_)));
		lua_pop(L_, lua_gettop(L_));
		Tracef("{E} %d top=%d %s", n, lua_gettop(L_), gettypename(lua_gettop(L_)));
	}
}

void LuaState::insert(int idx) {
	lua_insert(L_, idx);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::remove(int idx) {
	lua_remove(L_, idx);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushnil() {
	lua_pushnil(L_);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isnil(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushinteger(int val) {
	lua_pushinteger(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isinteger(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushfloat(float val) {
	lua_pushnumber(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isnumber(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushlong(long val) {
	lua_pushnumber(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isnumber(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushboolean(int val) {
	lua_pushboolean(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isboolean(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushnumber(double val) {
	lua_pushnumber(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isnumber(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void LuaState::pushstring(char const* s) {
	lua_pushstring(L_, s);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isstring(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), s, gettypename(lua_gettop(L_)));
}

void LuaState::pushstring(char const* s, size_t len) {
	lua_pushlstring(L_, s, len);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isstring(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), s, gettypename(lua_gettop(L_)));
}

char const* LuaState::pushfstring(char const* format, ...) {
#if 0
	va_list ap;
	va_start(ap, format);
	char const* s = lua_pushfstring(L_, format, ap);
	va_end(ap);
#else
	char s[BUFSZ] = { 0 };
	va_list ap;
	va_start(ap, format);
#ifdef _windows_
	size_t n = vsnprintf_s(s, BUFSZ, _TRUNCATE, format, ap);
#else
	size_t n = vsnprintf(s, BUFSZ, format, ap);
#endif
	va_end(ap);
	(void)n;
	lua_pushstring(L_, s);
#endif
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isstring(L_, lua_gettop(L_)));
	Tracef("top=%d %s %s", lua_gettop(L_), s, gettypename(lua_gettop(L_)));
}

int LuaState::loadstring(char const* s) {
#ifdef _LUA_TEST_
	int v = luaL_loadstring(L_, s);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	ASSERT(lua_isstring(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
	return v;
#else
	return luaL_loadstring(L_, s);
#endif
}

void LuaState::pushlightuserdata(void* val) {
	lua_pushlightuserdata(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));// 1
}

void LuaState::pushlightuserdata(char const* name, void* val) {
	lua_pushlightuserdata(L_, val);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));// 1
	setglobal(name);
}

void LuaState::pushvalue(int idx) {
	lua_pushvalue(L_, idx);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
}

void* LuaState::newuserdata(size_t size) {
#ifdef _LUA_TEST_
	void* v = lua_newuserdata(L_, size);
	ASSERT(lua_type(L_, -1) == lua_type(L_, lua_gettop(L_)));
	Tracef("top=%d %s", lua_gettop(L_), gettypename(lua_gettop(L_)));
	return v;
#else
	return lua_newuserdata(L_, size);
#endif
}

int LuaState::call(int argc, int retc, STD::variant* result) {
	int index = -(argc + 1);
	// 查检是否为函数类型
	if (!lua_isfunction(L_, index)) {
		Errorf("stack[%d] is not function", index);
		pop(argc + 1);
		return -1;
	}
	int traceback = 0;
	// 在main.lua中自定义错误追踪函数
	getglobal("__G__TRACKBACK__");
	if (!lua_isfunction(L_, -1)) {
		pop(1);
	}
	else {
		traceback = index - 1;
		insert(traceback);
	}
	int rc = -1;
	MY_TRY();
	ASSERT(lua_gettop(L_) > 0);
	ASSERT(lua_isfunction(L_, -1 - argc));
	TEST(CALLBACK_0([this]() {
		for (int i = 1; i <= lua_gettop(L_); ++i) {
			Tracef("idx=%d type -> %s", i, gettypename(i));
		}
	}));
	if ((rc = lua_pcall(L_, argc, retc, traceback)) != 0) {
		if (traceback == 0) {
			Errorf("%s", lua_tostring(L_, -1));
			pop(1);
		}
		else {
			Warnf("%s", lua_tostring(L_, -1));
			pop(2);
		}
		return rc;
	}
	TEST(CALLBACK_0([this](int traceback) {
		if (lua_gettop(L_) > 0) {
			for (int i = 1; i <= lua_gettop(L_); ++i) {
				Tracef("idx=%d rctype -> %s traceback=%d", i, gettypename(i), traceback);
			}
		}
		else {
			Tracef("top=%d rctype -> %s traceback=%d", lua_gettop(L_), gettypename(-1), traceback);
		}
	}, traceback));
	if (lua_isboolean(L_, -1)) {
		if (result) {
			*result = (int)lua_toboolean(L_, -1);
		}
	}
	else if (lua_isinteger(L_, -1)) {
		if (result) {
			*result = (int)lua_tointeger(L_, -1);
		}
	}
	else if (lua_isnumber(L_, -1)) {
		if (result) {
			*result = (int)lua_tonumber(L_, -1);
		}
	}
	else if (lua_isstring(L_, -1)) {
		if (result) {
			*result = lua_tostring(L_, -1);
		}
	}
	else if (lua_istable(L_, -1)) {
	}
	else if (lua_isuserdata(L_, -1)) {
		//lua_touserdata(L_, -1);
	}
	else if (lua_islightuserdata(L_, -1)) {
		//lua_topointer(L_, -1);
	}
	else if (lua_isfunction(L_, -1)) {
	}
	else if (!lua_isnil(L_, -1)) {
	}
	// 弹出栈顶返回值
	//if (retc == LUA_MULTRET || retc > 0) {
		pop(1);
	//}
	// 弹出栈顶错误追踪函数
	if (traceback) {
		pop(1);
	}
	MY_CATCH();
	Tracef("top=%d", lua_gettop(L_));
	ASSERT_V(lua_gettop(L_) == 0, "top=%d", lua_gettop(L_));
	return rc;
}

void lua_test() {
#if 1
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	ASSERT(lua_gettop(L) == 0 && lua_isnil(L, -1));
	lua_settop(L, 1);
	ASSERT(lua_gettop(L) > 0 && lua_isnil(L, -1));
	
	lua_settop(L, 0);
	Debugf("top=%d", lua_gettop(L)); // 0
	lua_pushstring(L, "1");
	Debugf("top=%d", lua_gettop(L)); // 1
	lua_pushstring(L, "2");
	Debugf("top=%d", lua_gettop(L)); // 2
	lua_pushstring(L, "3");
	Debugf("top=%d", lua_gettop(L));
	lua_pushstring(L, "4");
	Debugf("top=%d", lua_gettop(L));
	lua_pushstring(L, "5");
	Debugf("top=%d", lua_gettop(L));

	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, 5), lua_tostring(L, 1));// 5 1
	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, -1), lua_tostring(L, -5));// 5 1

	//lua_settop(L, -1);
	//Debugf("top=%d", lua_gettop(L)); // 0
	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, -1), lua_tostring(L,  1));// 5 1

	//lua_settop(L, -2);
	//Debugf("top=%d", lua_gettop(L)); // 4
	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, -1), lua_tostring(L, 1));// 4 1

	//lua_settop(L, 2);
	//Debugf("top=%d", lua_gettop(L)); // 2
	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, -1), lua_tostring(L, 1));// 2 1

	//lua_settop(L, 3);
	//Debugf("top=%d", lua_gettop(L)); // 3
	//Debugf("栈顶:%s 栈底:%s", lua_tostring(L, -1), lua_tostring(L, 1));// null 1
	
	//lua_settop(L, -2);
	//Debugf("top=%d", lua_gettop(L)); // 1

	//lua_settop(L, -lua_gettop(L)-1);
	//Debugf("top=%d", lua_gettop(L)); // 0

	lua_settop(L, -5-1);
	Debugf("top=%d", lua_gettop(L)); // 0

	//lua_settop(L, -6-1);
	//Debugf("top=%d", lua_gettop(L)); // crash
#else
	LuaState::setscriptpath("/home/lua_scripts");
	std::unique_ptr<LuaState> lua(new LuaState());
	//LuaLogger::luaRegister(lua->getL());

	//lua->settop(-1);
	Debugf("top=%d", lua->gettop()); // 0
	lua->pushstring("1");
	Debugf("top=%d", lua->gettop());
	lua->pushstring("2");
	Debugf("top=%d", lua->gettop());
	lua->pushstring("3");
	Debugf("top=%d", lua->gettop());
	lua->pushstring("4");
	Debugf("top=%d", lua->gettop());
	lua->pushstring("5");
	Debugf("top=%d", lua->gettop());

	Debugf("栈顶:%s 栈底:%s", lua->tostring(5), lua->tostring(1));// 5 1
	Debugf("栈顶:%s 栈底:%s", lua->tostring(-1), lua->tostring(-5));// 5 1

	//lua->settop(-1);
	//Debugf("top=%d", lua->gettop()); // 0
	//Debugf("栈顶:%s 栈底:%s", lua->tostring(-1), lua->tostring( 1));// 5 1

	lua->settop(-2);
	Debugf("top=%d", lua->gettop()); // 4
	Debugf("栈顶:%s 栈底:%s", lua->tostring(-1), lua->tostring(1));// 4 1

	lua->settop(2);
	Debugf("top=%d", lua->gettop()); // 2
	Debugf("栈顶:%s 栈底:%s", lua->tostring(-1), lua->tostring(1));// 2 1

	lua->settop(3);
	Debugf("top=%d", lua->gettop()); // 3
	Debugf("栈顶:%s 栈底:%s", lua->tostring(-1), lua->tostring(1));// null 1
#endif
}