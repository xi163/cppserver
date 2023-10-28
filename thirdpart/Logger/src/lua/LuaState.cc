#include "Logger/src/utils/utils.h"
//#include "lua_libs.h"
#include "LuaState.h"

std::string LuaState::workPath_ = utils::getModulePath();

LuaState::LuaState()
	: L_(NULL)
	, funcId_(-1)
	, obj_(NULL) {
	memset(funcValues_, 0, sizeof funcValues_);
	init();
}

LuaState::~LuaState() {
	if (L_ != NULL) {
		lua_close(L_);
		delete this;
	}
}

LuaState* LuaState::create() {
	LuaState* state = new LuaState();
	state->init();
	return state;
}

LuaState* LuaState::instance(lua_State* L) {
	lua_getglobal(L, "G_LuaStateInst");
	if (lua_isnil(L, lua_gettop(L))) {
		return NULL;
	}
	LuaState* state = (LuaState*)(lua_topointer(L, lua_gettop(L)));
	lua_pop(L, 1);
	return state;
}

bool LuaState::init() {
	L_ = luaL_newstate();
	luaL_openlibs(L_);
	//luaopen_cjson(L_);
	//luaopen_luasql_mysql(L_);

	openFuncMapping();

	int sumMapValues = MAX_FUNCTION_MAP_VALUES;
	while (--sumMapValues >= 0) {
		funcValues_[sumMapValues] = LUA_REFNIL;
	}

	funcId_ = -1;
	obj_ = NULL;
	pushUserData("G_LuaStateInst", this);

	std::string searchPath;
	// 添加LUA搜索路径
	std::string modulePath = utils::getModulePath();
#ifdef _windows_
	searchPath += modulePath + "\\..\\scripts\\public\\?.lua;";
	searchPath += workPath_ + "\\config\\?.lua;";
	searchPath += workPath_ + "\\?.lua";
#else
	searchPath += modulePath + "/../scripts/public/?.lua;";
	searchPath += workPath_ + "/config/?.lua;";
	searchPath += workPath_ + "/?.lua";
#endif
	this->addSearchPath(searchPath.c_str());
	return true;
}

void LuaState::addSearchPath(char const* path) {
	lua_getglobal(L_, "package");
	lua_getfield(L_, -1, "path");

	// 获取LUA环境字段的值(package.path)
	char const* szCurPath = lua_tostring(L_, -1);
	// 设置新的搜索路径到LUA环境字段(package.path)
	lua_pushfstring(L_, "%s;%s", szCurPath, path);
	lua_setfield(L_, -3, "path");
	lua_pop(L_, 2);
}

void LuaState::openFuncMapping() {
	lua_pushstring(L_, LUASTATE_FUNCTION_MAPPING);
	lua_newtable(L_);
	lua_rawset(L_, LUA_REGISTRYINDEX);
}

void LuaState::registerFunc(char const* funcName, lua_CFunction cb) {
	lua_register(L_, funcName, cb);
}

int LuaState::createBindFuncIndex() {
	int count = 0;
	while (count < MAX_FUNCTION_MAP_VALUES) {
		funcId_ = (funcId_ + 1) % MAX_FUNCTION_MAP_VALUES;
		if (funcValues_[funcId_] == LUA_REFNIL)
			return funcId_;
		++count;
	}
	return LUA_REFNIL;
}

int LuaState::bindFunc(int lo) {
	// 获取新的函数ID
	int handler = createBindFuncIndex();
	if (handler == LUA_REFNIL) {
		return LUA_REFNIL;
	}
	// 检查当前是否为有效的函数类型
	if (lua_isfunction(L_, lo)) {
		lua_pushstring(L_, LUASTATE_FUNCTION_MAPPING);
		lua_rawget(L_, LUA_REGISTRYINDEX);
		lua_pushinteger(L_, handler);
		lua_pushvalue(L_, lo);
		lua_rawset(L_, -3);
		lua_pop(L_, 1);
	}

	// 添加函数句柄设置到函数映射值内
	if (handler >= 0 && handler < MAX_FUNCTION_MAP_VALUES) {
		funcValues_[handler] = handler;
	}
	return handler;
}

void LuaState::removeFuncByMapValues(int index) {
	int handler = LUA_REFNIL;
	// 获取函数的句柄ID
	if (index >= 0 && index < MAX_FUNCTION_MAP_VALUES) {
		handler = funcValues_[index];
		funcValues_[index] = LUA_REFNIL;
	}
	// 从函数映射表内移除指定的函数
	lua_pushstring(L_, LUASTATE_FUNCTION_MAPPING);
	lua_rawget(L_, LUA_REGISTRYINDEX);
	lua_pushinteger(L_, handler);
	lua_pushnil(L_);
	lua_rawset(L_, -3);
	lua_pop(L_, 1);
}

int LuaState::execString(char const* s) {
	luaL_loadstring(L_, s);
	return call(0);
}

int LuaState::loadFile(char const* filename) {
	std::string scriptFile = utils::combineFilePath(workPath_, filename);
	//Tracef("%s", scriptFile.c_str());
	int rc = luaL_loadfile(L_, scriptFile.c_str());
	if (rc != 0) {
		Errorf("%s", lua_tostring(L_, -1));
		lua_pop(L_, 1);
	}
	return rc;
}

