#include "TcpServer.h"
// 主反应堆接收到新连接的回调函数
int TcpServer::acceptConnection(void *arg)
{
    TcpServer *server = (TcpServer *)arg;
    // 建立连接，获取用于通信的文件描述符
    int cfd = accept(server->m_lfd, nullptr, nullptr);
    if (cfd == -1)
    {
        perror("accept");
        exit(0);
    }
    // 从线程池轮询获取一个工作反应堆
    EventLoop *workerLoop = server->m_threadPool->takeWorkerEventLoop();
    // 创建tcp连接，另那个工作反应堆去处理这段连接工作
    new TcpConnection(cfd, workerLoop);
    return 0;
}

// 初始化服务器
TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_threadNum = threadNum;
    // 主反应堆初始化
    m_mainLoop = new EventLoop();
    // 线程池初始化
    m_threadPool = new ThreadPool(m_mainLoop, threadNum);
    // 监听初始化
    listenerInit(port);
}
// 初始化监听
void TcpServer::listenerInit(unsigned short port)
{
    // 1. 创建监听的套接字，获取监听文件描述符
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
        exit(0);
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
    m_lfd = lfd;
    m_port = port;
}
// 启动服务器
void TcpServer::Run()
{
    Debug("服务器程序已经启动了...");
    // 线程池启动
    m_threadPool->threadPoolRun();
    // 将用于监听以接受新连接的channel加入主反应堆
    Channel *channel = new Channel(m_lfd, EventType::READ_EVENT, acceptConnection, nullptr, nullptr, this);
    m_mainLoop->AddTask(channel, ElementType::ADD);
    // 启动主反应堆
    m_mainLoop->Run();
}