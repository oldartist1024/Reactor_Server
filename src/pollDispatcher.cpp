#include <unistd.h>
#include "Dispatcher.h"
#include "EventLoop.h"
#include <poll.h>
#define MAX 520
struct pollData
{
    int maxfd;
    pollfd fd[MAX];
};
static void *pollinit();
// 添加
static int polladd(struct Channel *channel, struct EventLoop *evLoop);
// 删除
static int pollremove(struct Channel *channel, struct EventLoop *evLoop);
// 修改
static int pollmodify(struct Channel *channel, struct EventLoop *evLoop);
// 事件监测
static int polldispatch(struct EventLoop *evLoop, int timeout); // 单位: s
// 清除数据(关闭fd或者释放内存)
static int pollclear(struct EventLoop *evLoop);

Dispatcher poll_dispatcher = {
    .init = pollinit,
    .add = polladd,
    .remove = pollremove,
    .modify = pollmodify,
    .dispatch = polldispatch,
    .clear = pollclear,
};

static void *pollinit()
{
    pollData *data = (pollData *)malloc(sizeof(pollData));
    data->maxfd = 0;
    for (int i = 0; i < MAX; i++)
    {
        data->fd[i].fd = -1;
        data->fd[i].events = 0;
        data->fd[i].revents = 0;
    }
    return data;
}
// 添加
static int polladd(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = -1;
    pollData *data = (pollData *)(evLoop->dispatcherData);
    int events = 0;
    if (channel->events & READ_EVENT)
        events |= POLLIN;
    if (channel->events & WRITE_EVENT)
        events |= POLLOUT;
    for (int i = 0; i < MAX; i++)
    {
        if (data->fd[i].fd == -1)
        {
            data->fd[i].fd = channel->fd;
            data->fd[i].events = events;
            if(data->maxfd < i) 
                data->maxfd = i;
            ret = 1;
            break;
        }
    }
    return ret;
}
// 删除
static int pollremove(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = -1;
    pollData *data = (pollData *)(evLoop->dispatcherData);
    for (int i = 0; i < MAX; i++)
    {
        if (data->fd[i].fd == channel->fd)
        {
            data->fd[i].fd = -1;
            data->fd[i].events = 0;
            data->fd[i].revents = 0;
            //要不要更新maxfd
            ret = 1;
            break;
        }
    }
    return ret;
}
// 修改
static int pollmodify(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = -1;
    pollData *data = (pollData *)(evLoop->dispatcherData);
    int events = 0;
    if (channel->events & READ_EVENT)
        events |= POLLIN;
    if (channel->events & WRITE_EVENT)
        events |= POLLOUT;
    for (int i = 0; i < MAX; i++)
    {
        if (data->fd[i].fd == channel->fd)
        {
            data->fd[i].events = events;
            ret = 1;
            break;
        }
    }
    return ret;
}
// 事件监测
static int polldispatch(struct EventLoop *evLoop, int timeout)
{
    pollData *data = (pollData *)(evLoop->dispatcherData);
    int res = poll(data->fd, data->maxfd + 1, timeout * 1000);
    if (res == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i < data->maxfd+1; i++)
    {
        int fd = data->fd[i].fd;
        if(fd == -1)
            continue;
        int events = data->fd[i].revents;
        if (events & POLLERR || events & POLLHUP)
        {
            // 对方断开了连接, 删除 fd
            continue;
        }
        if (events & POLLIN)
        {
            // 读事件
            eventActivate(evLoop,fd,READ_EVENT);
        }
        if (events & POLLOUT)
        {
            // 写事件
            eventActivate(evLoop,fd,WRITE_EVENT);
        }
    }
    return 0;
}
static int pollclear(struct EventLoop *evLoop)
{
    pollData *data = (pollData *)(evLoop->dispatcherData);
    free(data);
    return 0;
}