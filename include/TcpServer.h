#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
class TcpServer
{
public:
    // 初始化服务器
    TcpServer(unsigned short port, int threadNum);

    // 初始化监听
    void listenerInit(unsigned short port);
    // 启动服务器
    void Run();
    static int acceptConnection(void *arg);

private:
    int m_threadNum;          // 线程数量
    EventLoop *m_mainLoop;    // 主反应堆
    ThreadPool *m_threadPool; // 线程池
    int m_lfd;                // 监听的套接字
    unsigned short m_port;    // 端口
};
