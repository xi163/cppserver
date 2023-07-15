#include "utilsImpl.h"
#include "gettimeofday.h"

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
//#include <xlocbuf>
#endif

#include "../../log/impl/LoggerImpl.h"
#include "../../excp/impl/excpImpl.h"

namespace utils {
	
	/*tid_t*/std::string _gettid() {
		std::ostringstream oss;
		oss << std::this_thread::get_id();
		return oss.str();//(tid_t)std::stoull(oss.str());
	}

	void _trim_file(char const* _FILE_, char* buf, size_t size) {
#ifdef _windows_
		char const* p = strrchr(_FILE_, '\\');
#else
		char const* p = strrchr(_FILE_, '/');
#endif
		if (!p) {
			memcpy(buf, _FILE_, std::min<size_t>((size_t)strlen(_FILE_), size));
		}
		else {
			size_t len = 0;
			while (*++p) {
				if (len < size) {
					buf[len++] = *p;
				}
				else {
					break;
				}
			}
		}
	}

	void _trim_func(char const* _FUNC_, char* buf, size_t size) {
		char const* p = strrchr(_FUNC_, ':');
		if (!p) {
			memcpy(buf, _FUNC_, std::min<size_t>((size_t)strlen(_FUNC_), size));
		}
		else {
			while (*++p) {
				size_t len = 0;
				if (len < size) {
					buf[len++] = *p;
				}
				else {
					break;
				}
			}
		}
	}

	std::string const _trim_file(char const* _FILE_) {
		std::string f = _FILE_;
#ifdef _windows_
		f = f.substr(f.find_last_of('\\') + 1, -1);
#else
		f = f.substr(f.find_last_of('/') + 1, -1);
#endif
		return f;
	}

	std::string const _trim_func(char const* _FUNC_) {
		std::string f = _FUNC_;
		f = f.substr(f.find_last_of(':') + 1, -1);
		return f;
	}

	std::string _str_error(unsigned errnum) {
#ifdef _windows_
		char* msg;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
		std::string str(msg);
		LocalFree(msg);
		return str;
#else
		strerror(errnum);
#endif
	}

	//https://en.cppreference.com/w/c/chrono/gmtime
	//https://www.cplusplus.com/reference/ctime/gmtime/
	//https://www.cplusplus.com/reference/ctime/mktime/?kw=mktime
	//https://www.runoob.com/cprogramming/c-standard-library-time-h.html
	void _convertUTC(time_t const t, struct tm& tm, time_t* tp, int64_t timezone) {
		switch (timezone) {
		case MY_UTC: {
#ifdef _windows_
			gmtime_s(&tm, &t);//UTC/GMT
#else
			gmtime_r(&t, &tm);//UTC/GMT
#endif
			if (tp) {
				//tm -> time_t
				time_t t_zone = mktime(&tm);
				//tm -> time_t
				assert(t_zone == mktime(&tm));
				*tp = t_zone;
				if (t_zone != mktime(&tm)) {
					__LOG_S_FATAL("t_zone != mktime(&tm)");
				}
			}
			break;
		}
		case MY_PST:
		case MY_MST:
		case MY_EST:
		case MY_BST:
		case MY_GST:
		case MY_CST:
		case MY_JST:
		default: {
			struct tm tm_utc = { 0 };
#ifdef _windows_
			gmtime_s(&tm_utc, &t);//UTC/GMT
#else
			gmtime_r(&t, &tm_utc);//UTC/GMT
#endif
			//tm -> time_t
			time_t t_utc = mktime(&tm_utc);
			//(UTC+08:00) Asia/shanghai, Beijing(China) (tm_hour + MY_CST) % 24
			time_t t_zone = t_utc + timezone * 3600;
#ifdef _windows_
			//time_t -> tm
			localtime_s(&tm, &t_zone);
#else
			//time_t -> tm
			localtime_r(&t_zone, &tm);
#endif
			//tm -> time_t
			assert(t_zone == mktime(&tm));
			if (tp) {
				*tp = t_zone;
			}
			if (t_zone != mktime(&tm)) {
				__LOG_S_FATAL("t_zone != mktime(&tm)");
			}
			break;
		}
		}
	}

