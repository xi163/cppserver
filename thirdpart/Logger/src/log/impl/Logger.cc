#include "../Logger.h"
#include "LoggerImpl.h"
#include "../../utils/utils.h"
#include "../../auth/auth.h"

namespace LOGGER {

	//constructor
	Logger::Logger() :impl_(New<LoggerImpl>()) {
		//placement new
	}

	//constructor
	Logger::Logger(LoggerImpl* impl) : impl_(impl) {
	}

	//destructor
	Logger::~Logger() {
		if (impl_ &&
			impl_ != LoggerImpl::instance()) {
			Delete<LoggerImpl>(impl_);
		}
	}

	//instance
	Logger* Logger::instance() {
		static Logger logger(LoggerImpl::instance());
		return &logger;
	}
	
	void Logger::setPrename(char const* name) {
		AUTHORIZATION_CHECK;
		impl_->setPrename(name);
	}
	char const* Logger::getPrename() const {
		AUTHORIZATION_CHECK_P;
		return impl_->getPrename();
	}

	char const* Logger::timezoneString() const {
		AUTHORIZATION_CHECK_P;
		return impl_->timezoneString();
	}
	void Logger::setTimezone(int timezone/* = MY_CST*/) {
		AUTHORIZATION_CHECK;
		impl_->setTimezone(timezone);
	}
	int Logger::getTimezone() {
		AUTHORIZATION_CHECK_I;
		return impl_->getTimezone();
	}
	
	char const* Logger::levelString() const {
		AUTHORIZATION_CHECK_P;
		return impl_->levelString();
	}
	void Logger::setLevel(int level) {
		AUTHORIZATION_CHECK;
		impl_->setLevel(level);
	}
	int Logger::getLevel() {
		AUTHORIZATION_CHECK_I;
		return impl_->getLevel();
	}

	char const* Logger::modeString() const {
		AUTHORIZATION_CHECK_P;
		return impl_->modeString();
	}
	void Logger::setMode(int mode) {
		AUTHORIZATION_CHECK;
		impl_->setMode(mode);
	}
	int Logger::getMode() {
		AUTHORIZATION_CHECK_I;
		return impl_->getMode();
	}

	char const* Logger::styleString() const {
		AUTHORIZATION_CHECK_P;
		return impl_->styleString();
	}
	void Logger::setStyle(int style) {
		AUTHORIZATION_CHECK;
		impl_->setStyle(style);
	}
	int Logger::getStyle() {
		AUTHORIZATION_CHECK_I;
		return impl_->getStyle();
	}

	void Logger::setColor(int level, int title, int text) {
		AUTHORIZATION_CHECK;
		impl_->setColor(level, title, text);
	}

	//init
	void Logger::init(char const* dir, char const* logname, size_t logsize) {
		AUTHORIZATION_CHECK;
		impl_->init(dir, logname, logsize);
	}

	//write
	void Logger::write(int level, char const* file, int line, char const* func, char const* stack, int flag, char const* format, ...) {
		AUTHORIZATION_CHECK;
		if (impl_->check(level)) {
			static size_t const PATHSZ = 512;
			static size_t const MAXSZ = 81920;
			char msg[PATHSZ + MAXSZ + 2];
			size_t pos = impl_->format_s(file, line, func, level, flag, msg, PATHSZ);
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
			impl_->start();
			//impl_->wait_started();
			impl_->notify(msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
		}
	}

	//write_s
	void Logger::write_s(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		AUTHORIZATION_CHECK;
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
	}
	
	//write_s_fatal
	void Logger::write_s_fatal(int level, char const* file, int line, char const* func, char const* stack, int flag, std::string const& msg) {
		AUTHORIZATION_CHECK;
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
		wait();
		std::abort();
	}
	
	//wait
	void Logger::wait() {
		AUTHORIZATION_CHECK;
		impl_->wait();
	}

	//enable
	void Logger::enable() {
		AUTHORIZATION_CHECK;
		impl_->enable();
	}

	//disable
	void Logger::disable(int delay, bool sync) {
		AUTHORIZATION_CHECK;
		impl_->disable(delay, sync);
	}
}

#if 0
int main() {
	utils::setrlimit();
	_LOG_SET_MODE(M_FILE_ONLY);
	//_LOG_SET_STYLE(F_PURE);
	_LOG_SET_DEBUG;
	_LOG_INIT("/mnt/hgfs/presstest/deploy/log", "client_presstest", 10000);
	while (1) {
		for (int i = 0; i < 10; ++i) {
			xsleep(0);
			_LOG_DEBUG("_______Hi%d", i);
		}
	}
	//_LOG_FATAL("崩溃吧");
	return 0;
}
#endif