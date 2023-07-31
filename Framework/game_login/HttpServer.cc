#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "Login.h"
#include "public/Response.h"
#include "register/Login_handler.h"
#include "register/Register_handler.h"

bool LoginServ::onHttpCondition(const muduo::net::InetAddress& peerAddr) {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpenAccept);
	httpServer_.getLoop()->assertInLoopThread();
	{
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
		if (it != adminList_.end()) {
			return true;
		}
	}
	{
		//192.168.2.21:3640 192.168.2.21:3667
		std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(peerAddr.ipv4NetEndian());
		return (it != whiteList_.end()) && (IpVisitE::kEnable == it->second);
	}
#if 0
	//节点维护中
	if (serverState_ == ServiceStateE::kRepairing) {
		return false;
	}
#endif
	return true;
}

void LoginServ::onHttpConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		_LOG_INFO("WEB端[%s] -> 登陆服[%s] %s %d",
			conn->peerAddress().toIpPort().c_str(),
			conn->localAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		numTotalReq_.incrementAndGet();
		if (num > maxConnections_) {
			muduo::net::HttpResponse rsp(false);
			response::text::Result(
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 600 访问量限制(" + std::to_string(maxConnections_) + ")\r\n\r\n",
				rsp);
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
#if 0
			//不再发送数据
			conn->shutdown();
#elif 0
			conn->forceClose();
#elif 0
			conn->forceCloseWithDelay(0.2f);
#endif
			//会调用onHttpMessage函数
			assert(conn->getContext().empty());
			numTotalBadReq_.incrementAndGet();
			return;
		}
		EntryPtr entry(new Entry(Entry::TypeE::HttpTy, conn, "WEB前端", "登陆服"));
		RunInLoop(conn->getLoop(),
			std::bind(&Buckets::push, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
		conn->setContext(Context(entry, muduo::net::HttpContext()));
		conn->setTcpNoDelay(true);
		boost::any_cast<Context&>(conn->getContext()).setWorker(nextPool_, threadPool_);
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		_LOG_INFO("WEB端[%s] -> 登陆服[%s] %s %d",
			conn->peerAddress().toIpPort().c_str(),
			conn->localAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void LoginServ::onHttpMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	conn->getLoop()->assertInLoopThread();
	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		return;
	}
	//_LOG_DEBUG("\n%.*s", buf->readableBytes(), buf->peek());
	//先确定是HTTP数据报文，再解析
	//assert(buf->readableBytes() > 4 && buf->findCRLFCRLF());
	Context& entryContext = boost::any_cast<Context&>(conn->getContext());
	muduo::net::HttpContext& httpContext = boost::any_cast<muduo::net::HttpContext&>(entryContext.getContext());
	if (!httpContext.parseRequest(buf, receiveTime)) {
	}
	else if (httpContext.gotAll()) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == IpVisitCtrlE::kOpen) {
			std::string ipaddr;
			{
				std::string ipaddrs = httpContext.request().getHeader("X-Forwarded-For");
				if (ipaddrs.empty()) {
					ipaddr = conn->peerAddress().toIp();
				}
				else {
#if 0
					//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP
					std::string::size_type spos = ipaddrs.find_first_of(',');
					if (spos == std::string::npos) {
					}
					else {
						ipaddr = ipaddrs.substr(0, spos);
					}
#else
					boost::replace_all<std::string>(ipaddrs, " ", "");
					std::vector<std::string> vec;
					boost::algorithm::split(vec, ipaddrs, boost::is_any_of(","));
					for (std::vector<std::string>::const_iterator it = vec.begin();
						it != vec.end(); ++it) {
						/*if (!it->empty() &&
							boost::regex_match(*it, boost::regex(
								"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {

							if (strncasecmp(it->c_str(), "10.", 3) != 0 &&
								strncasecmp(it->c_str(), "192.168", 7) != 0 &&
								strncasecmp(it->c_str(), "172.16.", 7) != 0) {
								ipaddr = *it;
								break;
							}
						}*/
					}
#endif
				}
			}
			muduo::net::InetAddress peerAddr(muduo::StringArg(ipaddr), 0, false);
			bool is_ip_allowed = false;
			{
				//管理员挂维护/恢复服务
				std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
				is_ip_allowed = (it != adminList_.end());
			}
			if (!is_ip_allowed) {
				READ_LOCK(whiteList_mutex_);
				std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(peerAddr.ipv4NetEndian());
				is_ip_allowed = ((it != whiteList_.end()) && (IpVisitE::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 1
				conn->forceClose();
#elif 0
				muduo::net::HttpResponse rsp(false);
				response::text::Result(
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n",
					rsp);
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);
				conn->forceCloseWithDelay(0.2f);
#endif
				numTotalBadReq_.incrementAndGet();
				return;
			}
		}
		EntryPtr entry(entryContext.getWeakEntryPtr().lock());
		if (entry) {
			RunInLoop(conn->getLoop(),
				std::bind(&Buckets::update, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
			BufferPtr buffer;
			if (buf->readableBytes() > 0) {
				buffer.reset(new muduo::net::Buffer(buf->readableBytes()));
				buffer->append(buf->peek(), static_cast<size_t>(buf->readableBytes()));
			}
			entryContext.getWorker()->run(
				std::bind(
					&LoginServ::asyncHttpHandler,
					this, conn, buffer, receiveTime));
		}
		else {
			numTotalBadReq_.incrementAndGet();
			_LOG_FATAL("entry invalid");
		}
		return;
	}
	muduo::net::HttpResponse rsp(false);
	response::text::Result(
		muduo::net::HttpResponse::k404NotFound,
		"HTTP/1.1 400 Bad Request\r\n\r\n",
		rsp);
	muduo::net::Buffer buffer;
	rsp.appendToBuffer(&buffer);
	conn->send(&buffer);
	httpContext.reset();
#if 0
	//不再发送数据
	conn->shutdown();
#elif 0
	conn->forceClose();
#elif 0
	conn->forceCloseWithDelay(0.2f);
#endif
	numTotalBadReq_.incrementAndGet();
}

void LoginServ::asyncHttpHandler(
	const muduo::net::WeakTcpConnectionPtr& weakConn,
	BufferPtr const& buf, muduo::Timestamp receiveTime) {
	//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
	//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
	//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
	//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
	//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
	//
	//锁定同步业务操作，先锁超时对象entry，再锁conn，避免超时和业务同时处理的情况
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (conn) {
#if 0
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == IpVisitCtrlE::kOpen) {
			bool is_ip_allowed = false;
			{
				READ_LOCK(whiteList_mutex_);
				std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(conn->peerAddress().ipv4NetEndian());
				is_ip_allowed = ((it != whiteList_.end()) && (IpVisitE::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 0
				conn->forceClose();
#else
				muduo::net::HttpResponse rsp(false);
				response::text::Result(
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n",
					rsp);
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);
				conn->forceCloseWithDelay(0.2f);
#endif
				return;
			}
		}
#endif
		Context& entryContext = boost::any_cast<Context&>(conn->getContext());
		muduo::net::HttpContext& httpContext = boost::any_cast<muduo::net::HttpContext&>(entryContext.getContext());
		assert(httpContext.gotAll());
		const string& connection = httpContext.request().getHeader("Connection");
		bool close = (connection == "close") ||
			(httpContext.request().getVersion() == muduo::net::HttpRequest::kHttp10 && connection != "Keep-Alive");
		muduo::net::HttpResponse rsp(close);
		processHttpRequest(httpContext.request(), rsp, conn->peerAddress(), buf, receiveTime);
		{
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
		}
		if (rsp.closeConnection()) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 0
			conn->forceClose();
#elif 1
			conn->forceCloseWithDelay(0.2f);
#endif
		}
		httpContext.reset();
		}
	else {
		numTotalBadReq_.incrementAndGet();
		_LOG_ERROR("conn invalid");
	}
}

void LoginServ::onHttpWriteComplete(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
#if 0
	//不再发送数据
	conn->shutdown();
#elif 0
	conn->forceClose();
#else
	conn->forceCloseWithDelay(0.1f);
#endif
}

void LoginServ::processHttpRequest(
	const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
	muduo::net::InetAddress const& peerAddr,
	BufferPtr const& buf,
	muduo::Timestamp receiveTime) {
	if (req.path() == "/") {
		response::text::Result(
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n",
			rsp);
	}
	else if (req.path() == "/test") {
		response::xml::Test(req, rsp);
	}
	else {
		if (serverState_ == kRepairing) {
			response::text::Result(
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 405 服务维护中\r\n\r\n",
				rsp);
		}
		else {
			//登陆
			if (req.path() == "/login") {
				Login(req, rsp, buf, receiveTime);
			}
		}
	}
	
// 	if (req.path() == "/GameHandle") {
// 		int errcode = ApiErrorCode::NoError;
// 		std::string errmsg;
// 		std::string rspdata;
// 		boost::property_tree::ptree latest;
// 		int testTPS = 0;
// #ifdef _STAT_QPS_
// 		//起始时间戳(微秒)
// 		static muduo::Timestamp timeStart_;
// 		//统计请求次数
// 		static muduo::AtomicInt32 numRequest_;
// 		//历史请求次数
// 		static muduo::AtomicInt32 numRequestTotal_;
// 		//统计成功次数
// 		static muduo::AtomicInt32 numRequestSucc_;
// 		//统计失败次数
// 		static muduo::AtomicInt32 numRequestFailed_;
// 		//历史成功次数
// 		static muduo::AtomicInt32 numRequestTotalSucc_;
// 		//历史失败次数
// 		static muduo::AtomicInt32 numRequestTotalFailed_;
// 		static volatile long value = 0;
// 		if (0 == __sync_val_compare_and_swap(&value, 0, 1)) {
// 			timeStart_ = muduo::Timestamp::now();
// 		}
// 		//本次请求开始时间戳(微秒)
// 		muduo::Timestamp timestart = muduo::Timestamp::now();
// #endif
// 		if (serverState_ == kRepairing) {
// 			response::text::Result(
// 				muduo::net::HttpResponse::k404NotFound,
// 				"HTTP/1.1 405 服务维护中\r\n\r\n", rsp);
// 		}
// 		else {
// 			rspdata = onProcess(req.query(), receiveTime, errcode, errmsg, latest, testTPS);
// 			_LOG_INFO("\n%s", rspdata.c_str());
// #ifdef _STAT_QPS_
// 			if (errcode == ApiErrorCode::NoError) {
// 				numRequestSucc_.incrementAndGet();
// 				numRequestTotalSucc_.incrementAndGet();
// 			}
// 			else {
// 				numRequestFailed_.incrementAndGet();
// 				numRequestTotalFailed_.incrementAndGet();
// 			}
// #endif
// 			rsp.setContentType("application/json;charset=utf-8");
// 			rsp.setBody(rspdata);
// 		}
// #ifdef _STAT_QPS_
// 		//本次请求结束时间戳(微秒)
// 		muduo::Timestamp timenow = muduo::Timestamp::now();
// 		numRequest_.incrementAndGet();
// 		numRequestTotal_.incrementAndGet();
// 		static volatile long value = 0;
// 		if (0 == __sync_val_compare_and_swap(&value, 0, 1)) {
// 			//间隔时间(s)打印一次
// 			static int deltaTime_ = 10;
// 			//统计间隔时间(s)
// 			double totalTime = muduo::timeDifference(timenow, timeStart_);
// 			if (totalTime >= (double)deltaTime_) {
// 				//最近一次请求耗时(s)
// 				double timdiff = muduo::timeDifference(timenow, timestart);
// 				int64_t	requestNum = numRequest_.get();
// 				//统计成功次数
// 				int64_t requestNumSucc = numRequestSucc_.get();
// 				//统计失败次数
// 				int64_t requestNumFailed = numRequestFailed_.get();
// 				//统计命中率
// 				double ratio = (double)(requestNumSucc) / (double)(requestNum);
// 				//历史请求次数
// 				int64_t	requestNumTotal = numRequestTotal_.get();
// 				//历史成功次数
// 				int64_t requestNumTotalSucc = numRequestTotalSucc_.get();
// 				//历史失败次数
// 				int64_t requestNumTotalFailed = numRequestTotalFailed_.get();
// 				//历史命中率
// 				double ratioTotal = (double)(requestNumTotalSucc) / (double)(requestNumTotal);
// 				//平均请求耗时(s)
// 				double avgTime = totalTime / requestNum;
// 				//每秒请求次数(QPS)
// 				int64_t avgNum = (int64_t)(requestNum / totalTime);
// 				std::stringstream s;
// 				boost::property_tree::json_parser::write_json(s, latest, true);
// 				std::string json = s.str();
// 				_LOG_ERROR("\n%s\n" \
// 					"I/O线程数[%d] 业务线程数[%d] 累计接收请求数[%d] 累计未处理请求数[%d]\n" \
// 					"本次统计间隔时间[%d]s 请求超时时间[%d]s\n" \
// 					"本次统计请求次数[%d] 成功[%d] 失败[%d] 命中率[%.3f]\n" \
// 					"最近一次请求耗时[%d]ms [%s]\n" \
// 					"平均请求耗时[%d]ms\n" \
// 					"每秒请求次数(QPS) = [%d] 单线程每秒请求处理数(TPS) = [%d] 预计每秒请求处理总数(TPS) = [%d]\n" \
// 					"历史请求次数[%d] 成功[%d] 失败[%d] 命中率[%.3f]",
// 					json.c_str(),
// 					numThreads_, workerNumThreads_, numTotalReq_.get(), numTotalBadReq_.get(),
// 					totalTime, idleTimeout_,
// 					requestNum, requestNumSucc, requestNumFailed, ratio,
// 					timdiff * muduo::Timestamp::kMicroSecondsPerSecond / 1000, errmsg.c_str(),
// 					avgTime * muduo::Timestamp::kMicroSecondsPerSecond / 1000,
// 					avgNum, testTPS, testTPS * workerNumThreads_,
// 					requestNumTotal, requestNumTotalSucc, requestNumTotalFailed, ratioTotal);
// 				if (totalTime >= (double)deltaTime_) {
// 					std::string monitordata = createMonitorData(latest, totalTime, kTimeoutSeconds_,
// 						requestNum, requestNumSucc, requestNumFailed, ratio,
// 						requestNumTotal, requestNumTotalSucc, requestNumTotalFailed, ratioTotal, testTPS);
// 					REDISCLIENT.set("s.monitor.order", monitordata);
// 				}
// 				timeStart_ = timenow;
// 				numRequest_.getAndSet(0);
// 				numRequestSucc_.getAndSet(0);
// 				numRequestFailed_.getAndSet(0);
// 			}
// 			__sync_val_compare_and_swap(&value, 1, 0);
// 		}
// #endif
// #if 1
// 		//rsp.setBody(rspdata);
// #else
// 		rsp.setContentType("application/xml;charset=utf-8");
// 		rsp.setBody(getRequestStr(req));
// #endif
// 	}
// 	//刷新客户端访问IP黑名单信息
// 	else if (req.path() == "/refreshBlackList") {
// 		//管理员挂维护/恢复服务
// 		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
// 		if (it != adminList_.end()) {
// 			rsp.setContentType("text/plain;charset=utf-8");
// 			refreshBlackList();
// 			rsp.setBody("success");
// 		}
// 		else {
// 			//HTTP应答包(header/body)
// 			response::text::Result(
// 				muduo::net::HttpResponse::k404NotFound,
// 				"HTTP/1.1 504 权限不够\r\n\r\n", rsp);
// 		}
// 	}
// 	//刷新HTTP访问IP白名单信息
// 	else if (req.path() == "/refreshWhiteList") {
// 		//管理员挂维护/恢复服务
// 		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
// 		if (it != adminList_.end()) {
// 			rsp.setContentType("text/plain;charset=utf-8");
// 			refreshWhiteList();
// 			rsp.setBody("success");
// 		}
// 		else {
// 			//HTTP应答包(header/body)
// 			response::text::Result(
// 				muduo::net::HttpResponse::k404NotFound,
// 				"HTTP/1.1 504 权限不够\r\n\r\n", rsp);
// 		}
// 	}
// 	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
// 	else if (req.path() == "/repairServer") {
// 		//管理员挂维护/恢复服务
// 		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
// 		if (it != adminList_.end()) {
// 			rsp.setContentType("text/plain;charset=utf-8");
// 			std::string rspdata;
// 			repairServer(req.query(), rspdata);
// 			rsp.setContentType("application/json;charset=utf-8");
// 			rsp.setBody(rspdata);
// 		}
// 		else {
// 			response::text::Result(
// 				muduo::net::HttpResponse::k404NotFound,
// 				"HTTP/1.1 504 权限不够\r\n\r\n", rsp);
// 		}
// 	}
// 	else if (req.path() == "/help") {
// 		//管理员挂维护/恢复服务
// 		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipv4NetEndian());
// 		if (it != adminList_.end()) {
// 			rsp.setContentType("text/html;charset=utf-8");
// 			rsp.setBody("<html>"
// 				"<head><title>help</title></head>"
// 				"<body>"
// 				"<h4>/refreshAgentInfo</h4>"
// 				"<h4>/refreshWhiteList</h4>"
// 				"<h4>/repairServer?type=HallServer&name=192.168.2.158:20001&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
// 				"<h4>/repairServer?type=GameServer&name=4001:192.168.0.1:5847&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
// 				"</body>"
// 				"</html>");
// 		}
// 		else {
// 			response::text::Result(
// 				muduo::net::HttpResponse::k404NotFound,
// 				"HTTP/1.1 504 权限不够\r\n\r\n", rsp);
// 		}
// 	}
// 	else {
// #if 1
// 		response::text::Result(
// 			muduo::net::HttpResponse::k404NotFound,
// 			"HTTP/1.1 404 Not Found\r\n\r\n", rsp);
// #else
//		response::html::NotFound(rsp);
// #endif
// 	}
}

std::string LoginServ::onProcess(std::string const& reqStr, muduo::Timestamp receiveTime, int& errcode, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS) {
	bool isdecrypt_ = false;
	int opType = 0;
	int agentId = 0;
#ifdef _NO_LOGIC_PROCESS_
	int64_t userId = 0;
#endif
	errcode = 0;
	std::string account;
	std::string orderId;
	double score = 0;
	std::string timestamp;
	std::string paraValue, key;
	//agent_info_t /*_agent_info = { 0 },*/* p_agent_info = NULL;
	do {
		HttpParams params;
		_LOG_DEBUG(reqStr.c_str());
		utils::parseQuery(reqStr, params);
		std::string keyValues;
		for (auto param : params) {
			keyValues += "\n" + param.first + "=" + param.second;
		}
		_LOG_DEBUG(keyValues.c_str());
		if (!isdecrypt_) {
			HttpParams::const_iterator typeKey = params.find("type");
			if (typeKey == params.end() || typeKey->second.empty() ||
				(opType = atol(typeKey->second.c_str())) < 0) {
				//errcode = eApiErrorCode::GameHandleOperationTypeError;
				errmsg += "type ";
			}
// 			if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
// 				errcode = eApiErrorCode::GameHandleOperationTypeError;
// 				errmsg += "type value ";
// 			}
			//agentid
			HttpParams::const_iterator agentIdKey = params.find("agentid");
			if (agentIdKey == params.end() || agentIdKey->second.empty() ||
				(agentId = atol(agentIdKey->second.c_str())) <= 0) {
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "agentid ";
				break;
			}
#ifdef _NO_LOGIC_PROCESS_
			//userid
			HttpParams::const_iterator userIdKey = params.find("userid");
			if (userIdKey == params.end() || userIdKey->second.empty() ||
				(userId = atoll(userIdKey->second.c_str())) <= 0) {
				errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "userid ";
			}
#endif
			//account
			HttpParams::const_iterator accountKey = params.find("account");
			if (accountKey == params.end() || accountKey->second.empty()) {
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "account ";
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = params.find("orderid");
			if (orderIdKey == params.end() || orderIdKey->second.empty()) {
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "orderid ";
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = params.find("score");
#if 0
			if (scoreKey == params.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = atoll(scoreKey->second.c_str())) <= 0) {
#else
			if (scoreKey == params.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				utils::rate100(scoreKey->second) <= 0) {
#endif
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "score ";
			}
			if (errcode != 0) {
				errmsg += "invalid";
				break;
			}
			// 获取当前代理数据
			//agent_info_t _agent_info = { 0 };
// 			{
// 				READ_LOCK(agent_info_mutex_);
// 				std::map<int32_t, agent_info_t>::/*const_*/iterator it = agent_info_.find(agentId);
// 				if (it == agent_info_.end()) {
// 					// 代理ID不存在或代理已停用
// 					errcode = eApiErrorCode::GameHandleProxyIDError;
// 					errmsg = "agent_info not found";
// 					break;
// 				}
// 				else {
// 					p_agent_info = &it->second;
// 				}
// 			}
			// 没有找到代理，判断代理的禁用状态(0正常 1停用)
// 			if (p_agent_info->status == 1) {
// 				// 代理ID不存在或代理已停用
// 				errcode = eApiErrorCode::GameHandleProxyIDError;
// 				errmsg = "agent.status error";
// 				break;
// 			}
#ifndef _NO_LOGIC_PROCESS_
			errcode = execute(opType, account, score, orderId, errmsg, latest, testTPS);
#endif
			break;
		}
		//type
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty() ||
			(opType = atol(typeKey->second.c_str())) < 0) {
			//errcode = eApiErrorCode::GameHandleOperationTypeError;
			errmsg = "type invalid";
			break;
		}
// 		if (opType != int(eApiType::OpAddScore) && opType != int(eApiType::OpSubScore)) {
// 			errcode = eApiErrorCode::GameHandleOperationTypeError;
// 			errmsg = "type value invalid";
// 			break;
// 		}
				//agentid
		HttpParams::const_iterator agentIdKey = params.find("agentid");
		if (agentIdKey == params.end() || agentIdKey->second.empty() ||
			(agentId = atol(agentIdKey->second.c_str())) <= 0) {
			//errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "agentid invalid";
			break;
		}
		//timestamp
		HttpParams::const_iterator timestampKey = params.find("timestamp");
		if (timestampKey == params.end() || timestampKey->second.empty() ||
			atol(timestampKey->second.c_str()) <= 0) {
			//errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "timestamp invalid";
			break;
		}
		else {
			timestamp = timestampKey->second;
		}
		//paraValue
		HttpParams::const_iterator paramValueKey = params.find("paraValue");
		if (paramValueKey == params.end() || paramValueKey->second.empty()) {
			//errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "paraValue invalid";
			break;
		}
		else {
			paraValue = paramValueKey->second;
		}
		//key
		HttpParams::const_iterator keyKey = params.find("key");
		if (keyKey == params.end() || keyKey->second.empty()) {
			//errcode = eApiErrorCode::GameHandleParamsError;
			errmsg = "key invalid";
			break;
		}
		else {
			key = keyKey->second;
		}
		// 获取当前代理数据
		//agent_info_t _agent_info = { 0 };
// 		{
// 			READ_LOCK(agent_info_mutex_);
// 			std::map<int32_t, agent_info_t>::/*const_*/iterator it = agent_info_.find(agentId);
// 			if (it == agent_info_.end()) {
// 				// 代理ID不存在或代理已停用
// 				errcode = eApiErrorCode::GameHandleProxyIDError;
// 				errmsg = "agent_info not found";
// 				break;
// 			}
// 			else {
// 				p_agent_info = &it->second;
// 			}
// 		}
// 		// 没有找到代理，判断代理的禁用状态(0正常 1停用)
// 		if (p_agent_info->status == 1) {
// 			// 代理ID不存在或代理已停用
// 			errcode = eApiErrorCode::GameHandleProxyIDError;
// 			errmsg = "agent.status error";
// 			break;
// 		}
#if 0
		agentId = 10000;
		p_agent_info->md5code = "334270F58E3E9DEC";
		p_agent_info->descode = "111362EE140F157D";
		timestamp = "1579599778583";
		paraValue = "0RJ5GGzw2hLO8AsVvwORE2v16oE%2fXSjaK78A98ct5ajN7reFMf9YnI6vEnpttYHK%2fp04Hq%2fshp28jc4EIN0aAFeo4pni5jxFA9vg%2bLOx%2fek%3d";
		key = "C6656A2145BAEF7ED6D38B9AF2E35442";
#elif 0
		agentId = 111169;
		p_agent_info->md5code = "8B56598D6FB32329";
		p_agent_info->descode = "D470FD336AAB17E4";
		timestamp = "1580776071271";
		paraValue = "h2W2jwWIVFQTZxqealorCpSfOwlgezD8nHScU93UQ8g%2FDH1UnoktBusYRXsokDs8NAPFEG8WdpSe%0AY5rtksD0jw%3D%3D";
		key = "a7634b1e9f762cd4b0d256744ace65f0";
#elif 0
		agentId = 111190;
		timestamp = "1583446283986";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		paraValue = "KDtKjjnaaxKWNuK%252BBRwv9f2gBxLkSvY%252FqT4HBaZY2IrxqGMK3DYlWOW4dHgiMZV8Uu66h%252BHjH8MfAfpQIE5eIHoRZMplj7dljS7Tfyf3%252BlM%253D";
		key = "4F6F53FDC4D27EC33B3637A656DD7C9F";
#elif 0
		agentId = 111149;
		timestamp = "1583448714906";
		p_agent_info->md5code = "7196FD6921347DB1";
		p_agent_info->descode = "A5F8C07B7843C73E";
		paraValue = "nu%252FtdBhN6daQdad3aOVOgzUr6bHVMYNEpWE4yLdHkKRn%252F%252Fn1V3jIFR%252BI7wawXWNyjR3%252FW0M9qzcdzM8rNx8xwe%252BEW9%252BM92ZN4hlpUAhFH74%253D";
		key = "9EEC240FAFAD3E5B6AB6B2DDBCDFE1FF";
#elif 0
		agentId = 10033;
		timestamp = "1583543899005";
		p_agent_info->md5code = "5998F124C180A817";
		p_agent_info->descode = "38807549DEA3178D";
		paraValue = "9303qk%2FizHRAszhN33eJxG2aOLA2Wq61s9f96uxDe%2Btczf2qSG8O%2FePyIYhVAaeek9m43u7awgra%0D%0Au8a8b%2FDchcZSosz9SVfPjXdc7h4Vma2dA8FHYZ5dJTcxWY7oDLlSOHKVXYHFMIWeafVwCN%2FU5fzv%0D%0AHWyb1Oa%2FWJ%2Bnfx7QXy8%3D";
		key = "2a6b55cf8df0cd8824c1c7f4308fd2e4";
#elif 0
		agentId = 111190;
		timestamp = "1583543899005";
		p_agent_info->md5code = "728F0884A000FD72";
		p_agent_info->descode = "AAFFF4393E17DB6B";
		paraValue = "%252FPxIlQ9UaP6WljAYhfYZpBO6Hz2KTrxGN%252Bmdffv9sZpaii2avwhJn3APpIOozbD7T3%252BGE1rh5q4OdfrRokriWNEhlweRKzC6%252FtABz58Kdzo%253D";
		key = "9F1E44E8B61335CCFE2E17EE3DB7C05A";
#elif 1
		//p_agent_info->descode = "mjo3obaHdKXxs5Fqw4Gj6mO7rBc52Vdw";
		paraValue = "7q9Mu4JezLGpXaJcaxBUEmnQgrH%2BO0Wi%2F85z30vF%2FIdphHU13EMp2f%2FE5%2BfHtIXYFLbyNqnwDx8SyGP1cSYssZN6gniqouFeB3kBcwSXYZbFYhNBU6162n6BaFYFVbH6KMc43RRvjBtmbkMgCr5MlRz0Co%2BQEsX9Pt3zFJiXnm12oEdLeFBSSVIcDsqd3ize9do1pbxAm9Bb45KRvPhYvA%3D%3D";
#elif 0
		agentId = 111208;
		timestamp = "1585526277991";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		paraValue = "zLcIc13jvzFHqywSHCRWX7JpaXddpWpMzaHBu8necMyCU3L9NXaZpcDXqmI4NgXvuOc6FGa80GUj%250AXOI8uoQuyjm1MLYIbrFVz09z68uSREs%253D";
		key = "59063f588d6787eff28d3";
#elif 0
		agentId = 111208;
		timestamp = "1585612589828";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		paraValue = "KfNY6Jl8k%252BBnBJLE2KQlSefbpNujwXrVcTWvRm2rfOrxie4Sd65DgAsIPIQm0uPpGS2dAQRk1HEE%250AulhnCZ0OteZiMh060xxH%252FzTzPC8DUr0%253D";
		key = "be64cc7589bebf944fcd322f9923eca4";
#elif 0
		agentId = 111209;
		timestamp = "1585639941293";
		p_agent_info->md5code = "9074653AA2D0B02F";
		p_agent_info->descode = "A78AC14440288D74";
		paraValue = "YF%252BIk5Dk2nEyNUE1TUErZY1d9RaaVmU46cwE41HRJyiqwgqu%252BOBwJT9TfPTNBtxIFjBkOgoYGljg%250AtPMbgp%252BLZz995NkM3iHrdMYp5dwv6kE%253D";
		key = "06aad6a911a7e6ce2d3b2ad18c068ae6";
#elif 0
		agentId = 111208;
		timestamp = "1585681742640";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		paraValue = "LLgiWFvdQicKfSEsDA8lTkD2FUOOcQR0LnVwDNiGjdlqgK9i9b058FlOL1DJuEp9%252BPEi37YUyTIX%250AVT7bA2H6gTbhwb4mRLzzmIWd6l3KdBM%253D";
		key = "a890129e6b9e94f29";
#elif 0
		agentId = 111208;
		timestamp = "1585743294759";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		paraValue = "5dTmQiGn0iJHUIAEMDfjxtbTpuWWIZd0HmdlFKKF4HpqgK9i9b058FlOL1DJuEp9J%252FnZJqtPOv%252F7%250A6ikd4T%252FcwEJkkVFV6TQbCk3yHamOx3s%253D";
		key = "934fa90d6";
#elif 0
		agentId = 111208;
		timestamp = "1585766704760";
		p_agent_info->md5code = "1C2EC8CB023BE339";
		p_agent_info->descode = "03CAF4333EEAA80E";
		paraValue = "DmqMRX48r66cW8Oiw0xMhgLcBuKP94YHtoQNGhAvupjxie4Sd65DgAsIPIQm0uPp%252BR6ZDGMf1B3T%250AV4owq2ro6B9Ru1XHueMJMNVLhLTaY0M%253D";
		key = "bd3778912439c03a35de1a3ce3905ef0";
#endif
		std::string descode;
		std::string md5code;
		std::string decrypt;
		{
			//根据代理商信息中存储的md5密码，结合传输参数中的agentid和timestamp，生成判定标识key
			std::string src = std::to_string(agentId) + timestamp + md5code;
			char md5[32 + 1] = { 0 };
			utils::MD5(src.c_str(), src.length(), md5, 1);
			if (strncasecmp(md5, key.c_str(), std::min<size_t>(32, key.length())) != 0) {
				//errcode = eApiErrorCode::GameHandleProxyMD5CodeError;
				errmsg = "md5 check error";
				break;
			}
			paraValue = utils::HTML::Decode(paraValue);
			_LOG_DEBUG("utils::HTML::Decode:%s", paraValue.c_str());
			for (int c = 1; c < 3; ++c) {
				paraValue = utils::URL::MultipleDecode(paraValue);
#if 1
				//"\r\n"
				boost::replace_all<std::string>(paraValue, "\r\n", "");
				//"\r"
				boost::replace_all<std::string>(paraValue, "\r", "");
				//"\r\n"
				boost::replace_all<std::string>(paraValue, "\n", "");
#else
				paraValue = boost::regex_replace(paraValue, boost::regex("\r\n|\r|\n"), "");
#endif
				_LOG_DEBUG("utils::URL::MultipleDecode:%s", paraValue.c_str());
				std::string const& strURL = paraValue;
				decrypt = Crypto::AES_ECBDecrypt(strURL, descode);
				_LOG_DEBUG("ECBDecrypt[%d] >>> md5code[%s] descode[%s] [%s]", c, md5code.c_str(), descode.c_str(), decrypt.c_str());
				if (!decrypt.empty()) {
					//成功
					break;
				}
			}
			if (decrypt.empty()) {
				// 参数转码或代理解密校验码错误
				//errcode = eApiErrorCode::GameHandleProxyDESCodeError;
				errmsg = "DESDecrypt failed, AES_set_decrypt_key error";
				break;
			}
		}
		{
			HttpParams decryptParams;
			_LOG_DEBUG(decrypt.c_str());
			utils::parseQuery(decrypt, decryptParams);
			std::string keyValues;
			for (auto param : params) {
				keyValues += "\n" + param.first + "=" + param.second;
			}
			_LOG_DEBUG(keyValues.c_str());
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
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "userid ";
			}
#endif
			//account
			HttpParams::const_iterator accountKey = decryptParams.find("account");
			if (accountKey == decryptParams.end() || accountKey->second.empty()) {
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "account ";
			}
			else {
				account = accountKey->second;
			}
			//orderid
			HttpParams::const_iterator orderIdKey = decryptParams.find("orderid");
			if (orderIdKey == decryptParams.end() || orderIdKey->second.empty()) {
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "orderid ";
			}
			else {
				orderId = orderIdKey->second;
			}
			//score
			HttpParams::const_iterator scoreKey = decryptParams.find("score");
#if 0
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = atoll(scoreKey->second.c_str())) <= 0) {
#else
			if (scoreKey == decryptParams.end() || scoreKey->second.empty() || !utils::isDigitalStr(scoreKey->second) ||
				(score = utils::floors(scoreKey->second.c_str())) <= 0 || NotScore(score) ||
				 utils::rate100(scoreKey->second) <= 0) {
#endif
				//errcode = eApiErrorCode::GameHandleParamsError;
				errmsg += "score ";
			}
			if (errcode != 0) {
				errmsg += "invalid";
				break;
			}
		}
#ifndef _NO_LOGIC_PROCESS_
		errcode = execute(opType, account, score, orderId, errmsg, latest, testTPS);
#endif
	} while (0);
#ifdef _TCP_NOTIFY_CLIENT_
#ifdef _NO_LOGIC_PROCESS_
#if 0
	int mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY;
	int subId = ::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE;
	::Game::Common::ProxyNotifyOrderScoreMessage rspdata;
	::Game::Common::Header* header = rspdata.mutable_header();
	header->set_sign(HEADER_SIGN);
	rspdata.set_userid(userId);//userId
	rspdata.set_score(score);
	std::string content = rspdata.SerializeAsString();
	sendUserData(userId, mainId, subId, content.data(), content.length());
#endif
#endif
#endif
	//调试模式下，打印从接收网络请求(receive)到处理完逻辑业务所经历时间dt(s)
	if (true) {
		char ch[50] = { 0 };
		snprintf(ch, sizeof(ch), " dt(%.6fs)", muduo::timeDifference(muduo::Timestamp::now(), receiveTime));
		errmsg += ch;
	}
	std::string json = createResponse(opType, orderId, agentId, account, score, errcode, errmsg, true);
	_LOG_DEBUG("\n%s", json.c_str());
	return json;
}

int LoginServ::execute(int opType, std::string const& account, double score, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS) {
	int errcode = ApiErrorCode::NoError;
	return errcode;
}

void LoginServ::refreshWhiteList() {
	if (whiteListControl_ == IpVisitCtrlE::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		RunInLoop(httpServer_.getLoop(), std::bind(&LoginServ::refreshWhiteListInLoop, this));
	}
	else if (whiteListControl_ == IpVisitCtrlE::kOpen) {
		//同步刷新IP访问白名单
		refreshWhiteListSync();
	}
}

bool LoginServ::refreshWhiteListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpen);
	{
		WRITE_LOCK(whiteList_mutex_);
		whiteList_.clear();
	}
	std::string s;
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.begin();
		it != whiteList_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	_LOG_DEBUG("IP访问白名单\n%s", s.c_str());
	return false;
}

bool LoginServ::refreshWhiteListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpenAccept);
	httpServer_.getLoop()->assertInLoopThread();
	whiteList_.clear();
	std::string s;
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.begin();
		it != whiteList_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	_LOG_DEBUG("IP访问白名单\n%s", s.c_str());
	return false;
}

//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
bool LoginServ::repairServer(servTyE servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata) {
	_LOG_WARN("name[%s] status[%d]", name.c_str(), status);
	static std::string path[kMaxServTy] = {
		"/GAME/HallServers/",
		"/GAME/GameServers/",
	};
	static std::string pathrepair[kMaxServTy] = {
		"/GAME/HallServersInvalid/",
		"/GAME/GameServersInvalid/",
	};
	do {
		//请求挂维护
		if (status == ServiceStateE::kRepairing) {
			/* 如果之前服务中, 尝试挂维护中, 并返回之前状态
			* 如果返回服务中, 说明刚好挂维护成功, 否则说明之前已被挂维护 */
			//if (ServiceStateE::kRunning == __sync_val_compare_and_swap(&serverState_, ServiceStateE::kRunning, ServiceStateE::kRepairing)) {
			//
			//在指定类型服务中，并且不在维护节点中
			//
// 			if (clients_[servTy].exist(name) && !clients_[servTy].isRepairing(name)) {
// 				//当前仅有一个提供服务的节点，禁止挂维护
// 				if (clients_[servTy].remaining() <= 1) {
// 					_LOG_ERROR("当前仅有一个提供服务的节点，禁止挂维护!!!");
// 					rspdata = createResponse(status, servname, name, 2, "仅剩余一个服务节点，禁止挂维护");
// 					break;
// 				}
// 				//添加 repairnode
// 				std::string repairnode = pathrepair[servTy] + name;
// 				//if (ZNONODE == zkclient_->existsNode(repairnode)) {
// 					//创建维护节点
// 					//zkclient_->createNode(repairnode, name, true);
// 					//挂维护中状态
// 				clients_[servTy].repair(name);
// 				_LOG_ERROR("创建维护节点 %s", repairnode.c_str());
// 				//}
// 				//删除 servicenode
// 				std::string servicenode = path[servTy] + name;
// 				//if (ZNONODE != zkclient_->existsNode(servicenode)) {
// 					//删除服务节点
// 					//zkclient_->deleteNode(servicenode);
// 				_LOG_ERROR("删除服务节点 %s", servicenode.c_str());
// 				//}
// 				rspdata = createResponse(status, servname, name, 0, "success");
// 			}
// 			else {
// 				rspdata = createResponse(status, servname, name, 0, "节点不存在|已挂了维护");
// 			}
			return true;
		}
		//请求恢复服务
		else if (status == ServiceStateE::kRunning) {
			/* 如果之前挂维护中, 尝试恢复服务, 并返回之前状态
			* 如果返回挂维护中, 说明刚好恢复服务成功, 否则说明之前已在服务中 */
			//if (ServiceStateE::kRepairing == __sync_val_compare_and_swap(&serverState_, ServiceStateE::kRepairing, ServiceStateE::kRunning)) {
			//
			//在指定类型服务中，并且在维护节点中
			//
// 			if (clients_[servTy].exist(name) && clients_[servTy].isRepairing(name)) {
// 				//添加 servicenode
// 				std::string servicenode = path[servTy] + name;
// 				//if (ZNONODE == zkclient_->existsNode(servicenode)) {
// 					//创建服务节点
// 					//zkclient_->createNode(servicenode, name, true);
// 					//恢复服务状态
// 				clients_[servTy].recover(name);
// 				_LOG_ERROR("创建服务节点 %s", servicenode.c_str());
// 				//}
// 				//删除 repairnode
// 				std::string repairnode = pathrepair[servTy] + name;
// 				//if (ZNONODE != zkclient_->existsNode(repairnode)) {
// 					//删除维护节点
// 					//zkclient_->deleteNode(repairnode);
// 				_LOG_ERROR("删除维护节点 %s", repairnode.c_str());
// 				//}
// 				rspdata = createResponse(status, servname, name, 0, "success");
// 			}
// 			else {
// 				rspdata = createResponse(status, servname, name, 0, "节点不存在|已在服务中");
// 			}
			return true;
		}
		rspdata = createResponse2(status, servname, name, 1, "参数无效，无任何操作");
	} while (0);
	return false;
}

bool LoginServ::repairServer(std::string const& queryStr, std::string& rspdata) {
	std::string errmsg;
	servTyE servTy;
	std::string name;
	int status;
	do {
		HttpParams params;
		_LOG_DEBUG(queryStr.c_str());
		utils::parseQuery(queryStr, params);
		std::string keyValues;
		for (auto param : params) {
			keyValues += "\n" + param.first + "=" + param.second;
		}
		_LOG_DEBUG(keyValues.c_str());
		//type
		//type=HallServer name=192.168.2.158:20001
		//type=GameServer name=4001:192.168.0.1:5847
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty()) {
			rspdata = createResponse2(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			if (typeKey->second == "HallServer") {
				servTy = servTyE::kHallTy;
			}
			else if (typeKey->second == "GameServer") {
				servTy = servTyE::kGameTy;
			}
			else {
				rspdata = createResponse2(status, typeKey->second, name, 1, "参数无效，无任何操作");
				break;
			}
		}
		//name
		HttpParams::const_iterator nameKey = params.find("name");
		if (nameKey == params.end() || nameKey->second.empty()) {
			rspdata = createResponse2(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			name = nameKey->second;
		}
		//status
		HttpParams::const_iterator statusKey = params.find("status");
		if (statusKey == params.end() || statusKey->second.empty() ||
			(status = atol(statusKey->second.c_str())) < 0) {
			rspdata = createResponse2(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		//repairServer
		return repairServer(servTy, typeKey->second, name, status, rspdata);
	} while (0);
	return false;
}

void LoginServ::repairServerNotify(std::string const& msg, std::string& rspdata) {
	std::string errmsg;
	servTyE servTy;
	std::string name;
	int status;
	std::stringstream ss(msg);
	boost::property_tree::ptree root;
	boost::property_tree::read_json(ss, root);
	try {
		do {
			//type
			std::string servname = root.get<std::string>("type");
			if (servname == "HallServer") {
				servTy = servTyE::kHallTy;
			}
			else if (servname == "GameServer") {
				servTy = servTyE::kGameTy;
			}
			else {
				rspdata = createResponse2(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//name
			name = root.get<std::string>("name");
			if (name.empty()) {
				rspdata = createResponse2(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//status
			status = root.get<int>("status");
			if (status < 0) {
				rspdata = createResponse2(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//repairServer
			repairServer(servTy, servname, name, status, rspdata);
		} while (0);
	}
	catch (boost::property_tree::ptree_error& e) {
		_LOG_ERROR(e.what());
	}
}