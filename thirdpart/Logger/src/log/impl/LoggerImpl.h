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
		void init(char const* dir, char const* logname, size_t logsize);
		void write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...);
		void write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
		void write_fatal_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg);
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
		void write(char const* msg, size_t len, size_t pos, int flag);
		void write(char const* msg, size_t len);
		void close();
		bool mkdir();
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
		void openConsole();
		void closeConsole();
		void doConsole(int const cmd);
	private:
		bool _setTimezone(int64_t timezone);
		bool _setMode(int mode);
		bool _setStyle(int style);
		bool _setLevel(int level);
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
		char name_[30] = { 0 };
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
	template <typename T>
	T* _FatalNotNull(int level, char const* file, int line, char const* func, char const* stack, T* ptr, char const* msg) {
		if (ptr == NULL) {
			::LOGGER::LoggerImpl::instance()->write_fatal_s(level, file, line, func, stack, F_DETAIL | F_SYNC, msg);
		}
		return ptr;
	}
}

#define __LOG_INIT LOGGER::LoggerImpl::instance()->init
#define __LOG LOGGER::LoggerImpl::instance()->write
#define __LOG_S LOGGER::LoggerImpl::instance()->write_s
#define __LOG_F LOGGER::LoggerImpl::instance()->write_fatal_s
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
#define _Fatalf(fmt,...) __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf(fmt,...) __LOG(_PARAM_ERROR, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _Warnf(fmt,...)  __LOG(_PARAM_WARN,  __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _Infof(fmt,...)  __LOG(_PARAM_INFO,  __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _Tracef(fmt,...) __LOG(_PARAM_TRACE, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#define _Debugf(fmt,...) __LOG(_PARAM_DEBUG, __LOG_STYLE(),        fmt, ##__VA_ARGS__)
#else
#define _Fatalf(args...) __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf(args...) __LOG(_PARAM_ERROR, __LOG_STYLE(),        ##args)
#define _Warnf(args...)  __LOG(_PARAM_WARN,  __LOG_STYLE(),        ##args)
#define _Infof(args...)  __LOG(_PARAM_INFO,  __LOG_STYLE(),        ##args)
#define _Tracef(args...) __LOG(_PARAM_TRACE, __LOG_STYLE(),        ##args)
#define _Debugf(args...) __LOG(_PARAM_DEBUG, __LOG_STYLE(),        ##args)
#endif

#define _Fatal(msg) __LOG_F(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, msg)
#define _Error(msg) __LOG_S(_PARAM_ERROR, __LOG_STYLE(),        msg)
#define _Warn(msg)  __LOG_S(_PARAM_WARN,  __LOG_STYLE(),        msg)
#define _Info(msg)  __LOG_S(_PARAM_INFO,  __LOG_STYLE(),        msg)
#define _Trace(msg) __LOG_S(_PARAM_TRACE, __LOG_STYLE(),        msg)
#define _Debug(msg) __LOG_S(_PARAM_DEBUG, __LOG_STYLE(),        msg)

#ifdef _windows_
#define _SynFatalf(fmt,...)  __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf(args...)  __LOG(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal(msg) __LOG_S(_PARAM_FATAL, __LOG_STYLE()|F_SYNC, msg); __LOG_WAIT()

// F_DETAIL

#ifdef _windows_
#define _Fatalf_detail(fmt,...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_detail(fmt,...) __LOG(_PARAM_ERROR, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _Warnf_detail(fmt,...)  __LOG(_PARAM_WARN,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _Infof_detail(fmt,...)  __LOG(_PARAM_INFO,  F_DETAIL,        fmt, ##__VA_ARGS__)
#define _Tracef_detail(fmt,...) __LOG(_PARAM_TRACE, F_DETAIL,        fmt, ##__VA_ARGS__)
#define _Debugf_detail(fmt,...) __LOG(_PARAM_DEBUG, F_DETAIL,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_detail(args...) __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_detail(args...) __LOG(_PARAM_ERROR, F_DETAIL,        ##args)
#define _Warnf_detail(args...)  __LOG(_PARAM_WARN,  F_DETAIL,        ##args)
#define _Infof_detail(args...)  __LOG(_PARAM_INFO,  F_DETAIL,        ##args)
#define _Tracef_detail(args...) __LOG(_PARAM_TRACE, F_DETAIL,        ##args)
#define _Debugf_detail(args...) __LOG(_PARAM_DEBUG, F_DETAIL,        ##args)
#endif

