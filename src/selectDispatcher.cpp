#include <unistd.h>
#include "Dispatcher.h"
#include "EventLoop.h"
#include <sys/select.h>
#define MAX 1024
struct selectData
{
    fd_set readfds;
    fd_set writefds;
};
static void *selectinit();
// 添加
static int selectadd(struct Channel *channel, struct EventLoop *evLoop);
// 删除
static int selectremove(struct Channel *channel, struct EventLoop *evLoop);
// 修改
static int selectmodify(struct Channel *channel, struct EventLoop *evLoop);
// 事件监测
static int selectdispatch(struct EventLoop *evLoop, int timeout); // 单位: s
// 清除数据(关闭fd或者释放内存)
static int selectclear(struct EventLoop *evLoop);

Dispatcher select_dispatcher = {
    .init = selectinit,
    .add = selectadd,
    .remove = selectremove,
    .modify = selectmodify,
    .dispatch = selectdispatch,
    .clear = selectclear,
};

static void *selectinit()
{
    selectData *data = (selectData *)malloc(sizeof(selectData));
    FD_ZERO(&data->readfds);
    FD_ZERO(&data->writefds);
    return data;
}
// 添加
static int selectadd(struct Channel *channel, struct EventLoop *evLoop)
{
    selectData *data = (selectData *)(evLoop->dispatcherData);
    if (channel->fd >= MAX)
        return -1;
    if (channel->events & READ_EVENT)
        FD_SET(channel->fd, &data->readfds);
    if (channel->events & WRITE_EVENT)
        FD_SET(channel->fd, &data->writefds);
    return 1;
}
// 删除
static int selectremove(struct Channel *channel, struct EventLoop *evLoop)
{
    selectData *data = (selectData *)(evLoop->dispatcherData);
    if (channel->fd >= MAX)
        return -1;
    if (channel->events & READ_EVENT)
        FD_CLR(channel->fd, &data->readfds);
    if (channel->events & WRITE_EVENT)
        FD_CLR(channel->fd, &data->writefds);
    channel->destroyCallback(channel->arg);
    return 1;
}
// 修改
static int selectmodify(struct Channel *channel, struct EventLoop *evLoop)
{
    selectData *data = (selectData *)(evLoop->dispatcherData);
    if (channel->fd >= MAX)
        return -1;
    FD_CLR(channel->fd, &data->readfds);
    FD_CLR(channel->fd, &data->writefds);
    if (channel->events & READ_EVENT)
        FD_SET(channel->fd, &data->readfds);
    if (channel->events & WRITE_EVENT)
        FD_SET(channel->fd, &data->writefds);
    return 1;
}
// 事件监测
static int selectdispatch(struct EventLoop *evLoop, int timeout)
{
    selectData *data = (selectData *)(evLoop->dispatcherData);
    timeval val;
    val.tv_sec = timeout; /* Seconds.  */
    val.tv_usec = 0;      /* Microseconds.  */
    fd_set rdtemp = data->readfds;
    fd_set wrtemp = data->writefds;
    int res = select(MAX, &rdtemp, &wrtemp, NULL, &val);
    if (res == -1)
    {
        perror("select");
        exit(0);
    }
    for (int i = 0; i < MAX; i++)
    {
        if (FD_ISSET(i, &rdtemp))
        {
            // 读事件
            eventActivate(evLoop, i, READ_EVENT);
        }
        if (FD_ISSET(i, &wrtemp))
        {
            // 写事件
            eventActivate(evLoop, i, WRITE_EVENT);
        }
    }
    return 0;
}
static int selectclear(struct EventLoop *evLoop)
{
    selectData *data = (selectData *)(evLoop->dispatcherData);
    free(data);
    return 0;
}