#include "proto/sudoku.pb.h"
#include "public/IncMuduo.h"
#include "Logger.h"

using namespace muduo;
using namespace muduo::net;

namespace sudoku
{

class SudokuServiceImpl : public SudokuService
{
 public:
  virtual void Solve(const ::sudoku::SudokuRequestPtr& request,
                     const ::sudoku::SudokuResponse* responsePrototype,
                     const RpcDoneCallback& done)
  {
    Debugf("%s", request->DebugString().c_str());
    SudokuResponse response;
    response.set_solved(true);
    response.set_checkerboard("1234567");
    done(&response);
  }
};

}

int main()
{
  //LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  muduo::net::ReactorSingleton::init(&loop, "RWIOThreadPool");
  muduo::net::ReactorSingleton::setThreadNum(2);
  muduo::net::ReactorSingleton::start();
  InetAddress listenAddr(9981);
  sudoku::SudokuServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
}