#define _Fatal_detail(msg) __LOG_F(_PARAM_FATAL, F_DETAIL|F_SYNC, msg)
#define _Error_detail(msg) __LOG_S(_PARAM_ERROR, F_DETAIL,        msg)
#define _Warn_detail(msg)  __LOG_S(_PARAM_WARN,  F_DETAIL,        msg)
#define _Info_detail(msg)  __LOG_S(_PARAM_INFO,  F_DETAIL,        msg)
#define _Trace_detail(msg) __LOG_S(_PARAM_TRACE, F_DETAIL,        msg)
#define _Debug_detail(msg) __LOG_S(_PARAM_DEBUG, F_DETAIL,        msg)

#ifdef _windows_
#define _SynFatalf_detail(fmt,...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_detail(args...)  __LOG(_PARAM_FATAL, F_DETAIL|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_detail(msg) __LOG_S(_PARAM_FATAL, F_DETAIL|F_SYNC, msg); __LOG_WAIT()

// F_TMSTMP

#ifdef _windows_
#define _Fatalf_tmsp(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _Warnf_tmsp(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _Infof_tmsp(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _Tracef_tmsp(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP,        fmt, ##__VA_ARGS__)
#define _Debugf_tmsp(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_tmsp(args...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp(args...)  __LOG(_PARAM_ERROR, F_TMSTMP,        ##args)
#define _Warnf_tmsp(args...)   __LOG(_PARAM_WARN,  F_TMSTMP,        ##args)
#define _Infof_tmsp(args...)   __LOG(_PARAM_INFO,  F_TMSTMP,        ##args)
#define _Tracef_tmsp(args...)  __LOG(_PARAM_TRACE, F_TMSTMP,        ##args)
#define _Debugf_tmsp(args...)  __LOG(_PARAM_DEBUG, F_TMSTMP,        ##args)
#endif

#define _Fatal_tmsp(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg)
#define _Error_tmsp(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP,        msg)
#define _Warn_tmsp(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP,        msg)
#define _Info_tmsp(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP,        msg)
#define _Trace_tmsp(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP,        msg)
#define _Debug_tmsp(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP,        msg)

#ifdef _windows_
#define _SynFatalf_tmsp(fmt,...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_tmsp(args...)  __LOG(_PARAM_FATAL, F_TMSTMP|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_tmsp(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP|F_SYNC, msg); __LOG_WAIT()

// F_FN

#ifdef _windows_
#define _Fatalf_fn(fmt,...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_fn(fmt,...) __LOG(_PARAM_ERROR, F_FN,        fmt, ##__VA_ARGS__)
#define _Warnf_fn(fmt,...)  __LOG(_PARAM_WARN,  F_FN,        fmt, ##__VA_ARGS__)
#define _Infof_fn(fmt,...)  __LOG(_PARAM_INFO,  F_FN,        fmt, ##__VA_ARGS__)
#define _Tracef_fn(fmt,...) __LOG(_PARAM_TRACE, F_FN,        fmt, ##__VA_ARGS__)
#define _Debugf_fn(fmt,...) __LOG(_PARAM_DEBUG, F_FN,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_fn(args...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_fn(args...) __LOG(_PARAM_ERROR, F_FN,        ##args)
#define _Warnf_fn(args...)  __LOG(_PARAM_WARN,  F_FN,        ##args)
#define _Infof_fn(args...)  __LOG(_PARAM_INFO,  F_FN,        ##args)
#define _Tracef_fn(args...) __LOG(_PARAM_TRACE, F_FN,        ##args)
#define _Debugf_fn(args...) __LOG(_PARAM_DEBUG, F_FN,        ##args)
#endif

