#include "Privilege.h"

#if defined(_windows_)
#include <AccCtrl.h>
#include <AclAPI.h>
#include <DSRole.h>
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib,"Advapi32.lib")
#endif

#include "../../excp/impl/excpImpl.h"
#include "../../utils/impl/utilsImpl.h"

namespace utils {

	static inline bool _roleGetPrimaryDomain() {
#if defined(_windows_)
		DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = NULL;
		if (::DsRoleGetPrimaryDomainInformation(NULL,
			DsRolePrimaryDomainInfoBasic,
			(PBYTE*)&info) != ERROR_SUCCESS) {
			__TLOG_ERROR("DsRoleGetPrimaryDomainInformation: %u\n", ::GetLastError());
			return false;
		}
		if (info->DomainNameDns == NULL) {
			__TLOG_DEBUG("DomainNameDns is NULL\n");
		}
		else {
			__TLOG_DEBUG("DomainNameDns: %s\n", info->DomainNameDns);
		}
#endif
		return false;
	}

	static inline std::string _getUserName() {
#if defined(_windows_)
		char buffer[256] = { 0 };
		DWORD size = sizeof(buffer);
		::GetUserNameA(buffer, &size);
		return buffer;
#else
		return "";
#endif
	}

	static inline bool _enablePrivilege(std::string const& path, std::string const& username) {
#if defined(_windows_)
		std::wstring wpath = utils::_str2ws(path);
		std::wstring wusername = utils::_str2ws(username);
		BOOL ok = true;
		EXPLICIT_ACCESS ea = { 0 };
		PACL pNewDacl = NULL;
		PACL pOldDacl = NULL;
		do {

			//获取文件(夹)安全对象的DACL列表
			if (ERROR_SUCCESS !=
				::GetNamedSecurityInfo(
					(LPTSTR)wpath.c_str(),
					SE_FILE_OBJECT,
					DACL_SECURITY_INFORMATION,
					NULL, NULL, &pOldDacl, NULL, NULL)) {
				ok = FALSE;
				break;
			}

			//此处不可直接用AddAccessAllowedAce函数,因为已有的DACL长度是固定,必须重新创建一个DACL对象

			//生成指定用户帐户的访问控制信息(这里指定赋予全部的访问权限)
			::BuildExplicitAccessWithName(
				&ea,
				(LPTSTR)wusername.c_str(),
				GENERIC_ALL, GRANT_ACCESS, SUB_CONTAINERS_AND_OBJECTS_INHERIT);

			//创建新的ACL对象(合并已有的ACL对象和刚生成的用户帐户访问控制信息)
			if (ERROR_SUCCESS != ::SetEntriesInAcl(
				1,
				&ea,
				pOldDacl,
				&pNewDacl)) {
				ok = FALSE;
				break;
			}
			// 设置文件(夹)安全对象的DACL列表
			if (ERROR_SUCCESS !=
				::SetNamedSecurityInfo(
					(LPTSTR)wpath.c_str(),
					SE_FILE_OBJECT,
					DACL_SECURITY_INFORMATION,
					NULL, NULL, pNewDacl, NULL)) {
				ok = FALSE;
			}
		} while (0);

		if (NULL != pNewDacl) {
			::LocalFree(pNewDacl);
		}

		return ok == TRUE;
#else
		return false;
#endif
	}

	static inline bool _isRunAsRoot() {
#if defined(_windows_)
		BOOL as = FALSE;
		DWORD err = ERROR_SUCCESS;
		PSID psid = NULL;
		SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
		//获取Windows安全标识符
		if (!::AllocateAndInitializeSid(
			&sia,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0, &psid)) {
			err = GetLastError();
			goto end_;
		}
		//程序是否是管理员运行
		if (!::CheckTokenMembership(NULL, psid, &as)) {
			err = ::GetLastError();
			goto end_;
		}
	end_:
		if (psid) {
			::FreeSid(psid);
			psid = NULL;
		}
		if (ERROR_SUCCESS != err) {
			throw err;
		}
		return as == TRUE;
#else
		return false;
#endif
	}

	static inline bool _shellRunAs(std::string const& execname) {
#if defined(_windows_)
		SHELLEXECUTEINFO sei;
		memset(&sei, 0, sizeof(sei));
		std::wstring ws = utils::_str2ws(execname);
		sei.lpFile = (LPCTSTR)ws.c_str();
		sei.cbSize = sizeof(sei);
		sei.lpVerb = _T("runas");
		sei.fMask = SEE_MASK_NO_CONSOLE;
		sei.nShow = SW_SHOWDEFAULT;
		sei.hwnd = NULL;
		if (!::ShellExecuteEx(&sei)) {
			DWORD err = ::GetLastError();
			if (err == ERROR_CANCELLED ||
				err == ERROR_FILE_NOT_FOUND) {
				return false;
			}
			return false;
		}
		return true;
#else
		return false;
#endif
	}

	void _runAsRoot(std::string const& execname, bool bv) {
		__MY_TRY();
#if defined(_windows_)
		if (!utils::_isRunAsRoot()) {
			::CreateEvent(NULL, FALSE, FALSE, _T("{29544E05-024F-4BC1-A272-452DBC8E17A4}"));
			if (ERROR_SUCCESS != ::GetLastError()) {
				return;
			}
			else {
				if (utils::_shellRunAs(execname)) {
				}
				else {
				}
			}
			exit(0);
		}
		else if (bv) {
			__LOG_CONSOLE_OPEN();
			__TLOG_INFO("管理员身份启动...");
			__LOG_CONSOLE_CLOSE(1000);
		}
#endif
		__MY_CATCH();
	}

	bool _enablePrivilege(std::string const& path) {
		return utils::_enablePrivilege(path, _getUserName());
	}
}