
#include <iostream>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

// #include <functional>
// #include <memory>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpServer.h>

using namespace muduo;
using namespace muduo::net;

int main()
{
	muduo::net::TcpConnectionCallback cb;
	muduo::net::TcpConnectionPtr ptr;

	return 0;
}
