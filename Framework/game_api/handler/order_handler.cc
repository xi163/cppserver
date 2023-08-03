#include "public/Inc.h"
#include "public/Response.h"
#include "handler/order_handler.h"
#include "public/mgoOperation.h"
#include "public/mgoKeys.h"
#include "public/mgoModel.h"
#include "public/redisKeys.h"
#include "GateServList.h"
#include "public/ErrorCode.h"
#include "../Api.h"

extern ApiServ* gServer;

std::string md5code = "334270F58E3E9DEC";
std::string descode = "111362EE140F157D";

struct OrderReq {
	int	Type;
	int64_t userId;
	std::string Account;
	std::string orderId;
	int64_t	Timestamp;
	double Score;
	int64_t scoreI64;
	agent_info_t* p_agent_info;
	//boost::property_tree::ptree& latest;
	int* testTPS;
};

struct OrderRsp :
	public BOOST::Any {
	void bind(BOOST::Json& obj) {
	}
	int64_t	userId;
	BOOST::Any* data;
};

int doOrder(OrderReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime) {
	switch (req.Type) {
	case eApiType::OpAddScore: {
		_LOG_DEBUG("上分REQ orderId[%s] account[%s] agentId[%d] deltascore[%f] deltascoreI64[%lld]",
			req.orderId.c_str(), req.Account.c_str(), req.p_agent_info->agentId, req.Score, req.scoreI64);
		agent_user_t user;
		int64_t userId = 0;
		int64_t beforeScore = 0;
		int32_t onlinestatus = 0;
		std::string linecode;
		static __thread mongocxx::client_session session = MONGODBCLIENT.start_session();
		bool btransaction = false;
#ifdef _STAT_ORDER_QPS_DETAIL_
		//std::stringstream ss;
		muduo::Timestamp st_collect = muduo::Timestamp::now();
#endif
		try {
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_collect = muduo::Timestamp::now();
			//ss << "\n0.[mongo]getCollection elapsed " << muduo::timeDifference(et_collect, st_collect) << "s";
			createLatestElapsed(latest, "mongo.getCollection", "", muduo::timeDifference(et_collect, st_collect));
			muduo::Timestamp st_user_q = et_collect;
#endif
			if (!mgo::GetUserByAgent(
				document{} << "userid" << 1 << "score" << 1 << "onlinestatus" << 1 << "linecode" << 1 << finalize,
				document{} << "agentid" << b_int32{ req.p_agent_info->agentId } << "account" << req.Account << finalize, user)) {

#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_q = muduo::Timestamp::now();
				//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
				createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " query game_user agentid." << req.p_agent_info->agentId << ".account." << req.Account << " invalid";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleUserNotExistsError, ss.str().c_str(), BOOST::Any(), rsp);
			}
			userId = user.userId;
			beforeScore = user.score;
			onlinestatus = user.onlinestatus;
			linecode = user.linecode;
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_q = muduo::Timestamp::now();
			//ss << "\n1.[mongo]query game_user elapsed " << muduo::timeDifference(et_user_q, st_user_q) << "s";
			createLatestElapsed(latest, "mongo.query", "game_user", muduo::timeDifference(et_user_q, st_user_q));
			muduo::Timestamp st_redislock_q = et_user_q;
#endif
			//redis分布式加锁 ttl指定1s锁时长
			//1.避免玩家HTTP短连接重复Api请求情况 2.避免玩家同时多Api节点请求情况
			std::string strLockKey = "lock.uid:" + std::to_string(userId) + ".order";
			RedisLock::CGuardLock lock(REDISLOCK, strLockKey.c_str(), gServer->ttlUserLock_);
			if (!lock.IsLocked()) {
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " userid." << userId
					<< ".account." << req.Account << " redisLock failed";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataOutTime, BOOST::Any(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
			//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
			createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
			muduo::Timestamp st_addorder_q = et_redislock_q;
#endif
#if 0
			if (!mgo::ExistAddOrder(document{} << "orderid" << req.orderId << finalize)) {
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
				//ss << "\n3.[mongo]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
				createLatestElapsed(latest, "mongo.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " query add_score_order existed";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataOrderIDExists, BOOST::Any(), rsp);
			}
#else
			//redis查询注单orderid是否已经存在，mongodb[add_score_order]需要建立唯一约束 UNIQUE(orderid)
			if (REDISCLIENT.exists("s.order:" + req.orderId + ":add")) {
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
				//ss << "\n3.[redis]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
				createLatestElapsed(latest, "redis.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " query add_score_order existed";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataOrderIDExists, BOOST::Any(), rsp);
			}
#endif
			// 合作模式判断 1 买分 2 信用
			// 0，判断是否超过代理的库存分数，超过则退出
			if (req.p_agent_info->cooperationtype == (int)eCooType::buyScore && req.scoreI64 > req.p_agent_info->score) {
				std::stringstream ss;
				ss << "orderid." << req.orderId
					<< " account." << req.Account << ".score." << req.scoreI64 << " less than agentid." << req.p_agent_info->agentId
					<< ".score." << req.p_agent_info->score;
				_LOG_ERROR(ss.str().c_str());
				// 玩家上分超出代理现有总分值
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataOverScore, BOOST::Any(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_addorder_q = muduo::Timestamp::now();
			//ss << "\n3.[redis]query add_score_order elapsed " << muduo::timeDifference(et_addorder_q, st_addorder_q) << "s";
			createLatestElapsed(latest, "redis.query", "add_score_order", muduo::timeDifference(et_addorder_q, st_addorder_q));

			muduo::Timestamp st_addorder_i = et_addorder_q;
#endif
			time_point time_point_now = std::chrono::system_clock::now();
			session.start_transaction();
			btransaction = true;
			if (mgo::AddOrder(
				document{}
				<< "orderid" << req.orderId
				<< "userid" << userId
				<< "account" << req.Account
				<< "agentid" << b_int32{ req.p_agent_info->agentId }
				<< "score" << b_int64{ req.scoreI64 }
				<< "status" << b_int32{ 1 }
				<< "finishtime" << b_date{ time_point_now }
#ifdef _USE_SCORE_ORDERS_
				<< "scoretype" << b_int32{ 2 }
#endif
				<< finalize
			).empty()) {
				session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_addorder_i = muduo::Timestamp::now();
				//ss << "\n4.[mongo]insert add_score_order elapsed " << muduo::timeDifference(et_addorder_i, st_addorder_i) << "s";
				createLatestElapsed(latest, "mongo.insert", "add_score_order", muduo::timeDifference(et_addorder_i, st_addorder_i));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " insert add_score_order error";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataError, BOOST::Any(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_addorder_i = muduo::Timestamp::now();
			//ss << "\n4.[mongo]insert add_score_order elapsed " << muduo::timeDifference(et_addorder_i, st_addorder_i) << "s";
			createLatestElapsed(latest, "mongo.insert", "add_score_order", muduo::timeDifference(et_addorder_i, st_addorder_i));
			muduo::Timestamp st_user_u = et_addorder_i;
#endif
			if (!mgo::UpdateUser(
				document{} << "$inc" << open_document
				<< "score" << b_int64{ req.scoreI64 }
				<< "addscoretimes" << b_int32{ 1 }
				<< "alladdscore" << b_int64{ req.scoreI64 }
				<< close_document << finalize,
				document{} << "userid" << userId << finalize)) {
				session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_user_u = muduo::Timestamp::now();
				//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
				createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " update game_user error";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataError, BOOST::Any(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_user_u = muduo::Timestamp::now();
			//ss << "\n5.[mongo]update game_user elapsed " << muduo::timeDifference(et_user_u, st_user_u) << "s";
			createLatestElapsed(latest, "mongo.update", "game_user", muduo::timeDifference(et_user_u, st_user_u));
			muduo::Timestamp st_record_i = et_user_u;
#endif
			if (mgo::AddOrderRecord(
				document{}
				<< "userid" << userId
				<< "account" << req.Account
				<< "agentid" << b_int32{ req.p_agent_info->agentId }
				<< "changetype" << b_int32{ 2 }
				<< "changemoney" << b_int64{ req.scoreI64 }
				<< "beforechange" << b_int64{ beforeScore }
				<< "afterchange" << b_int64{ beforeScore + req.scoreI64 }
				<< "linecode" << linecode
				<< "logdate" << b_date{ time_point_now }
				<< finalize
			).empty()) {
				session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_record_i = muduo::Timestamp::now();
				//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
				createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " insert user_score_record error";
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataError, BOOST::Any(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_record_i = muduo::Timestamp::now();
			//ss << "\n6.[mongo]insert user_score_record elapsed " << muduo::timeDifference(et_record_i, st_record_i) << "s";
			createLatestElapsed(latest, "mongo.insert", "user_score_record", muduo::timeDifference(et_record_i, st_record_i));
#endif
			// 合作模式判断 1 买分 2 信用
			if (req.p_agent_info->cooperationtype == (int)eCooType::buyScore) {
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp st_redislock_q = et_record_i;
#endif
				//redis分布式加锁 ttl指定1s锁时长
				//1.避免玩家HTTP短连接重复Api请求情况 2.避免玩家同时多Api节点请求情况
				std::string strLockKey = "lock.agentid:" + std::to_string(req.p_agent_info->agentId) + ".order";
				RedisLock::CGuardLock lock(REDISLOCK, strLockKey.c_str(), gServer->ttlAgentLock_);
				if (!lock.IsLocked()) {
					session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
					muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
					//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
					createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
#endif
					std::stringstream ss;
					ss << "orderid." << req.orderId << " agentid." << req.p_agent_info->agentId << " userid." << userId
						<< ".account." << req.Account << " redisLock failed";
					_LOG_ERROR(ss.str().c_str());
					return response::json::err::Result(ERR::EAddScoreHandleInsertDataOutTime, BOOST::Any(), rsp);
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
				//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
				createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
				muduo::Timestamp st_agent_u = et_redislock_q;
#endif
				if(!mgo::UpdateAgent(
					document{} << "agentid" << req.p_agent_info->agentId << finalize,
					document{} << "$inc" << open_document << "score" << b_int64{ -req.scoreI64 } << close_document << finalize)) {
					session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
					muduo::Timestamp et_agent_u = muduo::Timestamp::now();
					//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
					createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
					std::stringstream ss;
					ss << "orderid." << req.orderId << " update agent_info error";
					_LOG_ERROR(ss.str().c_str());
					return response::json::err::Result(ERR::EAddScoreHandleInsertDataError, BOOST::Any(), rsp);
				}
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_agent_u = muduo::Timestamp::now();
				//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
				createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
				req.p_agent_info->score -= req.scoreI64;
			}
			session.commit_transaction();
			BOOST::Json root;
			root.put("orderid", req.orderId);
			root.put("userid", userId);
			root.put("account", req.Account);
			root.put("agentid", req.p_agent_info->agentId);
			root.put("score", req.scoreI64);
			root.put("status", 1);
			REDISCLIENT.set("s.order:" + req.orderId + ":add", root.to_json(), gServer->ttlExpired_);
		}
		catch (const bsoncxx::exception& e) {
			if (btransaction) {
				session.abort_transaction();
			}
			switch (mgo::opt::getErrCode(e.what())) {
			case 11000: {
				std::stringstream ss;
				ss << "orderid." << req.orderId << " " << e.what();
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EAddScoreHandleInsertDataOrderIDExists, BOOST::Any(), rsp);
			}
			default: {
				std::stringstream ss;
				ss << "orderid." << req.orderId << " " << e.what();
				_LOG_ERROR(ss.str().c_str());
				return response::json::err::Result(ERR::EInsideErrorOrNonExcutive, BOOST::Any(), rsp);
			}
			}
		}
		catch (const std::exception& e) {
			if (btransaction) {
				session.abort_transaction();
			}
			std::stringstream ss;
			ss << "orderid." << req.orderId << " " << e.what();
			_LOG_ERROR(ss.str().c_str());
			return response::json::err::Result(ERR::EInsideErrorOrNonExcutive, BOOST::Any(), rsp);
		}
		catch (...) {
			if (btransaction) {
				session.abort_transaction();
			}
			std::stringstream ss;
			ss << "orderid." << req.orderId << " unknown";
			_LOG_ERROR(ss.str().c_str());
			return response::json::err::Result(ERR::EInsideErrorOrNonExcutive, BOOST::Any(), rsp);
		}
#ifdef _STAT_ORDER_QPS_DETAIL_
		//估算每秒请求处理数 
		*req->testTPS = (int)(1 / muduo::timeDifference(muduo::Timestamp::now(), st_collect));
#endif
		//if (onlinestatus) {
#if 0
			BOOST::Json obj;
			obj.put("userId", userId);
			obj.put("score", beforeScore + req.scoreI64);
			REDISCLIENT.publishOrderScoreMessage(obj.to_json());
#else
			BroadcastGateUserScore(userId, beforeScore + req.scoreI64);
#endif
		//}
		//调试模式下，打印从接收网络请求(receive)到处理完逻辑业务所经历时间dt(s)
		utils::sprintf(" dt(%.6fs)", muduo::timeDifference(muduo::Timestamp::now(), receiveTime));
		return response::json::err::Result(ERR::EOk, BOOST::Any(), rsp);
	}
	case eApiType::OpSubScore:{
		_LOG_WARN("下分REQ orderId[%s] account[%s] agentId[%d] deltascore[%f] deltascoreI64[%lld]",
			req.orderId.c_str(), req.Account.c_str(), req.p_agent_info->agentId, req.Score, req.scoreI64);

		break;
	}
	default:
		break;
	}
	return ErrorCode::kFailed;
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
			return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
		}
		if (!isdecrypt_) {
			//type
			HttpParams::const_iterator typeKey = params.find("type");
			if (typeKey == params.end() || typeKey->second.empty() ||
				(opType = atol(typeKey->second.c_str())) < 0) {
				return response::json::err::Result(ERR::EGameHandleOperationTypeError, BOOST::Any(), rsp);
			}
			//2.上分 3.下分
			if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
				return response::json::err::Result(ERR::EGameHandleOperationTypeError, BOOST::Any(), rsp);
			}
			//agentid
			HttpParams::const_iterator agentIdKey = params.find("agentid");
			if (agentIdKey == params.end() || agentIdKey->second.empty() ||
				(agentId = atol(agentIdKey->second.c_str())) <= 0) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
#ifdef _NO_LOGIC_PROCESS_
			//userid
			HttpParams::const_iterator userIdKey = params.find("userid");
			if (userIdKey == params.end() || userIdKey->second.empty() ||
				(userId = atoll(userIdKey->second.c_str())) <= 0) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
#endif
			//account
			HttpParams::const_iterator accountKey = params.find("account");
			if (accountKey == params.end() || accountKey->second.empty()) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = params.find("orderid");
			if (orderIdKey == params.end() || orderIdKey->second.empty()) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
		}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = params.find("score");
			if (scoreKey == params.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				(scoreI64 = utils::rate100(scoreKey->second)) <= 0) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}

			// 获取当前代理数据
			{
				READ_LOCK(gServer->agent_info_mutex_);
				std::map<int32_t, agent_info_t>::iterator it = gServer->agent_info_.find(agentId);
				if (it == gServer->agent_info_.end()) {
					// 代理ID不存在或代理已停用
					return response::json::err::Result(ERR::EGameHandleProxyIDError, BOOST::Any(), rsp);
				}
				else {
					p_agent_info = &it->second;
				}
			}
			// 没有找到代理，判断代理的禁用状态(0正常 1停用)
			if (p_agent_info->status == 1) {
				// 代理ID不存在或代理已停用
				return response::json::err::Result(ERR::EGameHandleProxyIDError, BOOST::Any(), rsp);
			}
#ifndef _NO_LOGIC_PROCESS_
			//doOrder(opType, account, score, orderId, errmsg, latest, testTPS);
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
			return response::json::err::Result(ERR::EGameHandleOperationTypeError, BOOST::Any(), rsp);
		}
		//2.上分 3.下分
		if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
			return response::json::err::Result(ERR::EGameHandleOperationTypeError, BOOST::Any(), rsp);
		}
		//agentid
		HttpParams::const_iterator agentIdKey = params.find("agentid");
		if (agentIdKey == params.end() || agentIdKey->second.empty() ||
			(agentId = atol(agentIdKey->second.c_str())) <= 0) {
			return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
		}
		//timestamp
		HttpParams::const_iterator timestampKey = params.find("timestamp");
		if (timestampKey == params.end() || timestampKey->second.empty() ||
			atol(timestampKey->second.c_str()) <= 0) {
			return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
		}
		else {
			timestamp = timestampKey->second;
		}
		//param
		HttpParams::const_iterator paramValueKey = params.find("param");
		if (paramValueKey == params.end() || paramValueKey->second.empty()) {
			return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
		}
		else {
			param = paramValueKey->second;
		}
		//key
		HttpParams::const_iterator keyKey = params.find("key");
		if (keyKey == params.end() || keyKey->second.empty()) {
			return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
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
				return response::json::err::Result(ERR::EGameHandleProxyIDError, BOOST::Any(), rsp);
			}
			else {
				p_agent_info = &it->second;
			}
		}
		// 没有找到代理，判断代理的禁用状态(0正常 1停用)
		if (p_agent_info->status == 1) {
			// 代理ID不存在或代理已停用
			return response::json::err::Result(ERR::EGameHandleProxyIDError, BOOST::Any(), rsp);
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
				return response::json::err::Result(ERR::EGameHandleProxyMD5CodeError, BOOST::Any(), rsp);
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
				return response::json::err::Result(ERR::EGameHandleProxyDESCodeError, BOOST::Any(), rsp);
			}
		}
		{
			HttpParams decryptParams;
			_LOG_DEBUG(decrypt.c_str());
			if (!utils::parseQuery(decrypt, decryptParams)) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
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
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
#endif
			//account
			HttpParams::const_iterator accountKey = decryptParams.find("account");
			if (accountKey == decryptParams.end() || accountKey->second.empty()) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = decryptParams.find("orderid");
			if (orderIdKey == decryptParams.end() || orderIdKey->second.empty()) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = decryptParams.find("score");
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				(scoreI64 = utils::rate100(scoreKey->second)) <= 0) {
				return response::json::err::Result(ERR::EGameHandleParamsError, BOOST::Any(), rsp);
			}
		}
#ifndef _NO_LOGIC_PROCESS_
		//doOrder(opType, account, score, orderId, errmsg, latest, testTPS);
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