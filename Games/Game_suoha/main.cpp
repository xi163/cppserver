
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

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdarg.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <string.h>

#define INVALID_CHAIR (0xFFFF)
#define GAME_PLAYER    5	//最多5人局
#define MAX_ROUND      4		//最大局数

static short currentUser_ = INVALID_CHAIR;
static short firstUser_ = INVALID_CHAIR;
static bool bPlaying_[GAME_PLAYER];
int currentTurn_ = -1;

int main()
{
	
	// 836
	//♣9 ♥K ♦Q ♠6 ♦A
	//♣4 ♠10 ♦6 ♥8 ♣5
	//suoha::CGameLogic::TestCards("./conf/zjh_cardList.ini");
	return 0;
}
