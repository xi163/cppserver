#ifndef INCLUDE_LOGGER_H
#define INCLUDE_LOGGER_H

#include "Logger/src/utils/utils.h"

#ifdef __STACK__
#undef __STACK__
#endif
#define __STACK__      utils::stack_backtrace().c_str()

#define PARAM_FATAL    LVL_FATAL    ,__FILE__,__LINE__,__FUNC__,__STACK__
#define PARAM_ERROR    LVL_ERROR    ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_WARN     LVL_WARN     ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_CRITICAL LVL_CRITICAL ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_INFO     LVL_INFO     ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_DEBUG    LVL_DEBUG    ,__FILE__,__LINE__,__FUNC__,NULL
#define PARAM_TRACE    LVL_TRACE    ,__FILE__,__LINE__,__FUNC__,NULL

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
		void init(char const* dir, char const* logname = NULL, size_t logsize = 100000000);
		void write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void write_fatal_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
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
	template <typename T>
	T FatalNotNull(int level, char const* file, int line, char const* func, char const* stack, T ptr, char const* msg) {
		if (ptr == NULL) {
			::LOGGER::Logger::instance()->write_fatal_s(level, file, line, func, stack, F_DETAIL | F_SYNC, msg);
		}
		return ptr;
	}
	template <typename T>
	T& FatalNotNullRef(int level, char const* file, int line, char const* func, char const* stack, T& ptr, char const* msg) {
		if (ptr == NULL) {
			::LOGGER::Logger::instance()->write_fatal_s(level, file, line, func, stack, F_DETAIL | F_SYNC, msg);
		}
		return ptr;
	}
}

#define _LOG_INIT LOGGER::Logger::instance()->init
#define _LOG LOGGER::Logger::instance()->write
#define _LOG_S LOGGER::Logger::instance()->write_s
#define _LOG_SET_TIMEZONE LOGGER::Logger::instance()->setTimezone
#define _LOG_SET_LEVEL LOGGER::Logger::instance()->setLevel
#define _LOG_SET_MODE LOGGER::Logger::instance()->setMode
#define _LOG_SET_STYLE LOGGER::Logger::instance()->setStyle
#define _LOG_SET_COLOR LOGGER::Logger::instance()->setColor
#define _LOG_TIMEZONE LOGGER::Logger::instance()->getTimezone
#define _LOG_LEVEL LOGGER::Logger::instance()->getLevel
#define _LOG_MODE LOGGER::Logger::instance()->getMode
#define _LOG_STYLE LOGGER::Logger::instance()->getStyle
#define _LOG_TIMEZONE_STR LOGGER::Logger::instance()->timezoneString
#define _LOG_LEVEL_STR LOGGER::Logger::instance()->levelString
#define _LOG_MODE_STR LOGGER::Logger::instance()->modeString
#define _LOG_STYLE_STR LOGGER::Logger::instance()->styleString
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
#define Fatalf(fmt,...)   (_LOG(PARAM_FATAL,    _LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf(fmt,...)    _LOG(PARAM_ERROR,    _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define Warnf(fmt,...)     _LOG(PARAM_WARN,     _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define Criticalf(fmt,...) _LOG(PARAM_CRITICAL, _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define Infof(fmt,...)     _LOG(PARAM_INFO,     _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define Debugf(fmt,...)    _LOG(PARAM_DEBUG,    _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define Tracef(fmt,...)    _LOG(PARAM_TRACE,    _LOG_STYLE(),        fmt, ##__VA_ARGS__)
#else
#define Fatalf(args...)   (_LOG(PARAM_FATAL,    _LOG_STYLE()|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf(args...)    _LOG(PARAM_ERROR,    _LOG_STYLE(),        ##args)
#define Warnf(args...)     _LOG(PARAM_WARN,     _LOG_STYLE(),        ##args)
#define Criticalf(args...) _LOG(PARAM_CRITICAL, _LOG_STYLE(),        ##args)
#define Infof(args...)     _LOG(PARAM_INFO,     _LOG_STYLE(),        ##args)
#define Debugf(args...)    _LOG(PARAM_DEBUG,    _LOG_STYLE(),        ##args)
#define Tracef(args...)    _LOG(PARAM_TRACE,    _LOG_STYLE(),        ##args)
#endif

