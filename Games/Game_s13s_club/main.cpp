
#include <iostream>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

#include "s13s.h"
#include "cfg.h"
#include "funcC.h"

class AAA {

public:
	void Foo() {
		static time_t start = 0;
		time_t now = time(NULL);
		if ((now - start) > 30000) {
			printf("读取配置\n");
			start = now;
		}
	}
};
int main()
{
// 	AAA a, b;
// 	int i = 0;
// 	while (i++ < 100) {
// 		a.Foo();
// 		b.Foo();
// 	}
	S13S::CGameLogic::TestEnumCards("./conf/s13s_cardList.ini");
	return 0;
}
