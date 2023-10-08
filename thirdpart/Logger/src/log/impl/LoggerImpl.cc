#include "LoggerImpl.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../utils/impl/Console.h"

#ifdef _windows_
#include <process.h>
#include "../../utils/impl/gettimeofday.h"
#elif defined(_linux_)
#endif

#include "color.h"

#ifdef QT_SUPPORT
#include <QDebug>
#endif

#define _S (6)
#define _O (7)
#define _X (8)

#define _PARAM_S     _S,__FILE__,__LINE__,__FUNC__,__STACK__
#define _PARAM_O     _O,__FILE__,__LINE__,__FUNC__,__STACK__
#define _PARAM_X     _X,__FILE__,__LINE__,__FUNC__,__STACK__

#define __LOG_WAIT_S   __LOG_S(_PARAM_S, F_SYNC, ""); __LOG_WAIT()
#define __LOG_NOTIFY_O __LOG_S(_PARAM_O, F_PURE, "")
#define __LOG_NOTIFY_X __LOG_S(_PARAM_X, F_PURE, "")

#define UTC_OK 'T'
#define UTC_ER 'P'

namespace LOGGER {

	LoggerImpl::LoggerImpl() :pid_(getpid()) {
	}

	LoggerImpl::~LoggerImpl() {
		stop();
	}

	LoggerImpl* LoggerImpl::instance() {
		static LoggerImpl logger;
		return &logger;
	}

	void LoggerImpl::setPrename(char const* name) {
		if (name && name[0]) {
			snprintf(name_, sizeof(name_), "%s", name);
		}
	}

	char const* LoggerImpl::getPrename() const {
		return name_;
	}
	
	bool LoggerImpl::_setTimezone(int64_t timezone) {
		switch (timezone) {
		case MY_PST:
		case MY_MST:
		case MY_EST:
		case MY_BST:
		case MY_UTC:
		case MY_GST:
		case MY_CST:
		case MY_JST:
			timezone_.store(timezone);
			return true;
		}
		return false;
	}

	bool LoggerImpl::_setLevel(int level) {
#if 0
		switch (level) {
		case LVL_TRACE:
		case LVL_DEBUG:
		case LVL_INFO:
		case LVL_CRITICAL:
		case LVL_WARN:
		case LVL_ERROR:
		case LVL_FATAL:
			level_.store(level);
			return true;
		}
		return false;
#else
		if (level >= LVL_TRACE) {
			level_.store(LVL_TRACE);
		}
		else if (level <= LVL_FATAL) {
			level_.store(LVL_FATAL);
		}
		else {
			level_.store(level);
		}
		return true;
#endif
	}
	
	bool LoggerImpl::_setMode(int mode) {
		switch (mode) {
		case M_STDOUT_ONLY:
		case M_FILE_ONLY:
		case M_STDOUT_FILE:
			mode_.store(mode);
			return true;
		}
		return false;
	}
	
	bool LoggerImpl::_setStyle(int style) {
		switch (style) {
		case F_DETAIL:
		case F_TMSTMP:
		case F_FN:
		case F_TMSTMP_FN:
		case F_FL:
		case F_TMSTMP_FL:
		case F_FL_FN:
		case F_TMSTMP_FL_FN:
		case F_TEXT:
		case F_PURE:
			style_.store(style);
			return true;
		}
		return false;
	}

