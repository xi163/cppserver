#include "../utils.h"
#include "utilsImpl.h"
#include "backtrace.h"
#include "Privilege.h"
#include "regedt32.h"
#include "../../crypt/md5.h"
#include "../../auth/auth.h"

#ifdef _windows_
//#include <DbgHelp.h>
#include <ImageHlp.h>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Dbghelp.lib")
#elif defined(_linux_)
#include <cxxabi.h>
#include <execinfo.h>
#include <locale> 
//#include <codecvt>
#endif

namespace utils {
	
	int numCPU() {
		AUTHORIZATION_CHECK_I;
		return utils::_numCPU();
	}
	
	/*tid_t*/std::string gettid() {
		AUTHORIZATION_CHECK_S;
		return utils::_gettid();
	}
	
	std::string sprintf(char const* format, ...) {
		AUTHORIZATION_CHECK_S;
		char buf[BUFSZ] = { 0 };
		va_list ap;
		va_start(ap, format);
#ifdef _windows_
		size_t n = vsnprintf_s(buf, BUFSZ, _TRUNCATE, format, ap);
#else
		size_t n = vsnprintf(buf, BUFSZ, format, ap);
#endif
		va_end(ap);
		(void)n;
		return buf;
	}

	std::string format_s(char const* file, int line, char const* func) {
		AUTHORIZATION_CHECK_S;
		return utils::_format_s(file, line, func);
	}

	std::string const trim_file(char const* _FILE_) {
		AUTHORIZATION_CHECK_S;
		return utils::_trim_file(_FILE_);
	}

	std::string const trim_func(char const* _FUNC_) {
		AUTHORIZATION_CHECK_S;
		return utils::_trim_func(_FUNC_);
	}

	std::string str_error(unsigned errnum) {
		AUTHORIZATION_CHECK_S;
		return utils::_str_error(errnum);
	}

	void convertUTC(time_t const t, struct tm& tm, time_t* tp, int64_t timezone) {
		AUTHORIZATION_CHECK;
		utils::_convertUTC(t, tm, tp, timezone);
	}

	void timezoneInfo(struct tm const& tm, int64_t timezone) {
		AUTHORIZATION_CHECK;
		utils::_timezoneInfo(tm, timezone);
	}

	std::string strfTime(time_t const t, int64_t timezone) {
		AUTHORIZATION_CHECK_S;
		return utils::_strfTime(t, timezone);
	}

	time_t strpTime(char const* s, int64_t timezone) {
		AUTHORIZATION_CHECK_I;
		return utils::_strpTime(s, timezone);
	}

	namespace uuid {
		std::string createUUID() {
			AUTHORIZATION_CHECK_S;
			return utils::uuid::_createUUID();
		}
	}

	std::string buffer2HexStr(uint8_t const* buf, size_t len) {
		AUTHORIZATION_CHECK_S;
		return _buffer2HexStr(buf, len);
	}

	std::string clearDllPrefix(std::string const& path) {
		AUTHORIZATION_CHECK_S;
		return _clearDllPrefix(path);
	}

	std::string ws2str(std::wstring const& ws) {
		AUTHORIZATION_CHECK_S;
		return utils::_ws2str(ws);
	}

	std::wstring str2ws(std::string const& str) {
		return utils::_str2ws(str);
	}
	
	std::string gbk2UTF8(char const* gbk, size_t len) {
		AUTHORIZATION_CHECK_S;
		return gbk2UTF8(gbk, len);
	}

	std::string utf82GBK(char const* utf8, size_t len) {
		AUTHORIZATION_CHECK_S;
		return utils::_utf82GBK(utf8, len);
	}

	bool is_utf8(char const* str, size_t len) {
		AUTHORIZATION_CHECK_B;
		return utils::_is_utf8(str, len);
	}

	bool mkDir(char const* dir) {
		AUTHORIZATION_CHECK_B;
		return utils::_mkDir(dir);
	}
	
	char const* MD5Encode(char const* src, unsigned len, char dst[], int upper) {
		AUTHORIZATION_CHECK_P;
		return utils::MD5(src, len, dst, upper);

	}

	void replaceAll(std::string& s, std::string const& src, std::string const& dst) {
		AUTHORIZATION_CHECK;
		utils::_replaceAll(s, src, dst);
	}
	
	void replaceEscChar(std::string& s) {
		AUTHORIZATION_CHECK;
		utils::_replaceEscChar(s);
	}
	
