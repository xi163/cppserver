#ifndef INCLUDE_LOGGER_H
#define INCLUDE_LOGGER_H

#include "Logger/src/utils/utils.h"

#ifdef __STACK__
#undef __STACK__
#endif
#define __STACK__       utils::stack_backtrace().c_str()

#define PARAM_FATAL     0,__FILE__,__LINE__,__FUNC__,__STACK__
#define PARAM_ERROR     1,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_WARN      2,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_INFO      3,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_TRACE     4,__FILE__,__LINE__,__FUNC__,__STACK__
#define PARAM_DEBUG     5,__FILE__,__LINE__,__FUNC__,NULL

namespace LOGGER {

	class LoggerImpl;
	class Logger {
	private:
		Logger();
		explicit Logger(LoggerImpl* impl);
		~Logger();
	public:
		static Logger* instance();
		void set_timezone(int64_t timezone = MY_CST);
		void set_level(int level);
		char const* get_level();
		void set_color(int level, int title, int text);
		void init(char const* dir, int level, char const* prename = NULL, size_t logsize = 100000000);
		void write(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, char const* format, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, std::string const& msg);
		void wait();
		void enable();
		void disable(int delay = 0, bool sync = false);
		void cleanup();
	private:
		template <class T> static inline T* New() {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T();
		}
		template <class T> static inline void Delete(T* ptr) {
			ptr->~T();
			free(ptr);
		}
	private:
		LoggerImpl* impl_;
	};
}

#define LOG_INIT LOGGER::Logger::instance()->init
#define LOG LOGGER::Logger::instance()->write
#define LOG_S LOGGER::Logger::instance()->write_s
#define LOG_SET LOGGER::Logger::instance()->set_level
#define LOG_LVL LOGGER::Logger::instance()->get_level
#define LOG_TIMEZONE LOGGER::Logger::instance()->set_timezone
#define LOG_WAIT LOGGER::Logger::instance()->wait
#define LOG_COLOR LOGGER::Logger::instance()->set_color
#define LOG_CONSOLE_OPEN LOGGER::Logger::instance()->enable
#define LOG_CONSOLE_CLOSE LOGGER::Logger::instance()->disable
#define LOG_CLEANUP LOGGER::Logger::instance()->cleanup

#define LOG_SET_FATAL LOG_SET(LVL_FATAL)
#define LOG_SET_ERROR LOG_SET(LVL_ERROR)
#define LOG_SET_WARN  LOG_SET(LVL_WARN)
#define LOG_SET_INFO  LOG_SET(LVL_INFO)
#define LOG_SET_TRACE LOG_SET(LVL_TRACE)
#define LOG_SET_DEBUG LOG_SET(LVL_DEBUG)

#define LOG_COLOR_FATAL(a,b) LOG_COLOR(LVL_FATAL, a, b)
#define LOG_COLOR_ERROR(a,b) LOG_COLOR(LVL_ERROR, a, b)
#define LOG_COLOR_WARN(a,b)  LOG_COLOR(LVL_WARN,  a, b)
#define LOG_COLOR_INFO(a,b)  LOG_COLOR(LVL_INFO,  a, b)
#define LOG_COLOR_TRACE(a,b) LOG_COLOR(LVL_TRACE, a, b)
#define LOG_COLOR_DEBUG(a,b) LOG_COLOR(LVL_DEBUG, a, b)

//LOG_XXX("%s", msg)
#ifdef _windows_
#define _LOG_FATAL(fmt,...) LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT(); std::abort()
#define _LOG_ERROR(fmt,...) LOG(PARAM_ERROR, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _LOG_WARN(fmt,...)  LOG(PARAM_WARN,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _LOG_INFO(fmt,...)  LOG(PARAM_INFO,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _LOG_TRACE(fmt,...) LOG(PARAM_TRACE, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _LOG_DEBUG(fmt,...) LOG(PARAM_DEBUG, F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define _LOG_FATAL(args...) LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args); LOG_WAIT(); std::abort()
#define _LOG_ERROR(args...) LOG(PARAM_ERROR, F_DETAIL,        ##args)
#define _LOG_WARN(args...)  LOG(PARAM_WARN,  F_DETAIL,        ##args)
#define _LOG_INFO(args...)  LOG(PARAM_INFO,  F_DETAIL,        ##args)
#define _LOG_TRACE(args...) LOG(PARAM_TRACE, F_DETAIL,        ##args)
#define _LOG_DEBUG(args...) LOG(PARAM_DEBUG, F_DETAIL,        ##args)
#endif

//LOG_S_XXX(msg)
#define _LOG_S_FATAL(msg) LOG_S(PARAM_FATAL, F_DETAIL|F_SYNC, msg); LOG_WAIT(); std::abort()
#define _LOG_S_ERROR(msg) LOG_S(PARAM_ERROR, F_DETAIL,        msg)
#define _LOG_S_WARN(msg)  LOG_S(PARAM_WARN,  F_DETAIL,        msg)
#define _LOG_S_INFO(msg)  LOG_S(PARAM_INFO,  F_DETAIL,        msg)
#define _LOG_S_TRACE(msg) LOG_S(PARAM_TRACE, F_DETAIL,        msg)
#define _LOG_S_DEBUG(msg) LOG_S(PARAM_DEBUG, F_DETAIL,        msg)

//LOG_XXX("%s", msg)
#ifdef _windows_
#define _LOG_FATAL_SYN(fmt,...) LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT()
#else
#define _LOG_FATAL_SYN(args...) LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args); LOG_WAIT()
#endif

