#include "Channel.h"

Channel *ChannelInit(int fd, int events, handleFunc readHandler, handleFunc writeHandler, handleFunc destroyHandler, void *arg)
{
    Channel *channel = (Channel *)malloc(sizeof(Channel));
    channel->fd = fd;
    channel->events = events;
    channel->readHandler = readHandler;
    channel->writeHandler = writeHandler;
    channel->destroyCallback = destroyHandler;
    channel->arg = arg;
    return channel;
}

void writeEventEnable(Channel *channel, bool flag)
{
    if (flag)
    {
        channel->events |= WRITE_EVENT;
    }
    else
    {
        channel->events &= ~WRITE_EVENT;
    }
}

bool isWriteEventEnable(Channel *channel)
{
    return channel->events & WRITE_EVENT;
}
