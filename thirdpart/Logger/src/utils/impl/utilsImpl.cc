#include "utilsImpl.h"
#include "gettimeofday.h"
#include "../../rand/impl/StdRandomImpl.h"

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

__thread int t_cachedTid;
__thread char t_tidString[32];
__thread int t_tidStringLength;

static inline tid_t gettid() {
	return static_cast<tid_t>(::syscall(SYS_gettid));
}

static void cacheTid() {
	if (t_cachedTid == 0) {
		t_cachedTid = gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
	}
}
#endif

#include "../../log/impl/LoggerImpl.h"
#include "../../excp/impl/excpImpl.h"

namespace utils {
	
	int _numCPU() {
#if _windows_
		SYSTEM_INFO sysInfo;
		::GetSystemInfo(&sysInfo);
		int all = sysInfo.dwNumberOfProcessors;
#else
		//获取当前系统的所有CPU核数，包含禁用的
		int all = sysconf(_SC_NPROCESSORS_CONF);//sysconf(_SC_NPROCS_CONF) get_nprocs_conf()
		//获取当前系统的可用CPU核数
		int enable = sysconf(_SC_NPROCESSORS_ONLN);//sysconf(_SC_NPROCS_ONLN) get_nprocs()
#endif
		return all;
	}

	std::string _gettid() {
#ifdef _windows_
		std::ostringstream oss;
		oss << std::this_thread::get_id();
		return oss.str();//(tid_t)std::stoull(oss.str());
#elif defined(_linux_)
		if (__builtin_expect(t_cachedTid == 0, 0)) {
			cacheTid();
		}
		return std::to_string(t_cachedTid);
#endif
	}

	std::string _sprintf(char const* format, ...) {
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
	
	std::string _format_s(char const* file, int line, char const* func) {
		char buf[BUFSZ] = { 0 };
		snprintf(buf, BUFSZ, "%s:%d] %s",
			utils::_trim_file(file).c_str(), line, utils::_trim_func(func).c_str());
		return buf;
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
#if 0
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
#else
			boost::uuids::random_generator gen;
			boost::uuids::uuid u = gen();
			std::string uuid;
			uuid.assign(u.begin(), u.end());
			return uuid;
#endif
		}
	}
	std::string _buffer2HexStr(uint8_t const* buf, size_t len) {
		std::ostringstream oss;
		oss << std::hex << std::uppercase << std::setfill('0');
		for (size_t i = 0; i < len; ++i) {
			oss << std::setw(2) << (unsigned int)(buf[i]);
		}
		return oss.str();
	}

