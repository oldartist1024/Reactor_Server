#include "Channel.h"
Channel::Channel(int fd, EventType events, handleFunc readHandler, handleFunc writeHandler, handleFunc destroyHandler, void *arg)
{
    this->m_fd = fd;
    this->m_events = (int)events;
    this->readHandler = readHandler;
    this->writeHandler = writeHandler;
    this->destroyCallback = destroyHandler;
    this->m_arg = arg;
}

void Channel::writeEventEnable(bool flag)
{
    if (flag)
    {
        // 添加写事件
        // this->m_events |= (int)EventType::WRITE_EVENT;
        this->m_events |= static_cast<int>(EventType::WRITE_EVENT);
    }
    else
    {
        // 删除写事件
        this->m_events &= ~(int)EventType::WRITE_EVENT;
    }
}

bool Channel::isWriteEventEnable()
{
    return this->m_events & (int)EventType::WRITE_EVENT;
}
