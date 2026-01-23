#include "EventLoop.h"
// 初始化主反应堆
EventLoop *EventLoopInit_t()
{
    return EventLoopInit(nullptr);
}
void writeLocalMessage(EventLoop *evLoop)
{
    const char buf[] = "wake up";
    write(evLoop->socketPair[0], buf, strlen(buf));
}
int readLocalMessage(void *arg)
{
    EventLoop *evLoop = (EventLoop *)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf));
    return 0;
}
// 初始化反应堆
EventLoop *EventLoopInit(const char *name)
{
    EventLoop *evLoop = (EventLoop *)malloc(sizeof(EventLoop));
    evLoop->isQuit = false;

    evLoop->dispatcher = &Epoll_dispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();
    evLoop->head = nullptr;
    evLoop->tail = nullptr;

    evLoop->channelMap = ChannelMapInit(128);
    evLoop->threadId = pthread_self();
    // 初始化互斥锁
    pthread_mutex_init(&evLoop->lock, nullptr);
    // 给反应堆的名字赋值
    char threadName[32];
    if (name == nullptr)
        strcpy(evLoop->threadName, "MainThread");
    else
        strcpy(evLoop->threadName, name);
    // 设置本地通信的socketpair，实现自唤醒机制
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    if (ret < 0)
    {
        perror("socketpair");
        return nullptr;
    }
    Channel *channel = ChannelInit(evLoop->socketPair[1], READ_EVENT, readLocalMessage, nullptr, nullptr, evLoop);
    eventLoopAddTask(evLoop, channel, ADD);
    return evLoop;
}
int EventLoopRun(EventLoop *evLoop)
{
    assert(evLoop != NULL);
    if (evLoop->threadId != pthread_self())
    {
        return -1;
    }
    Dispatcher *dispatcher = evLoop->dispatcher;
    while (!evLoop->isQuit) // 当反应堆还没有退出时
    {
        // 调用epoll/poll/select的dispatch函数，等待事件的发生
        dispatcher->dispatch(evLoop, 10);
        eventLoopProcessTask(evLoop);
    }
    return 0;
}

int eventActivate(EventLoop *evLoop, int fd, int event)
{
    // 检测文件描述符与反应堆的合法性
    if (fd <= 0 || evLoop == nullptr)
    {
        return -1;
    }
    Channel *channel = evLoop->channelMap->list[fd];
    assert(fd == channel->fd);
    if (event & READ_EVENT && channel->readHandler)
    {
        channel->readHandler(channel->arg);
    }
    if (event & WRITE_EVENT && channel->writeHandler)
    {
        channel->writeHandler(channel->arg);
    }
    return 0;
}

int eventLoopAddTask(EventLoop *evLoop, Channel *channel, ElementType type)
{
    pthread_mutex_lock(&evLoop->lock);
    // 定义一个链表节点
    ChannelElement *element = (ChannelElement *)malloc(sizeof(ChannelElement));
    element->channel = channel;
    element->type = type; // 执行什么操作
    element->next = nullptr;
    if (!evLoop->head) // 当前链表为空
    {
        // 头节点和尾节点都指向同一个节点
        evLoop->head = evLoop->tail = element;
    }
    else
    {
        // 将新节点添加到链表尾部
        evLoop->tail->next = element;
        evLoop->tail = element;
    }
    pthread_mutex_unlock(&evLoop->lock);
    if (evLoop->threadId == pthread_self())
    {
        // 当前子线程，同一线程，直接处理
        eventLoopProcessTask(evLoop);
    }
    else
    {
        // 主线程在acceptConnection的时候，会轮询地取出一个子反应堆
        // 然后在TcpConnectionInit里面执行eventLoopAddTask
        // 把一个用于通信的channel加入到子反应堆的任务队类中
        // 此时就是evLoop->threadId是子线程的id，而pthread_self()是主线程的id

        // 主线程 -- 告诉子线程处理任务队列中的任务
        // 主线程只起到接受连接并给子线程分配连接的作用
        writeLocalMessage(evLoop);
    }
    return 0;
}

int eventLoopProcessTask(EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->lock);
    ChannelElement *head = evLoop->head; // 取出头节点
    while (head)
    {
        Channel *channel = head->channel;
        ElementType type = head->type;
        if (type == ADD)
        {
            eventLoopAdd(evLoop, channel);
        }
        else if (type == REMOVE)
        {
            eventLoopRemove(evLoop, channel);
        }
        else if (type == MODIFY)
        {
            eventLoopModify(evLoop, channel);
        }
        // 头节点后移
        ChannelElement *temp = head;
        head = head->next;
        free(temp); // 释放已经被取出的头节点
    }
    evLoop->head = evLoop->tail = nullptr; // 一次性全部取完
    pthread_mutex_unlock(&evLoop->lock);
    return 0;
}

int eventLoopAdd(EventLoop *evLoop, Channel *channel)
{
    int fd = channel->fd;
    ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        int res = makeMapRoom(channelMap, fd + 1, sizeof(Channel *));
        if (!res)
        {
            return -1;
        }
    }
    int ret = -1;
    if (!channelMap->list[fd])
    {
        channelMap->list[fd] = channel;
        ret = evLoop->dispatcher->add(channel, evLoop);
    }
    return ret;
}

int eventLoopRemove(EventLoop *evLoop, Channel *channel)
{
    int fd = channel->fd;
    ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        return -1;
    }
    int ret = -1;
    if (channelMap->list[fd])
    {
        ret = evLoop->dispatcher->remove(channel, evLoop);
    }
    return ret;
}

int eventLoopModify(EventLoop *evLoop, Channel *channel)
{
    int fd = channel->fd;
    ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size || channelMap->list[fd] == nullptr)
    {
        return -1;
    }
    int ret = -1;
    if (channelMap->list[fd])
    {
        ret = evLoop->dispatcher->modify(channel, evLoop);
    }
    return ret;
}

int destroyChannel(EventLoop *evLoop, Channel *channel)
{
    evLoop->channelMap->list[channel->fd] = nullptr;
    close(channel->fd);
    free(channel);
    return 0;
}