#define Fatal(msg)   (_LOG_S(PARAM_FATAL,    _LOG_STYLE()|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error(msg)    _LOG_S(PARAM_ERROR,    _LOG_STYLE(),        msg)
#define Warn(msg)     _LOG_S(PARAM_WARN,     _LOG_STYLE(),        msg)
#define Critical(msg) _LOG_S(PARAM_CRITICAL, _LOG_STYLE(),        msg)
#define Info(msg)     _LOG_S(PARAM_INFO,     _LOG_STYLE(),        msg)
#define Debug(msg)    _LOG_S(PARAM_DEBUG,    _LOG_STYLE(),        msg)
#define Trace(msg)    _LOG_S(PARAM_TRACE,    _LOG_STYLE(),        msg)

#ifdef _windows_
#define SynFatalf(fmt,...) (_LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf(args...) (_LOG(PARAM_FATAL, _LOG_STYLE()|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal(msg) (_LOG_S(PARAM_FATAL, _LOG_STYLE()|F_SYNC, msg), _LOG_WAIT())

// F_DETAIL

#ifdef _windows_
#define Fatalf_detail(fmt,...)   (_LOG(PARAM_FATAL,    F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_detail(fmt,...)    _LOG(PARAM_ERROR,    F_DETAIL,        fmt, ##__VA_ARGS__)
#define Warnf_detail(fmt,...)     _LOG(PARAM_WARN,     F_DETAIL,        fmt, ##__VA_ARGS__)
#define Criticalf_detail(fmt,...) _LOG(PARAM_CRITICAL, F_DETAIL,        fmt, ##__VA_ARGS__)
#define Infof_detail(fmt,...)     _LOG(PARAM_INFO,     F_DETAIL,        fmt, ##__VA_ARGS__)
#define Debugf_detail(fmt,...)    _LOG(PARAM_DEBUG,    F_DETAIL,        fmt, ##__VA_ARGS__)
#define Tracef_detail(fmt,...)    _LOG(PARAM_TRACE,    F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_detail(args...)   (_LOG(PARAM_FATAL,    F_DETAIL|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_detail(args...)    _LOG(PARAM_ERROR,    F_DETAIL,        ##args)
#define Warnf_detail(args...)     _LOG(PARAM_WARN,     F_DETAIL,        ##args)
#define Criticalf_detail(args...) _LOG(PARAM_CRITICAL, F_DETAIL,        ##args)
#define Infof_detail(args...)     _LOG(PARAM_INFO,     F_DETAIL,        ##args)
#define Debugf_detail(args...)    _LOG(PARAM_DEBUG,    F_DETAIL,        ##args)
#define Tracef_detail(args...)    _LOG(PARAM_TRACE,    F_DETAIL,        ##args)
#endif

#define Fatal_detail(msg)   (_LOG_S(PARAM_FATAL,    F_DETAIL|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_detail(msg)    _LOG_S(PARAM_ERROR,    F_DETAIL,        msg)
#define Warn_detail(msg)     _LOG_S(PARAM_WARN,     F_DETAIL,        msg)
#define Critical_detail(msg) _LOG_S(PARAM_CRITICAL, F_DETAIL,        msg)
#define Info_detail(msg)     _LOG_S(PARAM_INFO,     F_DETAIL,        msg)
#define Debug_detail(msg)    _LOG_S(PARAM_DEBUG,    F_DETAIL,        msg)
#define Trace_detail(msg)    _LOG_S(PARAM_TRACE,    F_DETAIL,        msg)

#ifdef _windows_
#define SynFatalf_detail(fmt,...) (_LOG(PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_detail(args...) (_LOG(PARAM_FATAL, F_DETAIL|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_detail(msg) (_LOG_S(PARAM_FATAL, F_DETAIL|F_SYNC, msg), _LOG_WAIT())

// F_TMSTMP

#ifdef _windows_
#define Fatalf_tmsp(fmt,...)   (_LOG(PARAM_FATAL,    F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_tmsp(fmt,...)    _LOG(PARAM_ERROR,    F_TMSTMP,        fmt, ##__VA_ARGS__)
#define Warnf_tmsp(fmt,...)     _LOG(PARAM_WARN,     F_TMSTMP,        fmt, ##__VA_ARGS__)
#define Criticalf_tmsp(fmt,...) _LOG(PARAM_CRITICAL, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define Infof_tmsp(fmt,...)     _LOG(PARAM_INFO,     F_TMSTMP,        fmt, ##__VA_ARGS__)
#define Debugf_tmsp(fmt,...)    _LOG(PARAM_DEBUG,    F_TMSTMP,        fmt, ##__VA_ARGS__)
#define Tracef_tmsp(fmt,...)    _LOG(PARAM_TRACE,    F_TMSTMP,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_tmsp(args...)   (_LOG(PARAM_FATAL,    F_TMSTMP|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_tmsp(args...)    _LOG(PARAM_ERROR,    F_TMSTMP,        ##args)
#define Warnf_tmsp(args...)     _LOG(PARAM_WARN,     F_TMSTMP,        ##args)
#define Criticalf_tmsp(args...) _LOG(PARAM_CRITICAL, F_TMSTMP,        ##args)
#define Infof_tmsp(args...)     _LOG(PARAM_INFO,     F_TMSTMP,        ##args)
#define Debugf_tmsp(args...)    _LOG(PARAM_DEBUG,    F_TMSTMP,        ##args)
#define Tracef_tmsp(args...)    _LOG(PARAM_TRACE,    F_TMSTMP,        ##args)
#endif

