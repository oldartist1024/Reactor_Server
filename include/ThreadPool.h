#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
struct ThreadPool
{
    bool IsStart;                // 线程池开启标志
    int Index;                   // 轮询索引（下一个被分配任务的子线程索引）
    int ThreadNum;               // 线程数量
    EventLoop *mainLoop;         // 主反应堆
    WorkerThread *workerThreads; // 工作线程数组
};
// 初始化线程池
ThreadPool *threadPoolInit(EventLoop *mainLoop, int count);
// 启动线程池
void threadPoolRun(ThreadPool *threadpool);
// 取出线程池中的某个子线程的反应堆实例
struct EventLoop *takeWorkerEventLoop(struct ThreadPool *pool);