	static inline void setting(struct tm const& tm, int timezone, int level, int mode, int style) {
		switch (timezone) {
		case MY_EST: {
			_Infof_fn("%s %s %s %s America/New_York %04d-%02d-%02d %02d:%02d:%02d",
				getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str(),
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_BST: {
			_Infof_fn("%s %s %s %s Europe/London %04d-%02d-%02d %02d:%02d:%02d",
				getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str(),
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_GST: {
			_Infof_fn("%s %s %s %s Asia/Dubai %04d-%02d-%02d %02d:%02d:%02d",
				getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str(),
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_CST: {
			_Infof_fn("%s %s %s %s Beijing (China) %04d-%02d-%02d %02d:%02d:%02d",
				getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str(),
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_JST: {
			_Infof_fn("%s %s %s %s Asia/Tokyo %04d-%02d-%02d %02d:%02d:%02d",
				getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str(),
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		default:
			_Errorf_fl_fn("%s %s %s %s", getLevelDesc(level).c_str(), getModeDesc(mode).c_str(), getStyleDesc(style).c_str(), getTimezoneDesc(timezone).c_str());
			break;
		}
	}
	
	bool LoggerImpl::utcOk() {
		return utcOk_;
	}
	
	void LoggerImpl::setting(bool v) {
		switch (v) {
		case true: {
			struct tm tm = { 0 };
			utcOk_ = utils::_convertUTC(time(NULL), tm, NULL, timezone_.load());
			switch (utcOk_) {
			case true:
				LOGGER::setting(tm, timezone_.load(), level_.load(), mode_.load(), style_.load());
				break;
			default:
				LOGGER::setting(tm, timezone_.load(), level_.load(), mode_.load(), style_.load());
				break;
			}
			break;
		}
		default:
			switch (utcOk()) {
			case true: {
				struct tm tm = { 0 };
				utcOk_ = utils::_convertUTC(time(NULL), tm, NULL, timezone_.load());
				switch (utcOk_) {
				case true:
					LOGGER::setting(tm, timezone_.load(), level_.load(), mode_.load(), style_.load());
					break;
				default:
					LOGGER::setting(tm, timezone_.load(), level_.load(), mode_.load(), style_.load());
					break;
				}
				break;
			}
			default: {
				struct tm tm = { 0 };
				LOGGER::setting(tm, timezone_.load(), level_.load(), mode_.load(), style_.load());
				break;
			}
			}
			break;
		}
	}
	
	char const* LoggerImpl::timezoneString() const {
		return getTimezoneDesc(timezone_.load()).c_str();
	}
	void LoggerImpl::setTimezone(int timezone/* = MY_CST*/) {
		switch (timezone == timezone_.load()) {
		case false:
			switch (_setTimezone(timezone)) {
			case true:
				setting(true);
				break;
			default:
				setting(true);
				break;
			}
			break;
		}
	}
	int LoggerImpl::getTimezone() {
		return timezone_.load();
	}

	char const* LoggerImpl::levelString() const {
		return getLevelDesc(level_.load()).c_str();
	}
	void LoggerImpl::setLevel(int level) {
		switch (level == level_.load()) {
		case false:
			switch (_setLevel(level)) {
			case true:
				setting(false);
				break;
			default:
				setting(false);
				break;
			}
		}
	}
	int LoggerImpl::getLevel() {
		return level_.load();
	}
	
	char const* LoggerImpl::modeString() const {
		return getModeDesc(mode_.load()).c_str();
	}
	void LoggerImpl::setMode(int mode) {
		switch (mode == mode_.load()) {
		case false:
			switch (_setMode(mode)) {
			case true:
				setting(false);
				break;
			default:
				setting(false);
				break;
			}
		}
	}
	int LoggerImpl::getMode() {
		return mode_.load();
	}

	char const* LoggerImpl::styleString() const {
		return getStyleDesc(style_.load()).c_str();
	}
	void LoggerImpl::setStyle(int style) {
		switch (style == style_.load()) {
		case false:
			switch (_setStyle(style)) {
			case true:
				setting(false);
				break;
			default:
				setting(false);
				break;
			}
		}
	}
	int LoggerImpl::getStyle() {
		return style_.load();
	}

	void LoggerImpl::setColor(int level, int clrtitle, int clrtext) {
	}
	
	void LoggerImpl::init(char const* dir, char const* name, size_t logsize) {
		size_ = logsize;
		setPrename(name);
		(name && name[0]) ?
			snprintf(prefix_, sizeof(prefix_), "%s/%s.", ((dir && dir[0]) ? dir : "."), name) :
			snprintf(prefix_, sizeof(prefix_), "%s/", ((dir && dir[0]) ? dir : "."));
		//printf("LoggerImpl::init...\n");
	}
	
	static inline std::string const logname(char const* name, bool space) {
		switch (name[0] == '\0') {
		case true: return "";
		default:
			switch (space) {
			case true:
				return std::string(" <").append(name).append("> ");
			default:
				return std::string(" <").append(name).append(">");
			}
		}
	}
	
	size_t LoggerImpl::format_s(char const* file, int line, char const* func, int level, int flag, char* buffer, size_t size) {
		struct tm tm;
		struct timeval tv;
		bool ok = update(tm, tv);
		static char const chr[] = { 'F','E','W','C','I','D','T','S','O','X' };
		switch (flag) {
		case F_DETAIL:
		case F_DETAIL_SYNC:
			//W101106 CST 21:17:00.024254 199 main.cc:103] server.run xxx
			return snprintf(buffer, size, "%c%c%d%s %s %02d:%02d:%02d.%.6lu %s %s:%d] %s ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				getTimezoneDesc(timezone_.load()).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_gettid().c_str(),
				utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str());
		case F_TMSTMP:
		case F_TMSTMP_SYNC:
			//W101106 CST 21:17:00.024254] xxx
			return snprintf(buffer, size, "%c%c%d%s %s %02d:%02d:%02d.%.6lu] ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				getTimezoneDesc(timezone_.load()).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
		case F_FN:
		case F_FN_SYNC:
			//W101106] server.run xxx
			return snprintf(buffer, size, "%c%c%d%s] %s ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				utils::_trim_func(func).c_str());
		case F_TMSTMP_FN:
		case F_TMSTMP_FN_SYNC:
			//W101106 CST 21:17:00.024254] server.run xxx
			return snprintf(buffer, size, "%c%c%d%s %s %02d:%02d:%02d.%.6lu] %s ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				getTimezoneDesc(timezone_.load()).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_trim_func(func).c_str());
		case F_FL:
		case F_FL_SYNC:
			//W101106 main.cc:103] xxx
			return snprintf(buffer, size, "%c%c%d%s %s:%d] ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				utils::_trim_file(file).c_str(), line);
		case F_TMSTMP_FL:
		case F_TMSTMP_FL_SYNC:
			//W101106 CST 21:17:00.024254 main.cc:103] xxx
			return snprintf(buffer, size, "%c%c%d%s %s %02d:%02d:%02d.%.6lu %s:%d] ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				getTimezoneDesc(timezone_.load()).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_trim_file(file).c_str(), line);
		case F_FL_FN:
		case F_FL_FN_SYNC:
			//W101106 main.cc:103] server.run xxx
			return snprintf(buffer, size, "%c%c%d%s %s:%d] %s ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str());
		case F_TMSTMP_FL_FN:
		case F_TMSTMP_FL_FN_SYNC:
			//W101106 CST 21:17:00.024254 main.cc:103] server.run xxx
			return snprintf(buffer, size, "%c%c%d%s %s %02d:%02d:%02d.%.6lu %s:%d] %s ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str(),
				getTimezoneDesc(timezone_.load()).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str());
		case F_TEXT:
		case F_TEXT_SYNC:
			//W101106] xxx
			return snprintf(buffer, size, "%c%c%d%s] ",
				(ok ? UTC_OK : UTC_ER),
				chr[level], pid_,
				logname(name_, false).c_str());
		case F_PURE:
		case F_PURE_SYNC:
		default:
			//xxx
			return snprintf(buffer, size, "%c%c%s",
				(ok ? UTC_OK : UTC_ER),
				chr[level],
				logname(name_, true).c_str());
		}
	}

	//打印level_及以下级别日志
	bool LoggerImpl::check(int level) {
		return level <= level_.load() || level == _S || level == _O || level == _X;
	}

	void LoggerImpl::write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...) {
		if (check(level)) {
			static size_t const PATHSZ = 512;
			static size_t const MAXSZ = 81920;
			char msg[PATHSZ + MAXSZ + 2];
			size_t pos = format_s(file, line, func, level, flag, msg, PATHSZ);
			va_list ap;
			va_start(ap, format);
#ifdef _windows_
			size_t n = vsnprintf_s(msg + pos, MAXSZ, _TRUNCATE, format, ap);
#else
			size_t n = vsnprintf(msg + pos, MAXSZ, format, ap);
#endif
			va_end(ap);
			msg[pos + n] = '\n';
			msg[pos + n + 1] = '\0';
			start();
			//wait_started();
			notify(msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
		}
	}

	void LoggerImpl::write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
	}

	void LoggerImpl::write_fatal_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
		wait();
		std::abort();
	}
	
	//open
	void LoggerImpl::open(char const* path) {
		assert(path);
#ifdef _windows_
		fd_ = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == fd_) {
			int err = GetLastError();
			std::string errmsg = utils::_str_error(err);
			fprintf(stderr, "open %s error[%d:%s]\n", path, err, errmsg.c_str());
		}
		else {
			SetFilePointer(fd_, 0, NULL, FILE_END);
		}
#else
		fd_ = ::open(path, O_WRONLY | O_CREAT | O_APPEND | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd_ == INVALID_HANDLE_VALUE) {
			std::string errmsg = utils::_str_error(errno);
			fprintf(stderr, "open %s error[%d:%s]\n", path, errno, errmsg.c_str());
	    }
#endif
	}

	//write
	static inline void write(fd_t fd, char const* msg, size_t len) {
#ifdef _windows_
		long size = 0;
		(void)::WriteFile(fd, msg, len, (LPDWORD)&size, NULL);
#else
		(void)::write(fd, msg, len);
#endif
	}

	//close
	void LoggerImpl::close() {

		if (INVALID_HANDLE_VALUE != fd_) {
#ifdef _windows_
			::CloseHandle(fd_);
#else
			::close(fd_);
#endif
			fd_ = INVALID_HANDLE_VALUE;
		}
	}
	
	static inline int getlevel(char const c) {
		switch (c) {
		case 'F': return LVL_FATAL;
		case 'E':return LVL_ERROR;
		case 'W':return LVL_WARN;
		case 'C':return LVL_CRITICAL;
		case 'I':return LVL_INFO;
		case 'D':return LVL_DEBUG;
		case 'T':return LVL_TRACE;
		case 'S':return _S;
		case 'O':return _O;
		case 'X':return _X;
		}
	}
	
	bool LoggerImpl::mkdir() {
		switch (mkdir_) {
		case false:
			//printf("LoggerImpl::mkdir...\n");
			switch (prefix_[0]) {
			case '\0':
				break;
			default:
				std::string s(prefix_);
				size_t pos = s.find_last_of('/');
				utils::_mkDir_p(s.substr(0, pos + 1).c_str());
				mkdir_ = true;
				break;
			}
			break;
		}
		return mkdir_;
	}
	
	void LoggerImpl::shift(struct tm const& tm, struct timeval const& tv) {
		assert(prefix_[0]);
		if (tm.tm_mday != day_) {//new day
			close();
			snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.log",
				prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
				open(path_);
				day_ = tm.tm_mday;
			}
			else {//0 existed
				snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.log",
					prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
				struct stat stStat;
				if (stat(path_, &stStat) < 0) {
					open(path_);
					day_ = tm.tm_mday;
				}
				else {//0 existed
					for (int i = 0;;) {
						snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.%d.log",
							prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec, i);
						if (stat(path_, &stStat) < 0) {
							open(path_);
							day_ = tm.tm_mday;
							break;
						}
						else {//0 existed
							++i;
						}
					}
				}
			}
		}
		else {//current day
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
				close();
				snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.log",
					prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				struct stat stStat;
				if (stat(path_, &stStat) < 0) {
					open(path_);
				}
				else {//0 existed
					snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.log",
						prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
					struct stat stStat;
					if (stat(path_, &stStat) < 0) {
						open(path_);
					}
					else {//0 existed
						for (int i = 0;;) {
							snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.%d.log",
								prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec, i);
							if (stat(path_, &stStat) < 0) {
								open(path_);
								break;
							}
							else {//0 existed
								++i;
							}
						}
					}
				}
			}
			else {//0 existed
				if (stStat.st_size < size_) {
				}
				else {
					close();
					snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.log",
						prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
					struct stat stStat;
					if (stat(path_, &stStat) < 0) {
						open(path_);
					}
					else {//0 existed
						snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.log",
							prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
						if (stat(path_, &stStat) < 0) {
							open(path_);
						}
						else {//0 existed
							for (int i = 0;;) {
								snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.%d.log",
									prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec, i);
								if (stat(path_, &stStat) < 0) {
									open(path_);
									break;
								}
								else {//0 existed
									++i;
								}
							}
						}
					}
				}
			}
		}
	}
	
	//需要调用utils::_initConsole()初始化
	void LoggerImpl::stdoutbuf(char const* msg, size_t len, size_t pos, int level, int flag, char const* stack, size_t stacklen) {
#ifdef QT_CONSOLE
		switch (level) {
		default:
		case LVL_FATAL: qFatal(msg); break;
		case LVL_ERROR: qCritical(msg); break;
		case LVL_WARN: qWarning(msg); break;
		case LVL_CRITICAL: qInfo(msg); break;
		case LVL_INFO: qInfo(msg); break;
		case LVL_DEBUG: qDebug(msg); break;
		case LVL_TRACE: qInfo(msg); break;
		}
#elif defined(_windows_)
		if (!isConsoleOpen_) {
			return;
		}
#endif
		switch (level) {
		case LVL_FATAL: {
			switch (flag) {
			case F_DETAIL:
			case F_DETAIL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_TMSTMP:
			case F_TMSTMP_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_FN:
			case F_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_TMSTMP_FN:
			case F_TMSTMP_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_FL:
			case F_FL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_TMSTMP_FL:
			case F_TMSTMP_FL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_FL_FN:
			case F_FL_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_TMSTMP_FL_FN:
			case F_TMSTMP_FL_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_TEXT:
			case F_TEXT_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			case F_PURE:
			case F_PURE_SYNC:
			default:
				Printf(color[level][0], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);
				break;
			}
			break;
		}
		case LVL_ERROR:
		case LVL_WARN:
		case LVL_CRITICAL:
		case LVL_INFO:
		case LVL_DEBUG:
		case LVL_TRACE: {
			switch (flag) {
			case F_DETAIL:
			case F_DETAIL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_TMSTMP:
			case F_TMSTMP_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_FN:
			case F_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_TMSTMP_FN:
			case F_TMSTMP_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_FL:
			case F_FL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_TMSTMP_FL:
			case F_TMSTMP_FL_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_FL_FN:
			case F_FL_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_TMSTMP_FL_FN:
			case F_TMSTMP_FL_FN_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_TEXT:
			case F_TEXT_SYNC:
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			case F_PURE:
			case F_PURE_SYNC:
			default:
				Printf(color[level][0], "%.*s", (int)len - (int)pos, msg + pos);
				break;
			}
			break;
		}
		}
	}
	
	void LoggerImpl::write(char const* msg, size_t len, size_t pos, int flag) {
		switch (fd_) {
		case INVALID_HANDLE_VALUE:
			break;
		default:
			switch (flag) {
			case F_DETAIL:
			case F_DETAIL_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_TMSTMP:
			case F_TMSTMP_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_FN:
			case F_FN_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_TMSTMP_FN:
			case F_TMSTMP_FN_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_FL:
			case F_FL_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_TMSTMP_FL:
			case F_TMSTMP_FL_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_FL_FN:
			case F_FL_FN_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_TMSTMP_FL_FN:
			case F_TMSTMP_FL_FN_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_TEXT:
			case F_TEXT_SYNC:
				LOGGER::write(fd_, msg, len);
				break;
			case F_PURE:
			case F_PURE_SYNC:
			default:
				LOGGER::write(fd_, msg + pos, (int)len - (int)pos);
				break;
			}
			break;
		}
	}
	
	void LoggerImpl::write(char const* msg, size_t len) {
		switch (fd_) {
		case INVALID_HANDLE_VALUE:
			break;
		default:
			LOGGER::write(fd_, msg, len);
			break;
		}
	}
	
	bool LoggerImpl::consume(struct tm const& tm, struct timeval const& tv, Messages& msgs) {
		assert(!msgs.empty());
		bool syn = false;
		for (Messages::const_iterator it = msgs.begin();
			it != msgs.end(); ++it) {
#define Msg(it) ((it)->first.first)
#define Stack(it) ((it)->first.second)
#define Pos(it) ((it)->second.first)
#define Flag(it) ((it)->second.second)
			int mode = getMode();
			switch (mode) {
			case M_FILE_ONLY:
			case M_STDOUT_FILE:
				switch (Msg(it).c_str()[0]) {
				case UTC_OK:
					switch (mkdir()) {
					default:
						mode = M_STDOUT_ONLY;
						break;
					case true:
						shift(tm, tv);
						break;
					}
					break;
				case UTC_ER:
					mode = M_STDOUT_ONLY;
					break;
				}
			}
			int level = getlevel(Msg(it).c_str()[1]);
			switch (level) {
			case LVL_FATAL: {
				switch (mode) {
				case M_STDOUT_ONLY:
					stdoutbuf(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, level, Flag(it), Stack(it).c_str(), Stack(it).size());
					break;
				case M_FILE_ONLY:
					write(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, Flag(it));
					write(Stack(it).c_str(), Stack(it).size());
					break;
				case M_STDOUT_FILE:
					stdoutbuf(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, level, Flag(it), Stack(it).c_str(), Stack(it).size());
					write(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, Flag(it));
					write(Stack(it).c_str(), Stack(it).size());
					break;
				}
				break;
			}
			case LVL_ERROR:
			case LVL_WARN:
			case LVL_CRITICAL:
			case LVL_INFO:
			case LVL_DEBUG:
			case LVL_TRACE: {
				switch (mode) {
				case M_STDOUT_ONLY:
					stdoutbuf(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, level, Flag(it));
					break;
				case M_FILE_ONLY:
					write(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, Flag(it));
					break;
				case M_STDOUT_FILE:
					stdoutbuf(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, level, Flag(it));
					write(&Msg(it).c_str()[1], Msg(it).size() - 1, Pos(it) - 1, Flag(it));
					break;
				}
				break;
			}
			case _O:
			case _X: {
				doConsole(level);
				break;
			}
			case _S: {
				break;
			}
			}
			if ((syn = (Flag(it) & F_SYNC))) {
				break;
			}
		}
#if 0
		Messages empty;
		msgs.swap(empty);
#else
		msgs.clear();
#endif
		return syn;
	}
	
	bool LoggerImpl::update(struct tm& tm, struct timeval& tv) {
		{
			write_lock(tm_mutex_); {
				switch (utcOk()) {
				case true: {
					gettimeofday(&tv_, NULL);
					time_t t = tv_.tv_sec;
					utcOk_ = utils::_convertUTC(t, tm_, NULL, timezone_.load());
					switch (utcOk_) {
					case true:
						tm = tm_;
						tv = tv_;
						break;
					default:
						goto ERR;
					}
					return true;
				}
				default:
					return false;
				}
			}
		}
	ERR:
		_Errorf_fl_fn("error");
		return false;
	}
	
	void LoggerImpl::get(struct tm& tm, struct timeval& tv) {
		{
			read_lock(tm_mutex_); {
				tm = tm_;
				tv = tv_;
			}
		}
	}
	
	void LoggerImpl::notify_started() {
		{
			std::unique_lock<std::mutex> lock(start_mutex_); {
				started_ = true;
				start_cond_.notify_all();
			}
		}
	}
	
	void LoggerImpl::wait_started() {
		while (!started_) {
			std::unique_lock<std::mutex> lock(start_mutex_); {
				while (!started_) {
					start_cond_.wait(lock);
				}
			}
		}
	}
	
	void LoggerImpl::notify(char const* msg, size_t len, size_t pos, int flag, char const* stack, size_t stacklen) {
		{
			std::unique_lock<std::mutex> lock(mutex_); {
				messages_.emplace_back(
					std::make_pair(
						std::make_pair(msg, stack ? stack : ""),
						std::make_pair(pos, flag)));
				cond_.notify_one();
				//std::this_thread::yield();
			}
		}
	}
	
	void LoggerImpl::wait(Messages& msgs) {
		{
			std::unique_lock<std::mutex> lock(mutex_); {
				while (messages_.empty()) {
					cond_.wait(lock);
				}
				messages_.swap(msgs);
			}
		}
	}
	
	void LoggerImpl::sync() {
		{
			std::unique_lock<std::mutex> lock(sync_mutex_); {
				sync_ = true;
				sync_cond_.notify_one();
			}
			//std::this_thread::yield();
		}
	}

	void LoggerImpl::wait() {
		{
			std::unique_lock<std::mutex> lock(sync_mutex_); {
				while (!sync_) {
					sync_cond_.wait(lock);
				}
				sync_ = false;
			}
		}
	}
	
	//detach()后get_id()/joinable()无效
	bool LoggerImpl::valid() {
#if 0
		std::ostringstream oss;
		oss << thread_.get_id();
		return oss.str() != "0";
#else
		return thread_.joinable();
#endif
	}
	
	bool LoggerImpl::start() {
		if (!started_ && !starting_.test_and_set()) {
			thread_ = std::thread([this](LoggerImpl* logger) {
				//notify_started();
				struct tm tm = { 0 };
				struct timeval tv = { 0 };
				bool syn = false;
				while (!done_.load()) {
					Messages msgs; {
						wait(msgs);
					}
					get(tm, tv);
					if ((syn = consume(tm, tv, msgs))) {
						done_.store(true);
						break;
					}
					//std::this_thread::yield();
				}
				close();
				started_ = false;
				if (syn) {
					sync();
				}
			}, this);
			if ((started_ = valid())) {
				thread_.detach();
			}
			starting_.clear();
		}
		return started_;
	}
	
	bool LoggerImpl::started() {
		return started_;
	}
	
	void LoggerImpl::stop() {
		switch (started()) {
		case true:
			__LOG_WAIT_S;
			break;
		}
		//done_.store(true);
		//if (thread_.joinable()) {
		//	thread_.join();
		//}
	}

	//enable
	void LoggerImpl::enable() {
		if (!enable_.load()) {
			enable_.store(true);
			openConsole();
			//_Warnf_tmsp("enable ...");
		}
	}

	//disable
	void LoggerImpl::disable(int delay, bool sync) {
		if (enable_.load()) {
			enable_.store(false);
			_Warnf_tmsp("disable after %d milliseconds ...", delay);
			if (sync) {
				timer_.SyncWait(delay, [&] {
					if (!enable_.load()) {
						_Warnf_tmsp("disable ...");
#if 0
						closeConsole();
#else
						__LOG_NOTIFY_X;
#endif
					}
					});
			}
			else {
				timer_.AsyncWait(delay, [&] {
					if (!enable_.load()) {
						_Warnf_tmsp("disable ...");
#if 0
						closeConsole();
#else
						__LOG_NOTIFY_X;
#endif
					}
					});
			}
		}
	}
	
	//openConsole
	void LoggerImpl::openConsole() {
		if (!isConsoleOpen_ && !isDoing_.test_and_set()) {
			utils::_initConsole();
			isConsoleOpen_ = true;
			isDoing_.clear();
		}
	}
	
	//closeConsole
	void LoggerImpl::closeConsole() {
		if (isConsoleOpen_ && !isDoing_.test_and_set()) {
			_Warnf_tmsp("closed ...");
			utils::_closeConsole();
			isConsoleOpen_ = false;
			isDoing_.clear();
		}
	}
	
	//doConsole
	void LoggerImpl::doConsole(int const cmd) {
		switch (cmd) {
		case _O: {
			openConsole();
			break;
		}
		case _X: {
			closeConsole();
			break;
		}
		}
	}
}

#if 0
int main() {
	utils::_setrlimit();
	__LOG_SET_MODE(M_STDOUT_FILE);
	__LOG_SET_STYLE(F_PURE);
	__LOG_SET_DEBUG;
	__LOG_INIT("/mnt/hgfs/presstest/deploy/log", "client_presstest", 100000000);
	for (int i = 0; i < 10; ++i) {
		xsleep(0);
		_Errorf("Hi%d", i);
	}
	//_Fatalf("崩溃吧");
	return 0;
}
#endif