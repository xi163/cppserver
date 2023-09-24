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

#define TAG_0 'T'
#define TAG_1 'P'

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
			snprintf(prename_, sizeof(prename_), "%s", name);
		}
	}

	char const* LoggerImpl::getPrename() const {
		return prename_;
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
	
	bool LoggerImpl::_setLevel(int level) {
#if 0
		switch (level) {
		case LVL_DEBUG:
		case LVL_TRACE:
		case LVL_INFO:
		case LVL_WARN:
		case LVL_ERROR:
		case LVL_FATAL:
			level_.store(level);
			return true;
		}
		return false;
#else
		if (level >= LVL_DEBUG) {
			level_.store(LVL_DEBUG);
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
	
	char const* LoggerImpl::timezoneString() const {
		return getTimeZoneDesc(timezone_.load()).c_str();
	}
	void LoggerImpl::setTimezone(int timezone/* = MY_CST*/) {
		switch (timezone == timezone_.load()) {
		case false:
			switch (_setTimezone(timezone)) {
			case true:
			default:
				break;
			}
			break;
		}
	}
	int LoggerImpl::getTimezone() {
		return timezone_.load();
	}

	char const* LoggerImpl::levelString() const {
		static char const* s[] = { "FATAL","ERROR","WARNING","INFO","TRACE","DEBUG" };
		return s[level_.load()];
	}
	void LoggerImpl::setLevel(int level) {
		switch (level == level_.load()) {
		case false:
			switch (_setLevel(level)) {
			case true:
			default:
				break;
			}
		}
	}
	int LoggerImpl::getLevel() {
		return level_.load();
	}
	
	char const* LoggerImpl::modeString() const {
		static char const* s[] = { "M_STDOUT_ONLY", "M_FILE_ONLY", "M_STDOUT_FILE" };
		return s[mode_.load()];
	}
	void LoggerImpl::setMode(int mode) {
		switch (mode == mode_.load()) {
		case false:
			switch (_setMode(mode)) {
			case true:
			default:
				break;
			}
		}
	}
	int LoggerImpl::getMode() {
		return mode_.load();
	}

	char const* LoggerImpl::styleString() const {
		switch (style_.load()) {
		case F_DETAIL: return "F_DETAIL";
		case F_TMSTMP: return "F_TMSTMP";
		case F_FN: return "F_FN";
		case F_TMSTMP_FN: return "F_TMSTMP_FN";
		case F_FL: return "F_FL";
		case F_TMSTMP_FL: return "F_TMSTMP_FL";
		case F_FL_FN: return "F_FL_FN";
		case F_TMSTMP_FL_FN: return "F_TMSTMP_FL_FN";
		case F_TEXT: return "F_TEXT";
		case F_PURE: return "F_PURE";
		default: return "F_UNKNOWN";
		}
	}
	void LoggerImpl::setStyle(int style) {
		switch (style == style_.load()) {
		case false:
			switch (_setStyle(style)) {
			case true:
			default:
				break;
			}
		}
	}
	int LoggerImpl::getStyle() {
		return style_.load();
	}

	void LoggerImpl::setColor(int level, int clrtitle, int clrtext) {
	}
	
	//update
	void LoggerImpl::update(struct tm& tm, struct timeval& tv) {
		{
			write_lock(tm_mutex_); {
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

	bool LoggerImpl::_checkDir() {
		switch (mkdir_) {
		case false:
			std::string s(prefix_);
			size_t pos = s.find_last_of('/');
			utils::_mkDir_p(s.substr(0, pos + 1).c_str());
			mkdir_ = true;
			break;
		}
	}
	
	std::string const LoggerImpl::_name(bool space) {
		switch (prename_[0] == '\0') {
		case true: return "";
		default:
			switch (space) {
			case true:
				return std::string(" <").append(prename_).append("> ");
			default:
				return std::string(" <").append(prename_).append(">");
			}
		}
	}
	
	//init
	void LoggerImpl::init(char const* dir, char const* prename, size_t logsize) {
		size_ = logsize;
		setPrename(prename);
		(prename && prename[0]) ?
			snprintf(prefix_, sizeof(prefix_), "%s/%s ", ((dir && dir[0]) ? dir : "."), prename) :
			snprintf(prefix_, sizeof(prefix_), "%s/", ((dir && dir[0]) ? dir : "."));
		switch (getMode()) {
		case M_FILE_ONLY:
		case M_STDOUT_FILE:
			_checkDir();
			break;
		}
		timezoneInfo();
	}
	
	//write
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

	//write_s
	void LoggerImpl::write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
	}
	
	//write_s_fatal
	void LoggerImpl::write_s_fatal(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
		wait();
		std::abort();
	}
	
	//timezoneInfo
	void LoggerImpl::timezoneInfo() {
		struct tm tm = { 0 };
		utils::_convertUTC(time(NULL), tm, NULL, timezone_);
		utils::_timezoneInfo(tm, timezone_);
	}
	
	//打印level_及以下级别日志
	bool LoggerImpl::check(int level) {
		return level <= level_.load() || level == _S || level == _O || level == _X;
	}

	//format
	size_t LoggerImpl::format_s(char const* file, int line, char const* func, int level, int flag, char* buffer, size_t size) {
		struct tm tm;
		struct timeval tv;
		update(tm, tv);
		static char const chr[] = { 'F','E','W','I','T','D','S','O','X' };
		size_t pos = (flag & F_DETAIL) ?
			snprintf(buffer, size, "%c%d %s %02d:%02d:%02d.%.6lu %s %s:%d] %s ",
				chr[level],
				pid_,
				getTimeZoneDesc(timezone_).c_str(),
				tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec,
				utils::_gettid().c_str(),
				utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str()) :
			snprintf(buffer, size, "%c%s %02d:%02d:%02d.%.6lu] ",
				chr[level],
				getTimeZoneDesc(timezone_).c_str(),
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
	void LoggerImpl::write(char const* msg, size_t len, size_t pos, int flag) {
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
			snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.log",
				prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
			}
			else {
				remove(path_);
			}
			open(path_);
			day_ = tm.tm_mday;
		}
		else {
			struct stat stStat;
			if (stat(path_, &stStat) < 0) {
				close();
				open(path_);
				return;
			}
			if (stStat.st_size < size_) {
			}
			else {
				close();
				snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.log",
					prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
				if (stat(path_, &stStat) < 0) {
					open(path_);
					//day_ = tm.tm_mday;
				}
				else {//0 existed
					for (int i = 0;;) {
						snprintf(path_, sizeof(path_), "%s%d_%04d-%02d-%02d.%02d.%02d.%02d.%.6lu.%d.log",
							prefix_, pid_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec, i);
						if (stat(path_, &stStat) < 0) {
							open(path_);
							//day_ = tm.tm_mday;
							break;
						}
						else {
							++i;
						}
					}
				}
			}
		}
	}

	//start
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
	
	bool LoggerImpl::started() {
		return started_;
	}
	
	//notify_started
	void LoggerImpl::notify_started() {
		{
			std::unique_lock<std::mutex> lock(start_mutex_); {
				started_ = true;
				start_cond_.notify_all();
			}
		}
	}
	
	//wait_started
	void LoggerImpl::wait_started() {
		while (!started_) {
			std::unique_lock<std::mutex> lock(start_mutex_); {
				while (!started_) {
					start_cond_.wait(lock);
				}
			}
		}
	}
	
	//notify
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
				if (prefix_[0] == '\0') {
					break;
				}
				_checkDir();
				shift(tm, tv);
// 				switch (prefix_[0]) {
// 				case TAG_0:
// 					_checkDir();
// 					shift(tm, tv);
// 					break;
// 				}
			}
			if (mode > M_STDOUT_ONLY && (!mkdir_ || prefix_[0] == TAG_1)) {
				mode = M_STDOUT_ONLY;
			}
			int level = getlevel(Msg(it).c_str()[0]);
			switch (level) {
			case LVL_FATAL: {
				switch (mode) {
				case M_FILE_ONLY:
				case M_STDOUT_FILE:
					write(Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it));
					write(Stack(it).c_str(), Stack(it).size(), 0, 0);
					break;
				}
				switch (mode) {
				case M_STDOUT_ONLY:
				case M_STDOUT_FILE:
					stdoutbuf(Msg(it).c_str(), Msg(it).size(), Pos(it), level, Flag(it), Stack(it).c_str(), Stack(it).size());
					break;
				}
				break;
			}
			case LVL_ERROR:
			case LVL_WARN:
			case LVL_INFO:
			case LVL_TRACE:
			case LVL_DEBUG: {
				switch (mode) {
				case M_FILE_ONLY:
				case M_STDOUT_FILE:
					write(Msg(it).c_str(), Msg(it).size(), Pos(it), Flag(it));
					break;
				}
				switch (mode) {
				case M_STDOUT_ONLY:
				case M_STDOUT_FILE:
					stdoutbuf(Msg(it).c_str(), Msg(it).size(), Pos(it), level, Flag(it));
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
	
	//stop
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

	//需要调用utils::_initConsole()初始化
	void LoggerImpl::stdoutbuf(char const* msg, size_t len, size_t pos, int level, int flag, char const* stack, size_t stacklen) {
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
			if ((flag & F_TMSTMP)) {
				Printf(color[level][0], "%.*s", (int)pos - 1, msg + 1);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);//stack
			}
			else if ((flag & F_DETAIL)) {
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);//stack
			}
			else {
				Printf(color[level][0], "%.*s", (int)len - (int)pos, msg + pos);
				Printf(color[level][0], "%.*s", (int)stacklen, stack);//stack
			}
			break;
		}
		case LVL_ERROR:
		case LVL_WARN:
		case LVL_INFO:
		case LVL_DEBUG: {
			if ((flag & F_TMSTMP)) {
				Printf(color[level][0], "%.*s", (int)pos - 1, msg + 1);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
			}
			else if ((flag & F_DETAIL)) {
				Printf(color[level][0], "%.*s", (int)pos, msg);
				Printf(color[level][1], "%.*s", (int)len - (int)pos, msg + pos);
			}
			else {
				Printf(color[level][0], "%.*s", (int)len - (int)pos, msg + pos);
			}
			break;
		}
		}
#endif
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

#if 0
int main() {
	utils::_setrlimit();
	__LOG_SET_MODE(M_STDOUT_FILE);
	__LOG_SET_DEBUG;
	__LOG_INIT("/mnt/hgfs/presstest/deploy/log", "client_presstest", 100000000);
	for (int i = 0; i < 10; ++i) {
		xsleep(0);
		__LOG_ERROR("Hi%d", i);
	}
	//__LOG_FATAL("崩溃吧");
	return 0;
}
#endif