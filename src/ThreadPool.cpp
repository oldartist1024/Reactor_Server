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
        exit(0);
    }
    threadpool->IsStart = true;
    if (threadpool->ThreadNum)
    {
        for (int i = 0; i < threadpool->ThreadNum; i++)
        {
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
        exit(0);
    }
    EventLoop *evLoop = threadpool->mainLoop;
    if (threadpool->ThreadNum)
    {
        evLoop = threadpool->workerThreads[threadpool->Index].eventLoop;
        threadpool->Index++;
        if (threadpool->Index >= threadpool->ThreadNum)
        {
            threadpool->Index = 0;
        }
    }
    return evLoop;
}