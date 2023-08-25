#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include "Logger/src/Macro.h"

#include "Logger/src/utils/Assert.h"

#include "Logger/src/excp/excp.h"

#include "Logger/src/log/Logger.h"

#include "Logger/src/track/MemTrack.h"

#include "Logger/src/ini/ini.h"

#include "Logger/src/zlib/gzip.h"

#include "Logger/src/crypt/aes.h"
#include "Logger/src/crypt/sha1.h"
#include "Logger/src/crypt/md5.h"
#include "Logger/src/crypt/url.h"
#include "Logger/src/crypt/html.h"
#include "Logger/src/crypt/base64.h"

#include "Logger/src/rand/StdRandom.h"

#include "Logger/src/lock/SpinLock.h"
#include "Logger/src/time/time.h"
#include "Logger/src/sign/sign.h"

#include "Logger/src/generic/generic.h"

#define _CODE_ utils::format_s(_PARAM_)

namespace utils {
	
	int numCPU();
	
	std::string format_s(char const* file, int line, char const* func);

	namespace uuid {
		std::string createUUID();
	}
	
	std::string sprintf(char const* format, ...);
	
	std::string buffer2HexStr(uint8_t const* buf, size_t len);

	std::string clearDllPrefix(std::string const& path);

	char const* MD5Encode(char const* src, unsigned len, char dst[], int upper);

	void replaceAll(std::string& s, std::string const& src, std::string const& dst);
	
	void replaceEscChar(std::string& s);
	
	bool parseQuery(std::string const& queryStr, std::map<std::string, std::string>& params);

	std::string GetModulePath(std::string* filename = NULL, bool exec = false);

	bool mkDir(char const* dir);

	/*tid_t*/std::string gettid();

	std::string const trim_file(char const* _FILE_);

	std::string const trim_func(char const* _FUNC_);

	std::string str_error(unsigned errnum);

	void convertUTC(time_t const t, struct tm& tm, time_t* tp = NULL, int64_t timezone = MY_CST);

	std::string strfTime(time_t const t, int64_t timezone);
	
	time_t strpTime(char const* s, int64_t timezone);

	void timezoneInfo(struct tm const& tm, int64_t timezone);

	std::string ws2str(std::wstring const& ws);

	std::wstring str2ws(std::string const& str);
	
	bool is_utf8(char const* str, size_t len);
	
	std::string gbk2UTF8(char const* gbk, size_t len);

	std::string utf82GBK(char const* utf8, size_t len);

	unsigned int now_ms();

	std::string stack_backtrace();

	void crashCoreDump(std::function<void()> cb);

	void runAsRoot(std::string const& execname, bool bv = false);

	bool enablePrivilege(std::string const& path);

	bool checkVCRedist();

	void registerSignalHandler(int signal, void(*handler)(int));

	void setrlimit();

	void setenv();

	int getNetCardIp(std::string const& netCardName, std::string& Ip);

	std::string inetToIp(uint32_t inetIp);

	std::string hnetToIp(uint32_t hnetIp);

	bool checkSubnetIpstr(char const* srcIp, char const* dstIp);

	bool checkSubnetInetIp(uint32_t srcInetIp, uint32_t dstInetIp);

	double floorx(double d, int bit);

	double roundx(double d, int bit);

	double floors(std::string const& s);

	int64_t rate100(std::string const& s);

	bool isDigitalStr(std::string const& str);
	
	namespace random {
		std::string charStr(int n, rTy x = rTy::CharNumber);
	}
}

#endif