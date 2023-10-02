#include <muduo/net/EventLoop.h>

#include <libwebsocket/websocket.h>
//#include <libwebsocket/ssl.h>
#include <muduo/net/websocket/context.h>
#include <muduo/net/websocket/server.h>

namespace muduo {
	namespace net {
		namespace websocket {
			
			/// <summary>
			/// Only connects but does not send messages, It should be closed after a timeout
			/// </summary>
			/// <param name="conn"></param>
			/// <param name="buf"></param>
			/// <param name="receiveTime"></param>
			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
				assert(conn);
				conn->getLoop()->assertInLoopThread();

				websocket::ContextPtr& context = conn->getWsContext();
				assert(context);
				//////////////////////////////////////////////////////////////////////////
				//parse_message_frame
				//////////////////////////////////////////////////////////////////////////
				context->parse_message_frame(buf, &receiveTime);
			}

			void onClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
				assert(conn);
				conn->getLoop()->assertInLoopThread();
				Warnf("websocket::onClosed - %s", conn->peerAddress().toIpPort().c_str());
				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_close_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				websocket::pack_unmask_close_frame(&rspdata, buf->peek(), buf->readableBytes());
				conn->send(&rspdata);
			}
			
			void hook(
				const muduo::net::wsVerifyCallback& vcb,
				const muduo::net::WsConnectedCallback& ccb,
				const muduo::net::WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn,
				std::string const& path_handshake) {
				conn->getLoop()->assertInLoopThread();
				//////////////////////////////////////////////////////////////////////////
				//websocket::Context::ctor
				//////////////////////////////////////////////////////////////////////////
				websocket::ContextPtr context(new muduo::net::websocket::Context(conn, path_handshake));
				{
					//wsVerifyCallback
					context->setWsVerifyCallback(vcb);
					//WsConnectedCallback
					context->setWsConnectedCallback(ccb);
					//WsClosedCallback
					context->setWsClosedCallback(
						std::bind(
							&muduo::net::websocket::onClosed,
							std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
					//WsMessageCallback
					context->setWsMessageCallback(mcb);
				}
				conn->setWsContext(context);
			}

			void reset(const muduo::net::TcpConnectionPtr& conn) {
				conn->getLoop()->assertInLoopThread();
				//////////////////////////////////////////////////////////////////////////
				//websocket::Context::dtor
				//////////////////////////////////////////////////////////////////////////
				conn->getWsContext().reset();
			}

			void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len, MessageT msgType) {
				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_data_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				websocket::pack_unmask_data_frame(&rspdata, data, len, msgType, false);
				conn->send(&rspdata);
			}

			void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len, MessageT msgType) {
				websocket::send(conn, (char const*)data, len, msgType);
			}

			void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data, MessageT msgType) {
				websocket::send(conn, (char const*)&data[0], data.size(), msgType);
			}
			
			Server::Server(muduo::net::EventLoop* loop,
				const muduo::net::InetAddress& listenAddr,
				const std::string& name)
				: server_(loop, listenAddr, name) {
				//MessageCallback
				server_.setMessageCallback(
					std::bind(&muduo::net::websocket::onMessage,
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#if 0
				//添加OpenSSL认证支持
				muduo::net::ssl::SSL_CTX_Init(
					cert_path,
					private_key_path,
					client_ca_cert_file_path, client_ca_cert_dir_path);
				//指定SSL_CTX
				server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
#endif
			}

			Server::~Server() {
				muduo::net::ssl::SSL_CTX_free();
			}

			void Server::setThreadNum(int numThreads) {
				server_.setThreadNum(numThreads);
			}

			void Server::start(bool et) {
				server_.start(et);
			}

			void Server::hook(
				const muduo::net::wsVerifyCallback& vcb,
				const muduo::net::WsConnectedCallback& ccb,
				const muduo::net::WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn,
				std::string const& path_handshake) {

				muduo::net::websocket::hook(vcb, ccb, mcb, conn, path_handshake);
			}

			void Server::reset(const muduo::net::TcpConnectionPtr& conn) {
				muduo::net::websocket::reset(conn);
			}

			void Server::send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len, MessageT msgType) {
				muduo::net::websocket::send(conn, data, len, msgType);
			}

			void Server::send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len, MessageT msgType) {
				send(conn, (char const*)data, len, msgType);
			}
			
			void Server::send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data, MessageT msgType) {
				send(conn, (char const*)&data[0], data.size(), msgType);
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo