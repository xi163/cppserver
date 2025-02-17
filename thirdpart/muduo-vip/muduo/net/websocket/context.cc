#include <muduo/net/EventLoop.h>

#include <libwebsocket/websocket.h>
//#include <libwebsocket/ssl.h>
#include <muduo/net/websocket/context.h>

namespace muduo {
	namespace net {
		namespace websocket {

			static inline IContext* create(Context* owner) {
				IHttpContextPtr a(new muduo::net::HttpContext());
				IBytesBufferPtr b(new muduo::net::Buffer());
				IBytesBufferPtr c(new muduo::net::Buffer());
				return websocket::create(owner, a, b, c);
			}

			Context::Holder::Holder(Context* owner) {
				ptr_ = create(owner);
			}
			
			Context::Holder::~Holder() {
				websocket::free(ptr_);
				ptr_ = NULL;
			}

			Context::Context(const muduo::net::WeakTcpConnectionPtr& weakConn, std::string const& path)
				: weakConn_(weakConn),path_handshake_(path) {
#if 1
				holder_.reset(create(this));
#else
				holder_.reset(new Holder(this));
#endif
			}
			
			// ~Context -> ~Context_ -> ~IContext
			Context::~Context() {
				Tracef("...");
			}
			
			void Context::send(const void* data, int len) {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->send(data, len);
				}
			}

			void Context::sendMessage(std::string const& message) {
				send(message.data(), message.size());
			}

			void Context::sendMessage(IBytesBuffer* message) {
				ASSERT(message);
				Buffer* buf = reinterpret_cast<Buffer*>(message);
				ASSERT(buf);
				send(buf->peek(), buf->readableBytes());
			}

			void Context::shutdown() {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->shutdown();
				}
			}

			void Context::forceClose() {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->forceClose();
				}
			}

			void Context::forceCloseWithDelay(double seconds) {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->forceCloseWithDelay(seconds);
				}
			}

			std::string Context::peerIpAddrToString() const {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->getLoop()->assertInLoopThread();
					return conn->peerAddress().toIp();
				}
				return "0.0.0.0";
			}
			
			bool Context::onVerifyCallback(http::IRequest const* request) {
//				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
// 				if (conn) {
// 					conn->getLoop()->assertInLoopThread();
// 
// 					muduo::net::Buffer* buff = reinterpret_cast<muduo::net::Buffer*>(buf);
// 					ASSERT(buff);
// 
// 					muduo::Timestamp* preceiveTime = reinterpret_cast<muduo::Timestamp*>(receiveTime);
// 					ASSERT(preceiveTime);

					if (wsVerifyCallback_) {
						return wsVerifyCallback_(request);
					}
					return true;
//				}
			}
			
			void Context::onConnectedCallback(std::string const& ipaddr) {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->getLoop()->assertInLoopThread();
					if (wsConnectedCallback_) {
						wsConnectedCallback_(conn, ipaddr);
					}
				}
			}

			void Context::onMessageCallback(IBytesBuffer* buf, int msgType, ITimestamp* receiveTime) {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->getLoop()->assertInLoopThread();

					muduo::net::Buffer* buff = reinterpret_cast<muduo::net::Buffer*>(buf);
					ASSERT(buff);

					muduo::Timestamp* preceiveTime = reinterpret_cast<muduo::Timestamp*>(receiveTime);
					ASSERT(preceiveTime);

					if (wsMessageCallback_) {
						wsMessageCallback_(conn, buff, msgType, *preceiveTime);
					}
				}
			}

			void Context::onClosedCallback(IBytesBuffer* buf, ITimestamp* receiveTime) {
				muduo::net::TcpConnectionPtr conn(weakConn_.lock());
				if (conn) {
					conn->getLoop()->assertInLoopThread();

					muduo::net::Buffer* buff = reinterpret_cast<muduo::net::Buffer*>(buf);
					ASSERT(buff);

					muduo::Timestamp* preceiveTime = reinterpret_cast<muduo::Timestamp*>(receiveTime);
					ASSERT(preceiveTime);

					if (wsClosedCallback_) {
						wsClosedCallback_(conn, buff, *preceiveTime);
					}
				}
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo