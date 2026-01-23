#pragma once
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"

// 当这个宏定义:当数据被全部写入到WriteBuffer后，才会触发写回调函数
// 当这个宏未定义:当数据被写入到WriteBuffer后，会被直接发送
#define MSG_SEND_AUTO

struct TcpConnection
{
    char name[32];        // 名字
    Channel *channel;     // 用于通信的channel
    Buffer *ReadBuffer;   // 读取缓冲区
    Buffer *WriteBuffer;  // 写入缓冲区
    EventLoop *eventloop; // 反应堆
    // http 协议
    struct HttpRequest *request;   // http 请求
    struct HttpResponse *response; // http 响应
};
// 初始化tcp连接
TcpConnection *TcpConnectionInit(int fd, EventLoop *eventloop);
