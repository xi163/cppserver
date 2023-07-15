#ifndef INCLUDE_ENV_H
#define INCLUDE_ENV_H

#include "ConsoleClr.h"

static const int            kRollSize = 1024*1024*1024;
static const int            gEastUTC  = 60*60*8;
//static muduo::AsyncLogging* gAsyncLog = NULL;

static void registerSignalHandler(int signal, void(*handler)(int)) {
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(signal, &sa, nullptr);
}

static void SetRLIMIT() {
	struct rlimit rlmt;
	if (getrlimit(RLIMIT_CORE, &rlmt) == -1) {
		exit(0);
	}
	//printf("Before set rlimit CORE dump current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
	rlmt.rlim_cur = 1024 * 1024 * 1024;
	rlmt.rlim_max = 1024 * 1024 * 1024;
	if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
		_LOG_ERROR("RLIMIT_CORE error");
		exit(0);
	}
#if 0
	if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
		return false;
	//printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
	rlmt.rlim_cur = (rlim_t)655350;
	rlmt.rlim_max = (rlim_t)655356;
	if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
		exit(0);
#elif 0
	//ulimit -a / ulimit -n
	//http://juduir.com/topic/1802500000000000075
	if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
		_LOG_ERROR("Ulimit fd number failed");
		exit(0);
	}
	//printf("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d\n", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
	rlmt.rlim_cur = (rlim_t)655350;
	rlmt.rlim_max = (rlim_t)655356;
	if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
		char buf[64];
		sprintf(buf, "ulimit -n %d", rlmt.rlim_max);
		if (-1 == system(buf)) {
			_LOG_ERROR("%s failed", buf);
			exit(0);
		}
		_LOG_ERROR("Set max fd open count failed");
		exit(0);
	}
#endif
}

// static void asyncOutput(const char* msg, int len) {
// 	std::string out = msg;
// 	// dump the error now.
// 	int pos = out.find("ERROR");
// 	if (pos >= 0) {
// 		out = RED + out + NONE;
// 	}
// 	// dump the warning now.
// 	pos = out.find("WARN");
// 	if (pos >= 0) {
// 		out = GREEN + out + NONE;
// 	}
// 	// dump the info now.
// 	pos = out.find("INFO");
// 	if (pos >= 0) {
// 		out = PURPLE + out + NONE;
// 	}
// 	// dump the debug now.
// 	pos = out.find("DEBUG");
// 	if (pos >= 0) {
// 		out = BROWN + out + NONE;
// 	}
// 	gAsyncLog->append(msg, len);
// 	// dump the special content for write the output window now.
// 	size_t n = std::fwrite(out.c_str(), 1, out.length(), stdout);
// 	(void)n;
// }

static int setEnv(std::string const& logdir, std::string const& logname, int loglevel) {
	SetRLIMIT();

	char const* old_library_path = getenv("LD_LIBRARY_PATH");
	std::string path = ".";
	setenv("LD_LIBRARY_PATH", path.c_str(), false);
	path = "/usr/lib:/usr/lib64:/usr/local/lib:/usr/local/lib64:/home/andy/cppserver/libs";
	setenv("LD_LIBRARY_PATH", path.c_str(), false);

	//muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	//muduo::Logger::setOutput(asyncOutput);

	//muduo::TimeZone beijing(gEastUTC, "CST");
	//muduo::Logger::setTimeZone(beijing);

	if (!boost::filesystem::exists(logdir)) {
		boost::filesystem::create_directories(logdir);
	}
	//static muduo::AsyncLogging log(::basename(logname.c_str()), kRollSize);
	//log.start();
	//gAsyncLog = &log;
	return 0;
}

#endif