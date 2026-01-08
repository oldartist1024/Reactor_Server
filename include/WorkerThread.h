#pragma once
using namespace std;
#include <pthread.h>
#include "EventLoop.h"
struct WorkerThread
{
    pthread_t threadId;
    char threadName[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    EventLoop* eventLoop;
};
int WorkerThreadInit(WorkerThread* workerThread, int index);
void WorkerThreadRun(WorkerThread* workerThread);