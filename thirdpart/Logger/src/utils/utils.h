#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include "../Macro.h"

namespace utils {

	namespace uuid {
		std::string createUUID();
	}

	char const* MD5Encode(char const* src, unsigned len, char dst[], int upper);

	void replaceAll(std::string& s, std::string const& src, std::string const& dst);
	
	void replaceEscChar(std::string& s);

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
}

#endif