int LuaState::execFile(char const* filename) {
	std::string scriptFile = utils::combineFilePath(workPath_, filename);
	//Tracef("%s", scriptFile.c_str());
	int rc = luaL_dofile(L_, scriptFile.c_str());
	if (rc != 0) {
		Errorf("%s", lua_tostring(L_, -1));
		lua_pop(L_, 1);
	}
	return rc;
}

bool LuaState::setFuncByMapValues(int index) {
	int handler = LUA_REFNIL;
	if (index >= 0 && index < MAX_FUNCTION_MAP_VALUES) {
		handler = funcValues_[index];
	}
	if (handler == LUA_REFNIL) {
		return false;
	}

	// 设置LUA回调函数
	lua_pushstring(L_, LUASTATE_FUNCTION_MAPPING);
	lua_rawget(L_, LUA_REGISTRYINDEX);
	lua_pushinteger(L_, handler);
	lua_rawget(L_, -2);
	lua_remove(L_, -2);
	return true;
}

bool LuaState::testFunc(int idx) {
	if (lua_isfunction(L_, idx)) {
		return true;
	}
	lua_pop(L_, 1);
	return false;
}

lua_CFunction LuaState::toFunc(int idx) {
	if (lua_isfunction(L_, idx)) {
		return lua_tocfunction(L_, idx);
	}
	return NULL;
}

int LuaState::toBoolean(int idx) {
	if (lua_isboolean(L_, idx)) {
		return lua_toboolean(L_, idx);
	}
	return 0;
}

double LuaState::toNumber(int idx) {
	if (lua_isnumber(L_, idx)) {
		return lua_tonumber(L_, idx);
	}
	return 0;
}

long long LuaState::toInteger(int idx) {
	if (lua_isinteger(L_, idx)) {
		return lua_tointeger(L_, idx);
	}
	return 0;
}

char const* LuaState::toString(int idx) {
	if (lua_isstring(L_, idx)) {
		return lua_tostring(L_, idx);
	}
	return "";
}

void* LuaState::toUserdata(int idx) {
	if (lua_isuserdata(L_, idx)) {
		return lua_touserdata(L_, idx);
	}
	return NULL;
}

void const* LuaState::toPointer(int idx) {
	return lua_topointer(L_, idx);
}

int LuaState::getTable(int idx) {
	if (lua_istable(L_, idx)) {
		return lua_gettable(L_, idx);
	}
	return -1;
}

int LuaState::getGlobal(char const* name) {
	return lua_getglobal(L_, name);
}

void LuaState::pushNull() {
	lua_pushnil(L_);
}

void LuaState::pushInt(int val) {
	lua_pushinteger(L_, val);
}

void LuaState::pushFloat(float val) {
	lua_pushnumber(L_, val);
}

void LuaState::pushLong(long val) {
	lua_pushnumber(L_, val);
}

void LuaState::pushBoolean(int val) {
	lua_pushboolean(L_, val);
}

void LuaState::pushNumber(double val) {
	lua_pushnumber(L_, val);
}

void LuaState::pushString(char const* val) {
	lua_pushstring(L_, val);
}

void LuaState::pushString(char const* val, size_t len) {
	lua_pushlstring(L_, val, len);
}

void LuaState::pushUserData(void* val) {
	lua_pushlightuserdata(L_, val);
}

void LuaState::pushUserData(char const* name, void* val) {
	lua_pushlightuserdata(L_, val);
	lua_setglobal(L_, name);
}

void LuaState::pushValue(int idx) {
	lua_pushvalue(L_, idx);
}

int LuaState::call(int argc, int retc, STD::variant* result) {
	int index = -(argc + 1);
	// 查检是否为函数类型
	if (!lua_isfunction(L_, index)) {
		Errorf("stack[%d] is not function", index);
		lua_pop(L_, -index);
		return -1;
	}
	int traceback = 0;
	// 在main.lua中自定义错误追踪函数
	lua_getglobal(L_, "__G__TRACKBACK__");
	if (!lua_isfunction(L_, -1)) {
		lua_pop(L_, 1);
	}
	else {
		traceback = index - 1;
		lua_insert(L_, traceback);
	}
	int rc = -1;
	MY_TRY();
	//ASSERT(lua_isfunction(L_, -1));
	if ((rc = lua_pcall(L_, argc, retc, traceback)) != 0) {
		if (traceback == 0) {
			Errorf("%s", lua_tostring(L_, -1));
			lua_pop(L_, 1);
			//lua_settop(L_, -2);
		}
		else {
			//Warnf("%s", lua_tostring(L_, -1));
			lua_pop(L_, 2);
		}
		return rc;
	}
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
	else if (lua_isuserdata(L_, -1)) {
		Fatalf("error");
	}
	else if (lua_isfunction(L_, -1)) {
		Fatalf("error");
	}
	else if (!lua_isnil(L_, -1)) {
		//Fatalf("error");
	}
	// 弹出栈顶返回值
	//if (retc == LUA_MULTRET || retc > 0) {
		if (!lua_isnil(L_, -1)) {
			lua_pop(L_, 1);
		}
	//}
	// 弹出栈顶错误追踪函数
	if (traceback) {
		if (!lua_isnil(L_, -1)) {
			lua_pop(L_, 1);
		}
	}
	MY_CATCH();
	return rc;
}