	std::string _clearDllPrefix(std::string const& path) {
#ifdef _linux_
		size_t pos = path.find("./");
		if (0 == pos) {
			const_cast<std::string&>(path).replace(pos, 2, "");
		}
		pos = path.find("lib");
		if (0 == pos) {
			const_cast<std::string&>(path).replace(pos, 3, "");
		}
		pos = path.find(".so");
		if (std::string::npos != pos) {
			const_cast<std::string&>(path).replace(pos, 3, "");
		}
#endif
		return path;
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

	bool _parseQuery(std::string const& queryStr, std::map<std::string, std::string>& params) {
		params.clear();
		do {
			std::string subStr;
			std::string::size_type npos = queryStr.find_first_of('?');
			if (npos != std::string::npos) {
				//skip '?'
				subStr = queryStr.substr(npos + 1, std::string::npos);
			}
			else {
				subStr = queryStr;
			}
			if (subStr.empty()) {
				break;
			}
			for (;;) {
				//key value separate
				std::string::size_type kpos = subStr.find_first_of('=');
				if (kpos == std::string::npos) {
					break;
				}
				//next start
				std::string::size_type spos = subStr.find_first_of('&');
				if (spos == std::string::npos) {
					std::string key = subStr.substr(0, kpos);
					//skip '='
					std::string val = subStr.substr(kpos + 1, std::string::npos);
					params[key] = val;
					break;
				}
				else if (kpos < spos) {
					std::string key = subStr.substr(0, kpos);
					//skip '='
					std::string val = subStr.substr(kpos + 1, spos - kpos - 1);
					params[key] = val;
					//skip '&'
					subStr = subStr.substr(spos + 1, std::string::npos);
				}
				else {
					break;
				}
			}
		} while (0);
		return params.size() > 0;
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

	void _registerSignalHandler(int signal, void(*handler)(int)) {
#ifdef _linux_
		struct sigaction sa;
		sa.sa_handler = handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(signal, &sa, nullptr);
#endif
	}

	void _setrlimit() {
#ifdef _linux_
		struct rlimit rlmt;
		if (getrlimit(RLIMIT_CORE, &rlmt) == -1) {
			exit(0);
		}
		//__LOG_DEBUG("Before set rlimit CORE dump current is:%d, max is:%d", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
		rlmt.rlim_cur = 1024 * 1024 * 1024;
		rlmt.rlim_max = 1024 * 1024 * 1024;
		if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
			__LOG_ERROR("RLIMIT_CORE error");
			exit(0);
		}
#if 0
		if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1)
			return false;
		//__LOG_DEBUG("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
		rlmt.rlim_cur = (rlim_t)655350;
		rlmt.rlim_max = (rlim_t)655356;
		if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1)
			exit(0);
#elif 0
		//ulimit -a / ulimit -n
		//http://juduir.com/topic/1802500000000000075
		if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
			__LOG_ERROR("ulimit fd number failed");
			exit(0);
		}
		//__LOG_DEBUG("Before set rlimit RLIMIT_NOFILE current is:%d, max is:%d", (int)rlmt.rlim_cur, (int)rlmt.rlim_max);
		rlmt.rlim_cur = (rlim_t)655350;
		rlmt.rlim_max = (rlim_t)655356;
		if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
			char buf[64];
			sprintf(buf, "ulimit -n %d", rlmt.rlim_max);
			if (-1 == system(buf)) {
				__LOG_ERROR("%s failed", buf);
				exit(0);
			}
			__LOG_ERROR("Set max fd open count failed");
			exit(0);
		}