#define _Fatal_fn(msg) __LOG_F(_PARAM_FATAL, F_FN|F_SYNC, msg)
#define _Error_fn(msg) __LOG_S(_PARAM_ERROR, F_FN,        msg)
#define _Warn_fn(msg)  __LOG_S(_PARAM_WARN,  F_FN,        msg)
#define _Info_fn(msg)  __LOG_S(_PARAM_INFO,  F_FN,        msg)
#define _Trace_fn(msg) __LOG_S(_PARAM_TRACE, F_FN,        msg)
#define _Debug_fn(msg) __LOG_S(_PARAM_DEBUG, F_FN,        msg)

#ifdef _windows_
#define _SynFatalf_fn(fmt,...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_fn(args...) __LOG(_PARAM_FATAL, F_FN|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_fn(msg) __LOG_S(_PARAM_FATAL, F_FN|F_SYNC, msg); __LOG_WAIT()

// F_TMSTMP_FN

#ifdef _windows_
#define _Fatalf_tmsp_fn(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fn(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define _Warnf_tmsp_fn(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define _Infof_tmsp_fn(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define _Tracef_tmsp_fn(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#define _Debugf_tmsp_fn(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP_FN,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_tmsp_fn(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fn(args...) __LOG(_PARAM_ERROR, F_TMSTMP_FN,        ##args)
#define _Warnf_tmsp_fn(args...)  __LOG(_PARAM_WARN,  F_TMSTMP_FN,        ##args)
#define _Infof_tmsp_fn(args...)  __LOG(_PARAM_INFO,  F_TMSTMP_FN,        ##args)
#define _Tracef_tmsp_fn(args...) __LOG(_PARAM_TRACE, F_TMSTMP_FN,        ##args)
#define _Debugf_tmsp_fn(args...) __LOG(_PARAM_DEBUG, F_TMSTMP_FN,        ##args)
#endif

#define _Fatal_tmsp_fn(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, msg)
#define _Error_tmsp_fn(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP_FN,        msg)
#define _Warn_tmsp_fn(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP_FN,        msg)
#define _Info_tmsp_fn(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP_FN,        msg)
#define _Trace_tmsp_fn(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP_FN,        msg)
#define _Debug_tmsp_fn(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP_FN,        msg)

#ifdef _windows_
#define _SynFatalf_tmsp_fn(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_tmsp_fn(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_tmsp_fn(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP_FN|F_SYNC, msg); __LOG_WAIT()

// F_FL

#ifdef _windows_
#define _Fatalf_fl(fmt,...) __LOG(_PARAM_FATAL, F_FL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_fl(fmt,...) __LOG(_PARAM_ERROR, F_FL,        fmt, ##__VA_ARGS__)
#define _Warnf_fl(fmt,...)  __LOG(_PARAM_WARN,  F_FL,        fmt, ##__VA_ARGS__)
#define _Infof_fl(fmt,...)  __LOG(_PARAM_INFO,  F_FL,        fmt, ##__VA_ARGS__)
#define _Tracef_fl(fmt,...) __LOG(_PARAM_TRACE, F_FL,        fmt, ##__VA_ARGS__)
#define _Debugf_fl(fmt,...) __LOG(_PARAM_DEBUG, F_FL,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_fl(args...) __LOG(_PARAM_FATAL, F_FL|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_fl(args...) __LOG(_PARAM_ERROR, F_FL,        ##args)
#define _Warnf_fl(args...)  __LOG(_PARAM_WARN,  F_FL,        ##args)
#define _Infof_fl(args...)  __LOG(_PARAM_INFO,  F_FL,        ##args)
#define _Tracef_fl(args...) __LOG(_PARAM_TRACE, F_FL,        ##args)
#define _Debugf_fl(args...) __LOG(_PARAM_DEBUG, F_FL,        ##args)
#endif

