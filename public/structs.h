#ifndef INCLUDE_STRUCTS_H
#define INCLUDE_STRUCTS_H

#include "Logger/src/Macro.h"

struct agent_info_t {
	agent_info_t() {
		score = 0;
		status = 0;
		agentId = 0;
		cooperationtype = 0;
	}
	int64_t		score;              //代理分数 
	int32_t		status;             //是否被禁用 0正常 1停用
	int32_t		agentId;            //agentId
	int32_t		cooperationtype;    //合作模式  1 买分 2 信用
	std::string descode;            //descode 
	std::string md5code;            //MD5 
};

struct agent_user_t {
	agent_user_t() {
		userId = 0;
		score = 0;
		onlinestatus = 0;
	}
	int64_t userId;
	int64_t score;
	int32_t onlinestatus;
	std::string linecode;
};

struct UserBaseInfo {
	UserBaseInfo() {
		userId = -1;
		account = "";
		headId = 0;
		nickName = "";
		userScore = 0;
		agentId = 0;
		lineCode = "";
		status = 0;
		location = "";

		takeMaxScore = 0;
		takeMinScore = 0;
		ip = 0;

		alladdscore = 0;
		allsubscore = 0;
		winlostscore = 0;
		allbetscore = 0;
	}

	int64_t userId;
	std::string account;
	uint8_t headId;
	std::string nickName;
	int64_t userScore;
	uint32_t agentId;
	std::string lineCode;
	uint32_t status;
	uint32_t ip;
	std::string location;
	int64_t takeMaxScore;
	int64_t takeMinScore;
	int64_t alladdscore;
	int64_t allsubscore;
	int64_t winlostscore;
	int64_t allbetscore;
};

#endif