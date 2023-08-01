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
				Registertime = 0;
				Lastlogintime = 0;
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
			std::string ID; //           primitive.ObjectID `bson:"_id,omitempty"`
			int64_t UserId;          //   `bson:"userid"`
			std::string Account;        //   `bson:"account"`
			int AgentId;//       int      // `bson:"agentid"`
			std::string Linecode;//      string    //`bson:"linecode"`
			std::string 	Nickname;//      string    //`bson:"nickname"`
			int 	Headindex;//     int       //`bson:"headindex"`
			time_t	Registertime;//  time.Time //`bson:"registertime"`
			std::string 	Regip;//        string    //`bson:"regip"`
			time_t	Lastlogintime;// time.Time //`bson:"lastlogintime"`
			std::string 	Lastloginip;// string    //`bson:"lastloginip"`
			int 	Activedays;// int       //`bson:"activedays"`
			int 	Keeplogindays;// int       //`bson:"keeplogindays"`
			int64_t 	Alladdscore;//int64     //`bson:"alladdscore"`
			int64_t 	Allsubscore;// int64     //`bson:"allsubscore"`
			int 	Addscoretimes;// int       //`bson:"addscoretimes"`
			int 	Subscoretimes;//int       //`bson:"subscoretimes"`
			int64_t Gamerevenue;//int64     //`bson:"gamerevenue"`
			int64_t WinLosescore;// int64     //`bson:"winorlosescore"`
			int64_t Score;//int64     //`bson:"score"`
			int 	Status;//      int      // `bson:"status"`
			int 	Onlinestatus;//  int      // `bson:"onlinestatus"`
			int 	Gender;//int      // `bson:"gender"`
			int64_t Integralvalue;//int64    // `bson:"integralvalue"`
		};
	}
	
	void CreateGuestUser(int64_t seq, std::string const& account, model::GameUser& model);
}

#endif