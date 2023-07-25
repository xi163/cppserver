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

	//set_timezone
	void Logger::set_timezone(int64_t timezone/* = MY_CST*/) {
		AUTHORIZATION_CHECK;
		impl_->set_timezone(timezone);
	}

	//set_level
	void Logger::set_level(int level) {
		AUTHORIZATION_CHECK;
		impl_->set_level(level);
	}

	//get_level
	char const* Logger::get_level() {
		AUTHORIZATION_CHECK_P;
		return impl_->get_level();
	}

	//set_color
	void Logger::set_color(int level, int title, int text) {
		AUTHORIZATION_CHECK;
		impl_->set_color(level, title, text);
	}

	//init
	void Logger::init(char const* dir, int level, char const* prename, size_t logsize) {
		//AUTHORIZATION_CHECK;
		impl_->init(dir, level, prename, logsize);
	}

	//write
	void Logger::write(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, char const* fmt, ...) {
		AUTHORIZATION_CHECK;
		if (impl_->check(level)) {
			static size_t const PATHSZ = 512;
			static size_t const MAXSZ = 81920;
			char msg[PATHSZ + MAXSZ + 2];
			size_t pos = impl_->format(level, file, line, func, flag, msg, PATHSZ);
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
			if (impl_->started()) {
				impl_->notify(msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
			}
			else {
				impl_->stdoutbuf(level, msg, pos + n + 1, pos, flag, stack, stack ? strlen(stack) : 0);
				impl_->checkSync(flag);
			}
		}
	}

	//write_s
	void Logger::write_s(int level, char const* file, int line, char const* func, char const* stack, uint8_t flag, std::string const& msg) {
		AUTHORIZATION_CHECK;
		write(level, file, line, func, stack, flag, "%s", msg.c_str());
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

	//cleanup
	void Logger::cleanup() {
		AUTHORIZATION_CHECK;
		impl_->stop();
	}
}

/*
int main() {
	//LOG_INIT(".", LVL_DEBUG, "test");
	LOG_SET_DEBUG;
	while (1) {
		for (int i = 0; i < 200000; ++i) {
			LOG_ERROR("Hi%d", i);
		}
	}
	LOG_FATAL("崩溃吧");
	xsleep(1000);
	return 0;
}
*/