#define Fatal_tmsp(msg)   (_LOG_S(PARAM_FATAL,    F_TMSTMP|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_tmsp(msg)    _LOG_S(PARAM_ERROR,    F_TMSTMP,        msg)
#define Warn_tmsp(msg)     _LOG_S(PARAM_WARN,     F_TMSTMP,        msg)
#define Critical_tmsp(msg) _LOG_S(PARAM_CRITICAL, F_TMSTMP,        msg)
#define Info_tmsp(msg)     _LOG_S(PARAM_INFO,     F_TMSTMP,        msg)
#define Debug_tmsp(msg)    _LOG_S(PARAM_DEBUG,    F_TMSTMP,        msg)
#define Trace_tmsp(msg)    _LOG_S(PARAM_TRACE,    F_TMSTMP,        msg)

#ifdef _windows_
#define SynFatalf_tmsp(fmt,...) (_LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_tmsp(args...) (_LOG(PARAM_FATAL, F_TMSTMP|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_tmsp(msg) (_LOG_S(PARAM_FATAL, F_TMSTMP|F_SYNC, msg), _LOG_WAIT())

// F_THRD 

// F_TMSTMP_THRD

#ifdef _windows_
#define Fatalf_tmsp_thrd(fmt,...)   (_LOG(PARAM_FATAL,    F_TMSTMP_THRD|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_thrd(fmt,...)    _LOG(PARAM_ERROR,    F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#define Warnf_tmsp_thrd(fmt,...)     _LOG(PARAM_WARN,     F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#define Criticalf_tmsp_thrd(fmt,...) _LOG(PARAM_CRITICAL, F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#define Infof_tmsp_thrd(fmt,...)     _LOG(PARAM_INFO,     F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#define Debugf_tmsp_thrd(fmt,...)    _LOG(PARAM_DEBUG,    F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#define Tracef_tmsp_thrd(fmt,...)    _LOG(PARAM_TRACE,    F_TMSTMP_THRD,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_tmsp_thrd(args...)   (_LOG(PARAM_FATAL,    F_TMSTMP_THRD|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_thrd(args...)    _LOG(PARAM_ERROR,    F_TMSTMP_THRD,        ##args)
#define Warnf_tmsp_thrd(args...)     _LOG(PARAM_WARN,     F_TMSTMP_THRD,        ##args)
#define Criticalf_tmsp_thrd(args...) _LOG(PARAM_CRITICAL, F_TMSTMP_THRD,        ##args)
#define Infof_tmsp_thrd(args...)     _LOG(PARAM_INFO,     F_TMSTMP_THRD,        ##args)
#define Debugf_tmsp_thrd(args...)    _LOG(PARAM_DEBUG,    F_TMSTMP_THRD,        ##args)
#define Tracef_tmsp_thrd(args...)    _LOG(PARAM_TRACE,    F_TMSTMP_THRD,        ##args)
#endif

#define Fatal_tmsp_thrd(msg)   (_LOG_S(PARAM_FATAL,    F_TMSTMP_THRD|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_tmsp_thrd(msg)    _LOG_S(PARAM_ERROR,    F_TMSTMP_THRD,        msg)
#define Warn_tmsp_thrd(msg)     _LOG_S(PARAM_WARN,     F_TMSTMP_THRD,        msg)
#define Critical_tmsp_thrd(msg) _LOG_S(PARAM_CRITICAL, F_TMSTMP_THRD,        msg)
#define Info_tmsp_thrd(msg)     _LOG_S(PARAM_INFO,     F_TMSTMP_THRD,        msg)
#define Debug_tmsp_thrd(msg)    _LOG_S(PARAM_DEBUG,    F_TMSTMP_THRD,        msg)
#define Trace_tmsp_thrd(msg)    _LOG_S(PARAM_TRACE,    F_TMSTMP_THRD,        msg)

#ifdef _windows_
#define SynFatalf_tmsp_thrd(fmt,...) (_LOG(PARAM_FATAL, F_TMSTMP_THRD|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_tmsp_thrd(args...) (_LOG(PARAM_FATAL, F_TMSTMP_THRD|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_tmsp_thrd(msg) (_LOG_S(PARAM_FATAL, F_TMSTMP_THRD|F_SYNC, msg), _LOG_WAIT())

// F_FN

#ifdef _windows_
#define Fatalf_fn(fmt,...)   (_LOG(PARAM_FATAL,    F_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_fn(fmt,...)    _LOG(PARAM_ERROR,    F_FN,        fmt, ##__VA_ARGS__)
#define Warnf_fn(fmt,...)     _LOG(PARAM_WARN,     F_FN,        fmt, ##__VA_ARGS__)
#define Criticalf_fn(fmt,...) _LOG(PARAM_CRITICAL, F_FN,        fmt, ##__VA_ARGS__)
#define Infof_fn(fmt,...)     _LOG(PARAM_INFO,     F_FN,        fmt, ##__VA_ARGS__)
#define Debugf_fn(fmt,...)    _LOG(PARAM_DEBUG,    F_FN,        fmt, ##__VA_ARGS__)
#define Tracef_fn(fmt,...)    _LOG(PARAM_TRACE,    F_FN,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_fn(args...)   (_LOG(PARAM_FATAL,    F_FN|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_fn(args...)    _LOG(PARAM_ERROR,    F_FN,        ##args)
#define Warnf_fn(args...)     _LOG(PARAM_WARN,     F_FN,        ##args)
#define Criticalf_fn(args...) _LOG(PARAM_CRITICAL, F_FN,        ##args)
#define Infof_fn(args...)     _LOG(PARAM_INFO,     F_FN,        ##args)
#define Debugf_fn(args...)    _LOG(PARAM_DEBUG,    F_FN,        ##args)
#define Tracef_fn(args...)    _LOG(PARAM_TRACE,    F_FN,        ##args)
#endif

#define Fatal_fn(msg)   (_LOG_S(PARAM_FATAL,    F_FN|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_fn(msg)    _LOG_S(PARAM_ERROR,    F_FN,        msg)
#define Warn_fn(msg)     _LOG_S(PARAM_WARN,     F_FN,        msg)
#define Critical_fn(msg) _LOG_S(PARAM_CRITICAL, F_FN,        msg)
#define Info_fn(msg)     _LOG_S(PARAM_INFO,     F_FN,        msg)
#define Debug_fn(msg)    _LOG_S(PARAM_DEBUG,    F_FN,        msg)
#define Trace_fn(msg)    _LOG_S(PARAM_TRACE,    F_FN,        msg)

#ifdef _windows_
#define SynFatalf_fn(fmt,...) (_LOG(PARAM_FATAL, F_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_fn(args...) (_LOG(PARAM_FATAL, F_FN|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_fn(msg) (_LOG_S(PARAM_FATAL, F_FN|F_SYNC, msg), _LOG_WAIT())

// F_TMSTMP_FN

#ifdef _windows_
#define Fatalf_tmsp_fn(fmt,...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fn(fmt,...)    _LOG(PARAM_ERROR,    F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define Warnf_tmsp_fn(fmt,...)     _LOG(PARAM_WARN,     F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define Criticalf_tmsp_fn(fmt,...) _LOG(PARAM_CRITICAL, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define Infof_tmsp_fn(fmt,...)     _LOG(PARAM_INFO,     F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define Debugf_tmsp_fn(fmt,...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define Tracef_tmsp_fn(fmt,...)    _LOG(PARAM_TRACE,    F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_tmsp_fn(args...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FN|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fn(args...)    _LOG(PARAM_ERROR,    F_TMSTMP_FN,        ##args)
#define Warnf_tmsp_fn(args...)     _LOG(PARAM_WARN,     F_TMSTMP_FN,        ##args)
#define Criticalf_tmsp_fn(args...) _LOG(PARAM_CRITICAL, F_TMSTMP_FN,        ##args)
#define Infof_tmsp_fn(args...)     _LOG(PARAM_INFO,     F_TMSTMP_FN,        ##args)
#define Debugf_tmsp_fn(args...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FN,        ##args)
#define Tracef_tmsp_fn(args...)    _LOG(PARAM_TRACE,    F_TMSTMP_FN,        ##args)
#endif

