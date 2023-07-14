#include "LoggerImpl.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../utils/impl/Console.h"

#ifdef _windows_
#include <process.h>
#include "../../utils/impl/gettimeofday.h"
#endif

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

namespace LOGGER {

	//constructor
	LoggerImpl::LoggerImpl() :pid_(getpid()) {
	};

	//destructor
	LoggerImpl::~LoggerImpl() {
		stop();
	}

	//instance
	LoggerImpl* LoggerImpl::instance() {
		static LoggerImpl logger;
		return &logger;
	}
	
	//set_timezone
	void LoggerImpl::set_timezone(int64_t timezone/* = MY_CST*/) {
		timezone_ = timezone;
	}
	
	//update
	void LoggerImpl::update(struct tm& tm, struct timeval& tv) {
		{
			read_lock(tm_mutex_); {
				gettimeofday(&tv_, NULL);
				time_t t = tv_.tv_sec;
				utils::_convertUTC(t, tm_, NULL, timezone_);
				tm = tm_;
				tv = tv_;
			}
		}
	}

	//get
	void LoggerImpl::get(struct tm& tm, struct timeval& tv) {
		{
			read_lock(tm_mutex_); {
				tm = tm_;
				tv = tv_;
			}
		}
	}

	//set_level
	void LoggerImpl::set_level(int level) {
		if (level >= LVL_DEBUG) {
			level_.store(LVL_DEBUG);
		}
		else if (level <= LVL_FATAL) {
			level_.store(LVL_FATAL);
		}
		else {
			level_.store(level);
		}
	}

	//get_level
	char const* LoggerImpl::get_level() {
		static char const* s[] = { "FATAL","ERROR","WARNING","INFO","TRACE","DEBUG" };
		return s[level_.load()];
	}

	//set_color
	void LoggerImpl::set_color(int level, int clrtitle, int clrtext) {
	}

	//init
	void LoggerImpl::init(char const* dir, int level, char const* prename, size_t logsize) {
		utils::_mkDir(dir);
		//打印level_及以下级别日志
		level_.store(level);
		if (start()) {
			size_ = logsize;
			(prename && prename[0]) ?
				snprintf(prefix_, sizeof(prefix_), "%s/%s ", ((dir && dir[0]) ? dir : "."), prename) :
				snprintf(prefix_, sizeof(prefix_), "%s/", ((dir && dir[0]) ? dir : "."));
			timezoneInfo();
		}
	}
	
