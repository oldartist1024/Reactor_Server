#include "ThreadPool.h"
// 初始化线程池
ThreadPool::ThreadPool(EventLoop *mainLoop, int count)
{
    m_IsStart = false;
    m_Index = 0;
    m_ThreadNum = count;
    m_mainLoop = mainLoop;
    m_workerThreads.clear();
}
ThreadPool::~ThreadPool()
{
    for (auto value : m_workerThreads)
    {
        delete value;
    }
}

// 启动线程池
void ThreadPool::threadPoolRun()
{
    assert(!m_IsStart);
    if (m_mainLoop->GetThreadid() != this_thread::get_id())
    {
        // 验证只能在主线程启动线程池
        exit(0);
    }
    m_IsStart = true;
    if (m_ThreadNum)
    {
        for (int i = 0; i < m_ThreadNum; i++)
        {
            // 依次初始化并启动工作线程
            WorkerThread *workerthread = new WorkerThread(i);
            workerthread->run();
            m_workerThreads.push_back(workerthread);
        }
    }
}
// 取出线程池中的某个子线程的反应堆实例
EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_IsStart);
    if (m_mainLoop->GetThreadid() != this_thread::get_id())
    {
        // 验证只能在主线程获取子线程反应堆实例
        exit(0);
    }
    // 没有子反应堆就让主反应堆来处理
    EventLoop *evLoop = m_mainLoop;
    if (m_ThreadNum)
    {
        // 轮询地取出一个子反应堆
        evLoop = m_workerThreads[m_Index]->GetWorkingThread();
        // 参数+1
        m_Index++;
        // 超过线程数量则重置为0
        if (m_Index >= m_ThreadNum)
        {
            m_Index = 0;
        }
    }
    return evLoop;
}