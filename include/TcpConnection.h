#pragma once
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"
#include <sstream>

using namespace std;
// 当这个宏定义:当数据被全部写入到WriteBuffer后，才会触发写回调函数
// 当这个宏未定义:当数据被写入到WriteBuffer后，会被直接发送
// #define MSG_SEND_AUTO
class HttpRequest;
class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop *eventloop);
    ~TcpConnection();
    static int processRead(void *arg);
    static int processWrite(void *arg);
    static int tcpConnectionDestroy(void *arg);

private:
private:
    string m_name;          // 名字
    Channel *m_channel;     // 用于通信的channel
    Buffer *m_ReadBuffer;   // 读取缓冲区
    Buffer *m_WriteBuffer;  // 写入缓冲区
    EventLoop *m_eventloop; // 反应堆
    // http 协议
    HttpRequest *m_request;   // http 请求
    HttpResponse *m_response; // http 响应
};