	//write
	void LoggerImpl::write(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, char const* fmt, ...) {
		if (check(level)) {
			static size_t const PATHSZ = 512;
			static size_t const MAXSZ = 81920;
			char msg[PATHSZ + MAXSZ + 2];
			size_t pos = format(level, file, line, func, flag, msg, PATHSZ);
			va_list ap;
			va_start(ap, fmt);
#ifdef _windows_
			size_t n = vsnprintf_s(msg + pos, MAXSZ, _TRUNCATE, fmt, ap);
#else
			size_t n = vsnprintf(msg + pos, MAXSZ, fmt, ap);
#endif
			va_end(ap);
			msg[pos + n] = '\n';
			msg[pos + n + 1] = '\0';
			if (started()) {
				notify(msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
			}
			else {
				stdoutbuf(level, msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
				checkSync(flag);
			}
		}
	}

	//write_s
	void LoggerImpl::write_s(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, std::string const& msg) {
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
	}
	
	//timezoneInfo
	void LoggerImpl::timezoneInfo() {
		struct tm tm = { 0 };
		utils::_convertUTC(time(NULL), tm, NULL, timezone_);
		utils::_timezoneInfo(tm, timezone_);
	}

	//started
	bool LoggerImpl::started() {
		return started_;
	}

	//check
	bool LoggerImpl::check(int level) {
		//打印level_及以下级别日志
		return level <= level_.load() || level == _S || level == _O || level == _X;
	}

	//_tz
	static inline char const* _tz(int64_t timezone) {
		switch (timezone) {
		case MY_PST: return "PST";
		case MY_MST: return "MST";
		case MY_EST: return "EST";
		case MY_BST: return "BST";
		case MY_UTC: return "UTC";
		case MY_GST: return "GST";
		case MY_CST: return "CST";
		case MY_JST: return "JST";
		}
		return "";
	}

	//format
	size_t LoggerImpl::format(int level, char const* file, int line, char const* func, uint8_t flag, char* buffer, size_t size) {
		struct tm tm;
		struct timeval tv;
		update(tm, tv);
		static char const chr[] = { 'F','E','W','I','T','D','S','O','X' };
		size_t pos = (flag & F_DETAIL) ?
			snprintf(buffer, size, "%c%d %s %02d:%02d:%02d.%.6lu %s %s:%d] %s ",
				chr[level],
				pid_,
				_tz(timezone_),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_gettid().c_str(),
				utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str()) :
			snprintf(buffer, size, "%c%s %02d:%02d:%02d.%.6lu] ",
				chr[level],
				_tz(timezone_),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
		return pos;
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
	void LoggerImpl::write(char const* msg, size_t len, size_t pos, uint8_t flag) {
#ifdef _windows_
		if (fd_ != INVALID_HANDLE_VALUE) {
			if (pos == 0) {
				long size = 0;
				WriteFile(fd_, msg, len, (LPDWORD)&size, NULL);
			}
			else if ((flag & F_TMSTMP)) {
				long size = 0;
				WriteFile(fd_, msg + 1, len - 1, (LPDWORD)&size, NULL);
			}
			else if ((flag & F_DETAIL)) {
				long size = 0;
				WriteFile(fd_, msg, len, (LPDWORD)&size, NULL);
			}
			else {
				long size = 0;
				WriteFile(fd_, msg + pos, len - pos, (LPDWORD)&size, NULL);
			}
		}
#else
		if (fd_ != INVALID_HANDLE_VALUE) {
			if (pos == 0) {
				(void)::write(fd_, msg, len);
			}
			else if ((flag & F_TMSTMP)) {
				(void)::write(fd_, msg + 1, len - 1);
			}
			else if ((flag & F_DETAIL)) {
				(void)::write(fd_, msg, len);
			}
			else {
				(void)::write(fd_, msg + pos, len - pos);
			}
		}
#endif
	}

	//close
	void LoggerImpl::close() {
#ifdef _windows_
		if (INVALID_HANDLE_VALUE != fd_) {
			CloseHandle(fd_);
			fd_ = INVALID_HANDLE_VALUE;
		}
#else
		if (INVALID_HANDLE_VALUE != fd_) {
			::close(fd_);
			fd_ = INVALID_HANDLE_VALUE;
		}
#endif
	}

	//shift
	void LoggerImpl::shift(struct tm const& tm, struct timeval const& tv) {
		assert(prefix_[0]);
		if (tm.tm_mday != day_) {
			close();
			snprintf(path_, sizeof(path_), "%s%d %04d-%02d-%02d.log",
				prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
				open(path_);
				day_ = tm.tm_mday;
			}
			else {
				std::abort();
			}
		}
		else {
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
				return;
			}
			if (stStat.st_size < size_) {
			}
			else {
				close();
				snprintf(path_, sizeof(path_), "%s%d %04d-%02d-%02d %02d.%02d.%02d.log",
					prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				if (stat(path_, &stStat) < 0) {
					//if (rename(path_, tmp) < 0) {
					//	return;
					//}
					open(path_);
					//day_ = tm.tm_mday;
				}
				else {//0 existed
					snprintf(path_, sizeof(path_), "%s%d %04d-%02d-%02d %02d.%02d.%02d.%.6lu.log",
						prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
					if (stat(path_, &stStat) < 0) {
						//if (rename(path_, tmp) < 0) {
						//	return;
						//}
						open(path_);
						//day_ = tm.tm_mday;
					}
					else {
						std::abort();
					}
				}
			}
		}
	}

	//start
	bool LoggerImpl::start() {
		if (!started_ && !starting_.test_and_set()) {
			thread_ = std::thread([this](LoggerImpl* logger) {
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

	//valid
	bool LoggerImpl::valid() {
		//detach()后get_id()/joinable()无效
#if 0
		std::ostringstream oss;
		oss << thread_.get_id();
		return oss.str() != "0";
#else
		return thread_.joinable();
#endif
	}

	//notify
	void LoggerImpl::notify(char const* msg, size_t len, size_t pos, uint8_t flag, char const* stack, size_t stacklen) {
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

	//wait
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

	//getlevel
	static inline int getlevel(char const c) {
		switch (c) {
		case 'F': return LVL_FATAL;
		case 'E':return LVL_ERROR;
		case 'W':return LVL_WARN;
		case 'I':return LVL_INFO;
		case 'T':return LVL_TRACE;
		case 'D':return LVL_DEBUG;
		case 'S':return _S;
		case 'O':return _O;
		case 'X':return _X;
		}
	}

	//consume
	bool LoggerImpl::consume(struct tm const& tm, struct timeval const& tv, Messages& msgs) {
		assert(!msgs.empty());
		shift(tm, tv);
		bool syn = false;
		for (Messages::const_iterator it = msgs.begin();
			it != msgs.end(); ++it) {
#define Msg(it) ((it)->first.first)
#define Stack(it) ((it)->first.second)
#define Pos(it) ((it)->second.first)
#define Flag(it) ((it)->second.second)
			int level = getlevel(Msg(it).c_str()[0]);
			switch (level) {
			case LVL_FATAL:
			case LVL_TRACE: {
				write(Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it));
				write(Stack(it).c_str(), Stack(it).size(), 0, 0);
				stdoutbuf(level, Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it), Stack(it).c_str(), Stack(it).size());
				break;
			}
			case LVL_ERROR:
			case LVL_WARN:
			case LVL_INFO:
			case LVL_DEBUG: {
				write(Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it));
				stdoutbuf(level, Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it));
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

	//flush
	void LoggerImpl::flush() {
		__LOG_WAIT_S;
	}

	//stop
	void LoggerImpl::stop() {
		flush();
		//done_.store(true);
		//if (thread_.joinable()) {
		//	thread_.join();
		//}
	}

	//stdoutbuf
	//需要调用utils::_initConsole()初始化
	void LoggerImpl::stdoutbuf(int level, char const* msg, size_t len, size_t pos, uint8_t flag, char const* stack, size_t stacklen) {
#ifdef QT_CONSOLE
		switch (level) {
		default:
		case LVL_FATAL: qFatal(msg); break;
		case LVL_ERROR: qCritical(msg); break;
		case LVL_WARN: qWarning(msg); break;
		case LVL_INFO: qInfo(msg); break;
		case LVL_TRACE: qInfo(msg); break;
		case LVL_DEBUG: qDebug(msg); break;
		}
#elif defined(_windows_)
		if (!isConsoleOpen_) {
			return;
		}
		//foreground color
#define FOREGROUND_Red         (FOREGROUND_RED)//红
#define FOREGROUND_Green       (FOREGROUND_GREEN)//绿
#define FOREGROUND_Blue        (FOREGROUND_BLUE)//蓝
#define FOREGROUND_Yellow      (FOREGROUND_RED|FOREGROUND_GREEN)//黄
#define FOREGROUND_Cyan        (FOREGROUND_GREEN|FOREGROUND_BLUE)//青
#define FOREGROUND_Purple      (FOREGROUND_RED|FOREGROUND_BLUE)//紫
#define FOREGROUND_White       (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)//白
#define FOREGROUND_Gray        (FOREGROUND_INTENSITY)//灰
#define FOREGROUND_Black       (0)//黑
#define FOREGROUND_LightRed    (FOREGROUND_INTENSITY|FOREGROUND_Red)//淡红
#define FOREGROUND_HighGreen   (FOREGROUND_INTENSITY|FOREGROUND_Green)//亮绿
#define FOREGROUND_LightBlue   (FOREGROUND_INTENSITY|FOREGROUND_Blue)//淡蓝
#define FOREGROUND_LightYellow (FOREGROUND_INTENSITY|FOREGROUND_Yellow)//淡黄
#define FOREGROUND_HighCyan    (FOREGROUND_INTENSITY|FOREGROUND_Cyan)//亮青
#define FOREGROUND_Pink        (FOREGROUND_INTENSITY|FOREGROUND_Purple)//粉红
#define FOREGROUND_HighWhite   (FOREGROUND_INTENSITY|FOREGROUND_White)//亮白
//background color
#define BACKGROUND_Red         (BACKGROUND_RED)//红
#define BACKGROUND_Green       (BACKGROUND_GREEN)//绿
#define BACKGROUND_Blue        (BACKGROUND_BLUE)//蓝
#define BACKGROUND_Yellow      (BACKGROUND_RED|BACKGROUND_GREEN)//黄
#define BACKGROUND_Cyan        (BACKGROUND_GREEN|BACKGROUND_BLUE)//青
#define BACKGROUND_Purple      (BACKGROUND_RED|BACKGROUND_BLUE)//紫
#define BACKGROUND_White       (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)//白
#define BACKGROUND_Gray        (BACKGROUND_INTENSITY)//灰
#define BACKGROUND_Black       (0)//黑
#define BACKGROUND_LightRed    (BACKGROUND_INTENSITY|BACKGROUND_Red)//淡红
#define BACKGROUND_HighGreen   (BACKGROUND_INTENSITY|BACKGROUND_Green)//亮绿
#define BACKGROUND_LightBlue   (BACKGROUND_INTENSITY|BACKGROUND_Blue)//淡蓝
#define BACKGROUND_LightYellow (BACKGROUND_INTENSITY|BACKGROUND_Yellow)//淡黄
#define BACKGROUND_HighCyan    (BACKGROUND_INTENSITY|BACKGROUND_Cyan)//亮青
#define BACKGROUND_Pink        (BACKGROUND_INTENSITY|BACKGROUND_Purple)//粉红
#define BACKGROUND_HighWhite   (BACKGROUND_INTENSITY|BACKGROUND_White)//亮白

		static int const color[][2] = {
			{FOREGROUND_Red, FOREGROUND_LightRed},//FATAL
			{FOREGROUND_Red, FOREGROUND_Purple},//ERROR
			{FOREGROUND_Cyan, FOREGROUND_HighCyan},//WARN
			{FOREGROUND_Pink, FOREGROUND_White},//INFO
			{FOREGROUND_Yellow, FOREGROUND_LightYellow},//TRACE
			{FOREGROUND_HighGreen, FOREGROUND_Gray},//DEBUG
		};
		HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
		switch (level) {
		case LVL_FATAL:
		case LVL_TRACE: {
			if ((flag & F_TMSTMP)) {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)pos - 1, msg + 1);
				::SetConsoleTextAttribute(h, color[level][1]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)stacklen, stack);//stack
			}
			else if ((flag & F_DETAIL)) {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)pos, msg);
				::SetConsoleTextAttribute(h, color[level][1]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)stacklen, stack);//stack
			}
			else {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)stacklen, stack);//stack
			}
			break;
		}
		case LVL_ERROR:
		case LVL_WARN:
		case LVL_INFO:
		case LVL_DEBUG: {
			if ((flag & F_TMSTMP)) {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)pos - 1, msg + 1);
				::SetConsoleTextAttribute(h, color[level][1]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
			}
			else if ((flag & F_DETAIL)) {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)pos, msg);
				::SetConsoleTextAttribute(h, color[level][1]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
			}
			else {
				::SetConsoleTextAttribute(h, color[level][0]);
				printf("%.*s", (int)len - (int)pos, msg + pos);
			}
			break;
		}
		}
		//::CloseHandle(h);
#else
		switch (level) {
		case LVL_FATAL:
		case LVL_TRACE: {
			printf("%.*s", (int)len, msg);
			printf("%.*s", (int)stacklen, stack);//stack
			break;
		}
		case LVL_ERROR:
		case LVL_WARN:
		case LVL_INFO:
		case LVL_DEBUG: {
			printf("%.*s", (int)len, msg);
			break;
		}
		}
#endif
	}
	
	//checkSync
	void LoggerImpl::checkSync(uint8_t flag) {
		if ((flag & F_SYNC)) {
			sync();
		}
	}

	//sync
	void LoggerImpl::sync() {
		{
			std::unique_lock<std::mutex> lock(sync_mutex_); {
				sync_ = true;
				sync_cond_.notify_one();
			}
			//std::this_thread::yield();
		}
	}

	//wait
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

	//enable
	void LoggerImpl::enable() {
		if (!enable_.load()) {
			enable_.store(true);
			openConsole();
			//__TLOG_WARN("enable ...");
		}
	}

	//disable
	void LoggerImpl::disable(int delay, bool sync) {
		if (enable_.load()) {
			enable_.store(false);
			__TLOG_WARN("disable after %d milliseconds ...", delay);
			if (sync) {
				timer_.SyncWait(delay, [&] {
					if (!enable_.load()) {
						__TLOG_WARN("disable ...");
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
						__TLOG_WARN("disable ...");
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
			__TLOG_WARN("closed ...");
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
