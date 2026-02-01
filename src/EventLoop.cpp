#include "EventLoop.h"
#include "EpollDispatcher.h"
void EventLoop::writeLocalMessage()
{
    const char buf[] = "wake up";
    write(this->m_socketPair[0], buf, strlen(buf));
}
int EventLoop::readMessage(void *arg)
{
    EventLoop *evLoop = (EventLoop *)arg;
    char buf[256];
    read(evLoop->m_socketPair[1], buf, sizeof(buf));
    return 0;
}
int EventLoop::readLocalMessage(void *arg)
{
    EventLoop *evLoop = (EventLoop *)arg;
    char buf[256];
    read(evLoop->m_socketPair[1], buf, sizeof(buf));
    return 0;
}

EventLoop::EventLoop() : EventLoop(string())
{
}

EventLoop::EventLoop(const string name)
{
    m_isQuit = false;

    m_dispatcher = new EpollDispatcher(this);

    m_channelMap.clear();
    m_threadId = this_thread::get_id();
    // 给反应堆的名字赋值
    char threadName[32];
    if (name == string())
        m_threadName = "MainThread";
    else
        m_threadName = name;
    // 设置本地通信的socketpair，实现自唤醒机制
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret < 0)
    {
        perror("socketpair");
        exit(0);
    }
#if 1
    Channel *channel = new Channel(m_socketPair[1], EventType::READ_EVENT, readLocalMessage, nullptr, nullptr, this);
#else
    // 使用可调用对象包装器和绑定器
#endif
    AddTask(channel, ElementType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::Run()
{
    if (this->m_threadId != this_thread::get_id())
    {
        return -1;
    }
    Dispatcher *dispatcher = this->m_dispatcher;
    while (!this->m_isQuit) // 当反应堆还没有退出时
    {
        // 调用epoll/poll/select的dispatch函数，等待事件的发生
        dispatcher->dispatch(10);
        ProcessTask();
    }
    return 0;
}
// 执行channel检测到的事件对应的读写回调函数
int EventLoop::Activate(int fd, int event)
{
    // 检测文件描述符与反应堆的合法性
    if (fd <= 0)
    {
        return -1;
    }
    Channel *channel = this->m_channelMap[fd];
    assert(fd == channel->getfd());
    if (event & (int)EventType::READ_EVENT && channel->readHandler)
    {
        channel->readHandler(const_cast<void *>(channel->getArg()));
    }
    if (event & (int)EventType::WRITE_EVENT && channel->writeHandler)
    {
        channel->writeHandler(const_cast<void *>(channel->getArg()));
    }
    return 0;
}
// 添加任务到任务队列
int EventLoop::AddTask(Channel *channel, ElementType type)
{
    m_lock.lock();
    // 定义一个链表节点
    ChannelElement *element = new ChannelElement;
    element->channel = channel;
    element->type = type;  // 执行什么操作
    m_taskQ.push(element); // 添加到任务队列
    m_lock.unlock();
    if (this->m_threadId != this_thread::get_id())
    {
        // 当前子线程，同一线程，直接处理
        ProcessTask();
    }
    else
    {
        // 主线程在acceptConnection的时候，会轮询地取出一个子反应堆
        // 然后在TcpConnectionInit里面执行eventLoopAddTask
        // 把一个用于通信的channel加入到子反应堆的任务队类中
        // 此时就是evLoop->threadId是子线程的id，而pthread_self()是主线程的id

        // 主线程 -- 告诉子线程处理任务队列中的任务
        // 主线程只起到接受连接并给子线程分配连接的作用
        writeLocalMessage();
    }
    return 0;
}
// 处理任务队列中的任务
int EventLoop::ProcessTask()
{
    while (!m_taskQ.empty())
    {
        m_lock.lock();
        ChannelElement *channelelement = m_taskQ.front();
        m_taskQ.pop();
        m_lock.unlock();
        if (channelelement->type == ElementType::ADD)
        {
            Add(channelelement->channel);
        }
        else if (channelelement->type == ElementType::REMOVE)
        {
            Remove(channelelement->channel);
        }
        else if (channelelement->type == ElementType::MODIFY)
        {
            Modify(channelelement->channel);
        }
        delete channelelement; // 释放已经被取出的头节点
    }
    return 0;
}
// 往evLoop的channelmap里添加channel
int EventLoop::Add(Channel *channel)
{
    int fd = channel->getfd();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        // m_channelMap[fd] = channel;
        m_channelMap.insert(make_pair(fd, channel));
        m_dispatcher->SetChannel(channel);
        int ret = m_dispatcher->add();
        return ret;
    }
    return -1;
}
// 往evLoop的channelmap里删除channel
int EventLoop::Remove(Channel *channel)
{
    int fd = channel->getfd();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->SetChannel(channel);
    int ret = this->m_dispatcher->remove();
    return ret;
}
// 往evLoop的channelmap里修改channel
int EventLoop::Modify(Channel *channel)
{
    int fd = channel->getfd();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->SetChannel(channel);
    int ret = this->m_dispatcher->modify();
    return ret;
}
// 释放channel
int EventLoop::DestroyChannel(Channel *channel)
{
    int fd = channel->getfd();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    int ret = m_channelMap.erase(fd);
    close(channel->getfd());
    delete channel;
    return ret;
}