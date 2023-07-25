#ifndef MUDUO_NET_WEBSOCKET_CONTEXT_H
#define MUDUO_NET_WEBSOCKET_CONTEXT_H

#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>
#include <libwebsocket/websocket.h>

namespace muduo {
    namespace net {
        namespace websocket {

			class Context : public muduo::noncopyable, public websocket::ICallbackHandler {
			public:
				class Holder {
				public:
					Holder(Context* owner);
					~Holder();
					websocket::IContext* ptr_;
				};
			public:
				explicit Context(const muduo::net::WeakTcpConnectionPtr& weakConn);
				~Context();
			public:
				inline void setWsConnectedCallback(WsConnectedCallback const& cb) {
					wsConnectedCallback_ = cb;
				}
				inline void setWsMessageCallback(WsMessageCallback const& cb) {
					wsMessageCallback_ = cb;
				}
				inline void setWsClosedCallback(WsClosedCallback const& cb) {
					wsClosedCallback_ = cb;
				}
				inline void parse_message_frame(IBytesBuffer* buf, ITimestamp* receiveTime) {
					//////////////////////////////////////////////////////////////////////////
					//parse_message_frame
					//////////////////////////////////////////////////////////////////////////
					websocket::parse_message_frame(getContext(), buf, receiveTime);
				}
			private:
				void send(const void* message, int len);
				void sendMessage(std::string const& message);
				void sendMessage(IBytesBuffer* message);
				void shutdown();
				void forceClose();
				void forceCloseWithDelay(double seconds);
				std::string peerIpAddrToString() const;
				void onConnectedCallback(std::string const& ipaddr);
				void onMessageCallback(IBytesBuffer* buf, int msgType, ITimestamp* receiveTime);
				void onClosedCallback(IBytesBuffer* buf, ITimestamp* receiveTime);
			private:
				inline websocket::IContext* getContext() {
					assert(holder_);
#if 1
					return holder_.get();
#else
					assert(holder_->ptr_);
					return holder_->ptr_;
#endif
				}
#if 1
				std::unique_ptr<websocket::IContext> holder_;
#else
				std::unique_ptr<Holder> holder_;
#endif
				muduo::net::WeakTcpConnectionPtr weakConn_;
				WsConnectedCallback wsConnectedCallback_;
				WsMessageCallback wsMessageCallback_;
				WsClosedCallback wsClosedCallback_;
			};

			typedef std::unique_ptr<Context> ContextPtr;

        }//namespace websocket

		typedef websocket::ContextPtr WsContextPtr;

    }//namespace net
}//namespace muduo

#endif
