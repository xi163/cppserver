#ifndef _MUDUO_NET_WEBSOCKET_ICONTEXT_H_
#define _MUDUO_NET_WEBSOCKET_ICONTEXT_H_

#include "Logger/src/utils/utils.h"

#include <libwebsocket/base.h>
#include <libwebsocket/IBytesBuffer.h>
#include <libwebsocket/ICallback.h>
#include <libwebsocket/IHttpContext.h>

namespace muduo {
	namespace net {
		namespace websocket {
			
			class IContext {
			public:
				virtual void resetAll() = 0;
				// ~Context_ -> ~IContext
				virtual ~IContext() {
					//Debugf("...");
				}
			};

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif