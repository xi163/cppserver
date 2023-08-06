#ifndef INCLUDE_CONST_H
#define INCLUDE_CONST_H

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ENUM_X(n, s) n,
#define ENUM_Y(n, i, s) n = i,

#define K_ENUM_X(n, s) k##n,
#define K_ENUM_Y(n, i, s) k##n = i,

#define TABLE_DECLARE(var, MAP_, DETAIL_X_, DETAIL_Y_) \
	static struct { \
		int id_; \
		char const* name_; \
		char const* desc_; \
	}var[] = { \
		MAP_(DETAIL_X_, DETAIL_Y_) \
	}

#define RETURN_NAME(var, id) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			return var[i].name_; \
		}\
	}
#define RETURN_DESC(var, id) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			return var[i].desc_; \
		}\
	}
#define RETURN_STR(var, id) \
	for (int i = 0; i < ARRAYSIZE(var); ++i) { \
		if (var[i].id_ == id) { \
			std::string name = var[i].name_; \
			std::string desc = var[i].desc_; \
			return name.empty() ? \
				desc : "[" + name + "]" + desc;\
		}\
	}

#define FUNCTION_DECLARE(varname) \
	std::string get##varname(int varname);
#define FUNCTION_IMPLEMENT(MAP_, DETAIL_X_, DETAIL_Y_, NAME_, varname) \
		std::string get##varname(int varname) { \
			TABLE_DECLARE(table_##varname##s_, MAP_, DETAIL_X_, DETAIL_Y_); \
			RETURN_##NAME_(table_##varname##s_, varname); \
		}
#define STATIC_FUNCTION_IMPLEMENT(MAP_, DETAIL_X_, DETAIL_Y_, NAME_, varname) \
		static std::string get##varname(int varname) { \
			TABLE_DECLARE(table_##varname##s_, MAP_, DETAIL_X_, DETAIL_Y_); \
			RETURN_##NAME_(table_##varname##s_, varname); \
		}

#define DETAIL_X(n, s) { n, #n, s },
#define DETAIL_Y(n, i, s){ n, #n, s },

#define K_DETAIL_X(n, s) { k##n, "k"#n, s },
#define K_DETAIL_Y(n, i, s){ k##n, "k"#n, s },

struct Msg {
	std::string const errmsg() const {
		return
			name.empty() ?
			desc : "[" + name + "]" + desc;
	}
	int code;
	std::string name;
	std::string desc;
};

#define P_MSG(p, n, s) \
	static const Msg p##n = Msg{ n, #n, s };
#define P_K_MSG(p, n, s) \
	static const Msg p##n = Msg{ k##n, "k"#n, s };

#define MSG_X(n, s) P_MSG(MSG_, n, s)
#define MSG_Y(n, i, s) P_MSG(MSG_, n, s)

#define K_MSG_X(n, s) P_K_MSG(MSG_, n, s)
#define K_MSG_Y(n, i, s) P_K_MSG(MSG_, n, s)

#define E_MSG_X(n, s) P_MSG(ERR_, n, s)
#define E_MSG_Y(n, i, s) P_MSG(ERR_, n, s)

#define E_K_MSG_X(n, s) P_K_MSG(ERR_, n, s)
#define E_K_MSG_Y(n, i, s) P_K_MSG(ERR_, n, s)

#endif