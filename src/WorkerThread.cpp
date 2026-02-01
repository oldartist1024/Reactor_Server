#include "WorkerThread.h"
WorkerThread::WorkerThread(int index)
{
    m_eventLoop = nullptr;
    m_threadId = thread::id();
    // 子线程名字
    m_threadName = "WorkerThread-" + to_string(index);
}
WorkerThread::~WorkerThread()
{
    if (m_thread != nullptr)
    {
        delete m_thread;
    }
}
// 工作线程启动
void WorkerThread::run()
{
    // 当前函数是在主线程执行的
    // 创建线程
    this->m_thread = new thread(&WorkerThread::subThreadRunning, this);
    // 加锁是因为访问量workerThread->eventLoop这个主线程与子线程共享的变量
    // 使用条件变量阻塞是因为防止子线程还没有初始化成功eventLoop
    // 主线程便在后面访问这个eventloop
    unique_lock<mutex> locker(m_mutex);
    while (m_eventLoop == nullptr)
    {
        // 在这里等待子线程初始化完反应堆
        m_cond.wait(locker);
    }
}
void WorkerThread::subThreadRunning()
{
    m_mutex.lock();
    // 初始化这个工作线程对应的反应堆
    m_eventLoop = new EventLoop(m_threadName);
    m_mutex.unlock();
    // 反应堆初始化完，唤醒主线程
    m_cond.notify_one();
    // 子反应堆启动
    m_eventLoop->Run();
}