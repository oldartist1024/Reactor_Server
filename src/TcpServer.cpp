#include "TcpServer.h"

TcpServer *tcpServerInit(unsigned short port, int threadNum)
{
    TcpServer *server = (TcpServer *)malloc(sizeof(TcpServer));
    server->threadNum = threadNum;
    server->mainLoop = EventLoopInit_t();
    server->threadPool = threadPoolInit(server->mainLoop, threadNum);
    server->listener = listenerInit(port);
    return server;
}

Listener *listenerInit(unsigned short port)
{
    Listener *listener = (Listener *)malloc(sizeof(Listener));
    // 1. 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("setsockopt SO_REUSEADDR");
        return nullptr;
    }
    // 2. 将socket()返回值和本地的IP端口绑定到一起
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // 大端端口
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        exit(0);
    }
    // 3. 设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        exit(0);
    }
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}
int acceptConnection(void *arg)
{
    TcpServer *server = (TcpServer *)arg;
    int cfd = accept(server->listener->lfd, nullptr, nullptr);
    if (cfd == -1)
    {
        perror("accept");
        exit(0);
    }
    EventLoop *workerLoop = takeWorkerEventLoop(server->threadPool);
    // 将其加入tcpconnection中
    TcpConnectionInit(cfd, workerLoop);
    return 0;
}
void tcpServerRun(TcpServer *server)
{
    Debug("服务器程序已经启动了...");
    threadPoolRun(server->threadPool);
    Channel *channel = ChannelInit(server->listener->lfd, READ_EVENT, acceptConnection, nullptr, nullptr, server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    EventLoopRun(server->mainLoop);
}