	bool parseQuery(std::string const& queryStr, std::map<std::string, std::string>& params) {
		AUTHORIZATION_CHECK;
		return utils::_parseQuery(queryStr, params);
	}

	std::string GetModulePath(std::string* filename, bool exec) {
		AUTHORIZATION_CHECK_S;
		return utils::_GetModulePath(filename, exec);
	}

	unsigned int now_ms() {
		AUTHORIZATION_CHECK_I;
		return utils::_now_ms();
	}

	std::string stack_backtrace() {
		/*
			windows
				SymInitialize
				StackWalk
				CaptureStackBackTrace 获取当前堆栈
				SymGetSymFromAddr
				SymFromAddr 获取符号信息
				SymGetLineFromAddr64 获取文件和行号信息
				SymCleanup

			linux
			   int backtrace(void **buffer, int size);
			   char **backtrace_symbols(void *const *buffer, int size);
			   void backtrace_symbols_fd(void *const *buffer, int size, int fd);
		*/
		AUTHORIZATION_CHECK_S;
		std::string stack;
		stack.append("STACK-BACKTRACE:\n");
#ifdef _windows_
		HANDLE process = ::GetCurrentProcess();
		::SymInitialize(process, NULL, TRUE);
#if 1
		static const int MAX_STACK_FRAMES = 20;
		void* pstack[MAX_STACK_FRAMES];
		WORD frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pstack, NULL);

		for (WORD i = 1; i < frames; ++i) {
			DWORD64 address = (uint64_t)(pstack[i]);
			DWORD64 sym_c = 0;

			char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
			PSYMBOL_INFO psymbol = (PSYMBOL_INFO)buf;
			psymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			psymbol->MaxNameLen = MAX_SYM_NAME;

			if (::SymFromAddr(process, address, &sym_c, psymbol)) {

				DWORD options = ::SymGetOptions();
				options |= SYMOPT_LOAD_LINES;
				options |= SYMOPT_FAIL_CRITICAL_ERRORS;
				options |= SYMOPT_DEBUG;
				::SymSetOptions(options);

				DWORD line_c = 0;
				IMAGEHLP_LINE64 line = { 0 };
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				if (SymGetLineFromAddr64(process, address, &line_c, &line)) {
					char s[256];
					snprintf(s, sizeof(s), "\t%s at %s:%d(%#x)\n",
						utils::_trim_func((char const*)psymbol->Name).c_str(),
						utils::_trim_file((char const*)line.FileName).c_str(),
						line.LineNumber,
						(uint64_t)psymbol->Address);
					stack.append(s);
				}
				else {
				}
			}
			else {
			}
		}
#else
		HANDLE thread = ::GetCurrentThread();
		CONTEXT ctx = { 0 };
		{
			::GetThreadContext(thread, &ctx);
			__asm {call $ + 5}
			__asm {pop eax}
			__asm {mov ctx.Eip, eax}
			__asm {mov ctx.Ebp, ebp}
			__asm {mov ctx.Esp, esp}
		}

