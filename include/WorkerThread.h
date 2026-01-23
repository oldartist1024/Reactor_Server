#pragma once
using namespace std;
#include <pthread.h>
#include "EventLoop.h"
struct WorkerThread
{
    pthread_t threadId;    // 线程id
    char threadName[24];   // 线程的名字
    pthread_mutex_t mutex; // 线程的互斥锁
    pthread_cond_t cond;   // 线程的条件变量
    EventLoop *eventLoop;  // 线程对应的反应堆
};
// 工作线程初始化
int WorkerThreadInit(WorkerThread *workerThread, int index);
// 工作线程启动
void WorkerThreadRun(WorkerThread *workerThread);