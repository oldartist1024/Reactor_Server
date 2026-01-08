#pragma once    
#include "EventLoop.h"
#include "ThreadPool.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
struct Listener
{
    int lfd;
    unsigned short port;
};
struct TcpServer
{
    int threadNum;
    struct EventLoop* mainLoop;
    struct ThreadPool* threadPool;
    struct Listener* listener;
};
// 初始化
TcpServer* tcpServerInit(unsigned short port, int threadNum);
// 初始化监听
Listener* listenerInit(unsigned short port);
// 启动服务器
void tcpServerRun(TcpServer* server);