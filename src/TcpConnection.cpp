#include "TcpConnection.h"
int processRead(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    int res = bufferSocketRead(conn->ReadBuffer, conn->channel->fd);
    Debug("接收到的http请求数据: %s", conn->ReadBuffer->data + conn->ReadBuffer->readPos);
    if (res > 0)
    {
#ifdef MSG_SEND_AUTO
        writeEventEnable(conn->channel, true);
        eventLoopAddTask(conn->eventloop, conn->channel, MODIFY);
#endif
        // 接收到了 http 请求, 解析http请求
        int flag = parseHttpRequest(conn->request, conn->ReadBuffer, conn->response, conn->WriteBuffer, conn->channel->fd);
        if (!flag)
        {
            // 解析失败，发送400错误响应
            char *errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->WriteBuffer, errMsg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        eventLoopAddTask(conn->eventloop, conn->channel, REMOVE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    eventLoopAddTask(conn->eventloop, conn->channel, REMOVE);
#endif
    return res;
}
int processWrite(void *arg)
{
    Debug("开始发送数据了(基于写事件发送)....");
    TcpConnection *conn = (TcpConnection *)arg;
    int count = bufferSendData(conn->WriteBuffer, conn->channel->fd);
    if (count > 0)
    {
        // 当数据全被发出
        if (bufferReadableSize(conn->WriteBuffer) == 0)
        {
            // 把这个连接从事件循环中移除
            eventLoopAddTask(conn->eventloop, conn->channel, REMOVE);
        }
    }
    return count;
}
int tcpConnectionDestroy(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    if (conn)
    {
        if (conn->ReadBuffer && conn->WriteBuffer && bufferReadableSize(conn->WriteBuffer) == 0 && bufferReadableSize(conn->ReadBuffer) == 0)
        {
            destroyChannel(conn->eventloop, conn->channel);
            bufferDestroy(conn->ReadBuffer);
            bufferDestroy(conn->WriteBuffer);
            httpRequestDestroy(conn->request);
            httpResponseDestroy(conn->response);
            free(conn);
        }
    }
    Debug("连接断开, 释放资源, gameover, connName: %s", conn->name);
    return 0;
}
TcpConnection *TcpConnectionInit(int fd, EventLoop *eventloop)
{
    TcpConnection *conn = (TcpConnection *)malloc(sizeof(TcpConnection));
    conn->eventloop = eventloop;
    char name[32];
    sprintf(name, "TcpConnection-%d", fd);
    conn->ReadBuffer = BufferInit(10240);
    conn->WriteBuffer = BufferInit(10240);
    conn->request = HttpRequestInit();
    conn->response = HttpResponseInit();
    conn->channel = ChannelInit(fd, READ_EVENT, processRead, processWrite, tcpConnectionDestroy, conn);
    eventLoopAddTask(eventloop, conn->channel, ADD);
    Debug("和客户端建立连接, threadName: %s, threadID:%s, connName: %s",
          eventloop->threadName, eventloop->threadId, conn->name);
    return conn;
}