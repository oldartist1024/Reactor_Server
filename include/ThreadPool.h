#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <vector>
class ThreadPool
{
public:
    // 初始化线程池
    ThreadPool(EventLoop *mainLoop, int count);
    ~ThreadPool();

    // 启动线程池
    void threadPoolRun();
    // 取出线程池中的某个子线程的反应堆实例
    EventLoop *takeWorkerEventLoop();

private:
    bool m_IsStart;                         // 线程池开启标志
    int m_Index;                            // 轮询索引（下一个被分配任务的子线程索引）
    int m_ThreadNum;                        // 线程数量
    EventLoop *m_mainLoop;                  // 主反应堆
    vector<WorkerThread *> m_workerThreads; // 工作线程数组
};
