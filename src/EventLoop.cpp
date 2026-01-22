#include "EventLoop.h"

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
    pthread_mutex_init(&evLoop->lock, nullptr);
    char threadName[32];
    if (name == nullptr)
        strcpy(evLoop->threadName, "MainThread");
    else
        strcpy(evLoop->threadName, name);
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
    while (!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop, 10);
        eventLoopProcessTask(evLoop);
    }
    return 0;
}

int eventActivate(EventLoop *evLoop, int fd, int event)
{
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

int eventLoopAddTask(EventLoop *evLoop, Channel *channel, int type)
{
    pthread_mutex_lock(&evLoop->lock);
    ChannelElement *element = (ChannelElement *)malloc(sizeof(ChannelElement));
    element->channel = channel;
    element->type = type;
    element->next = nullptr;
    if (!evLoop->head)
    {
        evLoop->head = evLoop->tail = element;
    }
    else
    {
        evLoop->tail->next = element;
        evLoop->tail = element;
    }
    pthread_mutex_unlock(&evLoop->lock);
    if (evLoop->threadId == pthread_self())
    {
        // 同一线程，直接处理
        eventLoopProcessTask(evLoop);
    }
    else
    {
        // 不同线程，唤醒线程处理
        writeLocalMessage(evLoop);
    }
    return 0;
}

int eventLoopProcessTask(EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->lock);
    ChannelElement *head = evLoop->head;
    while (head)
    {
        Channel *channel = head->channel;
        int type = head->type;
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
        ChannelElement *temp = head;
        head = head->next;
        free(temp);
    }
    evLoop->head = evLoop->tail = nullptr;
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
