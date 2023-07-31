#include "public/Inc.h"
#include "register/Login_handler.h"

#include "RpcClients.h"
#include "RpcContainer.h"

#include "RpcService.h"

#include "../Login.h"
#include "public/Response.h"

std::string md5code = "334270F58E3E9DEC";
std::string descode = "111362EE140F157D";

extern LoginServ* gServer;

namespace sign {
	struct Sign {
		std::string rand;
		int64_t timestamp;
		int64_t expired;
		BOOST::Any* data;
	};
	std::string Encode(BOOST::Any const& data, int64_t exp, std::string const& secret) {
		Sign token;
		MY_TRY()
		token.rand = utils::random::charStr(RANDOM().betweenInt(10, 20).randInt_mt());
		MY_CATCH()
		token.timestamp = (int64_t)time(NULL);
		token.expired = exp;
		BOOST::Json obj;
		obj.put("rand", token.rand);
		obj.put("timestamp", token.timestamp);
		obj.put("expired", token.expired);
		obj.put("data", data);
		std::string json = obj.to_json();
		_LOG_ERROR("\n%s", json.c_str());
		std::string strBase64 = Crypto::AES_ECBEncrypt(json, secret);
		//std::string strBase64 = Crypto::Des_Encrypt(json, (char *)secret.c_str());
		//return utils::URL::Encode(strBase64);
		return strBase64;
	}
}

struct LoginReq {
	std::string Account;
	std::string	Passwd;
	std::string	Phone;
	std::string	Email;
	int	Type;
	int64_t	Timestamp;
};

struct LoginRsp :public BOOST::Any {
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

struct ServerLoad :public BOOST::Any {
	ServerLoad(
		std::string const& Host,
		std::string const& Domain,
		int NumOfLoads)
		: Host(Host)
		, Domain(Domain)
		, NumOfLoads(NumOfLoads) {
	}
	void bind(BOOST::Json& obj, int i) {
		obj.put("host", Host);
		obj.put("domain", Domain);
		obj.put("numOfLoads", NumOfLoads, i);
	}
	std::string Host;
	std::string Domain;
	int NumOfLoads;
};

struct ServerLoadList :public std::vector<ServerLoad>, public BOOST::Any {
	void bind(BOOST::Json& obj) {
		for (iterator it = begin();
			it != end(); ++it) {
			obj.push_back(*it);
		}
	}
};

struct Token :public BOOST::Any {
	Token(std::string const& token)
		: token(token) {
	}
	void bind(BOOST::Json& obj) {
		obj.put("token", token);
	}
	std::string token;
};

void GetGateServerList(ServerLoadList& servList) {
	rpc::ClientConnList clients;
	gServer->rpcClients_[rpc::servTyE::kRpcGateTy].clients_->getAll(clients);
	for (rpc::ClientConnList::iterator it = clients.begin();
		it != clients.end(); ++it) {
		rpc::client::GameGate client(*it, 3);
		::ProxyServer::Message::GameGateReq req;
		::ProxyServer::Message::GameGateRspPtr rsp = client.GetGameGate(req);
		if (rsp) {
			servList.emplace_back(ServerLoad(rsp->host(),rsp->domain(),rsp->numofloads()));
		}
	}
}

void DoLogin(LoginReq const& req, muduo::net::HttpResponse& rsp) {
	//0-游客 1-账号密码 2-手机号 3-第三方(微信/支付宝等) 4-邮箱
	switch (req.Type) {
	case 0: {
		if (req.Account.empty()) {
			//查询网关节点
			ServerLoadList servList;
			GetGateServerList(servList);
			if (servList.size() == 0) {
				response::json::err::Result(response::json::err::ErrGameGateNotExist, BOOST::Any(), rsp);
				return;
			}
			//生成userid
			//创建并插入user表
			//token签名加密
			//更新redis account->uid
			//缓存token
			return;
		}
		//查询网关节点
		ServerLoadList servList;
		GetGateServerList(servList);
		if (servList.size() == 0) {
			response::json::err::Result(response::json::err::ErrGameGateNotExist, BOOST::Any(), rsp);
			return;
		}
		time_t now = time(NULL);
		int64_t exp = (int64_t)now + 500000;
		std::string account = "test_0";
		int64_t userId = 10120;
		//token签名加密
		std::string token = sign::Encode(LoginRsp(account,userId,&servList), exp, descode);
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