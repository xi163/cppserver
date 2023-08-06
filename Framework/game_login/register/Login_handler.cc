#include "public/Inc.h"
#include "public/response.h"
#include "register/Login_handler.h"
#include "public/mgoOperation.h"
#include "public/mgoModel.h"
#include "public/redisKeys.h"
#include "GateServList.h"
#include "public/errorCode.h"

std::string md5code = "334270F58E3E9DEC";
std::string descode = "111362EE140F157D";

struct LoginReq {
	std::string Account;
	std::string	Passwd;
	std::string	Phone;
	std::string	Email;
	int	Type;
	int64_t	Timestamp;
};

struct LoginRsp :
	public BOOST::Any {
	LoginRsp(
		std::string const& Account,
		int64_t userid,
		BOOST::Any* data)
		: Account(Account)
		, Userid(userid), data(data) {
	}
	void bind(BOOST::Json& obj) {
		obj.put("account", Account);
		obj.put("userid", Userid);
		obj.put("data", *data);
	}
	std::string Account;
	int64_t	Userid;
	BOOST::Any* data;
};

struct Token :
	public BOOST::Any {
	Token(std::string const& token)
		: token(token) {
	}
	void bind(BOOST::Json& obj) {
		obj.put("token", token);
	}
	std::string token;
};

