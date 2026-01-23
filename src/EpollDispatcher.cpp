#include <unistd.h>
#include "Dispatcher.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#define MAX 520
struct epoll_eventData
{
    int epfd;
    struct epoll_event *event;
};
static void *epollinit();
static int epollctl(struct Channel *channel, struct EventLoop *evLoop, int op);
// 添加
static int epolladd(struct Channel *channel, struct EventLoop *evLoop);
// 删除
static int epollremove(struct Channel *channel, struct EventLoop *evLoop);
// 修改
static int epollmodify(struct Channel *channel, struct EventLoop *evLoop);
// 事件监测
static int epolldispatch(struct EventLoop *evLoop, int timeout); // 单位: s
// 清除数据(关闭fd或者释放内存)
static int epollclear(struct EventLoop *evLoop);

Dispatcher Epoll_dispatcher = {
    .init = epollinit,
    .add = epolladd,
    .remove = epollremove,
    .modify = epollmodify,
    .dispatch = epolldispatch,
    .clear = epollclear,
};

static void *epollinit()
{
    epoll_eventData *data = (epoll_eventData *)malloc(sizeof(epoll_eventData));
    // 创建epoll树
    data->epfd = epoll_create(1);
    // 创建失败
    if (data->epfd == -1)
    {
        perror("epoll_create");
        return NULL;
    }
    data->event = (epoll_event *)malloc(sizeof(epoll_event) * MAX);
    return data;
}
static int epollctl(struct Channel *channel, struct EventLoop *evLoop, int op)
{
    epoll_eventData *data = (epoll_eventData *)(evLoop->dispatcherData);
    epoll_event ev;
    // 事件类型设置
    int events = 0;
    if (channel->events & READ_EVENT)
        events |= EPOLLIN;
    if (channel->events & WRITE_EVENT)
        events |= EPOLLOUT;
    ev.events = events;
    // 文件描述符设置
    ev.data.fd = channel->fd;
    // 对epoll树进行操作
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret;
}
// 添加
static int epolladd(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollctl(channel, evLoop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_ctl add");
        return 0;
    }
    return ret;
}
// 删除
static int epollremove(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollctl(channel, evLoop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_ctl del");
        return 0;
    }
    // 移除channel后，调用其销毁回调函数（断开tcp连接的函数）
    channel->destroyCallback(channel->arg);
    return ret;
}
// 修改
static int epollmodify(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollctl(channel, evLoop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_ctl mod");
        return 0;
    }
    return ret;
}
// 事件监测
static int epolldispatch(struct EventLoop *evLoop, int timeout)
{
    epoll_eventData *data = (epoll_eventData *)(evLoop->dispatcherData);
    int num = epoll_wait(data->epfd, data->event, MAX, timeout * 1000);
    for (int i = 0; i < num; i++)
    {
        int events = data->event[i].events;
        int fd = data->event[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开了连接, 删除 fd
            continue;
        }
        if (events & EPOLLIN)
        {
            // 读事件
            eventActivate(evLoop, fd, READ_EVENT);
        }
        if (events & EPOLLOUT)
        {
            // 写事件
            eventActivate(evLoop, fd, WRITE_EVENT);
        }
    }
    return 0;
}
static int epollclear(struct EventLoop *evLoop)
{
    epoll_eventData *data = (epoll_eventData *)(evLoop->dispatcherData);
    close(data->epfd);
    free(data->event);
    free(data);
    return 0;
}