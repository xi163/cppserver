#ifndef INCLUDE_RPCCLIENTS_H
#define INCLUDE_RPCCLIENTS_H

#include "Logger/src/Macro.h"
#include "public/IncMuduo.h"

namespace rpc {
	namespace client {
		typedef muduo::net::RpcChannel::ClientDoneCallback Done;
	}
	class Connector;

	class TcpClient :
		public muduo::noncopyable,
		public std::enable_shared_from_this<TcpClient> {
	public:
		TcpClient(muduo::net::EventLoop* loop,
			const muduo::net::InetAddress& serverAddr,
			const std::string& name, Connector* owner);
		
		const std::string& name() const;
		muduo::net::TcpConnectionPtr connection() const;
		muduo::net::EventLoop* getLoop() const;
		muduo::net::RpcChannelPtr channel() { return channel_; }

		void connect();
		void reconnect();
		void disconnect();
		void stop();

		bool retry() const;
		void enableRetry();

	private:
		void onConnection(
			const muduo::net::TcpConnectionPtr& conn);
		void onMessage(
			const muduo::net::TcpConnectionPtr& conn,
			muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
	private:
		muduo::net::RpcChannelPtr channel_;
		muduo::net::TcpClient client_;
		Connector* owner_;
	};

	typedef std::shared_ptr<TcpClient> TcpClientPtr;
	typedef std::weak_ptr<TcpClient> WeakTcpClientPtr;

	typedef boost::tuple<std::string,
		muduo::net::WeakTcpConnectionPtr,
		muduo::net::WeakRpcChannelPtr> ClientConn;
	typedef std::vector<ClientConn> ClientConnList;

	class Connector : muduo::noncopyable {
	public:
		friend class TcpClient;
	public:
		typedef std::pair<std::string, TcpClientPtr> TcpClientPair;
		typedef std::map<std::string, TcpClientPtr> TcpClientMap;
	public:
		Connector(muduo::net::EventLoop* loop);
		~Connector();

		void setConnectionCallback(const muduo::net::ConnectionCallback& cb)
		{
			connectionCallback_ = cb;
		}
		void setMessageCallback(const muduo::net::MessageCallback& cb)
		{
			messageCallback_ = cb;
		}
		void add(
			std::string const& name,
			const muduo::net::InetAddress& serverAddr);
		void remove(std::string const& name, bool lazy = false);
		void check(std::string const& name, bool exist);
		bool exists(std::string const& name) /*const*/;
		size_t const count() /*const*/;
		void get(std::string const& name, ClientConn& client);
		void getAll(ClientConnList& clients);
		void closeAll();
	protected:
		void onConnected(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client);
		void onClosed(const muduo::net::TcpConnectionPtr& conn, const std::string& name);
		void onMessage(
			const muduo::net::TcpConnectionPtr& conn,
			muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

		void addInLoop(
			std::string const& name,
			const muduo::net::InetAddress& serverAddr);

		void newConnection(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client);
		void removeConnection(const muduo::net::TcpConnectionPtr& conn, const std::string& name);

		void connectionCallback(const muduo::net::TcpConnectionPtr& conn);

		void countInLoop(size_t& size, bool& ok);
		void checkInLoop(std::string const& name, bool exist);
		void existInLoop(std::string const& name, bool& exist, bool& ok);
		void getInLoop(std::string const& name, ClientConn& client, bool& ok);
		void getAllInLoop(ClientConnList& clients, bool& ok);
		void removeInLoop(std::string const& name, bool lazy);
		void cleanupInLoop();
		void closeAllInLoop();
	private:
		muduo::net::EventLoop* loop_;
		TcpClientMap clients_;
		std::map<std::string, bool> removes_;
		muduo::AtomicInt32 numConnected_;
		muduo::net::ConnectionCallback connectionCallback_;
		muduo::net::MessageCallback messageCallback_;
	};
}

#endif