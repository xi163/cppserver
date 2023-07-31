#include "public/Inc.h"
#include "public/Response.h"
#include "register/Login_handler.h"
#include "public/MgoOperation.h"
#include "GateServList.h"

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

void DoLogin(LoginReq const& req, muduo::net::HttpResponse& rsp) {
	//0-游客 1-账号密码 2-手机号 3-第三方(微信/支付宝等) 4-邮箱
	switch (req.Type) {
	case 0: {
		if (req.Account.empty()) {
			//查询网关节点
			GateServList servList;
			GetGateServList(servList);
			if (servList.size() == 0) {
				response::json::err::Result(response::json::err::ErrGameGateNotExist, BOOST::Any(), rsp);
				return;
			}
			//生成userid
			int64_t userId = mgo::NewUserId(document{} << "seq" << 1 << finalize,
				document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize,
				document{} << "_id" << "userid" << finalize);
			if (userId <= 0) {
				response::json::err::Result(response::json::err::ErrCreateGameUser, BOOST::Any(), rsp);
				return;
			}
			_LOG_ERROR(">>>>>> userId = %d", userId);
			//创建并插入user表
			
			//token签名加密
			//更新redis account->uid
			//缓存token
			return;
		}
		int64_t userId = 0;
		auto result = FindOneAndUpdate(document{} << "seq" << 1 << finalize,
			document{} << "$inc" << open_document << "seq" << b_int64{ 1 } << close_document << finalize,
			document{} << "_id" << "userid" << finalize);
		if (!result) {
		}
		else {
			bsoncxx::document::view view = result->view();
			if (view["seq"]) {
				switch (view["seq"].type()) {
				case bsoncxx::type::k_int64:
					userId = view["seq"].get_int64();
					break;
				case bsoncxx::type::k_int32:
					userId = view["seq"].get_int32();
					break;
				}
			}
		}
		if (userId <= 0) {
			response::json::err::Result(response::json::err::ErrCreateGameUser, BOOST::Any(), rsp);
			return;
		}
		_LOG_DEBUG(">>> userId = %d", userId);
		//查询网关节点
		GateServList servList;
		GetGateServList(servList);
		if (servList.size() == 0) {
			response::json::err::Result(response::json::err::ErrGameGateNotExist, BOOST::Any(), rsp);
			return;
		}
		std::string account = "test_0";
		userId = 10120;
		//token签名加密
		std::string token = utils::sign::Encode(LoginRsp(account, userId, &servList), 500000, descode);
		//更新redis account->uid
		REDISCLIENT.SetAccountUid(account, userId);
		//缓存token
		REDISCLIENT.SetToken(token, userId, account);
		response::json::OkMsg("登陆成功", Token(token), rsp);
		return;
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
}

void Login(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
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
		if (sType.find(ContentType_Text) != string::npos){

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
				response::json::BadRequest(rsp);
				return;
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
				response::json::BadRequest(rsp);
				return;
			}
			LoginReq req;
			req.Account = account;
			req.Type = lType;
			req.Timestamp = timestamp;
			DoLogin(req, rsp);
			return;
		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		break;
	}
	}
	response::xml:: Test(req, rsp);
}