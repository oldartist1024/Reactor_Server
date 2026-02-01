#include "TcpConnection.h"
// 通信的读回调函数
int TcpConnection::processRead(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    // 接收数据到缓冲区
    int res = conn->m_ReadBuffer->SocketRead(conn->m_channel->getfd());
    Debug("接收到的http请求数据: %s", conn->m_ReadBuffer->data_position());
    // 数据大小大于0，说明接收到了数据
    if (res > 0)
    {
#ifdef MSG_SEND_AUTO
        // 启动写事件检测
        conn->m_channel->writeEventEnable(true);
        conn->m_eventloop->AddTask(conn->m_channel, ElementType::MODIFY);
#endif
        // 接收到了 http 请求, 解析http请求
        // 并将要响应的数据添加到写缓冲区
        int flag = conn->m_request->parseHttpRequest(conn->m_ReadBuffer, conn->m_response, conn->m_WriteBuffer, conn->m_channel->getfd());
        if (!flag)
        {
            // 解析失败，发送400错误响应
            char *errMsg = const_cast<char *>("HTTP/1.1 400 Bad Request\r\n\r\n");
            // 往写缓冲区添加报错信息
            conn->m_WriteBuffer->AppendString(errMsg);
#ifndef MSG_SEND_AUTO
            conn->m_WriteBuffer->SendData(conn->m_channel->getfd());
#endif
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 客户端关闭连接或读取出错，断开连接
        conn->m_eventloop->AddTask(conn->m_channel, ElementType::REMOVE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 接收完毕，断开连接
    conn->m_eventloop->AddTask(conn->m_channel, ElementType::REMOVE);
#endif
    return res;
}
// 通信的写回调函数
int TcpConnection::processWrite(void *arg)
{
    Debug("开始发送数据了(基于写事件发送)....");
    TcpConnection *conn = (TcpConnection *)arg;
    int count = conn->m_WriteBuffer->SendData(conn->m_channel->getfd());
    if (count > 0)
    {
        // 当数据全被发出
        if (conn->m_WriteBuffer->ReadableSize() == 0)
        {
            // 把这个连接从事件循环中移除
            conn->m_eventloop->AddTask(conn->m_channel, ElementType::REMOVE);
        }
    }
    return count;
}
// 通信的断开回调函数（释放对应的内存）
int TcpConnection::tcpConnectionDestroy(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    if (conn)
    {
        delete conn;
    }
    return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop *eventloop)
{
    m_eventloop = eventloop;
    m_name = "TcpConnection-" + to_string(fd);
    m_ReadBuffer = new Buffer(10240);
    m_WriteBuffer = new Buffer(10240);
    m_request = new HttpRequest();
    m_response = new HttpResponse();
    // 创建一个用于通信的channel
    m_channel = new Channel(fd, EventType::READ_EVENT, processRead, processWrite, tcpConnectionDestroy, this);
    eventloop->AddTask(m_channel, ElementType::ADD);

    // 将 thread::id 转换为字符串形式
    stringstream ss;
    ss << eventloop->GetThreadid();
    string threadIdStr = ss.str();

    Debug("和客户端建立连接, threadName: %s, threadID:%s, connName: %s",
          eventloop->getThreadName().c_str(), threadIdStr.c_str(), m_name.c_str());
}
TcpConnection::~TcpConnection()
{
    if (m_ReadBuffer && m_WriteBuffer && m_WriteBuffer->ReadableSize() == 0 && m_ReadBuffer->ReadableSize() == 0)
    {
        m_eventloop->DestroyChannel(m_channel);
        delete m_ReadBuffer;  // 读取缓冲区
        delete m_WriteBuffer; // 写入缓冲区
        delete m_request;     // http 请求
        delete m_response;    // http 响应
    }

    Debug("连接断开, 释放资源, gameover, connName: %s", m_name.c_str());
}
