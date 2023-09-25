#ifndef INCLUDE_LOGGER_H
#define INCLUDE_LOGGER_H

#include "Logger/src/utils/utils.h"

#ifdef __STACK__
#undef __STACK__
#endif
#define __STACK__       utils::stack_backtrace().c_str()

#define PARAM_FATAL     LVL_FATAL,__FILE__,__LINE__,__FUNC__,__STACK__
#define PARAM_ERROR     LVL_ERROR,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_WARN      LVL_WARN ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_INFO      LVL_INFO ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_TRACE     LVL_TRACE,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_DEBUG     LVL_DEBUG,__FILE__,__LINE__,__FUNC__,NULL

namespace LOGGER {

	class LoggerImpl;
	class Logger {
	private:
		Logger();
		explicit Logger(LoggerImpl* impl);
		~Logger();
	public:
		static Logger* instance();
		
		void setPrename(char const* name);
		char const* getPrename() const;

		char const* timezoneString() const;
		void setTimezone(int timezone = MY_CST);
		int getTimezone();

		char const* levelString() const;
		void setLevel(int level);
		int getLevel();

		char const* modeString() const;
		void setMode(int mode);
		int getMode();

		char const* styleString() const;
		void setStyle(int style);
		int getStyle();

		void setColor(int level, int title, int text);
	public:
		void init(char const* dir, char const* prename = NULL, size_t logsize = 100000000);
		void write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void write_s_fatal(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void wait();
		void enable();
		void disable(int delay = 0, bool sync = false);
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

#define _LOG_INIT LOGGER::Logger::instance()->init
#define _LOG LOGGER::Logger::instance()->write
#define _LOG_S LOGGER::Logger::instance()->write_s
#define _LOG_F LOGGER::Logger::instance()->write_s_fatal
#define _LOG_SET_TIMEZONE LOGGER::Logger::instance()->setTimezone
#define _LOG_SET_LEVEL LOGGER::Logger::instance()->setLevel
#define _LOG_SET_MODE LOGGER::Logger::instance()->setMode
#define _LOG_SET_STYLE LOGGER::Logger::instance()->setStyle
#define _LOG_SET_COLOR LOGGER::Logger::instance()->setColor
#define _LOG_STYLE LOGGER::Logger::instance()->getStyle
#define _LOG_WAIT LOGGER::Logger::instance()->wait
#define _LOG_CONSOLE_OPEN LOGGER::Logger::instance()->enable
#define _LOG_CONSOLE_CLOSE LOGGER::Logger::instance()->disable

#define _LOG_SET_FATAL _LOG_SET_LEVEL(LVL_FATAL)
#define _LOG_SET_ERROR _LOG_SET_LEVEL(LVL_ERROR)
#define _LOG_SET_WARN  _LOG_SET_LEVEL(LVL_WARN)
#define _LOG_SET_INFO  _LOG_SET_LEVEL(LVL_INFO)
#define _LOG_SET_TRACE _LOG_SET_LEVEL(LVL_TRACE)
#define _LOG_SET_DEBUG _LOG_SET_LEVEL(LVL_DEBUG)

#define _LOG_COLOR_FATAL(a,b) _LOG_SET_COLOR(LVL_FATAL, a, b)
#define _LOG_COLOR_ERROR(a,b) _LOG_SET_COLOR(LVL_ERROR, a, b)
#define _LOG_COLOR_WARN(a,b)  _LOG_SET_COLOR(LVL_WARN,  a, b)
#define _LOG_COLOR_INFO(a,b)  _LOG_SET_COLOR(LVL_INFO,  a, b)
#define _LOG_COLOR_TRACE(a,b) _LOG_SET_COLOR(LVL_TRACE, a, b)
#define _LOG_COLOR_DEBUG(a,b) _LOG_SET_COLOR(LVL_DEBUG, a, b)

// F_DETAIL/F_TMSTMP/F_FN/F_TMSTMP_FN/F_FL/F_TMSTMP_FL/F_FL_FN/F_TMSTMP_FL_FN/F_TEXT/F_PURE

#ifdef _windows_
#define _LOG_FATAL(fmt,...) _LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define _LOG_ERROR(fmt,...) _LOG(PARAM_ERROR, _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _LOG_WARN(fmt,...)  _LOG(PARAM_WARN,  _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _LOG_INFO(fmt,...)  _LOG(PARAM_INFO,  _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _LOG_TRACE(fmt,...) _LOG(PARAM_TRACE, _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _LOG_DEBUG(fmt,...) _LOG(PARAM_DEBUG, _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#else
#define _LOG_FATAL(args...) _LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define _LOG_ERROR(args...) _LOG(PARAM_ERROR, _LOG_STYLE(),        ##args)
#define _LOG_WARN(args...)  _LOG(PARAM_WARN,  _LOG_STYLE(),        ##args)
#define _LOG_INFO(args...)  _LOG(PARAM_INFO,  _LOG_STYLE(),        ##args)
#define _LOG_TRACE(args...) _LOG(PARAM_TRACE, _LOG_STYLE(),        ##args)
#define _LOG_DEBUG(args...) _LOG(PARAM_DEBUG, _LOG_STYLE(),        ##args)
#endif

#define _LOG_S_FATAL(msg) _LOG_F(PARAM_FATAL, _LOG_STYLE()|F_SYNC, msg)
#define _LOG_S_ERROR(msg) _LOG_S(PARAM_ERROR, _LOG_STYLE(),        msg)
#define _LOG_S_WARN(msg)  _LOG_S(PARAM_WARN,  _LOG_STYLE(),        msg)
#define _LOG_S_INFO(msg)  _LOG_S(PARAM_INFO,  _LOG_STYLE(),        msg)
#define _LOG_S_TRACE(msg) _LOG_S(PARAM_TRACE, _LOG_STYLE(),        msg)
#define _LOG_S_DEBUG(msg) _LOG_S(PARAM_DEBUG, _LOG_STYLE(),        msg)

#ifdef _windows_
#define _LOG_FATAL_SYN(fmt,...) _LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define _LOG_FATAL_SYN(args...) _LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, ##args); _LOG_WAIT()
#endif

#define _LOG_S_FATAL_SYN(msg) _LOG_S(PARAM_FATAL, _LOG_STYLE()|F_SYNC, msg); _LOG_WAIT()

// F_DETAIL

#ifdef _windows_
#define _DLOG_FATAL(fmt,...) _LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define _DLOG_ERROR(fmt,...) _LOG(PARAM_ERROR, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _DLOG_WARN(fmt,...)  _LOG(PARAM_WARN,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _DLOG_INFO(fmt,...)  _LOG(PARAM_INFO,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _DLOG_TRACE(fmt,...) _LOG(PARAM_TRACE, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _DLOG_DEBUG(fmt,...) _LOG(PARAM_DEBUG, F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define _DLOG_FATAL(args...) _LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define _DLOG_ERROR(args...) _LOG(PARAM_ERROR, F_DETAIL,        ##args)
#define _DLOG_WARN(args...)  _LOG(PARAM_WARN,  F_DETAIL,        ##args)
#define _DLOG_INFO(args...)  _LOG(PARAM_INFO,  F_DETAIL,        ##args)
#define _DLOG_TRACE(args...) _LOG(PARAM_TRACE, F_DETAIL,        ##args)
#define _DLOG_DEBUG(args...) _LOG(PARAM_DEBUG, F_DETAIL,        ##args)
#endif

#define _DLOG_S_FATAL(msg) _LOG_F(PARAM_FATAL, F_DETAIL|F_SYNC, msg)
#define _DLOG_S_ERROR(msg) _LOG_S(PARAM_ERROR, F_DETAIL,        msg)
#define _DLOG_S_WARN(msg)  _LOG_S(PARAM_WARN,  F_DETAIL,        msg)
#define _DLOG_S_INFO(msg)  _LOG_S(PARAM_INFO,  F_DETAIL,        msg)
#define _DLOG_S_TRACE(msg) _LOG_S(PARAM_TRACE, F_DETAIL,        msg)
#define _DLOG_S_DEBUG(msg) _LOG_S(PARAM_DEBUG, F_DETAIL,        msg)

#ifdef _windows_
#define _DLOG_FATAL_SYN(fmt,...) _LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define _DLOG_FATAL_SYN(args...) _LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args); _LOG_WAIT()
#endif

#define _DLOG_S_FATAL_SYN(msg) _LOG_S(PARAM_FATAL, F_DETAIL|F_SYNC, msg); _LOG_WAIT()

// F_TMSTMP

#ifdef _windows_
#define _TLOG_FATAL(fmt,...) _LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define _TLOG_ERROR(fmt,...) _LOG(PARAM_ERROR, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_WARN(fmt,...)  _LOG(PARAM_WARN,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_INFO(fmt,...)  _LOG(PARAM_INFO,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_TRACE(fmt,...) _LOG(PARAM_TRACE, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _TLOG_DEBUG(fmt,...) _LOG(PARAM_DEBUG, F_TMSTMP,        fmt, ##__VA_ARGS__)
#else
#define _TLOG_FATAL(args...) _LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define _TLOG_ERROR(args...) _LOG(PARAM_ERROR, F_TMSTMP,        ##args)
#define _TLOG_WARN(args...)  _LOG(PARAM_WARN,  F_TMSTMP,        ##args)
#define _TLOG_INFO(args...)  _LOG(PARAM_INFO,  F_TMSTMP,        ##args)
#define _TLOG_TRACE(args...) _LOG(PARAM_TRACE, F_TMSTMP,        ##args)
#define _TLOG_DEBUG(args...) _LOG(PARAM_DEBUG, F_TMSTMP,        ##args)
#endif

#define _TLOG_S_FATAL(msg) _LOG_F(PARAM_FATAL, F_TMSTMP|F_SYNC, msg)
#define _TLOG_S_ERROR(msg) _LOG_S(PARAM_ERROR, F_TMSTMP,        msg)
#define _TLOG_S_WARN(msg)  _LOG_S(PARAM_WARN,  F_TMSTMP,        msg)
#define _TLOG_S_INFO(msg)  _LOG_S(PARAM_INFO,  F_TMSTMP,        msg)
#define _TLOG_S_TRACE(msg) _LOG_S(PARAM_TRACE, F_TMSTMP,        msg)
#define _TLOG_S_DEBUG(msg) _LOG_S(PARAM_DEBUG, F_TMSTMP,        msg)

#ifdef _windows_
#define _TLOG_FATAL_SYN(fmt,...) _LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define _TLOG_FATAL_SYN(args...) _LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); _LOG_WAIT()
#endif

#define _TLOG_S_FATAL_SYN(msg) _LOG_S(PARAM_FATAL, F_TMSTMP|F_SYNC, msg); _LOG_WAIT()

// F_FN

// F_TMSTMP_FN

// F_FL

// F_TMSTMP_FL

// F_FL_FN

// F_TMSTMP_FL_FN

// F_TEXT

// F_PURE

#ifdef _windows_
#define _PLOG_FATAL(fmt,...) _LOG(PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define _PLOG_ERROR(fmt,...) _LOG(PARAM_ERROR, F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_WARN(fmt,...)  _LOG(PARAM_WARN,  F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_INFO(fmt,...)  _LOG(PARAM_INFO,  F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_TRACE(fmt,...) _LOG(PARAM_TRACE, F_PURE, fmt, ##__VA_ARGS__)
#define _PLOG_DEBUG(fmt,...) _LOG(PARAM_DEBUG, F_PURE, fmt, ##__VA_ARGS__)
#else
#define _PLOG_FATAL(args...) _LOG(PARAM_FATAL, F_PURE|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define _PLOG_ERROR(args...) _LOG(PARAM_ERROR, F_PURE, ##args)
#define _PLOG_WARN(args...)  _LOG(PARAM_WARN,  F_PURE, ##args)
#define _PLOG_INFO(args...)  _LOG(PARAM_INFO,  F_PURE, ##args)
#define _PLOG_TRACE(args...) _LOG(PARAM_TRACE, F_PURE, ##args)
#define _PLOG_DEBUG(args...) _LOG(PARAM_DEBUG, F_PURE, ##args)
#endif

#define _PLOG_S_FATAL(msg) _LOG_F(PARAM_FATAL, F_PURE|F_SYNC, msg)
#define _PLOG_S_ERROR(msg) _LOG_S(PARAM_ERROR, F_PURE, msg)
#define _PLOG_S_WARN(msg)  _LOG_S(PARAM_WARN,  F_PURE, msg)
#define _PLOG_S_INFO(msg)  _LOG_S(PARAM_INFO,  F_PURE, msg)
#define _PLOG_S_TRACE(msg) _LOG_S(PARAM_TRACE, F_PURE, msg)
#define _PLOG_S_DEBUG(msg) _LOG_S(PARAM_DEBUG, F_PURE, msg)

#ifdef _windows_
#define _PLOG_FATAL_SYN(fmt,...) LOG(PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define _PLOG_FATAL_SYN(args...) LOG(PARAM_FATAL, F_PURE|F_SYNC, ##args); _LOG_WAIT()
#endif

#define _PLOG_S_FATAL_SYN(msg) LOG_S(PARAM_FATAL, F_PURE|F_SYNC, msg); _LOG_WAIT()

#endif