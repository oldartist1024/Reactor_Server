#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
struct ThreadPool
{
    bool IsStart;
    int Index;
    int ThreadNum;
    EventLoop *mainLoop;
    WorkerThread *workerThreads;
};
// 初始化线程池
ThreadPool* threadPoolInit(EventLoop* mainLoop, int count);
//启动线程池
void threadPoolRun(ThreadPool *threadpool);
// 取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);
