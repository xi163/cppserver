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
				virtual ~IContext() {
					//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
				}
			};

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif