#include "public/Inc.h"
#include "Response/response.h"
#include "handler/order_handler.h"
#include "public/mgoOperation.h"
#include "public/mgoKeys.h"
#include "public/mgoModel.h"
#include "public/redisKeys.h"
#include "ServList.h"
#include "public/errorCode.h"
#include "../Api.h"

extern ApiServ* gServer;

int subScore(OrderReq const& req, muduo::net::HttpResponse& rsp,
	const muduo::net::TcpConnectionPtr& conn,
	muduo::Timestamp receiveTime) {
	agent_user_t user;
	int64_t userId = 0;
	int64_t beforeScore = 0;
	int32_t onlinestatus = 0;
	std::string linecode;
	/*static*/ /*__thread*/ mongocxx::client_session session = MongoDBClient::start_session();
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
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleUserNotExistsError, ss.str().c_str(), rsp);
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
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertDataOutTime, ss.str().c_str(), rsp);
		}
#ifdef _STAT_ORDER_QPS_DETAIL_
		muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
		//ss << "\n2.[redis]query redlock(userid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
		createLatestElapsed(latest, "redis.query", "redlock(userid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
		muduo::Timestamp st_suborder_q = et_redislock_q;
#endif
#if 0
		if (!mgo::ExistSubOrder(document{} << "orderid" << req.orderId << finalize)) {
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
			//ss << "\n3.[mongo]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
			createLatestElapsed(latest, "mongo.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
#endif
			std::stringstream ss;
			ss << "orderid." << req.orderId << " query sub_score_order existed";
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertOrderIDExists, ss.str().c_str(), rsp);
		}
#else
		//redis查询注单orderid是否已经存在，mongodb[sub_score_order]需要建立唯一约束 UNIQUE(orderid)
		if (REDISCLIENT.exists("s.order:" + req.orderId + ":sub")) {
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
			//ss << "\n3.[redis]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
			createLatestElapsed(latest, "redis.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
#endif
			std::stringstream ss;
			ss << "orderid." << req.orderId << " query sub_score_order existed";
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertOrderIDExists, ss.str().c_str(), rsp);
		}
#endif
#ifdef _STAT_ORDER_QPS_DETAIL_
		muduo::Timestamp et_suborder_q = muduo::Timestamp::now();
		//ss << "\n3.[redis]query sub_score_order elapsed " << muduo::timeDifference(et_suborder_q, st_suborder_q) << "s";
		createLatestElapsed(latest, "redis.query", "sub_score_order", muduo::timeDifference(et_suborder_q, st_suborder_q));
		muduo::Timestamp st_suborder_i = et_suborder_q;
#endif
		// 玩家游戏中不能下分
		if (REDISCLIENT.ExistOnlineInfo(userId)) {
			std::stringstream ss;
			ss << "orderid." << req.orderId << " " << req.Account << " is playing"/* << redisGameId << "." << redisRoomId*/;
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertDataUserInGaming, ss.str().c_str(), rsp);
		}
		if (beforeScore < req.scoreI64) {
			std::stringstream ss;
			ss << "orderid." << req.orderId << " " << "score not enough, score." << beforeScore << " < deltascore." << req.scoreI64;
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			// 玩家下分超出玩家现有总分
			return response::json::Result(ERR_SubScoreHandleInsertDataOverScore, ss.str().c_str(), rsp);
		}
		time_point time_point_now = std::chrono::system_clock::now();
		session.start_transaction();
		btransaction = true;
		if (mgo::SubOrder(
			document{}
			<< "orderid" << req.orderId
			<< "userid" << userId
			<< "account" << req.Account
			<< "agentid" << b_int32{ req.p_agent_info->agentId }
			<< "score" << b_int64{ req.scoreI64 }
			<< "status" << b_int32{ 1 }
			<< "finishtime" << b_date{ time_point_now }
#ifdef _USE_SCORE_ORDERS_
			<< "scoretype" << b_int32{ 3 }
#endif
			<< finalize
		).empty()) {
			session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_suborder_i = muduo::Timestamp::now();
			//ss << "\n4.[mongo]insert sub_score_order elapsed " << muduo::timeDifference(et_suborder_i, st_suborder_i) << "s";
			createLatestElapsed(latest, "mongo.insert", "sub_score_order", muduo::timeDifference(et_suborder_i, st_suborder_i));
#endif
			std::stringstream ss;
			ss << "orderid." << req.orderId << " insert sub_score_order error";
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertDataError, ss.str().c_str(), rsp);
		}
#ifdef _STAT_ORDER_QPS_DETAIL_
		muduo::Timestamp et_suborder_i = muduo::Timestamp::now();
		//ss << "\n4.[mongo]insert sub_score_order elapsed " << muduo::timeDifference(et_suborder_i, st_suborder_i) << "s";
		createLatestElapsed(latest, "mongo.insert", "sub_score_order", muduo::timeDifference(et_suborder_i, st_suborder_i));
		muduo::Timestamp st_user_u = et_suborder_i;
