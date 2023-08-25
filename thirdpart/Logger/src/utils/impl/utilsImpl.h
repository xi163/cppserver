#ifndef INCLUDE_UTILSIMPL_H
#define INCLUDE_UTILSIMPL_H

#include "../../Macro.h"

#define __CODE__ utils::_format_s(_PARAM_)

namespace utils {
	
	int _numCPU();
	
	std::string _format_s(char const* file, int line, char const* func);

	namespace uuid {
		std::string _createUUID();
	}
	
	std::string _sprintf(char _, char const* format, ...);
	
	std::string _sprintf(char const* format, ...);

	std::string _buffer2HexStr(uint8_t const* buf, size_t len);
	
	std::string _clearDllPrefix(std::string const& path);

	void _replaceAll(std::string& s, std::string const& src, std::string const& dst);
	
	void _replaceEscChar(std::string& s);

	bool _parseQuery(std::string const& queryStr, std::map<std::string, std::string>& params);

	std::string _GetModulePath(std::string* filename = NULL, bool exec = false);

	bool _mkDir(char const* dir);

	/*tid_t*/std::string _gettid();

	void _trim_file(char const* _FILE_, char* buf, size_t size);

	void _trim_func(char const* _FUNC_, char* buf, size_t size);

	std::string const _trim_file(char const* _FILE_);

	std::string const _trim_func(char const* _FUNC_);

	std::string _str_error(unsigned errnum);

	void _convertUTC(time_t const t, struct tm& tm, time_t* tp = NULL, int64_t timezone = MY_CST);

	std::string _strfTime(time_t const t, int64_t timezone);
	
	time_t _strpTime(char const* s, int64_t timezone);

	void _timezoneInfo(struct tm const& tm, int64_t timezone);

	std::string _ws2str(std::wstring const& ws);

	std::wstring _str2ws(std::string const& str);
	
	bool _is_utf8(char const* str, size_t len);
	
	std::string _gbk2UTF8(char const* gbk, size_t len);

	std::string _utf82GBK(char const* utf8, size_t len);

	unsigned int _now_ms();

	void _registerSignalHandler(int signal, void(*handler)(int));
	
	void _setrlimit();

	void _setenv();

	int _getNetCardIp(std::string const& netCardName, std::string& Ip);

	std::string _inetToIp(uint32_t inetIp);

	std::string _hnetToIp(uint32_t hnetIp);

	bool _checkSubnetIpstr(char const* srcIp, char const* dstIp);

	bool _checkSubnetInetIp(uint32_t srcInetIp, uint32_t dstInetIp);
	
	double _floorx(double d, int bit);
	
	double _roundx(double d, int bit);
	
	double _floors(std::string const& s);
	
	int64_t _rate100(std::string const& s);
	
	bool _isDigitalStr(std::string const& s);

	namespace random {
		std::string _charStr(int n, rTy x = rTy::CharNumber);
	}
}

#endif