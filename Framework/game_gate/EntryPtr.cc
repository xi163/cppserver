#include "EntryPtr.h"
#include "Response/response.h"

static inline void onIdleTimeout(const muduo::net::TcpConnectionPtr& conn, Entry::TypeE ty) {
	switch (ty) {
	case Entry::TypeE::HttpTy: {
		muduo::net::HttpResponse rsp(true);
		response::text::Result(
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 505 timeout\r\n\r\n", rsp);
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
		break;
	}
	case Entry::TypeE::TcpTy: {
#if 0
		//不再发送数据
		conn->shutdown();
#elif 1
		conn->forceClose();
#elif 0
		conn->forceCloseWithDelay(0.2f);
#endif
		break;
	}
	default: ASSERT(false); break;
	}
}

Entry::~Entry() {
	//触发析构调用销毁对象释放资源，有以下两种可能：
	//-------------------------------------------------------------------------------------------------------------------------------------
	//  1.弹出bucket，EntryPtr引用计数递减为0
	//   (conn有效，此时已经空闲超时，因为业务处理队列繁忙以致无法持有EntryPtr，进而锁定同步业务操作)
	//   (conn失效，此时连接已经关闭，可能是业务处理完毕或其它非法原因导致连接被提前关闭，但是由于Bucket一直持有EntryPtr引用计数而无法触发析构，直到空闲超时)
	//-------------------------------------------------------------------------------------------------------------------------------------
	//  2.业务处理完毕，持有EntryPtr业务处理函数退出离开其作用域，EntryPtr引用计数递减为0
	//   (此时早已空闲超时，并已弹出bucket，引用计数递减但不等于0，因为业务处理函数持有EntryPtr，锁定了同步业务操作，直到业务处理完毕，引用计数递减为0触发析构)
	muduo::net::TcpConnectionPtr conn(weakConn_.lock());
	if (conn) {
		//conn->getLoop()->assertInLoopThread();

		Context& entryContext = boost::any_cast<Context&>(conn->getContext());
		//ASSERT(!entryContext.getSession().empty());

		//判断是否锁定了同步业务操作
		switch (getLocked()) {
		case true: {
			//////////////////////////////////////////////////////////////////////////
			//早已空闲超时，业务处理完毕，响应客户端时间(>timeout)
			//////////////////////////////////////////////////////////////////////////
			Warnf("%s[%s] <- %s[%s] finished processing",
				localName_.c_str(), conn->localAddress().toIpPort().c_str(),
				peerName_.c_str(), conn->peerAddress().toIpPort().c_str(),
				entryContext.getSession().c_str());
			break;
		}
		default: {
			//////////////////////////////////////////////////////////////////////////
			//已经空闲超时，没有业务处理，响应客户端时间(<timeout)
			//////////////////////////////////////////////////////////////////////////
			Warnf("%s[%s] <- %s[%s] timeout closing",
				localName_.c_str(), conn->localAddress().toIpPort().c_str(),
				peerName_.c_str(), conn->peerAddress().toIpPort().c_str(),
				entryContext.getSession().c_str());
			onIdleTimeout(conn, ty_);
			break;
		}
		}
	}
	else {
		switch (getLocked()) {
		case true: {
			//////////////////////////////////////////////////////////////////////////
			//业务处理完毕，连接被提前关闭，响应客户端时间(<timeout)
			//////////////////////////////////////////////////////////////////////////
			//Warnf("finished, closed early");
			break;
		}
		default: {
			//////////////////////////////////////////////////////////////////////////
			//其它非法原因，连接被提前关闭，响应客户端时间(<timeout)
			//////////////////////////////////////////////////////////////////////////
			//Warnf("closed early");
			break;
		}
		}
	}
}