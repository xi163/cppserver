#ifndef INCLUDE_LOGGERIMPL_H
#define INCLUDE_LOGGERIMPL_H

#include "../../Macro.h"
#include "../../utils/impl/backtrace.h"

#include <mutex>
//#include <shared_mutex>
#include <thread>
#include <condition_variable>

#include "../../atomic/Atomic.h"

#include "../../timer/timer.h"

#ifdef __STACK__
#undef __STACK__
#endif
#define __STACK__       utils::_stack_backtrace().c_str()

#define _PARAM_FATAL     LVL_FATAL,__FILE__,__LINE__,__FUNC__,__STACK__
#define _PARAM_ERROR     LVL_ERROR,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_WARN      LVL_WARN ,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_INFO      LVL_INFO ,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_TRACE     LVL_TRACE,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_DEBUG     LVL_DEBUG,__FILE__,__LINE__,__FUNC__,NULL

namespace LOGGER {
	
	class Logger;
	class LoggerImpl {
		friend class Logger;
	private:
#if 0
		typedef std::pair<size_t, int> Flags;
		typedef std::pair<std::string, std::string> Message;
		typedef std::pair<Message, Flags> MessageT;
		typedef std::vector<MessageT> Messages;
#else
		typedef std::pair<size_t, int> Flags;
		typedef std::pair<std::string, std::string> Message;
		typedef std::pair<Message, Flags> MessageT;
		typedef std::list<MessageT> Messages;
#endif
	private:
		LoggerImpl();
		~LoggerImpl();
	public:
		static LoggerImpl* instance();
	public:
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
		void init(char const* dir, char const* prename, size_t logsize);
		void write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void write_s_fatal(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void wait();
		void enable();
		void disable(int delay = 0, bool sync = false);
	private:
		bool check(int level);
		size_t format_s(char const* file, int line, char const* func, int level, int flag, char* buffer, size_t size);
		void notify(char const* msg, size_t len, size_t pos, int flag, char const* stack, size_t stacklen);
		void stdoutbuf(char const* msg, size_t len, size_t pos, int level, int flag, char const* stack = NULL, size_t stacklen = 0);
	private:
		void open(char const* path);
		static void write(fd_t fd, char const* msg, size_t len);
		void write(char const* msg, size_t len, size_t pos, int flag);
		void write(char const* msg, size_t len);
		void close();
		void shift(struct tm const& tm, struct timeval const& tv);
		bool update(struct tm& tm, struct timeval& tv);
		void get(struct tm& tm, struct timeval& tv);
		void wait(Messages& msgs);
		bool consume(struct tm const& tm, struct timeval const& tv, Messages& msgs);
		bool started();
		bool start();
		bool valid();
		void notify_started();
		void wait_started();
		void sync();
		void stop();
		bool utcOk();
		void setting(bool v);
		static void setting(struct tm const& tm, int timezone);
		void openConsole();
		void closeConsole();
		void doConsole(int const cmd);
	private:
		bool _setTimezone(int64_t timezone);
		bool _setMode(int mode);
		bool _setStyle(int style);
		bool _setLevel(int level);
		bool _checkDir();
		std::string const _name(bool space);
	private:
		bool mkdir_ = false;
		bool utcOk_ = true;
		fd_t fd_ = INVALID_HANDLE_VALUE;
		pid_t pid_ = 0;
		int day_ = -1;
		size_t size_ = 0;
		std::atomic<int> timezone_{ MY_CST };
		std::atomic<int> level_{ LVL_DEBUG };
		std::atomic<int> mode_{ M_STDOUT_FILE };
		std::atomic<int> style_{ F_DETAIL };
		char prefix_[256] = { 0 };
		char path_[512] = { 0 };
		char prename_[30] = { 0 };
		struct timeval tv_ = { 0 };
		struct tm tm_ = { 0 };
		//mutable std::shared_mutex tm_mutex_;
		mutable boost::shared_mutex tm_mutex_;
		std::thread thread_;
		bool started_ = false;
		std::atomic_bool done_{ false };
		std::atomic_flag starting_{ ATOMIC_FLAG_INIT };
		std::mutex start_mutex_, mutex_;
		std::condition_variable start_cond_, cond_;
		Messages messages_;
		bool sync_ = false;
		std::mutex sync_mutex_;
		std::condition_variable sync_cond_;
		bool isConsoleOpen_ = false;
		std::atomic_bool enable_{ false };
		std::atomic_flag isDoing_{ ATOMIC_FLAG_INIT };
		utils::Timer timer_;
	};
}

#define __LOG_INIT LOGGER::LoggerImpl::instance()->init
#define __LOG LOGGER::LoggerImpl::instance()->write
#define __LOG_S LOGGER::LoggerImpl::instance()->write_s
#define __LOG_F LOGGER::LoggerImpl::instance()->write_s_fatal
#define __LOG_SET_TIMEZONE LOGGER::LoggerImpl::instance()->setTimezone
#define __LOG_SET_LEVEL LOGGER::LoggerImpl::instance()->setLevel
#define __LOG_SET_MODE LOGGER::LoggerImpl::instance()->setMode
#define __LOG_SET_STYLE LOGGER::LoggerImpl::instance()->setStyle
#define __LOG_SET_COLOR LOGGER::LoggerImpl::instance()->setColor
#define __LOG_TIMEZONE LOGGER::LoggerImpl::instance()->getTimezone
#define __LOG_LEVEL LOGGER::LoggerImpl::instance()->getLevel
#define __LOG_MODE LOGGER::LoggerImpl::instance()->getMode
#define __LOG_STYLE LOGGER::LoggerImpl::instance()->getStyle
#define __LOG_TIMEZONE_STR LOGGER::LoggerImpl::instance()->timezoneString
#define __LOG_LEVEL_STR LOGGER::LoggerImpl::instance()->levelString
#define __LOG_MODE_STR LOGGER::LoggerImpl::instance()->modeString
#define __LOG_STYLE_STR LOGGER::LoggerImpl::instance()->styleString
#define __LOG_WAIT LOGGER::LoggerImpl::instance()->wait
#define __LOG_CONSOLE_OPEN LOGGER::LoggerImpl::instance()->enable
#define __LOG_CONSOLE_CLOSE LOGGER::LoggerImpl::instance()->disable
#define __LOG_FORMAT LOGGER::LoggerImpl::instance()->format
#define __LOG_STARTED LOGGER::LoggerImpl::instance()->started
#define __LOG_CHECK LOGGER::LoggerImpl::instance()->check
#define __LOG_CHECK_NOTIFY LOGGER::LoggerImpl::instance()->notify
#define __LOG_CHECK_STDOUT LOGGER::LoggerImpl::instance()->stdoutbuf

#define __LOG_SET_FATAL __LOG_SET_LEVEL(LVL_FATAL)
#define __LOG_SET_ERROR __LOG_SET_LEVEL(LVL_ERROR)
#define __LOG_SET_WARN  __LOG_SET_LEVEL(LVL_WARN)
#define __LOG_SET_INFO  __LOG_SET_LEVEL(LVL_INFO)
#define __LOG_SET_TRACE __LOG_SET_LEVEL(LVL_TRACE)
#define __LOG_SET_DEBUG __LOG_SET_LEVEL(LVL_DEBUG)

#define __LOG_COLOR_FATAL(a,b) __LOG_SET_COLOR(LVL_FATAL, a, b)
#define __LOG_COLOR_ERROR(a,b) __LOG_SET_COLOR(LVL_ERROR, a, b)
#define __LOG_COLOR_WARN(a,b)  __LOG_SET_COLOR(LVL_WARN,  a, b)
#define __LOG_COLOR_INFO(a,b)  __LOG_SET_COLOR(LVL_INFO,  a, b)
#define __LOG_COLOR_TRACE(a,b) __LOG_SET_COLOR(LVL_TRACE, a, b)
#define __LOG_COLOR_DEBUG(a,b) __LOG_SET_COLOR(LVL_DEBUG, a, b)

// F_DETAIL/F_TMSTMP/F_FN/F_TMSTMP_FN/F_FL/F_TMSTMP_FL/F_FL_FN/F_TMSTMP_FL_FN/F_TEXT/F_PURE

#ifdef _windows_
#define __LOG_FATAL(fmt,...) __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __LOG_ERROR(fmt,...) __LOG(_PARAM_ERROR, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define __LOG_WARN(fmt,...)  __LOG(_PARAM_WARN,  __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define __LOG_INFO(fmt,...)  __LOG(_PARAM_INFO,  __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define __LOG_TRACE(fmt,...) __LOG(_PARAM_TRACE, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define __LOG_DEBUG(fmt,...) __LOG(_PARAM_DEBUG, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATAL(args...) __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __LOG_ERROR(args...) __LOG(_PARAM_ERROR, __LOG_STYLE(),        ##args)
#define __LOG_WARN(args...)  __LOG(_PARAM_WARN,  __LOG_STYLE(),        ##args)
#define __LOG_INFO(args...)  __LOG(_PARAM_INFO,  __LOG_STYLE(),        ##args)
#define __LOG_TRACE(args...) __LOG(_PARAM_TRACE, __LOG_STYLE(),        ##args)
#define __LOG_DEBUG(args...) __LOG(_PARAM_DEBUG, __LOG_STYLE(),        ##args)
#endif

#define __LOG_S_FATAL(msg) __LOG_F(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, msg)
#define __LOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, __LOG_STYLE(),        msg)
#define __LOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  __LOG_STYLE(),        msg)
#define __LOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  __LOG_STYLE(),        msg)
#define __LOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, __LOG_STYLE(),        msg)
#define __LOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, __LOG_STYLE(),        msg)

#ifdef _windows_
#define __LOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __LOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, ##args); __LOG_WAIT()
#endif

#define __LOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, msg); __LOG_WAIT()

// F_DETAIL

#ifdef _windows_
#define __DLOG_FATAL(fmt,...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __DLOG_ERROR(fmt,...) __LOG(_PARAM_ERROR, F_DETAIL,        fmt, ##__VA_ARGS__)
#define __DLOG_WARN(fmt,...)  __LOG(_PARAM_WARN,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define __DLOG_INFO(fmt,...)  __LOG(_PARAM_INFO,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define __DLOG_TRACE(fmt,...) __LOG(_PARAM_TRACE, F_DETAIL,        fmt, ##__VA_ARGS__)
#define __DLOG_DEBUG(fmt,...) __LOG(_PARAM_DEBUG, F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define __DLOG_FATAL(args...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __DLOG_ERROR(args...) __LOG(_PARAM_ERROR, F_DETAIL,        ##args)
#define __DLOG_WARN(args...)  __LOG(_PARAM_WARN,  F_DETAIL,        ##args)
#define __DLOG_INFO(args...)  __LOG(_PARAM_INFO,  F_DETAIL,        ##args)
#define __DLOG_TRACE(args...) __LOG(_PARAM_TRACE, F_DETAIL,        ##args)
#define __DLOG_DEBUG(args...) __LOG(_PARAM_DEBUG, F_DETAIL,        ##args)
#endif

#define __DLOG_S_FATAL(msg) __LOG_F(_PARAM_FATAL, F_DETAIL|F_SYNC, msg)
#define __DLOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_DETAIL,        msg)
#define __DLOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_DETAIL,        msg)
#define __DLOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_DETAIL,        msg)
#define __DLOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_DETAIL,        msg)
#define __DLOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_DETAIL,        msg)

#ifdef _windows_
#define __DLOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __DLOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT()
#endif

#define __DLOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_DETAIL|F_SYNC, msg); __LOG_WAIT()

// F_TMSTMP

#ifdef _windows_
#define __TLOG_FATAL(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __TLOG_ERROR(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define __TLOG_WARN(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define __TLOG_INFO(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define __TLOG_TRACE(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define __TLOG_DEBUG(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP,        fmt, ##__VA_ARGS__)
#else
#define __TLOG_FATAL(args...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __TLOG_ERROR(args...)  __LOG(_PARAM_ERROR, F_TMSTMP,        ##args)
#define __TLOG_WARN(args...)   __LOG(_PARAM_WARN,  F_TMSTMP,        ##args)
#define __TLOG_INFO(args...)   __LOG(_PARAM_INFO,  F_TMSTMP,        ##args)
#define __TLOG_TRACE(args...)  __LOG(_PARAM_TRACE, F_TMSTMP,        ##args)
#define __TLOG_DEBUG(args...)  __LOG(_PARAM_DEBUG, F_TMSTMP,        ##args)
#endif

#define __TLOG_S_FATAL(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg)
#define __TLOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP,        msg)
#define __TLOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP,        msg)
#define __TLOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP,        msg)
#define __TLOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP,        msg)
#define __TLOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP,        msg)

#ifdef _windows_
#define __TLOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __TLOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); __LOG_WAIT()
#endif

#define __TLOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg); __LOG_WAIT()

// F_FN

#ifdef _windows_
#define __LOG_FATALF(fmt,...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define __LOG_ERRORF(fmt,...) __LOG(_PARAM_ERROR, F_FN,        fmt, ##__VA_ARGS__)
#define __LOG_WARNF(fmt,...)  __LOG(_PARAM_WARN,  F_FN,        fmt, ##__VA_ARGS__)
#define __LOG_INFOF(fmt,...)  __LOG(_PARAM_INFO,  F_FN,        fmt, ##__VA_ARGS__)
#define __LOG_TRACEF(fmt,...) __LOG(_PARAM_TRACE, F_FN,        fmt, ##__VA_ARGS__)
#define __LOG_DEBUGF(fmt,...) __LOG(_PARAM_DEBUG, F_FN,        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATALF(args...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define __LOG_ERRORF(args...) __LOG(_PARAM_ERROR, F_FN,        ##args)
#define __LOG_WARNF(args...)  __LOG(_PARAM_WARN,  F_FN,        ##args)
#define __LOG_INFOF(args...)  __LOG(_PARAM_INFO,  F_FN,        ##args)
#define __LOG_TRACEF(args...) __LOG(_PARAM_TRACE, F_FN,        ##args)
#define __LOG_DEBUGF(args...) __LOG(_PARAM_DEBUG, F_FN,        ##args)
#endif

#define __LOG_S_FATALF(msg) __LOG_F(_PARAM_FATAL, F_FN|F_SYNC, msg)
#define __LOG_S_ERRORF(msg) __LOG_S(_PARAM_ERROR, F_FN,        msg)
#define __LOG_S_WARNF(msg)  __LOG_S(_PARAM_WARN,  F_FN,        msg)
#define __LOG_S_INFOF(msg)  __LOG_S(_PARAM_INFO,  F_FN,        msg)
#define __LOG_S_TRACEF(msg) __LOG_S(_PARAM_TRACE, F_FN,        msg)
#define __LOG_S_DEBUGF(msg) __LOG_S(_PARAM_DEBUG, F_FN,        msg)

#ifdef _windows_
#define __LOG_FATALF_SYN(fmt,...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define __LOG_FATALF_SYN(args...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, ##args); _LOG_WAIT()
#endif

#define __LOG_S_FATALF_SYN(msg) __LOG_S(_PARAM_FATAL, F_FN|F_SYNC, msg); _LOG_WAIT()

// F_TMSTMP_FN

#ifdef _windows_
#define __LOG_FATALTF(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define __LOG_ERRORTF(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define __LOG_WARNTF(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define __LOG_INFOTF(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define __LOG_TRACETF(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define __LOG_DEBUGTF(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATALTF(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define __LOG_ERRORTF(args...) __LOG(_PARAM_ERROR, F_TMSTMP_FN,        ##args)
#define __LOG_WARNTF(args...)  __LOG(_PARAM_WARN,  F_TMSTMP_FN,        ##args)
#define __LOG_INFOTF(args...)  __LOG(_PARAM_INFO,  F_TMSTMP_FN,        ##args)
#define __LOG_TRACETF(args...) __LOG(_PARAM_TRACE, F_TMSTMP_FN,        ##args)
#define __LOG_DEBUGTF(args...) __LOG(_PARAM_DEBUG, F_TMSTMP_FN,        ##args)
#endif

#define __LOG_S_FATALTF(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, msg)
#define __LOG_S_ERRORTF(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP_FN,        msg)
#define __LOG_S_WARNTF(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP_FN,        msg)
#define __LOG_S_INFOTF(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP_FN,        msg)
#define __LOG_S_TRACETF(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP_FN,        msg)
#define __LOG_S_DEBUGTF(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP_FN,        msg)

#ifdef _windows_
#define __LOG_FATALTF_SYN(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define __LOG_FATALTF_SYN(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, ##args); _LOG_WAIT()
#endif

#define __LOG_S_FATALTF_SYN(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, msg); _LOG_WAIT()

// F_FL

// F_TMSTMP_FL

// F_FL_FN

#ifdef _windows_
#define __LOG_FATALLF(fmt,...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define __LOG_ERRORLF(fmt,...) __LOG(_PARAM_ERROR, F_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_WARNLF(fmt,...)  __LOG(_PARAM_WARN,  F_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_INFOLF(fmt,...)  __LOG(_PARAM_INFO,  F_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_TRACELF(fmt,...) __LOG(_PARAM_TRACE, F_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_DEBUGLF(fmt,...) __LOG(_PARAM_DEBUG, F_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATALLF(args...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define __LOG_ERRORLF(args...) __LOG(_PARAM_ERROR, F_FL_FN,        ##args)
#define __LOG_WARNLF(args...)  __LOG(_PARAM_WARN,  F_FL_FN,        ##args)
#define __LOG_INFOLF(args...)  __LOG(_PARAM_INFO,  F_FL_FN,        ##args)
#define __LOG_TRACELF(args...) __LOG(_PARAM_TRACE, F_FL_FN,        ##args)
#define __LOG_DEBUGLF(args...) __LOG(_PARAM_DEBUG, F_FL_FN,        ##args)
#endif

#define __LOG_S_FATALLF(msg) __LOG_F(_PARAM_FATAL, F_FL_FN|F_SYNC, msg)
#define __LOG_S_ERRORLF(msg) __LOG_S(_PARAM_ERROR, F_FL_FN,        msg)
#define __LOG_S_WARNLF(msg)  __LOG_S(_PARAM_WARN,  F_FL_FN,        msg)
#define __LOG_S_INFOLF(msg)  __LOG_S(_PARAM_INFO,  F_FL_FN,        msg)
#define __LOG_S_TRACELF(msg) __LOG_S(_PARAM_TRACE, F_FL_FN,        msg)
#define __LOG_S_DEBUGLF(msg) __LOG_S(_PARAM_DEBUG, F_FL_FN,        msg)

#ifdef _windows_
#define __LOG_FATALLF_SYN(fmt,...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define __LOG_FATALLF_SYN(args...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, ##args); _LOG_WAIT()
#endif

#define __LOG_S_FATALLF_SYN(msg) __LOG_S(_PARAM_FATAL, F_FL_FN|F_SYNC, msg); _LOG_WAIT()

// F_TMSTMP_FL_FN

#ifdef _windows_
#define __LOG_FATALTLF(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT(); std::abort()
#define __LOG_ERRORTLF(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_WARNTLF(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_INFOTLF(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_TRACETLF(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define __LOG_DEBUGTLF(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATALTLF(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, ##args); _LOG_WAIT(); std::abort()
#define __LOG_ERRORTLF(args...) __LOG(_PARAM_ERROR, F_TMSTMP_FL_FN,        ##args)
#define __LOG_WARNTLF(args...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL_FN,        ##args)
#define __LOG_INFOTLF(args...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL_FN,        ##args)
#define __LOG_TRACETLF(args...) __LOG(_PARAM_TRACE, F_TMSTMP_FL_FN,        ##args)
#define __LOG_DEBUGTLF(args...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL_FN,        ##args)
#endif

#define __LOG_S_FATALTLF(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, msg)
#define __LOG_S_ERRORTLF(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP_FL_FN,        msg)
#define __LOG_S_WARNTLF(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP_FL_FN,        msg)
#define __LOG_S_INFOTLF(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP_FL_FN,        msg)
#define __LOG_S_TRACETLF(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP_FL_FN,        msg)
#define __LOG_S_DEBUGTLF(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP_FL_FN,        msg)

#ifdef _windows_
#define __LOG_FATALTLF_SYN(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); _LOG_WAIT()
#else
#define __LOG_FATALTLF_SYN(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, ##args); _LOG_WAIT()
#endif

#define __LOG_S_FATALTLF_SYN(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, msg); _LOG_WAIT()

// F_TEXT

// F_PURE

#ifdef _windows_
#define __PLOG_FATAL(fmt,...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __PLOG_ERROR(fmt,...)  __LOG(_PARAM_ERROR, F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_WARN(fmt,...)   __LOG(_PARAM_WARN,  F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_INFO(fmt,...)   __LOG(_PARAM_INFO,  F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_TRACE(fmt,...)  __LOG(_PARAM_TRACE, F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_DEBUG(fmt,...)  __LOG(_PARAM_DEBUG, F_PURE, fmt, ##__VA_ARGS__)
#else
#define __PLOG_FATAL(args...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __PLOG_ERROR(args...)  __LOG(_PARAM_ERROR, F_PURE, ##args)
#define __PLOG_WARN(args...)   __LOG(_PARAM_WARN,  F_PURE, ##args)
#define __PLOG_INFO(args...)   __LOG(_PARAM_INFO,  F_PURE, ##args)
#define __PLOG_TRACE(args...)  __LOG(_PARAM_TRACE, F_PURE, ##args)
#define __PLOG_DEBUG(args...)  __LOG(_PARAM_DEBUG, F_PURE, ##args)
#endif

#define __PLOG_S_FATAL(msg) __LOG_F(_PARAM_FATAL, F_PURE|F_SYNC, msg)
#define __PLOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_PURE, msg)
#define __PLOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_PURE, msg)
#define __PLOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_PURE, msg)
#define __PLOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_PURE, msg)
#define __PLOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_PURE, msg)

#ifdef _windows_
#define __PLOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __PLOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, ##args); __LOG_WAIT()
#endif

#define __PLOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_PURE|F_SYNC, msg); __LOG_WAIT()

#endif