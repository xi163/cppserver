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

#define _PARAM_FATAL     0,__FILE__,__LINE__,__FUNC__,__STACK__
#define _PARAM_ERROR     1,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_WARN      2,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_INFO      3,__FILE__,__LINE__,__FUNC__,NULL
#define _PARAM_TRACE     4,__FILE__,__LINE__,__FUNC__,__STACK__
#define _PARAM_DEBUG     5,__FILE__,__LINE__,__FUNC__,NULL

namespace LOGGER {
	
	class Logger;
	class LoggerImpl {
		friend class Logger;
	private:
#if 0
		typedef std::pair<size_t, uint8_t> Flags;
		typedef std::pair<std::string, std::string> Message;
		typedef std::pair<Message, Flags> MessageT;
		typedef std::vector<MessageT> Messages;
#else
		typedef std::pair<size_t, uint8_t> Flags;
		typedef std::pair<std::string, std::string> Message;
		typedef std::pair<Message, Flags> MessageT;
		typedef std::list<MessageT> Messages;
#endif
	private:
		LoggerImpl();
		~LoggerImpl();
	public:
		static LoggerImpl* instance();
		void set_timezone(int64_t timezone = MY_CST);
		void set_level(int level);
		char const* get_level();
		void set_color(int level, int title, int text);
		void init(char const* dir, int level, char const* prename, size_t logsize);
		void write(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, char const* fmt, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, std::string const& msg);
		void wait();
		void enable();
		void disable(int delay = 0, bool sync = false);
		void stop();
	private:
		bool started();
		bool check(int level);
		size_t format(int level, char const* file, int line, char const* func, uint8_t flag, char* buffer, size_t size);
		void notify(char const* msg, size_t len, size_t pos, uint8_t flag, char const* stack, size_t stacklen);
		void stdoutbuf(int level, char const* msg, size_t len, size_t pos, uint8_t flag, char const* stack = NULL, size_t stacklen = 0);
		void checkSync(uint8_t flag);
	private:
		void open(char const* path);
		void write(char const* msg, size_t len, size_t pos, uint8_t flag);
		void close();
		void shift(struct tm const& tm, struct timeval const& tv);
		void update(struct tm& tm, struct timeval& tv);
		void get(struct tm& tm, struct timeval& tv);
		void wait(Messages& msgs);
		bool consume(struct tm const& tm, struct timeval const& tv, Messages& msgs);
		bool start();
		bool valid();
		void sync();
		void flush();
		void timezoneInfo();
		void openConsole();
		void closeConsole();
		void doConsole(int const cmd);
	private:
#ifdef _windows_
		HANDLE fd_ = INVALID_HANDLE_VALUE;
#else
		int fd_ = INVALID_HANDLE_VALUE;
#endif
		pid_t pid_ = 0;
		int day_ = -1;
		size_t size_ = 0;
		std::atomic<int> level_{ LVL_DEBUG };
		char prefix_[256] = { 0 };
		char path_[512] = { 0 };
		int64_t timezone_ = MY_CST;
		struct timeval tv_ = { 0 };
		struct tm tm_ = { 0 };
		//mutable std::shared_mutex tm_mutex_;
		mutable boost::shared_mutex tm_mutex_;
		std::thread thread_;
		bool started_ = false;
		std::atomic_bool done_{ false };
		std::atomic_flag starting_{ ATOMIC_FLAG_INIT };
		std::mutex mutex_;
		std::condition_variable cond_;
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
#define __LOG_SET LOGGER::LoggerImpl::instance()->set_level
#define __LOG_LVL LOGGER::LoggerImpl::instance()->get_level
#define __LOG_TIMEZONE LOGGER::LoggerImpl::instance()->set_timezone
#define __LOG_WAIT LOGGER::LoggerImpl::instance()->wait
#define __LOG_COLOR LOGGER::LoggerImpl::instance()->set_color
#define __LOG_CONSOLE_OPEN LOGGER::LoggerImpl::instance()->enable
#define __LOG_CONSOLE_CLOSE LOGGER::LoggerImpl::instance()->disable
#define __LOG_STOP LOGGER::LoggerImpl::instance()->stop
#define __LOG_FORMAT LOGGER::LoggerImpl::instance()->format
#define __LOG_STARTED LOGGER::LoggerImpl::instance()->started
#define __LOG_CHECK LOGGER::LoggerImpl::instance()->check
#define __LOG_CHECK_SYNC LOGGER::LoggerImpl::instance()->checkSync
#define __LOG_CHECK_NOTIFY LOGGER::LoggerImpl::instance()->notify
#define __LOG_CHECK_STDOUT LOGGER::LoggerImpl::instance()->stdoutbuf

#define __LOG_SET_FATAL __LOG_SET(LVL_FATAL)
#define __LOG_SET_ERROR __LOG_SET(LVL_ERROR)
#define __LOG_SET_WARN  __LOG_SET(LVL_WARN)
#define __LOG_SET_INFO  __LOG_SET(LVL_INFO)
#define __LOG_SET_TRACE __LOG_SET(LVL_TRACE)
#define __LOG_SET_DEBUG __LOG_SET(LVL_DEBUG)

#define __LOG_COLOR_FATAL(a,b) __LOG_COLOR(LVL_FATAL, a, b)
#define __LOG_COLOR_ERROR(a,b) __LOG_COLOR(LVL_ERROR, a, b)
#define __LOG_COLOR_WARN(a,b)  __LOG_COLOR(LVL_WARN,  a, b)
#define __LOG_COLOR_INFO(a,b)  __LOG_COLOR(LVL_INFO,  a, b)
#define __LOG_COLOR_TRACE(a,b) __LOG_COLOR(LVL_TRACE, a, b)
#define __LOG_COLOR_DEBUG(a,b) __LOG_COLOR(LVL_DEBUG, a, b)

//__LOG_XXX("%s", msg)
#ifdef _windows_
#define __LOG_FATAL(fmt,...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __LOG_ERROR(fmt,...) __LOG(_PARAM_ERROR, F_DETAIL,        fmt, ##__VA_ARGS__)
#define __LOG_WARN(fmt,...)  __LOG(_PARAM_WARN,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define __LOG_INFO(fmt,...)  __LOG(_PARAM_INFO,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define __LOG_TRACE(fmt,...) __LOG(_PARAM_TRACE, F_DETAIL,        fmt, ##__VA_ARGS__)
#define __LOG_DEBUG(fmt,...) __LOG(_PARAM_DEBUG, F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define __LOG_FATAL(args...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __LOG_ERROR(args...) __LOG(_PARAM_ERROR, F_DETAIL,        ##args)
#define __LOG_WARN(args...)  __LOG(_PARAM_WARN,  F_DETAIL,        ##args)
#define __LOG_INFO(args...)  __LOG(_PARAM_INFO,  F_DETAIL,        ##args)
#define __LOG_TRACE(args...) __LOG(_PARAM_TRACE, F_DETAIL,        ##args)
#define __LOG_DEBUG(args...) __LOG(_PARAM_DEBUG, F_DETAIL,        ##args)
#endif

//__LOG_S_XXX(msg)
#define __LOG_S_FATAL(msg) __LOG_S(_PARAM_FATAL, F_DETAIL|F_SYNC, msg); __LOG_WAIT(); std::abort()
#define __LOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_DETAIL,        msg)
#define __LOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_DETAIL,        msg)
#define __LOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_DETAIL,        msg)
#define __LOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_DETAIL,        msg)
#define __LOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_DETAIL,        msg)

//__LOG_XXX("%s", msg)
#ifdef _windows_
#define __LOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __LOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT()
#endif

//__LOG_S_XXX(msg)
#define __LOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_DETAIL|F_SYNC, msg); __LOG_WAIT()

//__TLOG_XXX("%s", msg)
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

//__TLOG_S_XXX(msg)
#define __TLOG_S_FATAL(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg); __LOG_WAIT(); std::abort()
#define __TLOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP,        msg)
#define __TLOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP,        msg)
#define __TLOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP,        msg)
#define __TLOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP,        msg)
#define __TLOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP,        msg)

//__TLOG_XXX("%s", msg)
#ifdef _windows_
#define __TLOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __TLOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); __LOG_WAIT()
#endif

//__TLOG_S_XXX(msg)
#define __TLOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg); __LOG_WAIT()

//__PLOG_XXX("%s", msg)
#ifdef _windows_
#define __PLOG_FATAL(fmt,...)  __LOG(_PARAM_FATAL, F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define __PLOG_ERROR(fmt,...)  __LOG(_PARAM_ERROR, F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_WARN(fmt,...)   __LOG(_PARAM_WARN,  F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_INFO(fmt,...)   __LOG(_PARAM_INFO,  F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_TRACE(fmt,...)  __LOG(_PARAM_TRACE, F_PURE, fmt, ##__VA_ARGS__)
#define __PLOG_DEBUG(fmt,...)  __LOG(_PARAM_DEBUG, F_PURE, fmt, ##__VA_ARGS__)
#else
#define __PLOG_FATAL(args...)  __LOG(_PARAM_FATAL, F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define __PLOG_ERROR(args...)  __LOG(_PARAM_ERROR, F_PURE, ##args)
#define __PLOG_WARN(args...)   __LOG(_PARAM_WARN,  F_PURE, ##args)
#define __PLOG_INFO(args...)   __LOG(_PARAM_INFO,  F_PURE, ##args)
#define __PLOG_TRACE(args...)  __LOG(_PARAM_TRACE, F_PURE, ##args)
#define __PLOG_DEBUG(args...)  __LOG(_PARAM_DEBUG, F_PURE, ##args)
#endif

//__PLOG_S_XXX(msg)
#define __PLOG_S_FATAL(msg) __LOG_S(_PARAM_FATAL, F_SYNC, msg); __LOG_WAIT(); std::abort()
#define __PLOG_S_ERROR(msg) __LOG_S(_PARAM_ERROR, F_PURE, msg)
#define __PLOG_S_WARN(msg)  __LOG_S(_PARAM_WARN,  F_PURE, msg)
#define __PLOG_S_INFO(msg)  __LOG_S(_PARAM_INFO,  F_PURE, msg)
#define __PLOG_S_TRACE(msg) __LOG_S(_PARAM_TRACE, F_PURE, msg)
#define __PLOG_S_DEBUG(msg) __LOG_S(_PARAM_DEBUG, F_PURE, msg)

//__PLOG_XXX("%s", msg)
#ifdef _windows_
#define __PLOG_FATAL_SYN(fmt,...)  __LOG(_PARAM_FATAL, F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define __PLOG_FATAL_SYN(args...)  __LOG(_PARAM_FATAL, F_SYNC, ##args); __LOG_WAIT()
#endif

//__PLOG_S_XXX(msg)
#define __PLOG_S_FATAL_SYN(msg) __LOG_S(_PARAM_FATAL, F_SYNC, msg); __LOG_WAIT()

#endif