#define Fatal_tmsp_fn(msg)   (_LOG_S(PARAM_FATAL,    F_TMSTMP_FN|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_tmsp_fn(msg)    _LOG_S(PARAM_ERROR,    F_TMSTMP_FN,        msg)
#define Warn_tmsp_fn(msg)     _LOG_S(PARAM_WARN,     F_TMSTMP_FN,        msg)
#define Critical_tmsp_fn(msg) _LOG_S(PARAM_CRITICAL, F_TMSTMP_FN,        msg)
#define Info_tmsp_fn(msg)     _LOG_S(PARAM_INFO,     F_TMSTMP_FN,        msg)
#define Debug_tmsp_fn(msg)    _LOG_S(PARAM_DEBUG,    F_TMSTMP_FN,        msg)
#define Trace_tmsp_fn(msg)    _LOG_S(PARAM_TRACE,    F_TMSTMP_FN,        msg)

#ifdef _windows_
#define SynFatalf_tmsp_fn(fmt,...) (_LOG(PARAM_FATAL, F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_tmsp_fn(args...) (_LOG(PARAM_FATAL, F_TMSTMP_FN|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_tmsp_fn(msg) (_LOG_S(PARAM_FATAL, F_TMSTMP_FN|F_SYNC, msg), _LOG_WAIT())

// F_FL

#ifdef _windows_
#define Fatalf_fl(fmt,...)   (_LOG(PARAM_FATAL,    F_FL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_fl(fmt,...)    _LOG(PARAM_ERROR,    F_FL,        fmt, ##__VA_ARGS__)
#define Warnf_fl(fmt,...)     _LOG(PARAM_WARN,     F_FL,        fmt, ##__VA_ARGS__)
#define Criticalf_fl(fmt,...) _LOG(PARAM_CRITICAL, F_FL,        fmt, ##__VA_ARGS__)
#define Infof_fl(fmt,...)     _LOG(PARAM_INFO,     F_FL,        fmt, ##__VA_ARGS__)
#define Debugf_fl(fmt,...)    _LOG(PARAM_DEBUG,    F_FL,        fmt, ##__VA_ARGS__)
#define Tracef_fl(fmt,...)    _LOG(PARAM_TRACE,    F_FL,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_fl(args...)   (_LOG(PARAM_FATAL,    F_FL|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_fl(args...)    _LOG(PARAM_ERROR,    F_FL,        ##args)
#define Warnf_fl(args...)     _LOG(PARAM_WARN,     F_FL,        ##args)
#define Criticalf_fl(args...) _LOG(PARAM_CRITICAL, F_FL,        ##args)
#define Infof_fl(args...)     _LOG(PARAM_INFO,     F_FL,        ##args)
#define Debugf_fl(args...)    _LOG(PARAM_DEBUG,    F_FL,        ##args)
#define Tracef_fl(args...)    _LOG(PARAM_TRACE,    F_FL,        ##args)
#endif

#define Fatal_fl(msg)   (_LOG_S(PARAM_FATAL,    F_FL|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_fl(msg)    _LOG_S(PARAM_ERROR,    F_FL,        msg)
#define Warn_fl(msg)     _LOG_S(PARAM_WARN,     F_FL,        msg)
#define Critical_fl(msg) _LOG_S(PARAM_CRITICAL, F_FL,        msg)
#define Info_fl(msg)     _LOG_S(PARAM_INFO,     F_FL,        msg)
#define Debug_fl(msg)    _LOG_S(PARAM_DEBUG,    F_FL,        msg)
#define Trace_fl(msg)    _LOG_S(PARAM_TRACE,    F_FL,        msg)

#ifdef _windows_
#define SynFatalf_fl(fmt,...) (_LOG(PARAM_FATAL, F_FL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_fl(args...) (_LOG(PARAM_FATAL, F_FL|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_fl(msg) (_LOG_S(PARAM_FATAL, F_FL|F_SYNC, msg), _LOG_WAIT())

// F_TMSTMP_FL

#ifdef _windows_
#define Fatalf_tmsp_fl(fmt,...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fl(fmt,...)    _LOG(PARAM_ERROR,    F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define Warnf_tmsp_fl(fmt,...)     _LOG(PARAM_WARN,     F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define Criticalf_tmsp_fl(fmt,...) _LOG(PARAM_CRITICAL, F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define Infof_tmsp_fl(fmt,...)     _LOG(PARAM_INFO,     F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define Debugf_tmsp_fl(fmt,...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define Tracef_tmsp_fl(fmt,...)    _LOG(PARAM_TRACE,    F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_tmsp_fl(args...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FL|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fl(args...)    _LOG(PARAM_ERROR,    F_TMSTMP_FL,        ##args)
#define Warnf_tmsp_fl(args...)     _LOG(PARAM_WARN,     F_TMSTMP_FL,        ##args)
#define Criticalf_tmsp_fl(args...) _LOG(PARAM_CRITICAL, F_TMSTMP_FL,        ##args)
#define Infof_tmsp_fl(args...)     _LOG(PARAM_INFO,     F_TMSTMP_FL,        ##args)
#define Debugf_tmsp_fl(args...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FL,        ##args)
#define Tracef_tmsp_fl(args...)    _LOG(PARAM_TRACE,    F_TMSTMP_FL,        ##args)
#endif

#define Fatal_tmsp_fl(msg)   (_LOG_S(PARAM_FATAL,    F_TMSTMP_FL|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_tmsp_fl(msg)    _LOG_S(PARAM_ERROR,    F_TMSTMP_FL,        msg)
#define Warn_tmsp_fl(msg)     _LOG_S(PARAM_WARN,     F_TMSTMP_FL,        msg)
#define Critical_tmsp_fl(msg) _LOG_S(PARAM_CRITICAL, F_TMSTMP_FL,        msg)
#define Info_tmsp_fl(msg)     _LOG_S(PARAM_INFO,     F_TMSTMP_FL,        msg)
#define Debug_tmsp_fl(msg)    _LOG_S(PARAM_DEBUG,    F_TMSTMP_FL,        msg)
#define Trace_tmsp_fl(msg)    _LOG_S(PARAM_TRACE,    F_TMSTMP_FL,        msg)

#ifdef _windows_
#define SynFatalf_tmsp_fl(fmt,...) (_LOG(PARAM_FATAL, F_TMSTMP_FL|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_tmsp_fl(args...) (_LOG(PARAM_FATAL, F_TMSTMP_FL|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_tmsp_fl(msg) (_LOG_S(PARAM_FATAL, F_TMSTMP_FL|F_SYNC, msg), _LOG_WAIT())

// F_FL_FN

#ifdef _windows_
#define Fatalf_fl_fn(fmt,...)   (_LOG(PARAM_FATAL,    F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_fl_fn(fmt,...)    _LOG(PARAM_ERROR,    F_FL_FN,        fmt, ##__VA_ARGS__)
#define Warnf_fl_fn(fmt,...)     _LOG(PARAM_WARN,     F_FL_FN,        fmt, ##__VA_ARGS__)
#define Criticalf_fl_fn(fmt,...) _LOG(PARAM_CRITICAL, F_FL_FN,        fmt, ##__VA_ARGS__)
#define Infof_fl_fn(fmt,...)     _LOG(PARAM_INFO,     F_FL_FN,        fmt, ##__VA_ARGS__)
#define Debugf_fl_fn(fmt,...)    _LOG(PARAM_DEBUG,    F_FL_FN,        fmt, ##__VA_ARGS__)
#define Tracef_fl_fn(fmt,...)    _LOG(PARAM_TRACE,    F_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_fl_fn(args...)   (_LOG(PARAM_FATAL,    F_FL_FN|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_fl_fn(args...)    _LOG(PARAM_ERROR,    F_FL_FN,        ##args)
#define Warnf_fl_fn(args...)     _LOG(PARAM_WARN,     F_FL_FN,        ##args)
#define Criticalf_fl_fn(args...) _LOG(PARAM_CRITICAL, F_FL_FN,        ##args)
#define Infof_fl_fn(args...)     _LOG(PARAM_INFO,     F_FL_FN,        ##args)
#define Debugf_fl_fn(args...)    _LOG(PARAM_DEBUG,    F_FL_FN,        ##args)
#define Tracef_fl_fn(args...)    _LOG(PARAM_TRACE,    F_FL_FN,        ##args)
#endif

#define Fatal_fl_fn(msg)   (_LOG_S(PARAM_FATAL,    F_FL_FN|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_fl_fn(msg)    _LOG_S(PARAM_ERROR,    F_FL_FN,        msg)
#define Warn_fl_fn(msg)     _LOG_S(PARAM_WARN,     F_FL_FN,        msg)
#define Critical_fl_fn(msg) _LOG_S(PARAM_CRITICAL, F_FL_FN,        msg)
#define Info_fl_fn(msg)     _LOG_S(PARAM_INFO,     F_FL_FN,        msg)
#define Debug_fl_fn(msg)    _LOG_S(PARAM_DEBUG,    F_FL_FN,        msg)
#define Trace_fl_fn(msg)    _LOG_S(PARAM_TRACE,    F_FL_FN,        msg)

