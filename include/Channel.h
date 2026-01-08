#pragma once
#include <iostream>
using namespace std;   
typedef int(*handleFunc)(void*);
enum EventType
{
    TimeOutEvent=1,
    READ_EVENT=2,
    WRITE_EVENT=4
};
struct Channel
{
    int fd;
    int events;
    handleFunc readHandler;
    handleFunc writeHandler;
    void* arg;
};
Channel* ChannelInit(int fd,int events,handleFunc readHandler,handleFunc writeHandler,void* arg);
// 修改fd的写事件(检测 or 不检测)
void writeEventEnable(struct Channel* channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);