#define _Fatal_fl(msg) __LOG_F(_PARAM_FATAL, F_FL|F_SYNC, msg)
#define _Error_fl(msg) __LOG_S(_PARAM_ERROR, F_FL,        msg)
#define _Warn_fl(msg)  __LOG_S(_PARAM_WARN,  F_FL,        msg)
#define _Info_fl(msg)  __LOG_S(_PARAM_INFO,  F_FL,        msg)
#define _Trace_fl(msg) __LOG_S(_PARAM_TRACE, F_FL,        msg)
#define _Debug_fl(msg) __LOG_S(_PARAM_DEBUG, F_FL,        msg)

#ifdef _windows_
#define _SynFatalf_fl(fmt,...) __LOG(_PARAM_FATAL, F_FL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_fl(args...) __LOG(_PARAM_FATAL, F_FL|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_fl(msg) __LOG_S(_PARAM_FATAL, F_FL|F_SYNC, msg); __LOG_WAIT()

// F_TMSTMP_FL

#ifdef _windows_
#define _Fatalf_tmsp_fl(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fl(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define _Warnf_tmsp_fl(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define _Infof_tmsp_fl(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define _Tracef_tmsp_fl(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#define _Debugf_tmsp_fl(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_tmsp_fl(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fl(args...) __LOG(_PARAM_ERROR, F_TMSTMP_FL,        ##args)
#define _Warnf_tmsp_fl(args...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL,        ##args)
#define _Infof_tmsp_fl(args...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL,        ##args)
#define _Tracef_tmsp_fl(args...) __LOG(_PARAM_TRACE, F_TMSTMP_FL,        ##args)
#define _Debugf_tmsp_fl(args...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL,        ##args)
#endif

#define _Fatal_tmsp_fl(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, msg)
#define _Error_tmsp_fl(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP_FL,        msg)
#define _Warn_tmsp_fl(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP_FL,        msg)
#define _Info_tmsp_fl(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP_FL,        msg)
#define _Trace_tmsp_fl(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP_FL,        msg)
#define _Debug_tmsp_fl(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP_FL,        msg)

#ifdef _windows_
#define _SynFatalf_tmsp_fl(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_tmsp_fl(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_tmsp_fl(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP_FL|F_SYNC, msg); __LOG_WAIT()

// F_FL_FN

#ifdef _windows_
#define _Fatalf_fl_fn(fmt,...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_fl_fn(fmt,...) __LOG(_PARAM_ERROR, F_FL_FN,        fmt, ##__VA_ARGS__)
#define _Warnf_fl_fn(fmt,...)  __LOG(_PARAM_WARN,  F_FL_FN,        fmt, ##__VA_ARGS__)
#define _Infof_fl_fn(fmt,...)  __LOG(_PARAM_INFO,  F_FL_FN,        fmt, ##__VA_ARGS__)
#define _Tracef_fl_fn(fmt,...) __LOG(_PARAM_TRACE, F_FL_FN,        fmt, ##__VA_ARGS__)
#define _Debugf_fl_fn(fmt,...) __LOG(_PARAM_DEBUG, F_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_fl_fn(args...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_fl_fn(args...) __LOG(_PARAM_ERROR, F_FL_FN,        ##args)
#define _Warnf_fl_fn(args...)  __LOG(_PARAM_WARN,  F_FL_FN,        ##args)
#define _Infof_fl_fn(args...)  __LOG(_PARAM_INFO,  F_FL_FN,        ##args)
#define _Tracef_fl_fn(args...) __LOG(_PARAM_TRACE, F_FL_FN,        ##args)
#define _Debugf_fl_fn(args...) __LOG(_PARAM_DEBUG, F_FL_FN,        ##args)
#endif

#define _Fatal_fl_fn(msg) __LOG_F(_PARAM_FATAL, F_FL_FN|F_SYNC, msg)
#define _Error_fl_fn(msg) __LOG_S(_PARAM_ERROR, F_FL_FN,        msg)
#define _Warn_fl_fn(msg)  __LOG_S(_PARAM_WARN,  F_FL_FN,        msg)
#define _Info_fl_fn(msg)  __LOG_S(_PARAM_INFO,  F_FL_FN,        msg)
#define _Trace_fl_fn(msg) __LOG_S(_PARAM_TRACE, F_FL_FN,        msg)
#define _Debug_fl_fn(msg) __LOG_S(_PARAM_DEBUG, F_FL_FN,        msg)