//LOG_S_XXX(msg)
#define _LOG_S_FATAL_SYN(msg) LOG_S(PARAM_FATAL, F_DETAIL|F_SYNC, msg); LOG_WAIT()

//TLOG_XXX("%s", msg)
#ifdef _windows_
#define _TLOG_FATAL(fmt,...) LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT(); std::abort()
#define _TLOG_ERROR(fmt,...) LOG(PARAM_ERROR, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_WARN(fmt,...)  LOG(PARAM_WARN,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_INFO(fmt,...)  LOG(PARAM_INFO,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_TRACE(fmt,...) LOG(PARAM_TRACE, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_DEBUG(fmt,...) LOG(PARAM_DEBUG, F_TMSTMP,        fmt, ##__VA_ARGS__)
#else
#define _TLOG_FATAL(args...) LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); LOG_WAIT(); std::abort()
#define _TLOG_ERROR(args...) LOG(PARAM_ERROR, F_TMSTMP,        ##args)
#define _TLOG_WARN(args...)  LOG(PARAM_WARN,  F_TMSTMP,        ##args)
#define _TLOG_INFO(args...)  LOG(PARAM_INFO,  F_TMSTMP,        ##args)
#define _TLOG_TRACE(args...) LOG(PARAM_TRACE, F_TMSTMP,        ##args)
#define _TLOG_DEBUG(args...) LOG(PARAM_DEBUG, F_TMSTMP,        ##args)
#endif

//TLOG_S_XXX(msg)
#define _TLOG_S_FATAL(msg) LOG_S(PARAM_FATAL, F_TMSTMP|F_SYNC, msg); LOG_WAIT(); std::abort()
#define _TLOG_S_ERROR(msg) LOG_S(PARAM_ERROR, F_TMSTMP,        msg)
#define _TLOG_S_WARN(msg)  LOG_S(PARAM_WARN,  F_TMSTMP,        msg)
#define _TLOG_S_INFO(msg)  LOG_S(PARAM_INFO,  F_TMSTMP,        msg)
#define _TLOG_S_TRACE(msg) LOG_S(PARAM_TRACE, F_TMSTMP,        msg)
#define _TLOG_S_DEBUG(msg) LOG_S(PARAM_DEBUG, F_TMSTMP,        msg)

//TLOG_XXX("%s", msg)
#ifdef _windows_
#define _TLOG_FATAL_SYN(fmt,...) LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT()
#else
#define _TLOG_FATAL_SYN(args...) LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); LOG_WAIT()
#endif

//TLOG_S_XXX(msg)
#define _TLOG_S_FATAL_SYN(msg) LOG_S(PARAM_FATAL, F_TMSTMP|F_SYNC, msg); LOG_WAIT()

//PLOG_XXX("%s", msg)
#ifdef _windows_
#define _PLOG_FATAL(fmt,...) LOG(PARAM_FATAL, F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT(); std::abort()
#define _PLOG_ERROR(fmt,...) LOG(PARAM_ERROR, F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_WARN(fmt,...)  LOG(PARAM_WARN,  F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_INFO(fmt,...)  LOG(PARAM_INFO,  F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_TRACE(fmt,...) LOG(PARAM_TRACE, F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_DEBUG(fmt,...) LOG(PARAM_DEBUG, F_PURE, fmt, ##__VA_ARGS__)
#else
#define _PLOG_FATAL(args...) LOG(PARAM_FATAL, F_SYNC, ##args); LOG_WAIT(); std::abort()
#define _PLOG_ERROR(args...) LOG(PARAM_ERROR, F_PURE, ##args)
#define _PLOG_WARN(args...)  LOG(PARAM_WARN,  F_PURE, ##args)
#define _PLOG_INFO(args...)  LOG(PARAM_INFO,  F_PURE, ##args)
#define _PLOG_TRACE(args...) LOG(PARAM_TRACE, F_PURE, ##args)
#define _PLOG_DEBUG(args...) LOG(PARAM_DEBUG, F_PURE, ##args)
#endif

//PLOG_S_XXX(msg)
#define _PLOG_S_FATAL(msg) LOG_S(PARAM_FATAL, F_SYNC, msg); LOG_WAIT(); std::abort()
#define _PLOG_S_ERROR(msg) LOG_S(PARAM_ERROR, F_PURE, msg)
#define _PLOG_S_WARN(msg)  LOG_S(PARAM_WARN,  F_PURE, msg)
#define _PLOG_S_INFO(msg)  LOG_S(PARAM_INFO,  F_PURE, msg)
#define _PLOG_S_TRACE(msg) LOG_S(PARAM_TRACE, F_PURE, msg)
#define _PLOG_S_DEBUG(msg) LOG_S(PARAM_DEBUG, F_PURE, msg)

//PLOG_XXX("%s", msg)
#ifdef _windows_
#define _PLOG_FATAL_SYN(fmt,...) LOG(PARAM_FATAL, F_SYNC, fmt, ##__VA_ARGS__); LOG_WAIT()
#else
#define _PLOG_FATAL_SYN(args...) LOG(PARAM_FATAL, F_SYNC, ##args); LOG_WAIT()
#endif

//PLOG_S_XXX(msg)
#define _PLOG_S_FATAL_SYN(msg) LOG_S(PARAM_FATAL, F_SYNC, msg); LOG_WAIT()

#endif