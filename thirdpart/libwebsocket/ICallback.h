#ifndef _MUDUO_NET_WEBSOCKET_ICALLBACK_H_
#define _MUDUO_NET_WEBSOCKET_ICALLBACK_H_

#include "Logger/src/utils/utils.h"

#include <libwebsocket/base.h>
#include <libwebsocket/ITimestamp.h>
#include <libwebsocket/IBytesBuffer.h>

namespace muduo {
	namespace net {
		namespace websocket {
			
			class ICallback {
			public:
				virtual void send(const void* message, int len)      = 0;
				virtual void sendMessage(std::string const& message) = 0;
				virtual void sendMessage(IBytesBuffer* message)      = 0;
				virtual void shutdown()   = 0;
				virtual void forceClose() = 0;
				virtual void forceCloseWithDelay(double seconds) = 0;
				virtual std::string peerIpAddrToString() const   = 0;
				virtual void onConnectedCallback(std::string const& ipaddr)                             = 0;
				virtual void onMessageCallback(IBytesBuffer* buf, int msgType, ITimestamp* receiveTime) = 0;
				virtual void onClosedCallback(IBytesBuffer* buf, ITimestamp* receiveTime)               = 0;
			};

			typedef ICallback IHandler, ICallbackHandler;

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif