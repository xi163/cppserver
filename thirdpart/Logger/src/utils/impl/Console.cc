#include "Console.h"

namespace utils {

	void _initConsole() {
#if defined(_windows_)
		::AllocConsole();
		::SetConsoleOutputCP(65001);
		//setlocale(LC_ALL, "utf-8"/*"Chinese-simplified"*/);
		HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
//#if _MSC_VER > 1920
		FILE* fp = NULL;
		freopen_s(&fp, "CONOUT$", "w", stdout);
//#else
//		int tp = _open_osfhandle((long)h, _O_TEXT);
//		FILE* fp = _fdopen(tp, "w");
//		*stdout = *fp;
//		setvbuf(stdout, NULL, _IONBF, 0);
//#endif
		SMALL_RECT rc = { 5,5,800,600 };
		::SetConsoleWindowInfo(h, TRUE, &rc);
		CONSOLE_FONT_INFOEX cfi = { 0 };
		cfi.cbSize = sizeof(cfi);
		cfi.dwFontSize = { 0, 12 };
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL/*FW_LIGHT*/;
		lstrcpy(cfi.FaceName, _T("SimSun"));
		::SetCurrentConsoleFontEx(h, false, &cfi);
		//::CloseHandle(h);
		//::AttachConsole(GetCurrentProcessId());
#endif
	}

	void _closeConsole() {
#if defined(_windows_)
		::fclose(stdout);
		::FreeConsole();
#endif
	}
}