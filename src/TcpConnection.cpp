#include "TcpConnection.h"
int processRead(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    int res = bufferSocketRead(conn->ReadBuffer, conn->channel->fd);
    if (res > 0)
    {
        //接收到了 http 请求, 解析http请求
    }
    else
    {
        // 断开连接
    }
    return res;
}
TcpConnection *TcpConnectionInit(int fd, EventLoop *eventloop)
{
    TcpConnection *conn = (TcpConnection *)malloc(sizeof(TcpConnection));
    conn->eventloop = eventloop;
    char name[32];
    sprintf(name, "TcpConnection-%d", fd);
    conn->ReadBuffer = BufferInit(10240);
    conn->WriteBuffer = BufferInit(10240);

    conn->channel = ChannelInit(fd, READ_EVENT, processRead, nullptr, conn);
    eventLoopAddTask(eventloop, conn->channel, ADD);
    return conn;
}