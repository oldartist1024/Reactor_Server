#include "WorkerThread.h"
int WorkerThreadInit(WorkerThread *workerThread, int index)
{
    workerThread->eventLoop = nullptr;
    workerThread->threadId = 0;
    sprintf(workerThread->threadName, "WorkerThread-%d", index);
    pthread_mutex_init(&workerThread->mutex, nullptr);
    pthread_cond_init(&workerThread->cond, nullptr);
    return 0;
}
void *subThreadRunning(void *arg)
{
    WorkerThread *workerThread = (WorkerThread *)arg;
    pthread_mutex_lock(&workerThread->mutex);
    workerThread->eventLoop = EventLoopInit(workerThread->threadName);
    pthread_mutex_unlock(&workerThread->mutex);
    pthread_cond_signal(&workerThread->cond);
    EventLoopRun(workerThread->eventLoop);
    return nullptr;
}
void WorkerThreadRun(WorkerThread *workerThread)
{
    pthread_create(&workerThread->threadId, nullptr, subThreadRunning, workerThread);
    pthread_mutex_lock(&workerThread->mutex);
    while (workerThread->eventLoop == nullptr)
    {
        pthread_cond_wait(&workerThread->cond, &workerThread->mutex);
    }
    pthread_mutex_unlock(&workerThread->mutex);
}
