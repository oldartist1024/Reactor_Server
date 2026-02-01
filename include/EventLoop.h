#pragma once
#include "Dispatcher.h"
#include <sys/socket.h>
// #include <unistd.h>
#include <cassert>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <cstring>
#include <EpollDispatcher.h>
#include <selectDispatcher.h>
#include <pollDispatcher.h>
using namespace std;
// 操作类型
enum class ElementType
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
};
class EventLoop
{
public:
    // 初始化
    EventLoop();
    EventLoop(const string name);
    // 析构
    ~EventLoop();
    // 反应堆运行
    int Run();
    // 执行channel检测到的事件对应的读写回调函数
    int Activate(int fd, int event);
    // 添加任务到任务队列
    int AddTask(Channel *channel, ElementType type);
    // 处理任务队列中的任务
    int ProcessTask();
    // 往evLoop的channelmap里添加channel
    int Add(Channel *channel);
    // 往evLoop的channelmap里删除channel
    int Remove(Channel *channel);
    // 往evLoop的channelmap里修改channel
    int Modify(Channel *channel);
    // 释放channel
    int DestroyChannel(Channel *channel);
    int readMessage(void *arg);
    static int readLocalMessage(void *arg);
    inline thread::id GetThreadid()
    {
        return m_threadId;
    }
    inline string getThreadName()
    {
        return m_threadName;
    }

private:
    void writeLocalMessage();

private:
    bool m_isQuit;                   // 反应堆的启动标志
    Dispatcher *m_dispatcher;        // Dispatcher
    queue<ChannelElement *> m_taskQ; // 任务队列
    map<int, Channel *> m_channelMap;
    thread::id m_threadId; // 线程id
    mutex m_lock;          // 互斥锁
    string m_threadName;   // 反应堆名字
    int m_socketPair[2];   // 存储本地通信的fd 通过socketpair 初始化
};
