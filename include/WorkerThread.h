#pragma once
#include <thread>
#include "EventLoop.h"
#include <condition_variable>
using namespace std;
class WorkerThread
{
public:
    // 初始化工作线程
    WorkerThread(int index);
    ~WorkerThread();
    // 工作线程启动
    void run();
    inline EventLoop *GetWorkingThread()
    {
        return m_eventLoop;
    }

private:
    void subThreadRunning();

private:
    thread *m_thread;          // 线程类
    thread::id m_threadId;     // 线程id
    string m_threadName;       // 线程的名字
    mutex m_mutex;             // 线程的互斥锁
    condition_variable m_cond; // 线程的条件变量
    EventLoop *m_eventLoop;    // 线程对应的反应堆
};
