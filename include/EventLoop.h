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
// 操作类型
enum ElementType
{
    ADD = 0,
    REMOVE,
    MODIFY
};
// 任务队列(链表)中的节点类型
struct ChannelElement
{
    enum ElementType type;
    Channel *channel;
    ChannelElement *next;
};
struct EventLoop
{
    bool isQuit; // 反应堆的启动标志

    // Dispatcher
    Dispatcher *dispatcher;
    void *dispatcherData;
    // 链表
    ChannelElement *head;
    ChannelElement *tail;

    ChannelMap *channelMap;
    pthread_t threadId;
    pthread_mutex_t lock; // 互斥锁
    char threadName[32];  // 反应堆名字
    int socketPair[2];    // 存储本地通信的fd 通过socketpair 初始化
};
// 初始化
EventLoop *EventLoopInit_t();
EventLoop *EventLoopInit(const char *name);
// 反应堆运行
int EventLoopRun(EventLoop *evLoop);
// 执行channel检测到的事件对应的读写回调函数
int eventActivate(struct EventLoop *evLoop, int fd, int event);
// 添加任务到任务队列
int eventLoopAddTask(EventLoop *evLoop, Channel *channel, ElementType type);
// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop *evLoop);
// 往evLoop的channelmap里添加channel
int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel);
// 往evLoop的channelmap里删除channel
int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel);
// 往evLoop的channelmap里修改channel
int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel);
// 释放channel
int destroyChannel(struct EventLoop *evLoop, struct Channel *channel);
