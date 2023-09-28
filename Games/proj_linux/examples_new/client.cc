#include "proto/sudoku.pb.h"
#include "public/IncMuduo.h"
#include "Logger.h"
#include "utils.h"

using namespace muduo;
using namespace muduo::net;

class RpcClient : noncopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      client_(loop, serverAddr, "RpcClient"),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_))
  {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, std::placeholders::_1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage,
            get_pointer(channel_),
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      sudoku::SudokuRequest request;
      request.set_checkerboard("001010");
	  Warnf("...");
      stub_.Solve(request, std::bind(&RpcClient::solved, this, std::placeholders::_1));
	}
  }

  void solved(const sudoku::SudokuResponsePtr& resp)
  {
	Debugf("solved:\n%s", resp->DebugString().c_str());
	client_.disconnect();
	//loop_->quit();
	//muduo::net::ReactorSingleton::stop();
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  sudoku::SudokuService::Stub stub_;
};

int main(int argc, char* argv[])
{
    utils::setrlimit();

  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
	muduo::net::ReactorSingleton::inst(&loop, "RWIOThreadPool");
	muduo::net::ReactorSingleton::setThreadNum(2);
	muduo::net::ReactorSingleton::start();
    InetAddress serverAddr(argv[1], 9981);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}

