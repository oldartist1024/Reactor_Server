#pragma once
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"

// 当这个宏定义为1:其等待数据接受完，再发送数据
// 当这个宏定义为0:其等待数据接受的同时，直接发送数据
#define MSG_SEND_AUTO 1

struct TcpConnection
{
    char name[32];
    Channel *channel;
    Buffer *ReadBuffer;
    Buffer *WriteBuffer;
    EventLoop *eventloop;
    // http 协议
    struct HttpRequest *request;
    struct HttpResponse *response;
};
TcpConnection *TcpConnectionInit(int fd, EventLoop *eventloop);
