#pragma once
#include <iostream>
using namespace std;
using handleFunc = int (*)(void *);
enum EventType // 事件类型
{
    TimeOutEvent = 1,
    READ_EVENT = 2,
    WRITE_EVENT = 4
};
struct Channel
{
    int fd;     // 文件描述符
    int events; // 事件类型
    // 事件回调函数
    handleFunc readHandler;
    handleFunc writeHandler;
    handleFunc destroyCallback;
    // 回调函数的参数
    void *arg;
};
// Channel初始化
Channel *ChannelInit(int fd, int events, handleFunc readHandler, handleFunc writeHandler, handleFunc destroyHandler, void *arg);
// 修改fd的写事件(检测 or 不检测)
void writeEventEnable(struct Channel *channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel *channel);