#include "public/Inc.h"
#include "public/response.h"
#include "handler/router_handler.h"
#include "public/mgoOperation.h"
#include "public/mgoModel.h"
#include "public/redisKeys.h"
#include "ServList.h"
#include "public/errorCode.h"

std::string md5code = "334270F58E3E9DEC";
std::string descode = "111362EE140F157D";

struct RouterReq {
	int	Type;
	std::string Node;
	int64_t	Timestamp;
};

int doRouter(RouterReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime) {
	if (req.Node == "game_login") {
		ServList servList;
		GetServList(servList, rpc::containTy::kRpcLoginTy, req.Type);
		if (servList.size() == 0) {
			return response::json::Result(ERR_GameLoginNotExist, rsp);
		}
		BOOST::Json json;
		json.put("type", req.Type);
		json.put("node", req.Node);
		json.put("data", servList);
		std::string token = utils::sign::Encode(json, redisKeys::Expire_Token, descode);
		json.clear();
		json.put("token", token);
		return response::json::OkMsg("获取登陆节点成功", rsp, json);
	}
	if (req.Node == "game_api") {
		//查询登陆服节点
		ServList servList;
		GetServList(servList, rpc::containTy::kRpcApiTy, req.Type);
		if (servList.size() == 0) {
			return response::json::Result(ERR_GameApiNotExist, rsp);
		}
		BOOST::Json json;
		json.put("type", req.Type);
		json.put("node", req.Node);
		json.put("data", servList);
		std::string token = utils::sign::Encode(json, redisKeys::Expire_Token, descode);
		json.clear();
		json.put("token", token);
		return response::json::OkMsg("获取API节点成功", rsp, json);
	}
	return response::json::Result(ERR_GameHandleParamsError, rsp);
}

int Router(
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
			try {
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
					return response::json::Result(ERR_Decrypt, rsp);
				}
				pt.clear();
				{
					std::stringstream ss(decrypt);
					boost::property_tree::read_json(ss, pt);
				}
				int lType = pt.get<int>("type"); //0-websocket 1-http
				std::string node = pt.get<std::string>("node");//game_login， game_api
				int timestamp = pt.get<int64_t>("timestamp");
				std::string src = node + std::to_string(lType) + std::to_string(timestamp) + md5code;
				char md5[32 + 1] = { 0 };
				utils::MD5(src.c_str(), src.length(), md5, 1);
				if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
					return response::json::Result(ERR_CheckMd5, rsp);
				}
				if (lType != 0 && lType != 1) {
					return response::json::Result(ERR_GameHandleOperationTypeError, rsp);
				}
				RouterReq req;
				req.Node = node;
				req.Type = lType;
				req.Timestamp = timestamp;
				return doRouter(req, rsp, conn, receiveTime);
			}
			catch (boost::property_tree::ptree_error& e) {
				_LOG_ERROR(e.what());
				return response::json::Result(ERR_GameHandleParamsError, rsp);
			}
		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		break;
	}
	}
	response::xml::Test(req, rsp);
	return kFailed;
}