	void _timezoneInfo(struct tm const& tm, int64_t timezone) {
		switch (timezone) {
		case MY_EST: {
			__TLOG_INFO("America/New_York %04d-%02d-%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_BST: {
			__TLOG_INFO("Europe/London %04d-%02d-%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_GST: {
			__TLOG_INFO("Asia/Dubai %04d-%02d-%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_CST: {
			__TLOG_INFO("Beijing (China) %04d-%02d-%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		case MY_JST: {
			__TLOG_INFO("Asia/Tokyo %04d-%02d-%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			break;
		}
		default:
			break;
		}
	}

	//strfTime 2021-12-31 23:59:59
	std::string _strfTime(time_t const t, int64_t timezone) {
		struct tm tm = { 0 };
		utils::_convertUTC(t, tm, NULL, timezone);
		char chr[256];
		size_t n = strftime(chr, sizeof(chr), "%Y-%m-%d %H:%M:%S", &tm);
		(void)n;
		return chr;
	}

#ifdef _windows_
	static char* strptime(char const* s, char const* fmt, struct tm* tm) {
		// Isn't the C++ standard lib nice? std::get_time is defined such that its
		// format parameters are the exact same as strptime. Of course, we have to
		// create a string stream first, and imbue it with the current C locale, and
		// we also have to make sure we return the right things if it fails, or
		// if it succeeds, but this is still far simpler an implementation than any
		// of the versions in any of the C standard libraries.
		std::istringstream input(s);
		input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
		input >> std::get_time(tm, fmt);
		if (input.fail()) {
			return nullptr;
		}
		return (char*)(s + input.tellg());
	}
#endif

	time_t _strpTime(char const* s, int64_t timezone) {
		struct tm tm = { 0 };
		strptime(s, "%Y-%m-%d %H:%M:%S", &tm);
		//tm -> time_t
		time_t t = mktime(&tm);
		struct tm tm_zone = { 0 };
		time_t t_zone = 0;
		utils::_convertUTC(t, tm_zone, &t_zone, timezone);
		return t_zone;
	}

	namespace uuid {
		static std::random_device              rd;
		static std::mt19937                    gen(rd());
		static std::uniform_int_distribution<> dis(0, 15);
		static std::uniform_int_distribution<> dis2(8, 11);
		std::string _createUUID() {
			std::stringstream ss;
			int i;
			ss << std::hex;
			for (i = 0; i < 8; i++) {
				ss << dis(gen);
			}
			ss << "-";
			for (i = 0; i < 4; i++) {
				ss << dis(gen);
			}
			ss << "-4";
			for (i = 0; i < 3; i++) {
				ss << dis(gen);
			}
			ss << "-";
			ss << dis2(gen);
			for (i = 0; i < 3; i++) {
				ss << dis(gen);
			}
			ss << "-";
			for (i = 0; i < 12; i++) {
				ss << dis(gen);
			};
			return ss.str();
		}
	}

	std::string _ws2str(std::wstring const& ws) {
#ifdef _windows_
		_bstr_t const t = ws.c_str();
		return (char const*const)t;
#else
		//std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		//return cvt.to_bytes(ws);
		std::string s;
		return s;
#endif
	}

	std::wstring _str2ws(std::string const& str) {
#ifdef _windows_
		_bstr_t const t = str.c_str();
		return (wchar_t const*const)t;
#else
		//std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		//return cvt.from_bytes(str);
		std::wstring s;
		return s;
#endif
	}
	
	//https://blog.csdn.net/u012234115/article/details/83186386
	std::string _gbk2UTF8(char const* gbk, size_t len) {
#ifdef _windows_
		size_t length = MultiByteToWideChar(CP_ACP, 0, gbk, -1, NULL, 0);
		wchar_t* wc = new wchar_t[length + 1];
		memset(wc, 0, length * 2 + 2);
		MultiByteToWideChar(CP_ACP, 0, gbk, -1, wc, length);
		length = WideCharToMultiByte(CP_UTF8, 0, wc, -1, NULL, 0, NULL, NULL);
		char* c = new char[length + 1];
		memset(c, 0, length + 1);
		WideCharToMultiByte(CP_UTF8, 0, wc, -1, c, length, NULL, NULL);
		if (wc) delete[] wc;
		if (c) {
			std::string s(c, length + 1);
			delete[] c;
			return s;
		}
#endif
		return gbk;
	}

	std::string _utf82GBK(char const* utf8, size_t len) {
#ifdef _windows_
		size_t length = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
		wchar_t* wc = new wchar_t[length + 1];
		memset(wc, 0, length * 2 + 2);
		::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wc, length);
		length = ::WideCharToMultiByte(CP_ACP, 0, wc, -1, NULL, 0, NULL, NULL);
		char* c = new char[length + 1];
		memset(c, 0, length + 1);
		::WideCharToMultiByte(CP_ACP, 0, wc, -1, c, length, NULL, NULL);
		if (wc) delete[] wc;
		if (c) {
			std::string s(c, length + 1);
			delete[] c;
			return s;
		}
#endif
		return utf8;
	}

	//https://www.its404.com/article/sinolover/112461377
	bool _is_utf8(char const* str, size_t len) {
		bool isUTF8 = true;
		unsigned char* start = (unsigned char*)str;
		unsigned char* end = (unsigned char*)str + len;
		while (start < end) {
			if (*start < 0x80) {
				//(10000000): 值小于0x80的为ASCII字符    
				++start;
			}
			else if (*start < (0xC0)) {
				//(11000000): 值介于0x80与0xC0之间的为无效UTF-8字符    
				isUTF8 = false;
				break;
			}
			else if (*start < (0xE0)) {
				//(11100000): 此范围内为2字节UTF-8字符    
				if (start >= end - 1) {
					break;
				}
				if ((start[1] & (0xC0)) != 0x80) {
					isUTF8 = false;
					break;
				}
				start += 2;
			}
			else if (*start < (0xF0)) {
				// (11110000): 此范围内为3字节UTF-8字符    
				if (start >= end - 2) {
					break;
				}
				if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80) {
					isUTF8 = false;
					break;
				}
				start += 3;
			}
			else {
				isUTF8 = false;
				break;
			}
		}
		return isUTF8;
	}
	
	bool _mkDir(char const* dir) {
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#if 1
		struct stat stStat;
		if (stat(dir, &stStat) < 0) {
#else
		if (access(dir, 0) < 0) {
#endif
#ifdef _windows_
			if (mkdir(dir) < 0) {
				return false;
			}
#else
			if (mkdir(dir, /*0777*/S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
				return false;
			}
#endif
		}
#ifdef WIN32			
#pragma warning(pop) 
#endif
		return true;
	}
	
	void _replaceAll(std::string& s, std::string const& src, std::string const& dst) {
		std::string::size_type pos = s.find(src, 0);
		while (pos != std::string::npos) {
			s.replace(pos, src.length(), dst);
			//s.replace(pos, src.length(), dst, 0, dst.length());
			pos = s.find(src, pos + dst.length());
		}
	}

	void _replaceEscChar(std::string& s) {
		static struct replace_t {
			char const* src;
			char const* dst;
		} const chr[] = {
			{"\\r","\r"},
			{"\\n","\n"},
			{"\\t","\t"},
		};
		static int const n = sizeof(chr) / sizeof(chr[0]);
		for (int i = 0; i < n; ++i) {
			utils::_replaceAll(s, chr[i].src, chr[i].dst);
		}
	}

	std::string _GetModulePath(std::string* filename, bool exec) {
		char chr[512];
#ifdef _windows_
		::GetModuleFileNameA(NULL/*(HMODULE)GetModuleHandle(NULL)*/, chr, sizeof(chr));
#else
		char link[100];
		snprintf(link, sizeof(link), "/proc/%d/exe", getpid());
		readlink(link, chr, sizeof(chr));
		//readlink("/proc/self/exe", chr, sizeof(chr));
#endif
		std::string s(chr);
#ifdef _windows_
		utils::_replaceAll(s, "\\", "/");
#endif
		std::string::size_type pos = s.find_last_of('/');
		if (filename) {
			*filename = s.substr(pos + 1, -1);
		}
		return exec ? s : s.substr(0, pos);
	}

	unsigned int _now_ms() {

		//自开机经过的毫秒数
		return gettime() * 1000;
	}
}