#endif
		if (!mgo::UpdateUser(
			document{} << "$inc" << open_document
			<< "score" << b_int64{ -req.scoreI64 }
			<< "subscoretimes" << b_int32{ 1 }
			<< "allsubscore" << b_int64{ req.scoreI64 }
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
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertDataError, ss.str().c_str(), rsp);
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
			<< "changetype" << b_int32{ 3 }
			<< "changemoney" << b_int64{ -req.scoreI64 }
			<< "beforechange" << b_int64{ beforeScore }
			<< "afterchange" << b_int64{ beforeScore - req.scoreI64 }
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
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertDataError, ss.str().c_str(), rsp);
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
				Errorf(ss.str().c_str());
				ss << " " << _CODE_;
				return response::json::Result(ERR_SubScoreHandleInsertDataOutTime, ss.str().c_str(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_redislock_q = muduo::Timestamp::now();
			//ss << "\n7.[redis]query redlock(agentid) elapsed " << muduo::timeDifference(et_redislock_q, st_redislock_q) << "s";
			createLatestElapsed(latest, "redis.query", "redlock(agentid)", muduo::timeDifference(et_redislock_q, st_redislock_q));
			muduo::Timestamp st_agent_u = et_redislock_q;
#endif
			if (!mgo::UpdateAgent(
				document{} << "$inc" << open_document << "score" << b_int64{ req.scoreI64 } << close_document << finalize,
				document{} << "agentid" << req.p_agent_info->agentId << finalize)) {
				session.abort_transaction();
#ifdef _STAT_ORDER_QPS_DETAIL_
				muduo::Timestamp et_agent_u = muduo::Timestamp::now();
				//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
				createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
				std::stringstream ss;
				ss << "orderid." << req.orderId << " update agent_info error";
				Errorf(ss.str().c_str());
				ss << " " << _CODE_;
				return response::json::Result(ERR_SubScoreHandleInsertDataError, ss.str().c_str(), rsp);
			}
#ifdef _STAT_ORDER_QPS_DETAIL_
			muduo::Timestamp et_agent_u = muduo::Timestamp::now();
			//ss << "\n8.[mongo]update agent_info elapsed " << muduo::timeDifference(et_agent_u, st_agent_u) << "s";
			createLatestElapsed(latest, "mongo.update", "agent_info", muduo::timeDifference(et_agent_u, st_agent_u));
#endif
			req.p_agent_info->score += req.scoreI64;
		}
		session.commit_transaction();
		BOOST::Json json;
		json.put("orderid", req.orderId);
		json.put("userid", userId);
		json.put("account", req.Account);
		json.put("agentid", req.p_agent_info->agentId);
		json.put("score", req.scoreI64);
		json.put("status", 1);
		REDISCLIENT.set("s.order:" + req.orderId + ":sub", json.to_string(), gServer->ttlExpired_);
#ifdef _STAT_ORDER_QPS_DETAIL_
		//估算每秒请求处理数
		* req->testTPS = (int)(1 / muduo::timeDifference(muduo::Timestamp::now(), st_collect));
#endif
		//if (onlinestatus) {
#if 0
			BOOST::Json json;
			json.put("userId", userId);
			json.put("score", beforeScore - req.scoreI64);
			REDISCLIENT.publishOrderScoreMessage(json.to_string());
#else
			BroadcastGateUserScore(userId, beforeScore - req.scoreI64);
#endif
		//}
		//调试模式下，打印从接收网络请求(receive)到处理完逻辑业务所经历时间dt(s)
		std::string s = utils::sprintf(" dt(%.6fs)", muduo::timeDifference(muduo::Timestamp::now(), receiveTime));
		json.clear();
		json.put("account", req.Account);
		json.put("orderid", req.orderId);
		json.put("type", req.Type);
		json.put("score", req.scoreI64);
		return response::json::OkMsg("下分成功" + s, rsp, json);
	}
	catch (const bsoncxx::exception& e) {
		if (btransaction) {
			session.abort_transaction();
		}
		switch (mgo::opt::getErrCode(e.what())) {
		case 11000: {
			std::stringstream ss;
			ss << "orderid." << req.orderId << " " << e.what();
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_SubScoreHandleInsertOrderIDExists, ss.str().c_str(), rsp);
		}
		default: {
			std::stringstream ss;
			ss << "orderid." << req.orderId << " " << e.what();
			Errorf(ss.str().c_str());
			ss << " " << _CODE_;
			return response::json::Result(ERR_InsideErrorOrNonExcutive, ss.str().c_str(), rsp);
		}
		}
	}
	catch (const std::exception& e) {
		if (btransaction) {
			session.abort_transaction();
		}
		std::stringstream ss;
		ss << "orderid." << req.orderId << " " << e.what();
		Errorf(ss.str().c_str());
		ss << " " << _CODE_;
		return response::json::Result(ERR_InsideErrorOrNonExcutive, ss.str().c_str(), rsp);
	}
	catch (...) {
		if (btransaction) {
			session.abort_transaction();
		}
		std::stringstream ss;
		ss << "orderid." << req.orderId << " unknown";
		Errorf(ss.str().c_str());
		ss << " " << _CODE_;
		return response::json::Result(ERR_InsideErrorOrNonExcutive, ss.str().c_str(), rsp);
	}
}