#ifdef _windows_
#define SynFatalf_fl_fn(fmt,...) (_LOG(PARAM_FATAL, F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_fl_fn(args...) (_LOG(PARAM_FATAL, F_FL_FN|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_fl_fn(msg) (_LOG_S(PARAM_FATAL, F_FL_FN|F_SYNC, msg), _LOG_WAIT())

// F_TMSTMP_FL_FN

#ifdef _windows_
#define Fatalf_tmsp_fl_fn(fmt,...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fl_fn(fmt,...)    _LOG(PARAM_ERROR,    F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define Warnf_tmsp_fl_fn(fmt,...)     _LOG(PARAM_WARN,     F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define Criticalf_tmsp_fl_fn(fmt,...) _LOG(PARAM_CRITICAL, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define Infof_tmsp_fl_fn(fmt,...)     _LOG(PARAM_INFO,     F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define Debugf_tmsp_fl_fn(fmt,...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define Tracef_tmsp_fl_fn(fmt,...)    _LOG(PARAM_TRACE,    F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define Fatalf_tmsp_fl_fn(args...)   (_LOG(PARAM_FATAL,    F_TMSTMP_FL_FN|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_tmsp_fl_fn(args...)    _LOG(PARAM_ERROR,    F_TMSTMP_FL_FN,        ##args)
#define Warnf_tmsp_fl_fn(args...)     _LOG(PARAM_WARN,     F_TMSTMP_FL_FN,        ##args)
#define Criticalf_tmsp_fl_fn(args...) _LOG(PARAM_CRITICAL, F_TMSTMP_FL_FN,        ##args)
#define Infof_tmsp_fl_fn(args...)     _LOG(PARAM_INFO,     F_TMSTMP_FL_FN,        ##args)
#define Debugf_tmsp_fl_fn(args...)    _LOG(PARAM_DEBUG,    F_TMSTMP_FL_FN,        ##args)
#define Tracef_tmsp_fl_fn(args...)    _LOG(PARAM_TRACE,    F_TMSTMP_FL_FN,        ##args)
#endif

#define Fatal_tmsp_fl_fn(msg)   (_LOG_S(PARAM_FATAL,    F_TMSTMP_FL_FN|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_tmsp_fl_fn(msg)    _LOG_S(PARAM_ERROR,    F_TMSTMP_FL_FN,        msg)
#define Warn_tmsp_fl_fn(msg)     _LOG_S(PARAM_WARN,     F_TMSTMP_FL_FN,        msg)
#define Critical_tmsp_fl_fn(msg) _LOG_S(PARAM_CRITICAL, F_TMSTMP_FL_FN,        msg)
#define Info_tmsp_fl_fn(msg)     _LOG_S(PARAM_INFO,     F_TMSTMP_FL_FN,        msg)
#define Debug_tmsp_fl_fn(msg)    _LOG_S(PARAM_DEBUG,    F_TMSTMP_FL_FN,        msg)
#define Trace_tmsp_fl_fn(msg)    _LOG_S(PARAM_TRACE,    F_TMSTMP_FL_FN,        msg)

#ifdef _windows_
#define SynFatalf_tmsp_fl_fn(fmt,...) (_LOG(PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_tmsp_fl_fn(args...) (_LOG(PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_tmsp_fl_fn(msg) (_LOG_S(PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, msg), _LOG_WAIT())

// F_TEXT

#ifdef _windows_
#define Fatalf_text(fmt,...)   (_LOG(PARAM_FATAL,    F_TEXT|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_text(fmt,...)    _LOG(PARAM_ERROR,    F_TEXT, fmt, ##__VA_ARGS__)
#define Warnf_text(fmt,...)     _LOG(PARAM_WARN,     F_TEXT, fmt, ##__VA_ARGS__)
#define Criticalf_text(fmt,...) _LOG(PARAM_CRITICAL, F_TEXT, fmt, ##__VA_ARGS__)
#define Infof_text(fmt,...)     _LOG(PARAM_INFO,     F_TEXT, fmt, ##__VA_ARGS__)
#define Debugf_text(fmt,...)    _LOG(PARAM_DEBUG,    F_TEXT, fmt, ##__VA_ARGS__)
#define Tracef_text(fmt,...)    _LOG(PARAM_TRACE,    F_TEXT, fmt, ##__VA_ARGS__)
#else
#define Fatalf_text(args...)   (_LOG(PARAM_FATAL,    F_TEXT|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_text(args...)    _LOG(PARAM_ERROR,    F_TEXT, ##args)
#define Warnf_text(args...)     _LOG(PARAM_WARN,     F_TEXT, ##args)
#define Criticalf_text(args...) _LOG(PARAM_CRITICAL, F_TEXT, ##args)
#define Infof_text(args...)     _LOG(PARAM_INFO,     F_TEXT, ##args)
#define Debugf_text(args...)    _LOG(PARAM_DEBUG,    F_TEXT, ##args)
#define Tracef_text(args...)    _LOG(PARAM_TRACE,    F_TEXT, ##args)
#endif

