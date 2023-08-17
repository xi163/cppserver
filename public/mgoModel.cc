#include "mgoModel.h"
#include "public/gameConst.h"

namespace mgo {
	
	void CreateGuestUser(int64_t seq , std::string const& account, model::GameUser& model) {
		model.UserId = seq;
		if (account.empty()) {
			model.Account = "guest_" + std::to_string(model.UserId);
		}
		else {
			model.Account = account;
		}
		model.AgentId = 10001;
		model.Linecode = "10001_1";
		model.Nickname = model.Account;
		model.Headindex = 0;
		model.Registertime = std::chrono::system_clock::now();
		model.Lastlogintime = std::chrono::system_clock::now();
		model.Activedays = 0;
		model.Keeplogindays = 0;
		model.Alladdscore = 0;
		model.Allsubscore = 0;
		model.Addscoretimes = 0;
		model.Subscoretimes = 0;
		model.Gamerevenue = 0;
		model.WinLosescore = 0;
		model.Score = 0;
		model.Status = 1;//¶³½á <= 0
		model.Onlinestatus = 0;
		model.Gender = 0;
		model.Integralvalue = 0;
		model.Permission = Authorization::kGuest;
	}
}