#include "WorkerThread.h"
int WorkerThreadInit(WorkerThread *workerThread, int index)
{
    workerThread->eventLoop = nullptr;
    workerThread->threadId = 0;
    // 子线程名字
    sprintf(workerThread->threadName, "WorkerThread-%d", index);
    pthread_mutex_init(&workerThread->mutex, nullptr);
    pthread_cond_init(&workerThread->cond, nullptr);
    return 0;
}
void *subThreadRunning(void *arg)
{
    // 这是子线程的工作函数
    WorkerThread *workerThread = (WorkerThread *)arg;
    pthread_mutex_lock(&workerThread->mutex);
    // 初始化这个工作线程对应的反应堆
    workerThread->eventLoop = EventLoopInit(workerThread->threadName);
    pthread_mutex_unlock(&workerThread->mutex);
    // 反应堆初始化完，唤醒主线程
    pthread_cond_signal(&workerThread->cond);
    // 子反应堆启动
    EventLoopRun(workerThread->eventLoop);
    return nullptr;
}
void WorkerThreadRun(WorkerThread *workerThread)
{
    // 当前函数是在主线程执行的
    // 创建线程
    pthread_create(&workerThread->threadId, nullptr, subThreadRunning, workerThread);
    // 加锁是因为访问量workerThread->eventLoop这个主线程与子线程共享的变量
    // 使用条件变量阻塞是因为防止子线程还没有初始化成功eventLoop
    // 主线程便在后面访问这个eventloop
    pthread_mutex_lock(&workerThread->mutex);
    while (workerThread->eventLoop == nullptr)
    {
        // 在这里等待子线程初始化完反应堆
        pthread_cond_wait(&workerThread->cond, &workerThread->mutex);
    }
    pthread_mutex_unlock(&workerThread->mutex);
}
