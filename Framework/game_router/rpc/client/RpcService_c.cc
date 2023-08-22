#include "RpcService_c.h"

#include "RpcClients.h"

namespace rpc {
	namespace client {
		Service::Service(const ClientConn& conn, int timeout)
			: conn_(conn), lock_(timeout) {
		}
		
		::Game::Rpc::NodeInfoRspPtr Service::GetNodeInfo(
			const ::Game::Rpc::NodeInfoReq& req) {
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {

				//lock()线程安全
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
					if (channel) {
						channel->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
						stub.GetNodeInfo(req, std::bind(&Service::doneNodeInfoRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			//}
			return ptrNodeInfoRsp_;
		}
		
		//FIXME: 声明 rpc::client::Service client(conn, 3) 临时对象
		// 类对象成员函数done回调时类对象已经销毁 导致 done 访问内存地址无效 从而引发崩溃
		void Service::doneNodeInfoRsp(const ::Game::Rpc::NodeInfoRspPtr& rsp) {
			ptrNodeInfoRsp_ = rsp;
			lock_.notify();
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {
				
				//lock()线程安全
// 				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
// 				if (c) {
// 					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
// 					if (channel) {
// 						channel->setConnection(c);
// 						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
// 						//stub.channel()->connection()->conn->shutdown();
// 						//stub.channel()->connection()->conn->forceClose();
// 					}
// 				}
			//}
		}

		::Game::Rpc::UserScoreRspPtr Service::NotifyUserScore(
			const ::Game::Rpc::UserScoreReq& req) {
			_LOG_WARN("userId: %lld score: %lld", req.userid(), req.score());
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {
				
				//lock()线程安全
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
					if (channel) {
						channel->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
						stub.NotifyUserScore(req, rpc::client::Done());
						//lock_.wait();
					}
				}
			//}
			return ptrUserScoreRsp_;
		}
		
		//FIXME: 声明 rpc::client::Service client(conn, 3) 临时对象
		// 类对象成员函数done回调时类对象已经销毁 导致 done 访问内存地址无效 从而引发崩溃
		void Service::doneUserScoreReq(const ::Game::Rpc::UserScoreRspPtr& rsp) {
			//ptrUserScoreRsp_ = rsp;
			//lock_.notify();
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {
				
				//lock()线程安全
// 				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
// 				if (c) {
// 					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
// 					if (channel) {
// 						channel->setConnection(c);
// 						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
// 						//stub.channel()->connection()->conn->shutdown();
// 						//stub.channel()->connection()->conn->forceClose();
// 					}
// 				}
			//}
		}
		
		::Game::Rpc::RoomInfoRspPtr Service::GetRoomInfo(
			const ::Game::Rpc::RoomInfoReq& req) {
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {
				
				//lock()线程安全
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
					if (channel) {
						channel->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
						stub.GetRoomInfo(req, std::bind(&Service::doneRoomInfoRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			//}
			return ptrRoomInfoRsp_;
		}
		
		//FIXME: 声明 rpc::client::Service client(conn, 3) 临时对象
		// 类对象成员函数done回调时类对象已经销毁 导致 done 访问内存地址无效 从而引发崩溃
		void Service::doneRoomInfoRsp(const ::Game::Rpc::RoomInfoRspPtr& rsp) {
			ptrRoomInfoRsp_ = rsp;
			lock_.notify();
			
			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {
				
			//lock()线程安全
// 				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
// 				if (c) {
// 					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
// 					if (channel) {
// 						channel->setConnection(c);
// 						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
// 						//stub.channel()->connection()->conn->shutdown();
// 						//stub.channel()->connection()->conn->forceClose();
// 					}
// 				}
			//}
		}

		::Game::Rpc::TableInfoRspPtr Service::GetTableInfo(
			const ::Game::Rpc::TableInfoReq& req) {

			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {

				//lock()线程安全
				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
				if (c) {
					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
					if (channel) {
						channel->setConnection(c);
						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
						stub.GetTableInfo(req, std::bind(&Service::doneTableInfoRsp, this, std::placeholders::_1));
						lock_.wait();
					}
				}
			//}
			return ptrTableInfoRsp_;
		}

		//FIXME: 声明 rpc::client::Service client(conn, 3) 临时对象
		// 类对象成员函数done回调时类对象已经销毁 导致 done 访问内存地址无效 从而引发崩溃
		void Service::doneTableInfoRsp(const ::Game::Rpc::TableInfoRspPtr& rsp) {
			ptrTableInfoRsp_ = rsp;
			lock_.notify();

			//expired()非线程安全
			//if (!conn_.get<1>().expired()) {

				//lock()线程安全
// 				muduo::net::TcpConnectionPtr c(conn_.get<1>().lock());
// 				if (c) {
// 					muduo::net::RpcChannelPtr channel(conn_.get<2>().lock());
// 					if (channel) {
// 						channel->setConnection(c);
// 						::Game::Rpc::RpcService_Stub stub(muduo::get_pointer(channel));
// 						//stub.channel()->connection()->conn->shutdown();
// 						//stub.channel()->connection()->conn->forceClose();
// 					}
// 				}
			//}
		}
	}
}