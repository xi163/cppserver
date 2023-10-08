#include "Logger/src/log/Assert.h"
#include "RpcClients.h"

namespace rpc {
	TcpClient::TcpClient(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& serverAddr,
		const std::string& name,
		Connector* owner)
		: client_(loop, serverAddr, name)
		, owner_(owner)
		, channel_(new muduo::net::RpcChannel())
		/*, stub_(muduo::get_pointer(channel_))*/ {
		client_.setConnectionCallback(
			std::bind(&TcpClient::onConnection, this, std::placeholders::_1));
		client_.setMessageCallback(
			std::bind(&muduo::net::RpcChannel::onMessage,
				muduo::get_pointer(channel_),
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}

	const std::string& TcpClient::name() const {
		return client_.name();
	}

	muduo::net::TcpConnectionPtr TcpClient::connection() const {
		return client_.connection();
	}

	muduo::net::EventLoop* TcpClient::getLoop() const {
		return client_.getLoop();
	}

	void TcpClient::connect() {
		client_.connect();
	}

	void TcpClient::reconnect() {
#if 0
		client_.connect();
#else
		client_.reconnect();
#endif
	}

	void TcpClient::disconnect() {
		client_.disconnect();
	}

	void TcpClient::stop() {
		client_.stop();
	}

	bool TcpClient::retry() const {
		return client_.retry();
	}

	void TcpClient::enableRetry() {
		client_.enableRetry();
	}

	void TcpClient::onConnection(const muduo::net::TcpConnectionPtr& conn) {

		conn->getLoop()->assertInLoopThread();

		if (conn->connected()) {
			//channel_.reset(new muduo::net::RpcChannel(conn));
			channel_->setConnection(conn);
			//client_.setMessageCallback(
			//	std::bind(&muduo::net::RpcChannel::onMessage,
			//		muduo::get_pointer(channel_),
			//		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			owner_->onConnected(conn, shared_from_this());
		}
		else {
			//channel_.reset();
			owner_->onClosed(conn, client_.name());
		}
	}

	void TcpClient::onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
		owner_->onMessage(conn, buf, receiveTime);
	}

	Connector::Connector(
		muduo::net::EventLoop* loop)
		: loop_(ASSERT_NOTNULL(loop)) {
	}

	Connector::~Connector() {
		closeAll();
	}

	void Connector::add(
		std::string const& name,
		const muduo::net::InetAddress& serverAddr) {
		QueueInLoop(loop_,
			std::bind(&Connector::addInLoop, this, name, serverAddr));
	}

	void Connector::remove(std::string const& name, bool lazy) {
		QueueInLoop(loop_,
			std::bind(&Connector::removeInLoop, this, name, lazy));
	}

	void Connector::check(std::string const& name, bool exist) {
		QueueInLoop(loop_,
			std::bind(&Connector::checkInLoop, this, name, exist));
	}

	bool Connector::exists(std::string const& name) /*const*/ {
		bool ok = false;
		bool exist = false;
		QueueInLoop(loop_,
			std::bind(&Connector::existInLoop, this, name, std::ref(exist), std::ref(ok)));
		//spin lock until asynchronous return
		while (!ok);
		return exist;
	}

	size_t const Connector::count() /*const*/ {
		bool ok = false;
		size_t size = 0;
		QueueInLoop(loop_,
			std::bind(&Connector::countInLoop, this, std::ref(size), std::ref(ok)));
		//spin lock until asynchronous return
		while (!ok);
		return size;
	}

	void Connector::get(std::string const& name, ClientConn& client) {
		bool ok = false;
		QueueInLoop(loop_,
			std::bind(&Connector::getInLoop, this, name, std::ref(client), std::ref(ok)));
		//spin lock until asynchronous return
		while (!ok);
	}

	void Connector::getAll(ClientConnList& clients) {
		ASSERT(clients.size() == 0);
		bool ok = false;
		QueueInLoop(loop_,
			std::bind(&Connector::getAllInLoop, this, std::ref(clients), std::ref(ok)));
		//spin lock until asynchronous return
		while (!ok);
	}

	void Connector::getInLoop(std::string const& name, ClientConn& client, bool& ok) {

		loop_->assertInLoopThread();
#if 0
		TcpClientMap::const_iterator it = clients_.find(name);
#else
		TcpClientMap::const_iterator it = std::find_if(clients_.begin(), clients_.end(), [&](TcpClientPair const& kv) -> bool {
			return strncmp(kv.first.c_str(), name.c_str(), std::min(kv.first.size(), name.size())) == 0;
			});
#endif
		if (it != clients_.end()) {
			if (it->second->connection() &&
				it->second->connection()->connected()) {
				//client.first = it->first;
				//client.second = it->second->connection();
				client = boost::make_tuple<std::string,
					muduo::net::WeakTcpConnectionPtr,
					muduo::net::WeakRpcChannelPtr>(it->first, it->second->connection(), it->second->channel());
			}
			else {
#if 0
				muduo::net::WeakTcpConnectionPtr weakConn;
				muduo::net::WeakRpcChannelPtr weakChannel;
				client = boost::make_tuple<std::string,
					muduo::net::WeakTcpConnectionPtr,
					muduo::net::WeakRpcChannelPtr>(it->first, weakConn, weakChannel);
#endif
			}
		}
		else {
#if 0
			muduo::net::WeakTcpConnectionPtr weakConn;
			muduo::net::WeakRpcChannelPtr weakChannel;
			client = boost::make_tuple<std::string,
				muduo::net::WeakTcpConnectionPtr,
				muduo::net::WeakRpcChannelPtr>(name, weakConn, weakChannel);
#endif
		}
		ok = true;
	}

	void Connector::getAllInLoop(ClientConnList& clients, bool& ok) {

		loop_->assertInLoopThread();

		for (TcpClientMap::const_iterator it = clients_.begin();
			it != clients_.end(); ++it) {
			if (it->second->connection() &&
				it->second->connection()->connected()) {
				//clients.emplace_back(ClientConn(it->first, it->second->connection(),it->second->channel()));
				clients.emplace_back(boost::make_tuple<std::string,
					muduo::net::WeakTcpConnectionPtr,
					muduo::net::WeakRpcChannelPtr>(it->first, it->second->connection(), it->second->channel()));
			}
			else {
#if 0
				muduo::net::WeakTcpConnectionPtr weakConn;
				muduo::net::WeakRpcChannelPtr weakChannel;
				clients.emplace_back(boost::make_tuple<std::string,
					muduo::net::WeakTcpConnectionPtr,
					muduo::net::WeakRpcChannelPtr>(it->first, weakConn, weakChannel));
#endif
			}
		}
		ok = true;
	}

	void Connector::addInLoop(
		std::string const& name,
		const muduo::net::InetAddress& serverAddr) {

		loop_->assertInLoopThread();

		TcpClientMap::iterator it = clients_.find(name);
		if (it == clients_.end()) {
			//name新节点
			TcpClientPtr client(new TcpClient(loop_, serverAddr, name, this));
			Warnf("[RPC]添加节点 name = %s", client->name().c_str());
			//192.168.2.93:20000
			clients_[client->name()] = client;
			client->enableRetry();
			client->connect();
		}
		else {
			//name已存在
			TcpClientPtr& client = it->second;
			if (client) {
				if (!client->connection() ||
					!client->connection()->connected()) {
					//连接断开则重连
					if (!client->retry()) {
						Warnf("[RPC]重连节点 name = %s", client->name().c_str());
						client->reconnect();
					}
				}
				else {
					ASSERT(
						client->connection() &&
						client->connection()->connected());
				}
			}
			else {
				it->second.reset(new TcpClient(loop_, serverAddr, name, this));
				Warnf("[RPC]重建节点 name = %s", name.c_str());
				it->second->enableRetry();
				it->second->connect();
			}
		}
	}

	void Connector::countInLoop(size_t& size, bool& ok) {

		loop_->assertInLoopThread();

		size = clients_.size();
		ok = true;
	}

	void Connector::checkInLoop(std::string const& name, bool exist) {

		loop_->assertInLoopThread();
#if 0
		TcpClientMap::const_iterator it = clients_.find(name);
#else
		TcpClientMap::const_iterator it = std::find_if(clients_.begin(), clients_.end(), [&](TcpClientPair const& kv) -> bool {
			return strncmp(kv.first.c_str(), name.c_str(), std::min(kv.first.size(), name.size())) == 0;
			});
#endif
		if (it == clients_.end()) {
			//name不存在
			if (exist) {
				ASSERT(false);
			}
		}
		else {
			//name已存在
			TcpClientPtr const& client = it->second;;
			if (exist) {
				ASSERT(client);
				ASSERT(
					client->connection() &&
					client->connection()->connected());
			}
			else {
				//连接断开
				if (client) {
					ASSERT(
						!client->connection() ||
						!client->connection()->connected());
				}
			}
		}
	}

	void Connector::existInLoop(std::string const& name, bool& exist, bool& ok) {

		loop_->assertInLoopThread();

#if 0
		TcpClientMap::const_iterator it = clients_.find(name);
#else
		TcpClientMap::const_iterator it = std::find_if(clients_.begin(), clients_.end(), [&](TcpClientPair const& kv) -> bool {
			return strncmp(kv.first.c_str(), name.c_str(), std::min(kv.first.size(), name.size())) == 0;
			});
#endif
		exist = (it != clients_.end());
		ok = true;
	}

	void Connector::closeAll() {
		QueueInLoop(loop_,
			std::bind(&Connector::closeAllInLoop, this));
	}

	void Connector::onConnected(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {

		conn->getLoop()->assertInLoopThread();

		int32_t num = numConnected_.incrementAndGet();

		QueueInLoop(loop_,
			std::bind(&Connector::newConnection, this, conn, client));
	}

	void Connector::newConnection(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {

		loop_->assertInLoopThread();
		{
#if 0
			clients_[client->name()] = client;
			conn->setTcpNoDelay(true);
#else
			TcpClientMap::iterator it = clients_.find(client->name());
			ASSERT(it != clients_.end());
#endif
		}

		QueueInLoop(conn->getLoop(), std::bind(&Connector::connectionCallback, this, conn));
	}

	void Connector::onClosed(const muduo::net::TcpConnectionPtr& conn, const std::string& name) {

		conn->getLoop()->assertInLoopThread();

		int32_t num = numConnected_.decrementAndGet();
		//if (num == 0) {
		//	QueueInLoop(conn->getLoop(),
		//		std::bind(&muduo::net::EventLoop::quit, conn->getLoop()));
		//}

		QueueInLoop(loop_,
			std::bind(&Connector::removeConnection, this, conn, name));
	}

	void Connector::removeConnection(const muduo::net::TcpConnectionPtr& conn, const std::string& name) {

		loop_->assertInLoopThread();
		{
#if 1
			if (1 == removes_.erase(name)) {
				//TcpClientMap::const_iterator it = clients_.find(name);
				//ASSERT(it != clients_.end());
				//it->second->stop();
				//it->second.reset();
				//clients_.erase(it);
				QueueInLoop(loop_,
					std::bind(&Connector::removeInLoop, this, name, true));
			}
			else {
				//TcpClientMap::iterator it = clients_.find(name);
				//ASSERT(it != clients_.end());
				//it->second->stop();
			}
#else
			size_t n = clients_.erase(name);
			(void)n;
			ASSERT(n == 1);
#endif
		}
		QueueInLoop(conn->getLoop(), std::bind(&Connector::connectionCallback, this, conn));
	}

	void Connector::connectionCallback(const muduo::net::TcpConnectionPtr& conn) {

		conn->getLoop()->assertInLoopThread();

		if (connectionCallback_) {
			connectionCallback_(conn);
		}
	}

	void Connector::removeInLoop(std::string const& name, bool lazy) {

		loop_->assertInLoopThread();

#if 0
		TcpClientMap::const_iterator it = clients_.find(name);
#else
		TcpClientMap::const_iterator it = std::find_if(clients_.begin(), clients_.end(), [&](TcpClientPair const& kv) -> bool {
			return strncmp(kv.first.c_str(), name.c_str(), std::min(kv.first.size(), name.size())) == 0;
			});
#endif
		if (it != clients_.end()) {
			//连接已经无效直接删除
			if (!it->second->connection() ||
				!it->second->connection()->connected()) {
				Warnf("[RPC]移除节点 name = %s", it->first.c_str());
				it->second->stop();
				clients_.erase(it);
			}
			else if (lazy) {
				//先懒删除，连接关闭回调时清理
				removes_[name] = true;
			}
		}
	}

	void Connector::cleanupInLoop() {

		loop_->assertInLoopThread();

		for (TcpClientMap::const_iterator it = clients_.begin();
			it != clients_.end(); ++it) {
			//连接无效直接删除
			if (!it->second->connection() ||
				!it->second->connection()->connected()) {
				it->second->stop();
				clients_.erase(it);
			}
			else {
				removes_[it->first] = true;
			}
		}
	}

	void Connector::closeAllInLoop() {

		loop_->assertInLoopThread();

		RunInLoop(loop_,
			std::bind(&Connector::cleanupInLoop, this));

		for (TcpClientMap::const_iterator it = clients_.begin();
			it != clients_.end(); ++it) {
			it->second->disconnect();
		}
	}

	void Connector::onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
		if (messageCallback_) {
			messageCallback_(conn, buf, receiveTime);
		}
	}

	//void Connector::quit() {
	//	QueueInLoop(loop_,
	//		std::bind(&muduo::net::EventLoop::quit, loop_));
	//}
}

