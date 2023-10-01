#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "../Gate.h"

bool GateServ::onHttpCondition(const muduo::net::InetAddress& peerAddr) {
	std::string country, location;
	std::string ipaddr = peerAddr.toIp();
	ipLocator_.GetAddressByIp(ipaddr.c_str(), location, country);
	Infof("*** ip: %s %s %s", ipaddr.c_str(), country.c_str(), location.c_str());
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == eApiCtrl::kOpenAccept);
	httpserver_.getLoop()->assertInLoopThread();
	{
		//管理员挂维护/恢复服务
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
		if (it != admin_list_.end()) {
			return true;
		}
	}
	{
		//192.168.2.21:3640 192.168.2.21:3667
		std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(peerAddr.ipv4NetEndian());
		return (it != white_list_.end()) && (eApiVisit::kEnable == it->second);
	}
#if 0
	//节点维护中
	if (server_state_ == kRepairing) {
		return false;
	}
#endif
	return true;
}

void GateServ::onHttpConnection(const muduo::net::TcpConnectionPtr& conn) {
	conn->getLoop()->assertInLoopThread();
	if (conn->connected()) {
		int32_t num = numConnected_[KHttpTy].incrementAndGet();
		Infof("网关服[%s] <- WEB端[%s] %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
		numTotalReq_.incrementAndGet();
		if (num > maxConnections_) {
			muduo::net::HttpResponse rsp(false);
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 600 访问量限制(" + std::to_string(maxConnections_) + ")\r\n\r\n");
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
		EntryPtr entry(new Entry(Entry::TypeE::HttpTy, conn, "WEB前端", "网关服"));
		RunInLoop(conn->getLoop(),
			std::bind(&Buckets::push, &boost::any_cast<Buckets&>(conn->getLoop()->getContext()), entry));
		conn->setContext(Context(entry, muduo::net::HttpContext()));
		conn->setTcpNoDelay(true);
		boost::any_cast<Context&>(conn->getContext()).setWorker(nextPool_, threadPool_);
	}
	else {
		int32_t num = numConnected_[KHttpTy].decrementAndGet();
		Infof("网关服[%s] <- WEB端[%s] %s %d %s %d",
			conn->localAddress().toIpPort().c_str(),
			conn->peerAddress().toIpPort().c_str(),
			(conn->connected() ? "UP" : "DOWN"), num);
	}
}

void GateServ::onHttpMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	conn->getLoop()->assertInLoopThread();
	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		return;
	}
	//Errorf("%.*s", buf->readableBytes(), buf->peek());

	//先确定是HTTP数据报文，再解析
	//assert(buf->readableBytes() > 4 && buf->findCRLFCRLF());

	Context& entryContext = boost::any_cast<Context&>(conn->getContext());
	muduo::net::HttpContext& httpContext = boost::any_cast<muduo::net::HttpContext&>(entryContext.getContext());
	if (!httpContext.parseRequest(buf, receiveTime)) {
	}
	else if (httpContext.gotAll()) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == eApiCtrl::kOpen) {
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
				std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
				is_ip_allowed = (it != admin_list_.end());
			}
			if (!is_ip_allowed) {
				READ_LOCK(white_list_mutex_);
				std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(peerAddr.ipv4NetEndian());
				is_ip_allowed = ((it != white_list_.end()) && (eApiVisit::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 1
				conn->forceClose();
#elif 0
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
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
			entryContext.getWorker()->run(
				std::bind(
					&GateServ::asyncHttpHandler,
					this, conn, receiveTime));
		}
		else {
			numTotalBadReq_.incrementAndGet();
			Errorf("entry invalid");
		}
		return;
	}
	muduo::net::HttpResponse rsp(false);
	setFailedResponse(rsp,
		muduo::net::HttpResponse::k404NotFound,
		"HTTP/1.1 400 Bad Request\r\n\r\n");
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

void GateServ::asyncHttpHandler(const muduo::net::WeakTcpConnectionPtr& weakConn, muduo::Timestamp receiveTime) {
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
		if (whiteListControl_ == eApiCtrl::kOpen) {
			bool is_ip_allowed = false;
			{
				READ_LOCK(white_list_mutex_);
				std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.find(conn->peerAddress().ipv4NetEndian());
				is_ip_allowed = ((it != white_list_.end()) && (eApiVisit::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 0
				conn->forceClose();
#else
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
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
		const std::string& connection = httpContext.request().getHeader("Connection");
		bool close = (connection == "close") ||
			(httpContext.request().getVersion() == muduo::net::HttpRequest::kHttp10 && connection != "Keep-Alive");
		muduo::net::HttpResponse rsp(close);
		processHttpRequest(httpContext.request(), rsp, conn->peerAddress(), receiveTime);
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
		Errorf("TcpConnectionPtr.conn invalid");
	}
}

void GateServ::onHttpWriteComplete(const muduo::net::TcpConnectionPtr& conn) {
	Warnf("...");
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

static std::string getRequestStr(muduo::net::HttpRequest const& req) {
	std::string headers;
	for (std::map<std::string, std::string>::const_iterator it = req.headers().begin();
		it != req.headers().end(); ++it) {
		headers += it->first + ": " + it->second + "\n";
	}
	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		<< "<xs:head>" << headers << "</xs:head>"
		<< "<xs:body>"
		<< "<xs:method>" << req.methodString() << "</xs:method>"
		<< "<xs:path>" << req.path() << "</xs:path>"
		<< "<xs:query>" << utils::HTML::Encode(req.query()) << "</xs:query>"
		<< "</xs:body>"
		<< "</xs:root>";
	return ss.str();
}

static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg) {
	params.clear();
	Debugf(queryStr.c_str());
	do {
		std::string subStr;
		std::string::size_type npos = queryStr.find_first_of('?');
		if (npos != std::string::npos) {
			//skip '?'
			subStr = queryStr.substr(npos + 1, std::string::npos);
		}
		else {
			subStr = queryStr;
		}
		if (subStr.empty()) {
			break;
		}
		for (;;) {
			std::string::size_type kpos = subStr.find_first_of('=');
			if (kpos == std::string::npos) {
				break;
			}
			std::string::size_type spos = subStr.find_first_of('&');
			if (spos == std::string::npos) {
				std::string key = subStr.substr(0, kpos);
				//skip '='
				std::string val = subStr.substr(kpos + 1, std::string::npos);
				params[key] = val;
				break;
			}
			else if (kpos < spos) {
				std::string key = subStr.substr(0, kpos);
				//skip '='
				std::string val = subStr.substr(kpos + 1, spos - kpos - 1);
				params[key] = val;
				//skip '&'
				subStr = subStr.substr(spos + 1, std::string::npos);
			}
			else {
				break;
			}
		}
	} while (0);
	std::string keyValues;
	for (auto param : params) {
		keyValues += "\n" + param.first + "=" + param.second;
	}
	//Debugf(keyValues.c_str());
	return true;
}

void GateServ::processHttpRequest(
	const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
	muduo::net::InetAddress const& peerAddr,
	muduo::Timestamp receiveTime) {
	//Infof(getRequestStr(req).c_str());
	rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
	rsp.setStatusMessage("OK");
	//注意要指定connection状态
	rsp.setCloseConnection(true);
	rsp.addHeader("Server", "MUDUO");
	if (req.path() == "/") {
#if 0
		rsp.setContentType("text/html;charset=utf-8");
		std::string now = muduo::Timestamp::now().toFormattedString();
		rsp.setBody("<html><body>Now is " + now + "</body></html>");
#else
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#endif
	}
	else if (req.path() == "/GameHandle") {
		Errorf("%s\n%s", req.methodString(), req.query().c_str());
		rsp.setContentType("application/xml;charset=utf-8");
		rsp.setBody(getRequestStr(req));
	}
	//刷新客户端访问IP黑名单信息
	else if (req.path() == "/refreshBlackList") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			refreshBlackList();
			rsp.setBody("success");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//刷新HTTP访问IP白名单信息
	else if (req.path() == "/refreshWhiteList") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			refreshWhiteList();
			rsp.setBody("success");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
	else if (req.path() == "/repairServer") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			std::string rspdata;
			repairServer(req.query(), rspdata);
			rsp.setContentType("application/json;charset=utf-8");
			rsp.setBody(rspdata);
		}
		else {
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	else if (req.path() == "/help") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, eApiVisit>::const_iterator it = admin_list_.find(peerAddr.ipv4NetEndian());
		if (it != admin_list_.end()) {
			rsp.setContentType("text/html;charset=utf-8");
			rsp.setBody("<html>"
				"<head><title>help</title></head>"
				"<body>"
				"<h4>/refreshAgentInfo</h4>"
				"<h4>/refreshWhiteList</h4>"
				"<h4>/repairServer?type=HallServer&name=192.168.2.158:20001&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
				"<h4>/repairServer?type=GameServer&name=4001:192.168.0.1:5847&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
				"</body>"
				"</html>");
		}
		else {
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	else {
#if 1
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#else
		rsp.setBody("<html><head><title>httpServer</title></head>"
			"<body><h1>Not Found</h1>"
			"</body></html>");
		//rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
#endif
	}
}

void GateServ::refreshWhiteList() {
	if (whiteListControl_ == eApiCtrl::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		RunInLoop(httpserver_.getLoop(), std::bind(&GateServ::refreshWhiteListInLoop, this));
	}
	else if (whiteListControl_ == eApiCtrl::kOpen) {
		//同步刷新IP访问白名单
		refreshWhiteListSync();
	}
}

bool GateServ::refreshWhiteListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == eApiCtrl::kOpen);
	{
		WRITE_LOCK(white_list_mutex_);
		white_list_.clear();
	}
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.begin();
		it != white_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	Debugf("IP访问白名单\n%s", s.c_str());
	return false;
}

bool GateServ::refreshWhiteListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == eApiCtrl::kOpenAccept);
	httpserver_.getLoop()->assertInLoopThread();
	white_list_.clear();
	std::string s;
	for (std::map<in_addr_t, eApiVisit>::const_iterator it = white_list_.begin();
		it != white_list_.end(); ++it) {
		s += std::string("\nipaddr[") + utils::inetToIp(it->first) + std::string("] status[") + std::to_string(it->second) + std::string("]");
	}
	Debugf("IP访问白名单\n%s", s.c_str());
	return false;
}

static void replace(std::string& json, const std::string& placeholder, const std::string& value) {
	boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
}

static std::string createResponse(
	int opType,//status=1
	std::string const& servname,//type=HallSever
	std::string const& name,//name=192.168.2.93:10000
	int errcode, std::string const& errmsg) {
	boost::property_tree::ptree root, data;
	root.put("op", ":op");
	root.put("type", servname);
	root.put("name", name);
	root.put("code", ":code");
	root.put("errmsg", errmsg);
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	replace(json, ":op", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
}

//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
bool GateServ::repairServer(containTy servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata) {
	Warnf("name[%s] status[%d]", name.c_str(), status);
	static std::string path[kMaxContainTy] = {
		"/GAME/game_hall/",
		"/GAME/game_serv/",
	};
	static std::string pathrepair[kMaxContainTy] = {
		"/GAME/game_hallInvalid/",
		"/GAME/game_servInvalid/",
	};
	do {
		//请求挂维护
		if (status == kRepairing) {
			/* 如果之前服务中, 尝试挂维护中, 并返回之前状态
			* 如果返回服务中, 说明刚好挂维护成功, 否则说明之前已被挂维护 */
			//if (kRunning == __sync_val_compare_and_swap(&server_state_, kRunning, kRepairing)) {
			//
			//在指定类型服务中，并且不在维护节点中
			//
			if (clients_[servTy].exist(name) && !clients_[servTy].isRepairing(name)) {
				//当前仅有一个提供服务的节点，禁止挂维护
				if (clients_[servTy].remaining() <= 1) {
					Errorf("当前仅有一个提供服务的节点，禁止挂维护!!!");
					rspdata = createResponse(status, servname, name, 2, "仅剩余一个服务节点，禁止挂维护");
					break;
				}
				//添加 repairnode
				std::string repairnode = pathrepair[servTy] + name;
				//if (ZNONODE == zkclient_->existsNode(repairnode)) {
					//创建维护节点
					//zkclient_->createNode(repairnode, name, true);
					//挂维护中状态
				clients_[servTy].repair(name);
				Errorf("创建维护节点 %s", repairnode.c_str());
				//}
				//删除 servicenode
				std::string servicenode = path[servTy] + name;
				//if (ZNONODE != zkclient_->existsNode(servicenode)) {
					//删除服务节点
					//zkclient_->deleteNode(servicenode);
				Errorf("删除服务节点 %s", servicenode.c_str());
				//}
				rspdata = createResponse(status, servname, name, 0, "success");
			}
			else {
				rspdata = createResponse(status, servname, name, 0, "节点不存在|已挂了维护");
			}
			return true;
		}
		//请求恢复服务
		else if (status == kRunning) {
			/* 如果之前挂维护中, 尝试恢复服务, 并返回之前状态
			* 如果返回挂维护中, 说明刚好恢复服务成功, 否则说明之前已在服务中 */
			//if (kRepairing == __sync_val_compare_and_swap(&server_state_, kRepairing, kRunning)) {
			//
			//在指定类型服务中，并且在维护节点中
			//
			if (clients_[servTy].exist(name) && clients_[servTy].isRepairing(name)) {
				//添加 servicenode
				std::string servicenode = path[servTy] + name;
				//if (ZNONODE == zkclient_->existsNode(servicenode)) {
					//创建服务节点
					//zkclient_->createNode(servicenode, name, true);
					//恢复服务状态
				clients_[servTy].recover(name);
				Errorf("创建服务节点 %s", servicenode.c_str());
				//}
				//删除 repairnode
				std::string repairnode = pathrepair[servTy] + name;
				//if (ZNONODE != zkclient_->existsNode(repairnode)) {
					//删除维护节点
					//zkclient_->deleteNode(repairnode);
				Errorf("删除维护节点 %s", repairnode.c_str());
				//}
				rspdata = createResponse(status, servname, name, 0, "success");
			}
			else {
				rspdata = createResponse(status, servname, name, 0, "节点不存在|已在服务中");
			}
			return true;
		}
		rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
	} while (0);
	return false;
}

bool GateServ::repairServer(std::string const& queryStr, std::string& rspdata) {
	std::string errmsg;
	containTy servTy;
	std::string name;
	int status;
	do {
		HttpParams params;
		if (!parseQuery(queryStr, params, errmsg)) {
			break;
		}
		//type
		//type=HallServer name=192.168.2.158:20001
		//type=GameServer name=4001:192.168.0.1:5847
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty()) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			if (typeKey->second == "HallServer") {
				servTy = containTy::kHallTy;
			}
			else if (typeKey->second == "GameServer") {
				servTy = containTy::kGameTy;
			}
			else {
				rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
				break;
			}
		}
		//name
		HttpParams::const_iterator nameKey = params.find("name");
		if (nameKey == params.end() || nameKey->second.empty()) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			name = nameKey->second;
		}
		//status
		HttpParams::const_iterator statusKey = params.find("status");
		if (statusKey == params.end() || statusKey->second.empty() ||
			(status = atol(statusKey->second.c_str())) < 0) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		//repairServer
		return repairServer(servTy, typeKey->second, name, status, rspdata);
	} while (0);
	return false;
}

void GateServ::repairServerNotify(std::string const& msg, std::string& rspdata) {
	std::string errmsg;
	containTy servTy;
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
				servTy = containTy::kHallTy;
			}
			else if (servname == "GameServer") {
				servTy = containTy::kGameTy;
			}
			else {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//name
			name = root.get<std::string>("name");
			if (name.empty()) {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//status
			status = root.get<int>("status");
			if (status < 0) {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//repairServer
			repairServer(servTy, servname, name, status, rspdata);
		} while (0);
	}
	catch (boost::property_tree::ptree_error& e) {
		Errorf(e.what());
	}
}