		STACKFRAME sf = { 0 };
		sf.AddrPC.Offset = ctx.Eip;
		sf.AddrPC.Mode = AddrModeFlat;
		sf.AddrFrame.Offset = ctx.Ebp;
		sf.AddrFrame.Mode = AddrModeFlat;
		sf.AddrStack.Offset = ctx.Esp;
		sf.AddrStack.Mode = AddrModeFlat;
		int c = 0;
		while (StackWalk(IMAGE_FILE_MACHINE_I386,
			process, thread, &sf, &ctx,
			NULL,
			SymFunctionTableAccess, SymGetModuleBase, NULL)) {
			if (c++ == 0)
				continue;
			DWORD64 sym_c = 0;
			char symbol[sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME * sizeof(TCHAR)];
			PIMAGEHLP_SYMBOL64 psymbol = (PIMAGEHLP_SYMBOL64)&symbol;
			psymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
			psymbol->MaxNameLength = MAX_SYM_NAME;

			if (::SymGetSymFromAddr64(process, sf.AddrPC.Offset, &sym_c, psymbol)) {

				DWORD options = ::SymGetOptions();
				options |= SYMOPT_LOAD_LINES;
				options |= SYMOPT_FAIL_CRITICAL_ERRORS;
				options |= SYMOPT_DEBUG;
				::SymSetOptions(options);

				DWORD line_c = 0;
				IMAGEHLP_LINE64 line = { 0 };
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				if (SymGetLineFromAddr64(process, sf.AddrPC.Offset, &line_c, &line)) {
					char s[256];
					snprintf(s, sizeof(s), "\t%s at %s:%d(%#x)\n",
						utils::_trim_func((char const*)psymbol->Name).c_str(),
						utils::_trim_file((char const*)line.FileName).c_str(),
						line.LineNumber,
						(uint64_t)psymbol->Address);
					stack.append(s);
				}
				else {
				}
			}
			else {
			}
		}
#endif
		::SymCleanup(process);
#elif defined(_linux_)
		static const int MAX_STACK_FRAMES = 200;
		void* pstack[MAX_STACK_FRAMES];
		int nptrs = ::backtrace(pstack, MAX_STACK_FRAMES);
		char** strings = ::backtrace_symbols(pstack, nptrs);
		if (strings) {
			bool demangle = false;
			size_t len = 256;
			char* demangled = demangle ? static_cast<char*>(::malloc(len)) : NULL;
			for (int i = 1; i < nptrs; ++i) {
				if (demangled) {
					//https://panthema.net/2008/0901-stacktrace-demangled/bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
					char* left_par = NULL;
					char* plus = NULL;
					for (char* p = strings[i]; *p; ++p) {
						if (*p == '(')
							left_par = p;
						else if (*p == '+')
							plus = p;
					}
					if (left_par && plus) {
						*plus = '\0';
						int status = 0;
						char* ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
						*plus = '+';
						if (status == 0) {
							demangled = ret;
							stack.append(strings[i], left_par + 1);
							stack.append(demangled);
							stack.append(plus);
							stack./*emplace_back*/push_back('\n');
							continue;
						}
					}
				}
				stack.append(strings[i]);
				stack./*emplace_back*/push_back('\n');
			}
			if (demangled)
				free(demangled);
			free(strings);
		}
#endif
		return stack;
	}

	void crashCoreDump(std::function<void()> cb) {
		AUTHORIZATION_CHECK;
		utils::_crash_coredump(cb);
	}

	void runAsRoot(std::string const& execname, bool bv) {
		AUTHORIZATION_CHECK;
		utils::_runAsRoot(execname, bv);
	}

	bool enablePrivilege(std::string const& path) {
		AUTHORIZATION_CHECK_B;
		return utils::_enablePrivilege(path);
	}

	bool checkVCRedist() {
#if defined(_windows_)
		AUTHORIZATION_CHECK_B;
		return utils::_checkVCRedist();
#else
		return true;
#endif
	}

	void registerSignalHandler(int signal, void(*handler)(int)) {
		AUTHORIZATION_CHECK;
		_registerSignalHandler(signal, handler);
	}

	void setrlimit() {
		AUTHORIZATION_CHECK;
		_setrlimit();
	}

	void setenv() {
		AUTHORIZATION_CHECK;
		_setenv();
	}

	int getNetCardIp(std::string const& netCardName, std::string& Ip) {
		AUTHORIZATION_CHECK_I;
		return _getNetCardIp(netCardName, Ip);
	}
	
	std::string inetToIp(uint32_t inetIp) {
		AUTHORIZATION_CHECK_S;
		return _inetToIp(inetIp);
	}

	std::string hnetToIp(uint32_t hnetIp) {
		AUTHORIZATION_CHECK_S;
		return _hnetToIp(hnetIp);
	}
	
	bool checkSubnetIpstr(char const* srcIp, char const* dstIp) {
		AUTHORIZATION_CHECK_B;
		return _checkSubnetIpstr(srcIp, dstIp);
	}

	bool checkSubnetInetIp(uint32_t srcInetIp, uint32_t dstInetIp) {
		AUTHORIZATION_CHECK_B;
		return _checkSubnetInetIp(srcInetIp, dstInetIp);
	}

	double floorx(double d, int bit) {
		AUTHORIZATION_CHECK_I;
		return _floorx(d, bit);
	}

	double roundx(double d, int bit) {
		AUTHORIZATION_CHECK_I;
		return _roundx(d, bit);
	}

	double floors(std::string const& s) {
		AUTHORIZATION_CHECK_I;
		return _floors(s);
	}

	int64_t rate100(std::string const& s) {
		AUTHORIZATION_CHECK_I;
		return _rate100(s);
	}

	bool isDigitalStr(std::string const& s) {
		AUTHORIZATION_CHECK_B;
		return _isDigitalStr(s);
	}

	namespace random {
		std::string charStr(int n, rTy x) {
			AUTHORIZATION_CHECK_S;
			return _charStr(n, x);
		}
	}
}