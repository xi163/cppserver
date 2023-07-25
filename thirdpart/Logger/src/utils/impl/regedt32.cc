#include "regedt32.h"
#include "utilsImpl.h"
#include "../../log/impl/LoggerImpl.h"

namespace utils {

#if defined(_windows_)
	//注册表查询
	static inline std::string _regQuery(HKEY hkey, char const* subkey, char const* valName) {
		HKEY hSubKey;
		LSTATUS err;
		if (ERROR_SUCCESS == (err = ::RegOpenKeyA(hkey, subkey, &hSubKey))) {
			DWORD type_ = REG_SZ;
			BYTE data[1024];
			DWORD size = sizeof(data);
			if (ERROR_SUCCESS == (err = ::RegQueryValueExA(
				hSubKey,
				valName,
				0,
				&type_,
				(LPBYTE)&data, &size))) {
				::RegCloseKey(hkey);
				return (char const*)data;
			}
			else {
				//__PLOG_ERROR("RegQueryValueExA failed(%d) %s", err, utils::_str_error(err).c_str());
			}
			::RegCloseKey(hkey);
		}
		else {
			//__PLOG_ERROR("RegOpenKeyA failed(%d) %s", err, utils::_str_error(err).c_str());
		}
		return "";
	}
	
	//检查是否安装VC运行时库
	bool _checkVCRedist() {
		std::string val = utils::_regQuery(
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{57a73df6-4ba9-4c1d-bbbb-517289ff6c13}",
			"DisplayName");
		char const* dst = "Microsoft Visual C++ 2015-2022 Redistributable";
		return !val.empty() && 0 == strncasecmp(val.c_str(), dst, strlen(dst));
	}
#endif
}