#define Fatal_text(msg)   (_LOG_S(PARAM_FATAL,    F_TEXT|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_text(msg)    _LOG_S(PARAM_ERROR,    F_TEXT, msg)
#define Warn_text(msg)     _LOG_S(PARAM_WARN,     F_TEXT, msg)
#define Critical_text(msg) _LOG_S(PARAM_CRITICAL, F_TEXT, msg)
#define Info_text(msg)     _LOG_S(PARAM_INFO,     F_TEXT, msg)
#define Debug_text(msg)    _LOG_S(PARAM_DEBUG,    F_TEXT, msg)
#define Trace_text(msg)    _LOG_S(PARAM_TRACE,    F_TEXT, msg)

#ifdef _windows_
#define SynFatalf_text(fmt,...) (_LOG(PARAM_FATAL, F_TEXT|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_text(args...) (_LOG(PARAM_FATAL, F_TEXT|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_text(msg) (_LOG_S(PARAM_FATAL, F_TEXT|F_SYNC, msg), _LOG_WAIT())

// F_PURE

#ifdef _windows_
#define Fatalf_pure(fmt,...)   (_LOG(PARAM_FATAL,    F_PURE|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT(), std::abort())
#define Errorf_pure(fmt,...)    _LOG(PARAM_ERROR,    F_PURE, fmt, ##__VA_ARGS__)
#define Warnf_pure(fmt,...)     _LOG(PARAM_WARN,     F_PURE, fmt, ##__VA_ARGS__)
#define Criticalf_pure(fmt,...) _LOG(PARAM_CRITICAL, F_PURE, fmt, ##__VA_ARGS__)
#define Infof_pure(fmt,...)     _LOG(PARAM_INFO,     F_PURE, fmt, ##__VA_ARGS__)
#define Debugf_pure(fmt,...)    _LOG(PARAM_DEBUG,    F_PURE, fmt, ##__VA_ARGS__)
#define Tracef_pure(fmt,...)    _LOG(PARAM_TRACE,    F_PURE, fmt, ##__VA_ARGS__)
#else
#define Fatalf_pure(args...)   (_LOG(PARAM_FATAL,    F_PURE|F_SYNC, ##args), _LOG_WAIT(), std::abort())
#define Errorf_pure(args...)    _LOG(PARAM_ERROR,    F_PURE, ##args)
#define Warnf_pure(args...)     _LOG(PARAM_WARN,     F_PURE, ##args)
#define Criticalf_pure(args...) _LOG(PARAM_CRITICAL, F_PURE, ##args)
#define Infof_pure(args...)     _LOG(PARAM_INFO,     F_PURE, ##args)
#define Debugf_pure(args...)    _LOG(PARAM_DEBUG,    F_PURE, ##args)
#define Tracef_pure(args...)    _LOG(PARAM_TRACE,    F_PURE, ##args)
#endif

#define Fatal_pure(msg)   (_LOG_S(PARAM_FATAL,    F_PURE|F_SYNC, msg), _LOG_WAIT(), std::abort())
#define Error_pure(msg)    _LOG_S(PARAM_ERROR,    F_PURE, msg)
#define Warn_pure(msg)     _LOG_S(PARAM_WARN,     F_PURE, msg)
#define Critical_pure(msg) _LOG_S(PARAM_CRITICAL, F_PURE, msg)
#define Info_pure(msg)     _LOG_S(PARAM_INFO,     F_PURE, msg)
#define Debug_pure(msg)    _LOG_S(PARAM_DEBUG,    F_PURE, msg)
#define Trace_pure(msg)    _LOG_S(PARAM_TRACE,    F_PURE, msg)

#ifdef _windows_
#define SynFatalf_pure(fmt,...) (_LOG(PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__), _LOG_WAIT())
#else
#define SynFatalf_pure(args...) (_LOG(PARAM_FATAL, F_PURE|F_SYNC, ##args), _LOG_WAIT())
#endif

#define SynFatal_pure(msg) (_LOG_S(PARAM_FATAL, F_PURE|F_SYNC, msg), _LOG_WAIT())

#endif