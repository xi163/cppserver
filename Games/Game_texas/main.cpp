
#include <iostream>
#include <stdlib.h>……

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
#include <regex>

#define INVALID_CHAIR (0xFFFF)
#define GAME_PLAYER    5	//最多5人局
#define MAX_ROUND      4		//最大局数

#include "texas.h"


static void parseStringAtUsers(std::string const& parseStr, std::vector<std::string>& usersList) {
	std::string subStr(parseStr);
	do {
		std::string::size_type spos = subStr.find_first_of('@');//'@'
		if (spos != std::string::npos) {
			subStr = subStr.substr(spos + 1, std::string::npos);
			std::string::size_type epos = subStr.find_first_of(' ');//' '
			if (epos != std::string::npos) {
				std::string const& name = subStr.substr(0, epos);
				//^[a-zA-Z0-9_\u4e00-\u9fa5\d]+$
				//^(?=.{1,39}$)[a-zA-Z0-9_\u4e00-\u9fa5\d]+(?:-[a-zA-Z0-9_\u4e00-\u9fa5\d]+)*$
				//^             字符串开头
				//(?=.{1,39}$)  长度必须为1到39个字符
				//[a-zA-Z\d]+   1+个字母数字字符
				//(?:-[a-zA-Z\d]+)* 0次或多次重复
				//-             连字符
				//$             字符串结尾
				std::string name1 = "{@群主大人 @菜鸡321 @    }";
				std::regex regex("\\[(emoticon_a_([1-9]|[1][0-5]))\\]");
				if (std::regex_match(name1, regex)) {
					usersList.emplace_back(name);
				}
				subStr = subStr.substr(epos + 1, std::string::npos);
			}
			else {
				std::string const& name = subStr.substr(0, std::string::npos);
				//std::string name1 = "@群主大人 @菜鸡321 @    ";
				//if (std::regex_match(name1, std::regex("^[a-zA-Z0-9_\u4e00-\u9fa5\d]+$"))) {
				usersList.emplace_back(name);
				//}
				break;
			}
		}
		else {
			break;
		}
	} while (true);
	for (std::vector<std::string>::const_iterator it = usersList.begin();
		it != usersList.end(); ++it) {
		std::cout << "--------------- *** user: " << it->c_str();
	}
}


int main()
{
	
	// 836
	//♣9 ♥K ♦Q ♠6 ♦A
	//♣4 ♠10 ♦6 ♥8 ♣5
	//TEXAS::CGameLogic::TestEnumCards("./conf/Texas_cardList.ini");
	std::vector<std::string> usersList;
	parseStringAtUsers("@群主大人 @菜鸡321 @    ",usersList);
	return 0;
}
