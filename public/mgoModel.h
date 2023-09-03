#ifndef INCLUDE_MGO_MODEL_H
#define INCLUDE_MGO_MODEL_H

#include "Logger/src/Macro.h"

namespace mgo {
	namespace model {
		struct  GameUser {
			GameUser() {
				UserId = 0;
				AgentId = 0;
				Headindex = 0;
				//Registertime = 0;
				//Lastlogintime = 0;
				Activedays = 0;
				Keeplogindays = 0;
				Alladdscore = 0;
				Allsubscore = 0;
				Addscoretimes = 0;
				Subscoretimes = 0;
				Gamerevenue = 0;
				WinLosescore = 0;
				Score = 0;
				Status = 0;
				Onlinestatus = 0;
				Gender = 0;
				Integralvalue = 0;
			}
			//b_oid Id; //`bson:"_id,omitempty"`
			int64_t UserId;//`bson:"userid"`
			std::string Account;//`bson:"account"`
			int AgentId;//`bson:"agentid"`
			std::string Linecode;//`bson:"linecode"`
			std::string Nickname;//`bson:"nickname"`
			int Headindex;//`bson:"headindex"`
			int32_t Permission;//`bson:"privilege"`
			time_point Registertime;//`bson:"registertime"`
			std::string Regip;//`bson:"regip"`
			time_point Lastlogintime;//`bson:"lastlogintime"`
			std::string Lastloginip;//`bson:"lastloginip"`
			int Activedays;//`bson:"activedays"`
			int Keeplogindays;//`bson:"keeplogindays"`
			int64_t Alladdscore;//`bson:"alladdscore"`
			int64_t Allsubscore;//`bson:"allsubscore"`
			int Addscoretimes;//`bson:"addscoretimes"`
			int Subscoretimes;//`bson:"subscoretimes"`
			int64_t Gamerevenue;//`bson:"gamerevenue"`
			int64_t WinLosescore;//`bson:"winorlosescore"`
			int64_t Score;//`bson:"score"`
			int Status;//`bson:"status"`
			int Onlinestatus;//`bson:"onlinestatus"`
			int Gender;//`bson:"gender"`
			int64_t Integralvalue;//`bson:"integralvalue"`
		};
	}

	void CreateGuestUser(int64_t seq, std::string const& account, model::GameUser& model, int64_t score = 0);
}

#endif