int doLogin(LoginReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime) {
	//0-游客 1-账号密码 2-手机号 3-第三方(微信/支付宝等) 4-邮箱
	switch (req.Type) {
	case 0: {
		if (req.Account.empty()) {
			//查询网关节点
			GateServList servList;
			GetGateServList(servList);
			if (servList.size() == 0) {
				return response::json::Result(ERR_GameGateNotExist, BOOST::Any(), rsp);
			}
			//生成userid
			int64_t userId = mgo::NewUserId(
				document{} << "seq" << 1 << finalize,
				document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize,
				document{} << "_id" << "userid" << finalize);
			if (userId <= 0) {
				return response::json::Result(ERR_CreateGameUser, BOOST::Any(), rsp);
			}
			//创建并插入user表
			mgo::model::GameUser model;
			mgo::CreateGuestUser(userId, req.Account, model);
			model.Regip = conn->peerAddress().toIp();
			model.Lastloginip = conn->peerAddress().toIp();
			std::string insert_id = mgo::AddUser(
				make_document(
					kvp("userid", b_int64{ userId }),
					kvp("account", model.Account),
					kvp("agentid", model.AgentId),
					kvp("linecode", model.Linecode),
					kvp("nickname", model.Nickname),
					kvp("headindex", model.Headindex),
					kvp("registertime", b_date{ model.Registertime }),
					kvp("regip", model.Regip),
					kvp("lastlogintime", b_date{ model.Lastlogintime }),
					kvp("lastloginip", model.Lastloginip),
					kvp("activedays", model.Activedays),
					kvp("keeplogindays", model.Keeplogindays),
					kvp("alladdscore", b_int64{ model.Alladdscore }),
					kvp("allsubscore", b_int64{ model.Allsubscore }),
					kvp("addscoretimes", model.Addscoretimes),
					kvp("subscoretimes", model.Subscoretimes),
					kvp("gamerevenue", b_int64{ model.Gamerevenue }),
					kvp("winorlosescore", b_int64{ model.WinLosescore }),
					kvp("score", b_int64{ model.Score }),
					kvp("status", model.Status),
					kvp("onlinestatus", model.Onlinestatus),
					kvp("gender", model.Gender),
					kvp("integralvalue", b_int64{ model.Integralvalue })
				).view());
			if (insert_id.empty()) {
				return response::json::Result(ERR_CreateGameUser, BOOST::Any(), rsp);
			}
			_LOG_ERROR(">>>>>> insert_id = %s", insert_id.c_str());
			//token签名加密
			std::string token = utils::sign::Encode(LoginRsp(model.Account, model.UserId, &servList), redisKeys::Expire_Token, descode);
			//更新redis account->uid
			REDISCLIENT.SetAccountUid(model.Account, userId);
			//缓存token
			REDISCLIENT.SetTokenInfo(token, userId, model.Account);
			//return response::json::OkMsg("登陆成功", Token(token), rsp);
			return response::json::Result(ERR_Ok, Token(token), rsp);
		}
		//先查redis
		int64_t userId = 0;
		REDISCLIENT.GetAccountUid(req.Account, userId);
		if (userId <= 0) {
			//再查mongo
			userId = mgo::GetUserId(
				document{} << "userid" << 1 << finalize,
				document{} << "account" << req.Account << finalize);
			if (userId <= 0) {
				//查询网关节点
				GateServList servList;
				GetGateServList(servList);
				if (servList.size() == 0) {
					return response::json::Result(ERR_GameGateNotExist, BOOST::Any(), rsp);
				}
				//生成userid
				int64_t userId = mgo::NewUserId(
					document{} << "seq" << 1 << finalize,
					document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize,
					document{} << "_id" << "userid" << finalize);
				if (userId <= 0) {
					return response::json::Result(ERR_CreateGameUser, BOOST::Any(), rsp);
				}
				//创建并插入user表
				mgo::model::GameUser model;
				mgo::CreateGuestUser(userId, req.Account, model);
				model.Regip = conn->peerAddress().toIp();
				model.Lastloginip = conn->peerAddress().toIp();
				std::string insert_id = mgo::AddUser(
					make_document(
						kvp("userid", b_int64{ userId }),
						kvp("account", model.Account),
						kvp("agentid", model.AgentId),
						kvp("linecode", model.Linecode),
						kvp("nickname", model.Nickname),
						kvp("headindex", model.Headindex),
						kvp("registertime", b_date{ model.Registertime }),
						kvp("regip", model.Regip),
						kvp("lastlogintime", b_date{ model.Lastlogintime }),
						kvp("lastloginip", model.Lastloginip),
						kvp("activedays", model.Activedays),
						kvp("keeplogindays", model.Keeplogindays),
						kvp("alladdscore", b_int64{ model.Alladdscore }),
						kvp("allsubscore", b_int64{ model.Allsubscore }),
						kvp("addscoretimes", model.Addscoretimes),
						kvp("subscoretimes", model.Subscoretimes),
						kvp("gamerevenue", b_int64{ model.Gamerevenue }),
						kvp("winorlosescore", b_int64{ model.WinLosescore }),
						kvp("score", b_int64{ model.Score }),
						kvp("status", model.Status),
						kvp("onlinestatus", model.Onlinestatus),
						kvp("gender", model.Gender),
						kvp("integralvalue", b_int64{ model.Integralvalue })
					).view());
				if (insert_id.empty()) {
					return response::json::Result(ERR_CreateGameUser, BOOST::Any(), rsp);
				}
				_LOG_ERROR(">>>>>> insert_id = %s", insert_id.c_str());
				//token签名加密
				std::string token = utils::sign::Encode(LoginRsp(model.Account, userId, &servList), redisKeys::Expire_Token, descode);
				//更新redis account->uid
				REDISCLIENT.SetAccountUid(model.Account, userId);
				//缓存token
				REDISCLIENT.SetTokenInfo(token, userId, model.Account);
				//return response::json::OkMsg("登陆成功", Token(token), rsp);
				return response::json::Result(ERR_Ok, Token(token), rsp);
			}
			//查询mongo命中
			else {
				//查询网关节点
				GateServList servList;
				GetGateServList(servList);
				if (servList.size() == 0) {
					return response::json::Result(ERR_GameGateNotExist, BOOST::Any(), rsp);
				}
				//token签名加密
				std::string token = utils::sign::Encode(LoginRsp(req.Account, userId, &servList), redisKeys::Expire_Token, descode);
				//更新redis account->uid
				REDISCLIENT.SetAccountUid(req.Account, userId);
				//缓存token
				REDISCLIENT.SetTokenInfo(token, userId, req.Account);
				//return response::json::OkMsg("登陆成功", Token(token), rsp);
				return response::json::Result(ERR_Ok, Token(token), rsp);
			}
		}
		//查询redis命中
		else {
			//查询网关节点
			GateServList servList;
			GetGateServList(servList);
			if (servList.size() == 0) {
				return response::json::Result(ERR_GameGateNotExist, BOOST::Any(), rsp);
			}
			//token签名加密
			std::string token = utils::sign::Encode(LoginRsp(req.Account, userId, &servList), redisKeys::Expire_Token, descode);
			//更新redis account->uid
			REDISCLIENT.ResetExpiredAccountUid(req.Account);
			//缓存token
			REDISCLIENT.SetTokenInfo(token, userId, req.Account);
			//return response::json::OkMsg("登陆成功", Token(token), rsp);
			return response::json::Result(ERR_Ok, Token(token), rsp);
		}
	}
	case 1: {
		break;
	}
	case 2: {
		break;
	}
	case 3: {
		break;
	}
	default:
		break;
	}
	return ErrorCode::kFailed;
}