#endif
#endif
	}

	void _setenv() {
#ifdef _linux_
#if 0
		std::string path(getenv("LD_LIBRARY_PATH"));
		path += ":.:/usr/lib:/usr/lib64:/usr/local/lib:/usr/local/lib64:/home/andy/cppserver/libs";
		setenv("LD_LIBRARY_PATH", path.c_str(), false);
#else
		std::string path = ".";
		setenv("LD_LIBRARY_PATH", path.c_str(), false);
		path = getenv("LD_LIBRARY_PATH");
		path += ":/usr/lib:/usr/lib64:/usr/local/lib:/usr/local/lib64:/home/andy/cppserver/libs";
		setenv("LD_LIBRARY_PATH", path.c_str(), false);
#endif
#endif
	}
	
	int _getNetCardIp(std::string const& netCardName, std::string& Ip) {
#ifdef _linux_
		int sockfd;
		struct ifconf conf;
		struct ifreq* ifr;
		char buf[BUFSZ] = { 0 };
		int num;
		int i;
		sockfd = ::socket(PF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) {
			return -1;
		}
		conf.ifc_len = BUFSZ;
		conf.ifc_buf = buf;
		if (::ioctl(sockfd, SIOCGIFCONF, &conf) < 0) {
			::close(sockfd);
			return -1;
		}
		num = conf.ifc_len / sizeof(struct ifreq);
		ifr = conf.ifc_req;

		for (int i = 0; i < num; ++i) {
			struct sockaddr_in* sin = (struct sockaddr_in*)(&ifr->ifr_addr);
			if (::ioctl(sockfd, SIOCGIFFLAGS, ifr) < 0) {
				::close(sockfd);
				return -1;
			}
			if ((ifr->ifr_flags & IFF_UP) &&
				strcmp(netCardName.c_str(), ifr->ifr_name) == 0) {
				Ip = ::inet_ntoa(sin->sin_addr);
				::close(sockfd);
				return 0;
			}

			++ifr;
		}
		::close(sockfd);
#endif
		return -1;
	}

	static void toIp_c(char* buf, size_t size, sockaddr const* addr)
	{
		// char *inet_ntoa(struct in_addr in);
		if (AF_INET == addr->sa_family) {
			if (NULL == ::inet_ntop(AF_INET, &((sockaddr_in*)addr)->sin_addr, buf, size)) {
				assert(false);
			}
		}
		else if (AF_INET6 == addr->sa_family) {
			if (NULL == ::inet_ntop(AF_INET6, &((sockaddr_in6*)addr)->sin6_addr, buf, size))
				assert(false);
		}
	}

	static std::string toIp(sockaddr const* addr)
	{
		char buf[64] = { 0 };
		toIp_c(buf, sizeof(buf), addr);
		return buf;
	}

	std::string _inetToIp(uint32_t inetIp) {
		sockaddr_in inaddr = { 0 };
		inaddr.sin_family = AF_INET;
		inaddr.sin_addr.s_addr = in_addr_t(inetIp);
		return toIp((sockaddr*)&inaddr);
	}

	std::string _hnetToIp(uint32_t hnetIp) {
		//主机字节序转换网络字节序
		return _inetToIp(htonl(hnetIp));
	}

	//判断srcIpstr与dstIpstr是否在同一个子网，比较 x.y.z.0 与 a.b.c.0
	bool _checkSubnetIpstr(char const* srcIp, char const* dstIp) {
		////////////////////////////////////////
		//ipstr -> 网络字节序 -> 主机字节序
		//192.168.161.12 -> inet_pton -> 0XCA1A8C0 -> ntohl -> 0XC0A8A10C
		//11000000 10101000 10100001 00001100
		////////////////////////////////////////
		//192.168.160.2  -> inet_pton -> 0X2A0A8C0 -> ntohl -> 0XC0A8A002
		//11000000 10101000 10100000 00000010
		////////////////////////////////////////
		//255.255.255.0  -> inet_pton -> 0X00FFFFFF -> ntohl -> 0XFFFFFF00
		//11111111 11111111 11111111 00000000
		sockaddr_in sinaddr, dinaddr, minaddr;
		//char const* srcIp = "192.168.161.12";
		if (inet_pton(AF_INET, srcIp, &sinaddr.sin_addr) < 1) {
			__LOG_ERROR("inet_pton error:%s", srcIp);
			assert(false);
		}
		//char const* dstIp = "192.168.160.2";
		if (inet_pton(AF_INET, dstIp, &dinaddr.sin_addr) < 1) {
			__LOG_ERROR("inet_pton error:%s", dstIp);
			assert(false);
		}
		char const* maskIp = "255.255.255.0";
		if (inet_pton(AF_INET, maskIp, &minaddr.sin_addr) < 1) {
			__LOG_ERROR("inet_pton error:%s", maskIp);
			assert(false);
		}
		//11000000 10101000 10100001 00001100
		uint32_t sinetaddr = ntohl(sinaddr.sin_addr.s_addr);
		//11000000 10101000 10100000 00000010
		uint32_t dinetaddr = ntohl(dinaddr.sin_addr.s_addr);
		//11111111 11111111 11111111 00000000
		uint32_t minetaddr = ntohl(minaddr.sin_addr.s_addr);
		//比较 x.y.z.0 与 a.b.c.0
		uint32_t srcmask = sinetaddr & minetaddr;
		uint32_t dstmask = dinetaddr & minetaddr;
		if (srcmask == dstmask) {
			__LOG_WARN("same subnet: %#X:%s & %#X:%s", sinetaddr, srcIp, dinetaddr, dstIp);
		}
		else {
			__LOG_WARN("different subnet: %#X:%s & %#X:%s", sinetaddr, srcIp, dinetaddr, dstIp);
		}
		return srcmask == dstmask;
	}

	bool _checkSubnetInetIp(uint32_t srcInetIp, uint32_t dstInetIp) {
		sockaddr_in sinaddr, dinaddr, minaddr;
		char const* maskIpstr = "255.255.255.0";
		if (inet_pton(AF_INET, maskIpstr, &minaddr.sin_addr) < 1) {
			__LOG_ERROR("inet_pton error:%s\n", maskIpstr);
			assert(false);
		}
		//11000000 10101000 10100001 00001100
		uint32_t sinetaddr = ntohl(in_addr_t(srcInetIp));
		//11000000 10101000 10100000 00000010
		uint32_t dinetaddr = ntohl(in_addr_t(dstInetIp));
		//11111111 11111111 11111111 00000000
		uint32_t minetaddr = ntohl(minaddr.sin_addr.s_addr);
		//比较 x.y.z.0 与 a.b.c.0
		uint32_t srcmask = sinetaddr & minetaddr;
		uint32_t dstmask = dinetaddr & minetaddr;
		char const* srcIp = _inetToIp(srcInetIp).c_str();
		char const* dstIp = _inetToIp(dstInetIp).c_str();
		if (srcmask == dstmask) {
			__LOG_WARN("same subnet: %#X:%s & %#X:%s", sinetaddr, srcIp, dinetaddr, dstIp);
		}
		else {
			__LOG_WARN("different subnet: %#X:%s & %#X:%s", sinetaddr, srcIp, dinetaddr, dstIp);
		}
		return srcmask == dstmask;
	}

	//截取double小数点后bit位，直接截断
	double _floorx(double d, int bit) {
		int rate = pow(10, bit);
		int64_t x = int64_t(d * rate);
		return (((double)x) / rate);
	}

	//截取double小数点后bit位，四舍五入
	double _roundx(double d, int bit) {
		int rate = pow(10, bit);
		int64_t x = int64_t(d * rate + 0.5f);
		return (((double)x) / rate);
	}

	//截取double小数点后2位，直接截断
	double _floors(std::string const& s) {
		std::string::size_type npos = s.find_first_of('.');
		if (npos != std::string::npos) {
			std::string dot;
			std::string prefix = s.substr(0, npos);
			std::string sufix = s.substr(npos + 1, -1);
			if (!sufix.empty()) {
				if (sufix.length() >= 2) {
					dot = sufix.substr(0, 2);
				}
				else {
					dot = sufix.substr(0, 1);
				}
			}
			std::string x = prefix + "." + dot;
			return atof(x.c_str());
		}
		else {
			return atof(s.c_str());
		}
	}

	//截取double小数点后2位，直接截断并乘以100转int64_t
	int64_t _rate100(std::string const& s) {
		std::string::size_type npos = s.find_first_of('.');
		if (npos != std::string::npos) {
			std::string prefix = s.substr(0, npos);
			std::string sufix = s.substr(npos + 1, -1);
			std::string x;
			assert(!sufix.empty());
			if (sufix.length() >= 2) {
				x = prefix + sufix.substr(0, 2);
			}
			else {
				x = prefix + sufix.substr(0, 1) + "0";
			}
			//带小数点，整数位限制9+2位
			return x.length() <= 11 ? atoll(x.c_str()) : 0;
		}
		//不带小数点，整数位限制9+2位
		return s.length() <= 9 ? (atoll(s.c_str()) * 100) : 0;
	}

	bool _isDigitalStr(std::string const& s) {
#if 0
		boost::regex reg("^[1-9]\d*\,\d*|[1-9]\d*$");
#elif 1
		boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
#elif 0
		boost::regex reg("^[+-]?(0|([1-9]\d*))(\.\d+)?$");
#elif 0
		boost::regex reg("[1-9]\d*\.?\d*)|(0\.\d*[1-9]");
#endif
		return boost::regex_match(s, reg);
	}

	namespace random {
		static char const* arr[rTy::rMax] = {
				"0123456789",
				"abcdefghijklmnopqrstuvwxyz",
				"abcdefghijklmnopqrstuvwxyz0123456789",
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
				"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",
				"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789",
		};
		std::string _charStr(int n, rTy x) {
			std::string s;
			s.resize(n/* + 1*/);
			static int const size = strlen((char const*)arr[x]);
			//__LOG_TRACE("len=%d size=%d arr[%d]=%s", n, size, x, arr[x]);
			for (int i = 0; i < n; ++i) {
				int r = _RANDOM().betweenInt(0, size - 1).randInt_mt();
				//__LOG_TRACE("s[%d] = arr[%d][%d] = %c", i, x, r, arr[x][r]);
				s[i] = arr[x][r];
			}
			//s[n] = '\0';
			return s;
		}
	}
}