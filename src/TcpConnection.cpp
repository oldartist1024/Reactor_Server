#include "TcpConnection.h"
// 通信的读回调函数
int processRead(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    // 接收数据到缓冲区
    int res = bufferSocketRead(conn->ReadBuffer, conn->channel->fd);
    Debug("接收到的http请求数据: %s", conn->ReadBuffer->data + conn->ReadBuffer->readPos);
    // 数据大小大于0，说明接收到了数据
    if (res > 0)
    {
#ifdef MSG_SEND_AUTO
        // 启动写事件检测
        writeEventEnable(conn->channel, true);
        eventLoopAddTask(conn->eventloop, conn->channel, MODIFY);
#endif
        // 接收到了 http 请求, 解析http请求
        // 并将要响应的数据添加到写缓冲区
        int flag = parseHttpRequest(conn->request, conn->ReadBuffer, conn->response, conn->WriteBuffer, conn->channel->fd);
        if (!flag)
        {
            // 解析失败，发送400错误响应
            char *errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            // 往写缓冲区添加报错信息
            bufferAppendString(conn->WriteBuffer, errMsg);
#ifndef MSG_SEND_AUTO
            bufferSendData(conn->WriteBuffer, conn->channel->fd);
#endif
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 客户端关闭连接或读取出错，断开连接
        eventLoopAddTask(conn->eventloop, conn->channel, REMOVE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 接收完毕，断开连接
    eventLoopAddTask(conn->eventloop, conn->channel, REMOVE);
#endif
    return res;
}
// 通信的写回调函数
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
// 通信的断开回调函数（释放对应的内存）
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
    // 创建一个用于通信的channel
    conn->channel = ChannelInit(fd, READ_EVENT, processRead, processWrite, tcpConnectionDestroy, conn);
    eventLoopAddTask(eventloop, conn->channel, ADD);
    Debug("和客户端建立连接, threadName: %s, threadID:%s, connName: %s",
          eventloop->threadName, eventloop->threadId, conn->name);
    return conn;
}