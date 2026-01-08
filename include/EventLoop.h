#pragma once
#include <iostream>
#include "Dispatcher.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <pthread.h>
extern Dispatcher Epoll_dispatcher;
extern Dispatcher poll_dispatcher;
extern Dispatcher select_dispatcher;

enum ElementType{ADD=0,REMOVE,MODIFY};
struct ChannelElement
{
    int type;
    Channel* channel;
    ChannelElement* next;
};
struct EventLoop
{
    bool isQuit;
    Dispatcher* dispatcher;
    void* dispatcherData;
    ChannelElement* head;
    ChannelElement* tail;

    ChannelMap* channelMap;
    pthread_t threadId;
    pthread_mutex_t lock;
    char threadName[32];
    int socketPair[2];  // 存储本地通信的fd 通过socketpair 初始化
};
//初始化
EventLoop* EventLoopInit_t();
EventLoop* EventLoopInit(const char *name);
//启动
int EventLoopRun(EventLoop* evLoop);
int eventActivate(struct EventLoop* evLoop, int fd, int event);
//添加任务
int eventLoopAddTask(EventLoop* evLoop,Channel* channel, int type);
// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop);
// 处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// 释放channel
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);
