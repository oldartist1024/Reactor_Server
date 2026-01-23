#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
// 监听结构体
struct Listener
{
    int lfd;             // 监听的套接字
    unsigned short port; // 端口
};
struct TcpServer
{
    int threadNum;                 // 线程数量
    struct EventLoop *mainLoop;    // 主反应堆
    struct ThreadPool *threadPool; // 线程池
    struct Listener *listener;     // 监听结构体
};
// 初始化服务器
TcpServer *tcpServerInit(unsigned short port, int threadNum);
// 初始化监听
Listener *listenerInit(unsigned short port);
// 启动服务器
void tcpServerRun(TcpServer *server);