int Login(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	switch (req.method()) {
	case muduo::net::HttpRequest::kGet: {
		std::string sType = req.getHeader(ContentType);
		if (sType.find(ContentType_Text) != string::npos) {
		}
		else if (sType.find(ContentType_Json) != string::npos) {

		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		break;
	}
	case muduo::net::HttpRequest::kPost: {
		std::string sType = req.getHeader(ContentType);
		if (sType.find(ContentType_Text) != string::npos) {

		}
		else if (sType.find(ContentType_Json) != string::npos) {
			boost::property_tree::ptree pt;
			{
				std::string s(buf->peek(), buf->readableBytes());
				std::stringstream ss(s);
				boost::property_tree::read_json(ss, pt);
			}
			std::string key = pt.get<std::string>("key");
			std::string param = pt.get<std::string>("param");
			//param = utils::HTML::Decode(param);
			std::string decrypt;
			for (int c = 1; c < 3; ++c) {
				param = utils::URL::Decode(param);
#if 1
				boost::replace_all<std::string>(param, "\r\n", "");
				boost::replace_all<std::string>(param, "\r", "");
				boost::replace_all<std::string>(param, "\n", "");
#else
				param = boost::regex_replace(param, boost::regex("\r\n|\r|\n"), "");
#endif
				std::string const& strURL = param;
				decrypt = Crypto::AES_ECBDecrypt(strURL, descode);
				_LOG_DEBUG("ECBDecrypt[%d] >>> md5code[%s] descode[%s] [%s]", c, md5code.c_str(), descode.c_str(), decrypt.c_str());
				if (!decrypt.empty()) {
					break;
				}
			}
			if (decrypt.empty()) {
				return response::json::Result(ERR_Decrypt, BOOST::Any(), rsp);
			}
			pt.clear();
			{
				std::stringstream ss(decrypt);
				boost::property_tree::read_json(ss, pt);
			}
			std::string account = pt.get<std::string>("account");
			int lType = pt.get<int>("type");
			int timestamp = pt.get<int64_t>("timestamp");
			std::string src = account + std::to_string(lType) + std::to_string(timestamp) + md5code;
			char md5[32 + 1] = { 0 };
			utils::MD5(src.c_str(), src.length(), md5, 1);
			if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
				return response::json::Result(ERR_CheckMd5, BOOST::Any(), rsp);
			}
			LoginReq req;
			req.Account = account;
			req.Type = lType;
			req.Timestamp = timestamp;
			return doLogin(req, rsp, conn, receiveTime);
		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		break;
	}
	}
	response::xml::Test(req, rsp);
	return ErrorCode::kFailed;
}