#ifdef _windows_
#define _SynFatalf_fl_fn(fmt,...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_fl_fn(args...) __LOG(_PARAM_FATAL, F_FL_FN|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_fl_fn(msg) __LOG_S(_PARAM_FATAL, F_FL_FN|F_SYNC, msg); __LOG_WAIT()

// F_TMSTMP_FL_FN

#ifdef _windows_
#define _Fatalf_tmsp_fl_fn(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fl_fn(fmt,...) __LOG(_PARAM_ERROR, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define _Warnf_tmsp_fl_fn(fmt,...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define _Infof_tmsp_fl_fn(fmt,...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define _Tracef_tmsp_fl_fn(fmt,...) __LOG(_PARAM_TRACE, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#define _Debugf_tmsp_fl_fn(fmt,...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL_FN,        fmt, ##__VA_ARGS__)
#else
#define _Fatalf_tmsp_fl_fn(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_tmsp_fl_fn(args...) __LOG(_PARAM_ERROR, F_TMSTMP_FL_FN,        ##args)
#define _Warnf_tmsp_fl_fn(args...)  __LOG(_PARAM_WARN,  F_TMSTMP_FL_FN,        ##args)
#define _Infof_tmsp_fl_fn(args...)  __LOG(_PARAM_INFO,  F_TMSTMP_FL_FN,        ##args)
#define _Tracef_tmsp_fl_fn(args...) __LOG(_PARAM_TRACE, F_TMSTMP_FL_FN,        ##args)
#define _Debugf_tmsp_fl_fn(args...) __LOG(_PARAM_DEBUG, F_TMSTMP_FL_FN,        ##args)
#endif

#define _Fatal_tmsp_fl_fn(msg) __LOG_F(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, msg)
#define _Error_tmsp_fl_fn(msg) __LOG_S(_PARAM_ERROR, F_TMSTMP_FL_FN,        msg)
#define _Warn_tmsp_fl_fn(msg)  __LOG_S(_PARAM_WARN,  F_TMSTMP_FL_FN,        msg)
#define _Info_tmsp_fl_fn(msg)  __LOG_S(_PARAM_INFO,  F_TMSTMP_FL_FN,        msg)
#define _Trace_tmsp_fl_fn(msg) __LOG_S(_PARAM_TRACE, F_TMSTMP_FL_FN,        msg)
#define _Debug_tmsp_fl_fn(msg) __LOG_S(_PARAM_DEBUG, F_TMSTMP_FL_FN,        msg)

#ifdef _windows_
#define _SynFatalf_tmsp_fl_fn(fmt,...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_tmsp_fl_fn(args...) __LOG(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_tmsp_fl_fn(msg) __LOG_S(_PARAM_FATAL, F_TMSTMP_FL_FN|F_SYNC, msg); __LOG_WAIT()

// F_TEXT

#ifdef _windows_
#define _Fatalf_text(fmt,...)  __LOG(_PARAM_FATAL, F_TEXT|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_text(fmt,...)  __LOG(_PARAM_ERROR, F_TEXT, fmt, ##__VA_ARGS__)
#define _Warnf_text(fmt,...)   __LOG(_PARAM_WARN,  F_TEXT, fmt, ##__VA_ARGS__)
#define _Infof_text(fmt,...)   __LOG(_PARAM_INFO,  F_TEXT, fmt, ##__VA_ARGS__)
#define _Tracef_text(fmt,...)  __LOG(_PARAM_TRACE, F_TEXT, fmt, ##__VA_ARGS__)
#define _Debugf_text(fmt,...)  __LOG(_PARAM_DEBUG, F_TEXT, fmt, ##__VA_ARGS__)
#else
#define _Fatalf_text(args...)  __LOG(_PARAM_FATAL, F_TEXT|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_text(args...)  __LOG(_PARAM_ERROR, F_TEXT, ##args)
#define _Warnf_text(args...)   __LOG(_PARAM_WARN,  F_TEXT, ##args)
#define _Infof_text(args...)   __LOG(_PARAM_INFO,  F_TEXT, ##args)
#define _Tracef_text(args...)  __LOG(_PARAM_TRACE, F_TEXT, ##args)
#define _Debugf_text(args...)  __LOG(_PARAM_DEBUG, F_TEXT, ##args)
#endif

