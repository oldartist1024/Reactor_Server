#include "ThreadPool.h"

ThreadPool *threadPoolInit(EventLoop *mainLoop, int count)
{
    ThreadPool *threadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    threadPool->IsStart = false;
    threadPool->Index = 0;
    threadPool->ThreadNum = count;
    threadPool->mainLoop = mainLoop;
    threadPool->workerThreads = (WorkerThread *)malloc(sizeof(WorkerThread) * count);
    return threadPool;
}

void threadPoolRun(ThreadPool *threadpool)
{
    assert(threadpool != nullptr && !threadpool->IsStart);
    if (threadpool->mainLoop->threadId != pthread_self())
    {
        // 验证只能在主线程启动线程池
        exit(0);
    }
    threadpool->IsStart = true;
    if (threadpool->ThreadNum)
    {
        for (int i = 0; i < threadpool->ThreadNum; i++)
        {
            // 依次初始化并启动工作线程
            WorkerThreadInit(&threadpool->workerThreads[i], i);
            WorkerThreadRun(&threadpool->workerThreads[i]);
        }
    }
}
EventLoop *takeWorkerEventLoop(ThreadPool *threadpool)
{
    assert(threadpool != nullptr && threadpool->IsStart);
    if (threadpool->mainLoop->threadId != pthread_self())
    {
        // 验证只能在主线程获取子线程反应堆实例
        exit(0);
    }
    EventLoop *evLoop = threadpool->mainLoop;
    if (threadpool->ThreadNum)
    {
        // 轮询地取出一个子反应堆
        evLoop = threadpool->workerThreads[threadpool->Index].eventLoop;
        // 参数+1
        threadpool->Index++;
        // 超过线程数量则重置为0
        if (threadpool->Index >= threadpool->ThreadNum)
        {
            threadpool->Index = 0;
        }
    }
    return evLoop;
}