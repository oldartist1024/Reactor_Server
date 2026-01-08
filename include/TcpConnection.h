#pragma once
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
struct TcpConnection
{
    char name[32];
    Channel* channel;
    Buffer* ReadBuffer;
    Buffer* WriteBuffer;
    EventLoop *eventloop;
    // http 协议

};
TcpConnection* TcpConnectionInit(int fd,EventLoop *eventloop);