#define _Fatal_text(msg) __LOG_F(_PARAM_FATAL, F_TEXT|F_SYNC, msg)
#define _Error_text(msg) __LOG_S(_PARAM_ERROR, F_TEXT, msg)
#define _Warn_text(msg)  __LOG_S(_PARAM_WARN,  F_TEXT, msg)
#define _Info_text(msg)  __LOG_S(_PARAM_INFO,  F_TEXT, msg)
#define _Trace_text(msg) __LOG_S(_PARAM_TRACE, F_TEXT, msg)
#define _Debug_text(msg) __LOG_S(_PARAM_DEBUG, F_TEXT, msg)

#ifdef _windows_
#define _SynFatalf_text(fmt,...)  __LOG(_PARAM_FATAL, F_TEXT|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_text(args...)  __LOG(_PARAM_FATAL, F_TEXT|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_text(msg) __LOG_S(_PARAM_FATAL, F_TEXT|F_SYNC, msg); __LOG_WAIT()

// F_PURE

#ifdef _windows_
#define _Fatalf_pure(fmt,...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT(); std::abort()
#define _Errorf_pure(fmt,...)  __LOG(_PARAM_ERROR, F_PURE, fmt, ##__VA_ARGS__)
#define _Warnf_pure(fmt,...)   __LOG(_PARAM_WARN,  F_PURE, fmt, ##__VA_ARGS__)
#define _Infof_pure(fmt,...)   __LOG(_PARAM_INFO,  F_PURE, fmt, ##__VA_ARGS__)
#define _Tracef_pure(fmt,...)  __LOG(_PARAM_TRACE, F_PURE, fmt, ##__VA_ARGS__)
#define _Debugf_pure(fmt,...)  __LOG(_PARAM_DEBUG, F_PURE, fmt, ##__VA_ARGS__)
#else
#define _Fatalf_pure(args...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, ##args); __LOG_WAIT(); std::abort()
#define _Errorf_pure(args...)  __LOG(_PARAM_ERROR, F_PURE, ##args)
#define _Warnf_pure(args...)   __LOG(_PARAM_WARN,  F_PURE, ##args)
#define _Infof_pure(args...)   __LOG(_PARAM_INFO,  F_PURE, ##args)
#define _Tracef_pure(args...)  __LOG(_PARAM_TRACE, F_PURE, ##args)
#define _Debugf_pure(args...)  __LOG(_PARAM_DEBUG, F_PURE, ##args)
#endif

#define _Fatal_pure(msg) __LOG_F(_PARAM_FATAL, F_PURE|F_SYNC, msg)
#define _Error_pure(msg) __LOG_S(_PARAM_ERROR, F_PURE, msg)
#define _Warn_pure(msg)  __LOG_S(_PARAM_WARN,  F_PURE, msg)
#define _Info_pure(msg)  __LOG_S(_PARAM_INFO,  F_PURE, msg)
#define _Trace_pure(msg) __LOG_S(_PARAM_TRACE, F_PURE, msg)
#define _Debug_pure(msg) __LOG_S(_PARAM_DEBUG, F_PURE, msg)

#ifdef _windows_
#define _SynFatalf_pure(fmt,...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, fmt, ##__VA_ARGS__); __LOG_WAIT()
#else
#define _SynFatalf_pure(args...)  __LOG(_PARAM_FATAL, F_PURE|F_SYNC, ##args); __LOG_WAIT()
#endif

#define _SynFatal_pure(msg) __LOG_S(_PARAM_FATAL, F_PURE|F_SYNC, msg); __LOG_WAIT()

#endif