#include "public/Inc.h"
#include "public/response.h"
#include "handler/order_handler.h"
#include "public/mgoOperation.h"
#include "public/mgoKeys.h"
#include "public/mgoModel.h"
#include "public/redisKeys.h"
#include "GateServList.h"
#include "public/errorCode.h"
#include "../Api.h"

extern ApiServ* gServer;

int doOrder(OrderReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime) {
	switch (req.Type) {
	case eApiType::OpAddScore: {
		_LOG_DEBUG("上分REQ orderId[%s] account[%s] agentId[%d] deltascore[%f] deltascoreI64[%lld]",
			req.orderId.c_str(), req.Account.c_str(), req.p_agent_info->agentId, req.Score, req.scoreI64);
		return addScore(req, rsp, conn, receiveTime);
	}
	case eApiType::OpSubScore:{
		_LOG_WARN("下分REQ orderId[%s] account[%s] agentId[%d] deltascore[%f] deltascoreI64[%lld]",
			req.orderId.c_str(), req.Account.c_str(), req.p_agent_info->agentId, req.Score, req.scoreI64);
		return subScore(req, rsp, conn, receiveTime);
	}
	default:
		return ErrorCode::kFailed;
	}
}

int Order(
	const muduo::net::HttpRequest& req,
	muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	switch (req.method()) {
	case muduo::net::HttpRequest::kGet: {
		bool isdecrypt_ = false;
		int opType = 0;
		int agentId = 0;
		double score = 0;
		int64_t scoreI64 = 0;
		std::string account;
		std::string orderId;
		std::string timestamp;
		std::string key, param;
		agent_info_t *p_agent_info = NULL;
#ifdef _NO_LOGIC_PROCESS_
		int64_t userId = 0;
#endif
		std::string sType = req.getHeader(ContentType);
		if (sType.find(ContentType_Text) != string::npos) {
		}
		else if (sType.find(ContentType_Json) != string::npos) {

		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		HttpParams params;
		if (!utils::parseQuery(req.query(), params)) {
			return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
		if (!isdecrypt_) {
			//type
			HttpParams::const_iterator typeKey = params.find("type");
			if (typeKey == params.end() || typeKey->second.empty() ||
				(opType = atol(typeKey->second.c_str())) < 0) {
				return response::json::Result(ERR_GameHandleOperationTypeError, BOOST::Any(), rsp);
			}
			//2.上分 3.下分
			if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
				return response::json::Result(ERR_GameHandleOperationTypeError, BOOST::Any(), rsp);
			}
			//agentid
			HttpParams::const_iterator agentIdKey = params.find("agentid");
			if (agentIdKey == params.end() || agentIdKey->second.empty() ||
				(agentId = atol(agentIdKey->second.c_str())) <= 0) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
#ifdef _NO_LOGIC_PROCESS_
			//userid
			HttpParams::const_iterator userIdKey = params.find("userid");
			if (userIdKey == params.end() || userIdKey->second.empty() ||
				(userId = atoll(userIdKey->second.c_str())) <= 0) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
#endif
			//account
			HttpParams::const_iterator accountKey = params.find("account");
			if (accountKey == params.end() || accountKey->second.empty()) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = params.find("orderid");
			if (orderIdKey == params.end() || orderIdKey->second.empty()) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = params.find("score");
			if (scoreKey == params.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				(scoreI64 = utils::rate100(scoreKey->second)) <= 0) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}

			// 获取当前代理数据
			{
				READ_LOCK(gServer->agent_info_mutex_);
				std::map<int32_t, agent_info_t>::iterator it = gServer->agent_info_.find(agentId);
				if (it == gServer->agent_info_.end()) {
					// 代理ID不存在或代理已停用
					return response::json::Result(ERR_GameHandleProxyIDError, BOOST::Any(), rsp);
				}
				else {
					p_agent_info = &it->second;
				}
			}
			// 没有找到代理，判断代理的禁用状态(0正常 1停用)
			if (p_agent_info->status == 1) {
				// 代理ID不存在或代理已停用
				return response::json::Result(ERR_GameHandleProxyIDError, BOOST::Any(), rsp);
			}
#ifndef _NO_LOGIC_PROCESS_
			OrderReq orderReq;
			orderReq.Type = opType;
			orderReq.Account = account;
			orderReq.Score = score * 100;
			orderReq.scoreI64 = scoreI64;
			orderReq.orderId = orderId;
			orderReq.p_agent_info = p_agent_info;
			return doOrder(orderReq, rsp, conn, receiveTime);
#endif
		}
		//type
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty() ||
			(opType = atol(typeKey->second.c_str())) < 0) {
			return response::json::Result(ERR_GameHandleOperationTypeError, BOOST::Any(), rsp);
		}
		//2.上分 3.下分
		if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
			return response::json::Result(ERR_GameHandleOperationTypeError, BOOST::Any(), rsp);
		}
		//agentid
		HttpParams::const_iterator agentIdKey = params.find("agentid");
		if (agentIdKey == params.end() || agentIdKey->second.empty() ||
			(agentId = atol(agentIdKey->second.c_str())) <= 0) {
			return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
		//timestamp
		HttpParams::const_iterator timestampKey = params.find("timestamp");
		if (timestampKey == params.end() || timestampKey->second.empty() ||
			atol(timestampKey->second.c_str()) <= 0) {
			return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
		else {
			timestamp = timestampKey->second;
		}
		//param
		HttpParams::const_iterator paramValueKey = params.find("param");
		if (paramValueKey == params.end() || paramValueKey->second.empty()) {
			return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
		else {
			param = paramValueKey->second;
		}
		//key
		HttpParams::const_iterator keyKey = params.find("key");
		if (keyKey == params.end() || keyKey->second.empty()) {
			return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
		}
		else {
			key = keyKey->second;
		}
		// 获取当前代理数据
		{
			READ_LOCK(gServer->agent_info_mutex_);
			std::map<int32_t, agent_info_t>::iterator it = gServer->agent_info_.find(agentId);
			if (it == gServer->agent_info_.end()) {
				// 代理ID不存在或代理已停用
				return response::json::Result(ERR_GameHandleProxyIDError, BOOST::Any(), rsp);
			}
			else {
				p_agent_info = &it->second;
			}
		}
		// 没有找到代理，判断代理的禁用状态(0正常 1停用)
		if (p_agent_info->status == 1) {
			// 代理ID不存在或代理已停用
			return response::json::Result(ERR_GameHandleProxyIDError, BOOST::Any(), rsp);
		}
#if 0
		agentId = 10000;
		p_agent_info->md5code = "334270F58E3E9DEC";
		p_agent_info->descode = "111362EE140F157D";
		timestamp = "1579599778583";
		param = "0RJ5GGzw2hLO8AsVvwORE2v16oE%2fXSjaK78A98ct5ajN7reFMf9YnI6vEnpttYHK%2fp04Hq%2fshp28jc4EIN0aAFeo4pni5jxFA9vg%2bLOx%2fek%3d";
		key = "C6656A2145BAEF7ED6D38B9AF2E35442";
#elif 0
		agentId = 111169;
		p_agent_info->md5code = "8B56598D6FB32329";
		p_agent_info->descode = "D470FD336AAB17E4";
		timestamp = "1580776071271";
		param = "h2W2jwWIVFQTZxqealorCpSfOwlgezD8nHScU93UQ8g%2FDH1UnoktBusYRXsokDs8NAPFEG8WdpSe%0AY5rtksD0jw%3D%3D";
		key = "a7634b1e9f762cd4b0d256744ace65f0";
#elif 0
		agentId = 111190;
		timestamp = "1583446283986";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		param = "KDtKjjnaaxKWNuK%252BBRwv9f2gBxLkSvY%252FqT4HBaZY2IrxqGMK3DYlWOW4dHgiMZV8Uu66h%252BHjH8MfAfpQIE5eIHoRZMplj7dljS7Tfyf3%252BlM%253D";
		key = "4F6F53FDC4D27EC33B3637A656DD7C9F";
#elif 0
		agentId = 111149;
		timestamp = "1583448714906";
		p_agent_info->md5code = "7196FD6921347DB1";
		p_agent_info->descode = "A5F8C07B7843C73E";
		param = "nu%252FtdBhN6daQdad3aOVOgzUr6bHVMYNEpWE4yLdHkKRn%252F%252Fn1V3jIFR%252BI7wawXWNyjR3%252FW0M9qzcdzM8rNx8xwe%252BEW9%252BM92ZN4hlpUAhFH74%253D";
		key = "9EEC240FAFAD3E5B6AB6B2DDBCDFE1FF";
#elif 0
		agentId = 10033;
		timestamp = "1583543899005";
		p_agent_info->md5code = "5998F124C180A817";
		p_agent_info->descode = "38807549DEA3178D";
		param = "9303qk%2FizHRAszhN33eJxG2aOLA2Wq61s9f96uxDe%2Btczf2qSG8O%2FePyIYhVAaeek9m43u7awgra%0D%0Au8a8b%2FDchcZSosz9SVfPjXdc7h4Vma2dA8FHYZ5dJTcxWY7oDLlSOHKVXYHFMIWeafVwCN%2FU5fzv%0D%0AHWyb1Oa%2FWJ%2Bnfx7QXy8%3D";
		key = "2a6b55cf8df0cd8824c1c7f4308fd2e4";
#elif 0
		agentId = 111190;
		timestamp = "1583543899005";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		param = "%252FPxIlQ9UaP6WljAYhfYZpBO6Hz2KTrxGN%252Bmdffv9sZpaii2avwhJn3APpIOozbD7T3%252BGE1rh5q4OdfrRokriWNEhlweRKzC6%252FtABz58Kdzo%253D";
		key = "9F1E44E8B61335CCFE2E17EE3DB7C05A";
#elif 1
		//p_agent_info->descode = "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw";
		//param = "7q9Mu4JezLGpXaJcaxBUEmnQgrH%2BO0Wi%2F85z30vF%2FIdphHU13EMp2f%2FE5%2BfHtIXYFLbyNqnwDx8SyGP1cSYssZN6gniqouFeB3kBcwSXYZbFYhNBU6162n6BaFYFVbH6KMc43RRvjBtmbkMgCr5MlRz0Co%2BQEsX9Pt3zFJiXnm12oEdLeFBSSVIcDsqd3ize9do1pbxAm9Bb45KRvPhYvA%3D%3D";
#elif 0
		agentId = 111208;
		timestamp = "1585526277991";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		param = "zLcIc13jvzFHqywSHCRWX7JpaXddpWpMzaHBu8necMyCU3L9NXaZpcDXqmI4NgXvuOc6FGa80GUj%250AXOI8uoQuyjm1MLYIbrFVz09z68uSREs%253D";
		key = "59063f588d6787eff28d3";
#elif 0
		agentId = 111208;
		timestamp = "1585612589828";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		param = "KfNY6Jl8k%252BBnBJLE2KQlSefbpNujwXrVcTWvRm2rfOrxie4Sd65DgAsIPIQm0uPpGS2dAQRk1HEE%250AulhnCZ0OteZiMh060xxH%252FzTzPC8DUr0%253D";
		key = "be64cc7589bebf944fcd322f9923eca4";
#elif 0
		agentId = 111209;
		timestamp = "1585639941293";
		p_agent_info->md5code = "9074653AA2D0B02F";
		p_agent_info->descode = "A78AC14440288D74";
		param = "YF%252BIk5Dk2nEyNUE1TUErZY1d9RaaVmU46cwE41HRJyiqwgqu%252BOBwJT9TfPTNBtxIFjBkOgoYGljg%250AtPMbgp%252BLZz995NkM3iHrdMYp5dwv6kE%253D";
		key = "06aad6a911a7e6ce2d3b2ad18c068ae6";
#elif 0
		agentId = 111208;
		timestamp = "1585681742640";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		param = "LLgiWFvdQicKfSEsDA8lTkD2FUOOcQR0LnVwDNiGjdlqgK9i9b058FlOL1DJuEp9%252BPEi37YUyTIX%250AVT7bA2H6gTbhwb4mRLzzmIWd6l3KdBM%253D";
		key = "a890129e6b9e94f29";
#elif 0
		agentId = 111208;
		timestamp = "1585743294759";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		param = "5dTmQiGn0iJHUIAEMDfjxtbTpuWWIZd0HmdlFKKF4HpqgK9i9b058FlOL1DJuEp9J%252FnZJqtPOv%252F7%250A6ikd4T%252FcwEJkkVFV6TQbCk3yHamOx3s%253D";
		key = "934fa90d6";
#elif 0
		agentId = 111208;
		timestamp = "1585766704760";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		param = "DmqMRX48r66cW8Oiw0xMhgLcBuKP94YHtoQNGhAvupjxie4Sd65DgAsIPIQm0uPp%252BR6ZDGMf1B3T%250AV4owq2ro6B9Ru1XHueMJMNVLhLTaY0M%253D";
		key = "bd3778912439c03a35de1a3ce3905ef0";
#endif
		std::string decrypt;
		{
			//根据代理商信息中存储的md5密码，结合传输参数中的agentid和timestamp，生成判定标识key
			std::string src = std::to_string(agentId) + timestamp + p_agent_info->md5code;
			char md5[32 + 1] = { 0 };
			utils::MD5(src.c_str(), src.length(), md5, 1);
			if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
				return response::json::Result(ERR_GameHandleProxyMD5CodeError, BOOST::Any(), rsp);
			}
			//param = utils::HTML::Decode(param);
			//_LOG_DEBUG("HTML::Decode >>> %s", param.c_str());
			for (int c = 1; c < 3; ++c) {
				param = utils::URL::Decode(param);
#if 1
				boost::replace_all<std::string>(param, "\r\n", "");
				boost::replace_all<std::string>(param, "\r", "");
				boost::replace_all<std::string>(param, "\n", "");
#else
				param = boost::regex_replace(param, boost::regex("\r\n|\r|\n"), "");
#endif
				//_LOG_DEBUG("URL::Decode[%d] >>> %s", c, param.c_str());
				std::string const& strURL = param;
				decrypt = Crypto::AES_ECBDecrypt(strURL, p_agent_info->descode);
				_LOG_DEBUG("ECBDecrypt[%d] >>> md5code[%s] descode[%s] [%s]", c, p_agent_info->md5code.c_str(), p_agent_info->descode.c_str(), decrypt.c_str());
				if (!decrypt.empty()) {
					break;
				}
			}
			if (decrypt.empty()) {
				// 参数转码或代理解密校验码错误
				return response::json::Result(ERR_GameHandleProxyDESCodeError, BOOST::Any(), rsp);
			}
		}
		{
			HttpParams decryptParams;
			_LOG_DEBUG(decrypt.c_str());
			if (!utils::parseQuery(decrypt, decryptParams)) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
			//agentid
			//HttpParams::const_iterator agentIdKey = decryptParams.find("agentid");
			//if (agentIdKey == decryptParams.end() || agentIdKey->second.empty()) {
			//	break;
			//}
#ifdef _NO_LOGIC_PROCESS_
				//userid
			HttpParams::const_iterator userIdKey = decryptParams.find("userid");
			if (userIdKey == decryptParams.end() || userIdKey->second.empty() ||
				(userId = atoll(userIdKey->second.c_str())) <= 0) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
#endif
			//account
			HttpParams::const_iterator accountKey = decryptParams.find("account");
			if (accountKey == decryptParams.end() || accountKey->second.empty()) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = decryptParams.find("orderid");
			if (orderIdKey == decryptParams.end() || orderIdKey->second.empty()) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = decryptParams.find("score");
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				(scoreI64 = utils::rate100(scoreKey->second)) <= 0) {
				return response::json::Result(ERR_GameHandleParamsError, BOOST::Any(), rsp);
			}
		}
#ifndef _NO_LOGIC_PROCESS_
		OrderReq orderReq;
		orderReq.Type = opType;
		orderReq.Account = account;
		orderReq.Score = score * 100;
		orderReq.scoreI64 = scoreI64;
		orderReq.orderId = orderId;
		orderReq.p_agent_info = p_agent_info;
		return doOrder(orderReq, rsp, conn, receiveTime);
#endif
	}
	case muduo::net::HttpRequest::kPost: {
		std::string sType = req.getHeader(ContentType);
		if (sType.find(ContentType_Text) != string::npos) {

		}
		else if (sType.find(ContentType_Json) != string::npos) {
		}
		else if (sType.find(ContentType_Xml) != string::npos) {

		}
		break;
	}
	}
	response::xml::Test(req, rsp);
	return ErrorCode::kFailed;
}