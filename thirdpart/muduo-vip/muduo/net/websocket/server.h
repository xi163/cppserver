#ifndef MUDUO_NET_WEBSOCKET_SERVER_H
#define MUDUO_NET_WEBSOCKET_SERVER_H

#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>
#include <libwebsocket/websocket.h>

namespace muduo {
    namespace net {
        namespace websocket {

			void hook(
				const WsConnectedCallback& ccb,
				const WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn,
				std::string const& path_handshake);

			void reset(const muduo::net::TcpConnectionPtr& conn);

			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			void onClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
			void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
			void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);

			class Server : muduo::noncopyable {
			public:
				Server(muduo::net::EventLoop* loop,
					const muduo::net::InetAddress& listenAddr,
					const std::string& name);
				~Server();

				muduo::net::EventLoop* getLoop() const { return server_.getLoop(); }

				void setThreadNum(int numThreads);

				void start(bool et = false);

				static void hook(
					const WsConnectedCallback& ccb,
					const WsMessageCallback& mcb,
					const muduo::net::TcpConnectionPtr& conn,
					std::string const& path_handshake);

				static void reset(const muduo::net::TcpConnectionPtr& conn);

				static void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
				static void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
				static void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);

			public:
				muduo::net::TcpServer server_;
			};

        }//namespace websocket
    }//namespace net
}//namespace muduo

#endif
